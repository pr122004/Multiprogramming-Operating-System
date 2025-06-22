#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <bitset>
#include <mutex>
#include <functional>
#include <string.h>

using namespace std;

// Forward declarations
class MOS;
struct PCB;

// Constants
const int MEM_SIZE = 100;
const int PAGE_SIZE = 10;
const int FRAME_COUNT = 10;
const int WORD_SIZE = 4;
const int MAX_TIMER = 1000000;
const int NUM_INTERRUPTS = 8;

// Error and Interrupt Codes
enum EM_Code { 
    EM_NO_ERR, EM_OUT_OF_DATA, EM_LINE_LIMIT, EM_TIME_LIMIT, 
    EM_OP_CODE_ERR, EM_OPERAND_ERR, EM_INVALID_PAGE 
};

enum SI_Type { READ=1, WRITE=2, TERM=3 };
enum PI_Type { PI_OP_ERR=1, PI_OPERAND_ERR=2, PI_PAGE_FAULT=3 };

// Interrupt priorities (higher number = higher priority)
enum InterruptPriority {
    PRIORITY_TIMER = 3,    
    PRIORITY_PAGE_FAULT = 2,
    PRIORITY_PROGRAM = 1,  // Program errors
    PRIORITY_SYSCALL = 0   
};

// Interrupt Vector Table Entry
struct InterruptVectorEntry {
    int type;  // SI_Type or PI_Type
    int priority;
    function<void()> handler;  
};

// CPU State
struct CPUState {
    char IR[WORD_SIZE]; //Holds the current instruction being executed.
    int IC = 0; //Points to  (VA) of the instruction being executed.
    char R[WORD_SIZE] = {'\0'}; //General purpose register
    bool C = false; //Stores result of comparison operations.
    int SI = 0; //service interrupts
    int PI = 0;
    int TI = 0;
    int RA = 0; // real address
};

// Page Table Entry
struct PageTableEntry {
    int frame;
    bool valid;
};

// Process states for context switching
enum ProcessState {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// Memory structure
struct Memory {
    char data[MEM_SIZE][WORD_SIZE];
    bool allocated[FRAME_COUNT] = {false};
    bitset<FRAME_COUNT> locked_frames;
    
    void clearFrame(int frame) {
        fill_n(data[frame*PAGE_SIZE], PAGE_SIZE*WORD_SIZE, '\0');
    }

    void lockFrame(int frame) {
        locked_frames.set(frame);
    }

    void unlockFrame(int frame) {
        locked_frames.reset(frame);
    }
};

// Process Context for context switching
struct ProcessContext {
    int registers[8];
    int programCounter;
    int stackPointer;
    int statusRegister;
    ProcessState state;
};

// Enhanced PCB with context information
struct PCB {
    int pid;
    int TTL;
    int TLL;
    int TTC = 0;
    int LLC = 0;
    PageTableEntry pageTable[FRAME_COUNT];
    int PTR;
    vector<string> dataCards;
    bool terminated = false;
    ProcessContext context;
    int priority;
    bitset<NUM_INTERRUPTS> interruptMask;
};

void debugPrint(const string& msg) {
    cout << "[DEBUG] " << msg << endl;
}

class MOS {
private:
    Memory mem;
    queue<PCB*> readyQueue;
    PCB* currentPCB = nullptr;
    CPUState cpu;
    ifstream inFile;
    ofstream outFile;
    int globalTimer = 0;
    bool systemRunning = true;
    bool interruptsEnabled = true;
    
    // Interrupt Vector Table
    vector<InterruptVectorEntry> interruptVectorTable;
    
    // Hardware ISR state
    struct HardwareISR {
        bool diskReady = false;
        bool printerReady = false;
        bool networkReady = false;
        queue<string> diskBuffer;
        queue<string> printerBuffer;
        queue<string> networkBuffer;
    } hardwareISR;

    // Critical section lock
    mutex criticalSectionLock;

    // Initialize interrupt vector table
    void initInterruptVectorTable() {
        // Timer interrupts
        interruptVectorTable.push_back(InterruptVectorEntry{
            0, 
            PRIORITY_TIMER, 
            bind(&MOS::handleTimerInterrupt, this)
        });
        
        // Program interrupts
        interruptVectorTable.push_back(InterruptVectorEntry{
            PI_OP_ERR, 
            PRIORITY_PROGRAM, 
            bind(&MOS::handleOpCodeError, this)
        });
        
        interruptVectorTable.push_back(InterruptVectorEntry{
            PI_OPERAND_ERR, 
            PRIORITY_PROGRAM, 
            bind(&MOS::handleOperandError, this)
        });
        
        interruptVectorTable.push_back(InterruptVectorEntry{
            PI_PAGE_FAULT, 
            PRIORITY_PAGE_FAULT, 
            bind(&MOS::handlePageFault, this)
        });
        
        // System call interrupts
        interruptVectorTable.push_back(InterruptVectorEntry{
            READ, 
            PRIORITY_SYSCALL, 
            bind(&MOS::handleRead, this)
        });
        
        interruptVectorTable.push_back(InterruptVectorEntry{
            WRITE, 
            PRIORITY_SYSCALL, 
            bind(&MOS::handleWrite, this)
        });
        
        interruptVectorTable.push_back(InterruptVectorEntry{
            TERM, 
            PRIORITY_SYSCALL, 
            bind(&MOS::handleTerminate, this)
        });
    }

    // Hardware ISR handlers
    void handleDiskInterrupt() {
        lock_guard<mutex> lock(criticalSectionLock);
        if (!hardwareISR.diskBuffer.empty()) {
            string data = hardwareISR.diskBuffer.front();
            hardwareISR.diskBuffer.pop();
            // Process disk data
            debugPrint("Processing disk data: " + data);
        }
        hardwareISR.diskReady = true;
    }

    void handlePrinterInterrupt() {
        lock_guard<mutex> lock(criticalSectionLock);
        if (!hardwareISR.printerBuffer.empty()) {
            string data = hardwareISR.printerBuffer.front();
            hardwareISR.printerBuffer.pop();
            outFile << data << endl;
            debugPrint("Printed data: " + data);
        }
        hardwareISR.printerReady = true;
    }

    void handleNetworkInterrupt() {
        lock_guard<mutex> lock(criticalSectionLock);
        if (!hardwareISR.networkBuffer.empty()) {
            string data = hardwareISR.networkBuffer.front();
            hardwareISR.networkBuffer.pop();
            // Process network data
            debugPrint("Processing network data: " + data);
        }
        hardwareISR.networkReady = true;
    }

    // Interrupt handlers
    void handleTimerInterrupt() {
        debugPrint("Time limit interrupt");
        terminate(EM_TIME_LIMIT);
    }

    void handleOpCodeError() {
        debugPrint("Operation code error");
        terminate(EM_OP_CODE_ERR);
    }

    void handleOperandError() {
        debugPrint("Operand error");
        terminate(EM_OPERAND_ERR);
    }

    void handlePageFault() {
        debugPrint("Page fault");
        terminate(EM_INVALID_PAGE);
    }

    void handleTerminate() {
        debugPrint("Terminate system call");
        terminate(EM_NO_ERR);
    }

    

    void saveContext() {
        if(currentPCB) {
            currentPCB->context.state = BLOCKED;
            currentPCB->context.programCounter = cpu.IC;
            // Save other CPU state
            for(int i = 0; i < 8; i++) {
                currentPCB->context.registers[i] = 0; // Save actual register values here
            }
            currentPCB->context.stackPointer = 0; // Save actual SP
            currentPCB->context.statusRegister = 0; // Save status flags
        }
    }

    void restoreContext() {
        if (currentPCB) {
            currentPCB->context.state = RUNNING;
    
            // Check if this process is being executed for the first time
            if (currentPCB->context.programCounter == -1) {
                debugPrint("First-time execution: setting IC to 0");
                cpu.IC = 0;  // Start from beginning
            } else {
                cpu.IC = currentPCB->context.programCounter;
                debugPrint("Restoring IC from context: IC = " + to_string(cpu.IC));
            }
    
            // Restore additional CPU state if needed
            // Example: Restore general-purpose registers
            
            
        }
    }
    

    void handleInterrupt() {
        if (!interruptsEnabled) return;

        // Find highest priority pending interrupt
        int highestPriority = -1;
        InterruptVectorEntry* highestPriorityEntry = nullptr;

        // Check timer interrupt first (highest priority)
        if (cpu.TI) {
            highestPriorityEntry = &interruptVectorTable[0]; // Timer interrupt is first in table
            highestPriority = highestPriorityEntry->priority;
        }

        // Check program interrupts
        if (cpu.PI) {
            for (auto& entry : interruptVectorTable) {
                if (entry.type == cpu.PI && entry.priority > highestPriority) {
                    highestPriority = entry.priority;
                    highestPriorityEntry = &entry;
                }
            }
        }

        // Check system call interrupts
        if (cpu.SI) {
            for (auto& entry : interruptVectorTable) {
                if (entry.type == cpu.SI && entry.priority > highestPriority) {
                    highestPriority = entry.priority;
                    highestPriorityEntry = &entry;
                }
            }
        }

        // Handle the highest priority interrupt
        if (highestPriorityEntry) {
            // Save context before handling interrupt
            saveContext();
            
            // Call the appropriate handler
            highestPriorityEntry->handler();
            
            // Clear the interrupt
            if (highestPriorityEntry->type == 0) cpu.TI = 0;
            else if (highestPriorityEntry->type == cpu.PI) cpu.PI = 0;
            else if (highestPriorityEntry->type == cpu.SI) cpu.SI = 0;
            
            // Restore context after handling interrupt
            restoreContext();
        }
    }

    // Memory management
    int allocateFrame() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(0, FRAME_COUNT-1);
        
        for(int attempts = 0; attempts < FRAME_COUNT*2; attempts++) {
            int frame = dist(gen);
            if (!mem.allocated[frame]) {
                mem.allocated[frame] = true;
                return frame;
            }
        }
        return -1;
    }

    // Address translation
    bool addressMap(int VA, int& RA) {
        // Step 1: Validate virtual address range
        if (VA < 0 || VA >= MEM_SIZE) {
            debugPrint("Invalid VA: " + to_string(VA));
            cpu.PI = PI_OPERAND_ERR;
            return false;
        }
    
        // Step 2: Calculate page and offset
        int page = VA / PAGE_SIZE;
        int offset = VA % PAGE_SIZE;
        
        // Step 3: Validate page number
        if (page >= FRAME_COUNT) {
            debugPrint("Invalid page: " + to_string(page));
            cpu.PI = PI_PAGE_FAULT;
            return false;
        }
    
        // Step 4: Check page table entry
        if (!currentPCB->pageTable[page].valid) {
            debugPrint("Page not allocated: " + to_string(page));
            cpu.PI = PI_PAGE_FAULT;
            return false;
        }
    
        int frame = currentPCB->pageTable[page].frame;
        
        // Step 5: Validate frame number
        if (frame < 0 || frame >= FRAME_COUNT) {
            debugPrint("Invalid frame: " + to_string(frame));
            cpu.PI = PI_PAGE_FAULT;
            return false;
        }
    
        // Step 6: Calculate real address
        RA = frame * PAGE_SIZE + offset;
        
        // Final validation
        if (RA < 0 || RA >= MEM_SIZE) {
            debugPrint("Invalid RA: " + to_string(RA));
            cpu.PI = PI_OPERAND_ERR;
            return false;
        }
    
        debugPrint("Successful mapping: VA=" + to_string(VA) + 
                  " → page=" + to_string(page) + 
                  " → frame=" + to_string(frame) + 
                  " → RA=" + to_string(RA));
        return true;
    }

    // I/O operations
    void handleRead() {
        // Check data availability
        if (currentPCB->dataCards.empty()) {
            debugPrint("No more data cards");
            cpu.SI = TERM;
            return;
        }
    
        string data = currentPCB->dataCards.front();
        currentPCB->dataCards.erase(currentPCB->dataCards.begin());
        debugPrint("Reading data: " + data);
    
        // Process data in WORD_SIZE chunks
        for (size_t i = 0; i < data.length(); i += WORD_SIZE) {
            string word = data.substr(i, min(static_cast<size_t>(WORD_SIZE), data.length()-i));
            word.resize(WORD_SIZE, ' '); // Pad with spaces if needed
    
            // Calculate target address
            int targetVA = cpu.RA + (i/WORD_SIZE);
            int RA;
            
            // Map virtual address
            if (!addressMap(targetVA, RA)) {
                debugPrint("Address mapping failed for VA: " + to_string(targetVA));
                return;
            }
    
            // Write to memory
            for (int j = 0; j < WORD_SIZE; j++) {
                mem.data[RA][j] = word[j];
            }
            debugPrint("Wrote '" + word + "' to RA " + to_string(RA));
        }
    }
    

    void handleWrite() {
    // Check line limit
    currentPCB->LLC++;
    if (currentPCB->LLC > currentPCB->TLL) {
        debugPrint("Line limit exceeded (" + 
                 to_string(currentPCB->LLC) + "/" + 
                 to_string(currentPCB->TLL) + ")");
        cpu.SI = TERM;
        return;
    }

    int RA;
    if (!addressMap(cpu.RA, RA)) {
        debugPrint("Address mapping failed for write");
        return;
    }

    // Read from memory
    string output;
    for (int i = 0; i < WORD_SIZE; i++) {
        if (mem.data[RA][i] != '\0') {
            output += mem.data[RA][i];
        }
    }

    // Write to output file
    outFile << output << endl;
    debugPrint("Wrote to output: " + output);
}
    

    // Termination handling
    void terminate(EM_Code code) {
        if (!currentPCB) return;
    
        // 1. Log termination details
        debugPrint("Terminating process " + to_string(currentPCB->pid));
        outFile << "\n\nProcess " << currentPCB->pid << " terminated: ";
        
        // Termination reason message
        switch(code) {
            case EM_NO_ERR: outFile << "Normal termination"; break;
            case EM_OUT_OF_DATA: outFile << "Out of data"; break;
            case EM_LINE_LIMIT: outFile << "Line limit exceeded"; break;
            case EM_TIME_LIMIT: outFile << "Time limit exceeded"; break;
            case EM_OP_CODE_ERR: outFile << "Invalid operation code"; break;
            case EM_OPERAND_ERR: outFile << "Invalid operand"; break;
            case EM_INVALID_PAGE: outFile << "Invalid page access"; break;
        }
        outFile << "\nTTC: " << currentPCB->TTC << ", LLC: " << currentPCB->LLC << endl;
    
        // 2. Release all resources systematically
        
        // a) Release memory frames (including page table frame)
        if (currentPCB->PTR != -1) {
            int pageTableFrame = currentPCB->PTR / PAGE_SIZE;
            mem.allocated[pageTableFrame] = false;
            mem.clearFrame(pageTableFrame);
            mem.unlockFrame(pageTableFrame);
        }
    
        for (int i = 0; i < FRAME_COUNT; i++) {
            if (currentPCB->pageTable[i].valid) {
                int frame = currentPCB->pageTable[i].frame;
                if (frame >= 0 && frame < FRAME_COUNT) {
                    mem.allocated[frame] = false;
                    mem.clearFrame(frame);
                    mem.unlockFrame(frame);
                }
                currentPCB->pageTable[i].valid = false;
            }
        }
    
        // b) Clear CPU context if this was the current process
        if (currentPCB->context.state == RUNNING) {
            memset(&cpu, 0, sizeof(CPUState));
            cpu.IC = 0;
            cpu.TI = 0;
            cpu.SI = 0;
            cpu.PI = 0;
        }
    
        // c) Clean up data cards
        currentPCB->dataCards.clear();
        currentPCB->dataCards.shrink_to_fit();
    
        // 3. Update process state
        currentPCB->terminated = true;
        currentPCB->context.state = TERMINATED;
    
        // 4. Process cleanup and context switch
        PCB* terminatedPCB = currentPCB;
        currentPCB = nullptr;
    
        // Flush output before context switch
        outFile.flush();
    
        // 5. Schedule next process or shutdown
        if (!readyQueue.empty()) {
            currentPCB = readyQueue.front();
            readyQueue.pop();
            
            // Initialize new process context
            currentPCB->context.state = RUNNING;
            cpu.IC = 0;  // Start from beginning
            
            debugPrint("Switched to process " + to_string(currentPCB->pid));
        } else {
            debugPrint("No more processes in ready queue");
            systemRunning = false;
        }
    
        // 6. Final cleanup after state management
        if (terminatedPCB) {
            // Clear any remaining pointers
            terminatedPCB->PTR = -1;
            memset(&terminatedPCB->context, 0, sizeof(ProcessContext));
            
            // Delete the PCB
            delete terminatedPCB;
            terminatedPCB = nullptr;
        }
    
        // 7. Reset interrupt flags
        cpu.TI = cpu.SI = cpu.PI = 0;
    }
public:
    MOS(const string& input, const string& output) 
        : inFile(input), outFile(output) {
        if (!inFile.is_open()) {
            throw runtime_error("Failed to open input file: " + input);
        }
        if (!outFile.is_open()) {
            throw runtime_error("Failed to open output file: " + output);
        }
        initInterruptVectorTable();
        debugPrint("MOS initialized with interrupt vector table");
    }

    void loadJobs() {
        debugPrint("Starting to load jobs");
        string line;
        PCB* pcb = nullptr;
        bool readingData = false;
        string programCode;

        while (getline(inFile, line)) {
            debugPrint("Read line: " + line);
            
            if (line.find("$AMJ") == 0) {
                debugPrint("Found new job");
                pcb = new PCB();
                pcb->pid = stoi(line.substr(4, 4));
                pcb->TTL = stoi(line.substr(8, 4));
                pcb->TLL = stoi(line.substr(12, 4));
                pcb->context.state = READY;
                pcb->context.programCounter = -1; // Means "not set yet"

                programCode.clear();
                
                // Initialize page table
                for (int i = 0; i < FRAME_COUNT; i++) {
                    pcb->pageTable[i] = {-1, false};
                }
                
                // Allocate frame for page table
                int frame = allocateFrame();
                if (frame == -1) {
                    debugPrint("Failed to allocate frame for page table");
                    delete pcb;
                    throw runtime_error("Memory allocation failed for page table");
                }
                pcb->PTR = frame * PAGE_SIZE;
                debugPrint("Allocated frame " + to_string(frame) + " for page table");
            }
            else if (line.find("$DTA") == 0) {
                debugPrint("Found data section");
                readingData = true;
                
                // Load program code into memory
                if (!programCode.empty() && pcb) {
                    loadProgramIntoMemory(pcb, programCode);
                }
            }
            else if (line.find("$END") == 0) {
                debugPrint("End of job");
                readingData = false;
                if (pcb) {
                    readyQueue.push(pcb);
                    debugPrint("Added job " + to_string(pcb->pid) + " to ready queue");
                }
                pcb = nullptr;
            }
            else if (readingData && pcb) {
                pcb->dataCards.push_back(line);
                debugPrint("Added data card: " + line);
            }
            else if (pcb) {
                programCode += line + "\n";
            }
        }
        debugPrint("Finished loading jobs");
    }

    void loadProgramIntoMemory(PCB* pcb, const string& code) {
        debugPrint("Loading program into memory for PID " + to_string(pcb->pid));
        
        // Split code into instructions
        vector<string> instructions;
        stringstream ss(code);
        string instruction;
        while (getline(ss, instruction)) {
            // Remove whitespace and newlines
            instruction.erase(remove_if(instruction.begin(), instruction.end(), ::isspace), instruction.end());
            if (!instruction.empty()) {
                instructions.push_back(instruction);
            }
        }
        
        debugPrint("Number of instructions: " + to_string(instructions.size()));
        
        // Calculate number of pages needed
        int instructionsPerPage = PAGE_SIZE / WORD_SIZE;
        int pagesNeeded = (instructions.size() + instructionsPerPage - 1) / instructionsPerPage;
        debugPrint("Instructions per page: " + to_string(instructionsPerPage) + 
                  ", Pages needed: " + to_string(pagesNeeded));

        // Initialize all page table entries as invalid
        for (int i = 0; i < FRAME_COUNT; i++) {
            pcb->pageTable[i] = {-1, false};
        }

        // Allocate frames for program
        for (int i = 0; i < pagesNeeded && i < FRAME_COUNT; i++) {
            int frame = allocateFrame();
            if (frame == -1) {
                debugPrint("Failed to allocate frame for program page " + to_string(i));
                throw runtime_error("Memory allocation failed for program");
            }
            
            pcb->pageTable[i] = {frame, true};
            debugPrint("Allocated frame " + to_string(frame) + " for page " + to_string(i));

            // Clear frame before use
            for (int j = 0; j < PAGE_SIZE; j++) {
                for (int k = 0; k < WORD_SIZE; k++) {
                    mem.data[frame * PAGE_SIZE + j][k] = '\0';
                }
            }

            // Copy instructions to frame
            int startInstr = i * instructionsPerPage;
            int endInstr = min(startInstr + instructionsPerPage, (int)instructions.size());
            
            for (int j = startInstr; j < endInstr; j++) {
                string instr = instructions[j];
                // Pad instruction to WORD_SIZE
                instr.resize(WORD_SIZE, ' ');
                
                int offset = (j - startInstr);
                int addr = frame * PAGE_SIZE + offset;
                
                for (int k = 0; k < WORD_SIZE; k++) {
                    mem.data[addr][k] = instr[k];
                }
                debugPrint("Loaded instruction: [" + instr + "] at frame " + to_string(frame) + 
                          " address " + to_string(addr));
            }
        }
    }

    void executeJob() {
        if (!currentPCB || currentPCB->terminated) {
            return;
        }

        debugPrint("Executing job PID " + to_string(currentPCB->pid));
        currentPCB->context.state = RUNNING;
        
        try {
            while (!currentPCB->terminated && currentPCB->TTC < currentPCB->TTL) {
                if (currentPCB->TTC >= currentPCB->TTL) {
                    debugPrint("Time limit exceeded");
                    terminate(EM_TIME_LIMIT);
                    return;
                }

                // Map virtual address (IC) to real address
                int realAddr;
                if (!addressMap(cpu.IC, realAddr)) {
                    debugPrint("Failed to map instruction address");
                    return;
                }
                
                // Fetch instruction
                string instruction;
                for (int i = 0; i < WORD_SIZE; i++) {
                    char c = mem.data[realAddr][i];
                    if (c != '\0') {
                        instruction += c;
                    }
                }
                
                if (instruction.empty()) {
                    debugPrint("Empty instruction at address " + to_string(realAddr));
                    return;
                }

                debugPrint("Fetched instruction: [" + instruction + "] from address " + to_string(realAddr));
                
                // Copy instruction to IR
                instruction.resize(WORD_SIZE, ' ');
                for (int i = 0; i < WORD_SIZE; i++) {
                    cpu.IR[i] = instruction[i];
                }
                
                cpu.IC++;  // Increment instruction counter

                // Execute the instruction
                executeInstruction();
                
                // Update timers
                currentPCB->TTC++;
                globalTimer++;

                // Handle interrupts
                if (cpu.SI || cpu.PI || cpu.TI) {
                    handleInterrupt();
                    if (!currentPCB || currentPCB->terminated) {
                        return;
                    }
                }

                // Context switch if time slice expired
                if (globalTimer % 10 == 0 && !readyQueue.empty()) {
                    debugPrint("Time slice expired, switching process");
                    saveContext();
                    readyQueue.push(currentPCB);
                    currentPCB = readyQueue.front();
                    readyQueue.pop();
                    restoreContext();
                }
            }
        }
        catch (const exception& e) {
            // debugPrint("Error executing process: " + string(e.what()));
            // terminate(EM_OP_CODE_ERR);
        }
    }

    void executeInstruction() {
        string instruction(cpu.IR, WORD_SIZE);
        debugPrint("Executing instruction: [" + instruction + "]");
        
        // Remove any trailing whitespace or null characters
        instruction.erase(find_if(instruction.rbegin(), instruction.rend(), 
                                [](char c) { return c != ' ' && c != '\0'; }).base(), 
                        instruction.end());
        
        if (instruction.length() < 3) {
            debugPrint("Invalid instruction length: " + to_string(instruction.length()));
            cpu.PI = PI_OP_ERR;
            terminate(EM_OP_CODE_ERR);
            return;
        }
    
        string op = instruction.substr(0, 2);
        string operandStr = instruction.substr(2);
        
        // Remove any whitespace
        op.erase(remove_if(op.begin(), op.end(), ::isspace), op.end());
        operandStr.erase(remove_if(operandStr.begin(), operandStr.end(), ::isspace), operandStr.end());
        
        debugPrint("Parsed op: [" + op + "] operand: [" + operandStr + "]");
        
        if (op == "GD") {
            int target;
            try {
                target = stoi(operandStr);
            } catch (const exception& e) {
                debugPrint("Invalid operand: " + operandStr);
                cpu.PI = PI_OPERAND_ERR;
                return;
            }
            
            // Map virtual address first
            // if (!addressMap(target, cpu.RA)) {
            //     debugPrint("Address mapping failed for GD instruction");
            //     return; // Error already set in addressMap()
            // }
            
            cpu.SI = READ;
            handleRead();
            handleTerminate();
        }
        else if (op == "PD") {
            int target;
            try {
                target = stoi(operandStr);
            } catch (const exception& e) {
                debugPrint("Invalid operand: " + operandStr);
                cpu.PI = PI_OPERAND_ERR;
                return;
            }
    
            if (!addressMap(target, cpu.RA)) {
                debugPrint("Address mapping failed for PD instruction");
                return;
            }
            cpu.SI = WRITE;
            handleWrite();
        }
        else if (op == "H") {
            debugPrint("Executing H instruction");
            cpu.SI = TERM;
            handleInterrupt();
        }
        else if (op == "LR" || op == "SR" || op == "CR" || op == "BT") {
            debugPrint("Executing arithmetic/logic instruction: " + op);
            int target;
            try {
                target = stoi(operandStr);
            } catch (const exception& e) {
                debugPrint("Invalid operand: " + operandStr);
                cpu.PI = PI_OPERAND_ERR;
                return;
            }
            executeArithmeticLogic(op, target);
        }
        else {
            debugPrint("Invalid operation code: " + op);
            cpu.PI = PI_OP_ERR;
            terminate(EM_OP_CODE_ERR);
        }

        
    }

    void executeArithmeticLogic(const string& op, int target) {
        int realAddr;
        if (!addressMap(target, realAddr)) {
            debugPrint("Address mapping failed for " + op + " instruction");
            return;
        }

        if (op == "LR") {
            copy(mem.data[realAddr], mem.data[realAddr]+WORD_SIZE, cpu.R);
        }
        else if (op == "SR") {
            copy(cpu.R, cpu.R+WORD_SIZE, mem.data[realAddr]);
        }
        else if (op == "CR") {
            cpu.C = equal(cpu.R, cpu.R+WORD_SIZE, mem.data[realAddr]);
        }
        else if (op == "BT" && cpu.C) {
            cpu.IC = target;
        }
    }

    void run() {
        loadJobs();
        
        while (systemRunning && globalTimer < MAX_TIMER) {
            if (!currentPCB) {
                if (!readyQueue.empty()) {
                    currentPCB = readyQueue.front();
                    readyQueue.pop();
                    restoreContext();
                    debugPrint("Starting execution of process " + to_string(currentPCB->pid));
                } else {
                    debugPrint("No more processes to execute");
                    systemRunning = false;
                    break;
                }
            }

            if (currentPCB) {
                cout << "🕑 GLOBAL TIMER => [" << globalTimer << "] Processing PID: " 
                     << currentPCB->pid << " State: " << currentPCB->context.state << endl;
                
                executeJob();
                
                // Check if current process terminated
                if (!currentPCB || currentPCB->terminated) {
                    continue;
                }
            }
        }

        if (globalTimer >= MAX_TIMER) {
            cout << "System halted: Maximum time limit reached" << endl;
        }
    }
};

int main() {
    try {
        MOS mos("input.txt", "output.txt");
        mos.run();
        cout << "System shutdown normally" << endl;
    }
    catch (const exception& e) {
        cerr << "System error: " << e.what() << endl;
        return 1;
    }
    return 0;
}



