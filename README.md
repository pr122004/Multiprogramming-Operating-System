# MOS (Multiprogramming Operating System) Simulation

## Overview

This project is a simulation of a **Memory Operating System (MOS)** that demonstrates core operating system concepts including:

- **Process management** (PCB, context switching)  
- **Memory management** (paging, address translation)  
- **Interrupt handling** (timer, I/O, program errors)  
- **Job scheduling**  
- **System calls** (read, write, terminate)  

The simulation implements a virtual machine with CPU, memory, and I/O device emulation to execute simple programs loaded from an input file.

---

## Key Features

### 🔄 Process Management

- **Process Control Blocks (PCBs)** track process state  
- **Context switching** between processes  
- **Ready queue** for process scheduling  
- **Time slicing** for multiprogramming  

### 🧠 Memory Management

- **Paged virtual memory system**  
- **Address translation** (VA → PA)  
- **Page fault handling**  
- **Frame allocation and deallocation**  

### ⚡ Interrupt Handling

- **Priority-based interrupt vector table**  
- **Timer interrupts**  
- **Program interrupts** (invalid opcodes, operands)  
- **System call interrupts** (read, write, terminate)  

### 🖨️ I/O Operations

- **Simulated input/output devices**  
- **Buffered I/O operations**  
- **Device interrupt handling**

---

## Program Instructions

The simulated CPU supports these instructions:

| Opcode | Description                |
|--------|----------------------------|
| `GD`   | Get Data (read from input) |
| `PD`   | Put Data (write to output) |
| `H`    | Halt (terminate process)   |
| `LR`   | Load Register              |
| `SR`   | Store Register             |
| `CR`   | Compare Register           |
| `BT`   | Branch if True             |

---

## Implementation Details

### 🧱 Memory Organization

- **Total memory:** 100 words  
- **Page size:** 10 words  
- **Frame count:** 10  
- **Word size:** 4 bytes  

### ⚙️ Process States

- **READY:** Process is ready to execute  
- **RUNNING:** Process is currently executing  
- **BLOCKED:** Process is waiting for I/O  
- **TERMINATED:** Process has completed  

---

## ⚠️ Error Handling

The system detects and handles several error conditions:

- **Out of data**  
- **Line limit exceeded**  
- **Time limit exceeded**  
- **Invalid operation code**  
- **Invalid operand**  
- **Invalid page access**

---

## 📄 Sample Output Logs

<details>
<summary><strong>Click to expand logs</strong></summary>

```
[DEBUG] MOS initialized with interrupt vector table
[DEBUG] Starting to load jobs
[DEBUG] Read line: $AMJ0001001000020  
[DEBUG] Found new job
[DEBUG] Allocated frame 0 for page table
[DEBUG] Read line: GD10
[DEBUG] Read line: PD10
[DEBUG] Read line: H
[DEBUG] Read line: $DTA
[DEBUG] Found data section
[DEBUG] Loading program into memory for PID 1
[DEBUG] Number of instructions: 3
[DEBUG] Instructions per page: 2, Pages needed: 2
[DEBUG] Allocated frame 4 for page 0
[DEBUG] Loaded instruction: [GD10] at frame 4 address 40
[DEBUG] Loaded instruction: [PD10] at frame 4 address 41
[DEBUG] Allocated frame 3 for page 1
[DEBUG] Loaded instruction: [H   ] at frame 3 address 30
[DEBUG] Read line: Hello
[DEBUG] Added data card: Hello
[DEBUG] Read line: $END
[DEBUG] End of job
[DEBUG] Added job 1 to ready queue
[DEBUG] Read line: $AMJ0002000500005  
[DEBUG] Found new job
[DEBUG] Allocated frame 6 for page table
[DEBUG] Read line: GD50
[DEBUG] Read line: H
[DEBUG] Read line: $DTA
[DEBUG] Found data section
[DEBUG] Loading program into memory for PID 2
[DEBUG] Number of instructions: 2
[DEBUG] Instructions per page: 2, Pages needed: 1
[DEBUG] Allocated frame 8 for page 0
[DEBUG] Loaded instruction: [GD50] at frame 8 address 80
[DEBUG] Loaded instruction: [H   ] at frame 8 address 81
[DEBUG] Read line: ShouldCausePageFault
[DEBUG] Added data card: ShouldCausePageFault
[DEBUG] Read line: $END
[DEBUG] End of job
[DEBUG] Added job 2 to ready queue
[DEBUG] Read line: $AMJ0003001000010  
[DEBUG] Found new job
[DEBUG] Allocated frame 9 for page table
[DEBUG] Read line: XX10
[DEBUG] Read line: H
[DEBUG] Read line: $DTA
[DEBUG] Found data section
[DEBUG] Loading program into memory for PID 3
[DEBUG] Number of instructions: 2
[DEBUG] Instructions per page: 2, Pages needed: 1
[DEBUG] Allocated frame 1 for page 0
[DEBUG] Loaded instruction: [XX10] at frame 1 address 10
[DEBUG] Loaded instruction: [H   ] at frame 1 address 11
[DEBUG] Read line: $END
[DEBUG] End of job
[DEBUG] Added job 3 to ready queue
[DEBUG] Read line: $AMJ0004001000010  
[DEBUG] Found new job
[DEBUG] Allocated frame 2 for page table
[DEBUG] Read line: GDAB
[DEBUG] Read line: H
[DEBUG] Read line: $DTA
[DEBUG] Found data section
[DEBUG] Loading program into memory for PID 4
[DEBUG] Number of instructions: 2
[DEBUG] Instructions per page: 2, Pages needed: 1
[DEBUG] Allocated frame 7 for page 0
[DEBUG] Loaded instruction: [GDAB] at frame 7 address 70
[DEBUG] Loaded instruction: [H   ] at frame 7 address 71
[DEBUG] Read line: $END
[DEBUG] End of job
[DEBUG] Added job 4 to ready queue
[DEBUG] Read line: 
[DEBUG] Finished loading jobs
[DEBUG] First-time execution: setting IC to 0
[DEBUG] Starting execution of process 1
🕑 GLOBAL TIMER => [0] Processing PID: 1 State: 1
[DEBUG] Executing job PID 1
[DEBUG] Successful mapping: VA=0 → page=0 → frame=4 → RA=40
[DEBUG] Fetched instruction: [GD10] from address 40
[DEBUG] Executing instruction: [GD10]
[DEBUG] Parsed op: [GD] operand: [10]
[DEBUG] Reading data: Hello
[DEBUG] Successful mapping: VA=0 → page=0 → frame=4 → RA=40
[DEBUG] Wrote 'Hell' to RA 40
[DEBUG] Successful mapping: VA=1 → page=0 → frame=4 → RA=41
[DEBUG] Wrote 'o   ' to RA 41
[DEBUG] Terminate system call
[DEBUG] Terminating process 1
[DEBUG] Switched to process 2
[DEBUG] Successful mapping: VA=0 → page=0 → frame=8 → RA=80
[DEBUG] Fetched instruction: [GD50] from address 80
[DEBUG] Executing instruction: [GD50]
[DEBUG] Parsed op: [GD] operand: [50]
[DEBUG] Reading data: ShouldCausePageFault
[DEBUG] Successful mapping: VA=0 → page=0 → frame=8 → RA=80
[DEBUG] Wrote 'Shou' to RA 80
[DEBUG] Successful mapping: VA=1 → page=0 → frame=8 → RA=81
[DEBUG] Wrote 'ldCa' to RA 81
[DEBUG] Successful mapping: VA=2 → page=0 → frame=8 → RA=82
[DEBUG] Wrote 'useP' to RA 82
[DEBUG] Successful mapping: VA=3 → page=0 → frame=8 → RA=83
[DEBUG] Wrote 'ageF' to RA 83
[DEBUG] Successful mapping: VA=4 → page=0 → frame=8 → RA=84
[DEBUG] Wrote 'ault' to RA 84
[DEBUG] Terminate system call
[DEBUG] Terminating process 2
[DEBUG] Switched to process 3
[DEBUG] Successful mapping: VA=0 → page=0 → frame=1 → RA=10
[DEBUG] Fetched instruction: [XX10] from address 10
[DEBUG] Executing instruction: [XX10]
[DEBUG] Parsed op: [XX] operand: [10]
[DEBUG] Invalid operation code: XX
[DEBUG] Terminating process 3
[DEBUG] Switched to process 4
[DEBUG] Successful mapping: VA=0 → page=0 → frame=7 → RA=70
[DEBUG] Fetched instruction: [GDAB] from address 70
[DEBUG] Executing instruction: [GDAB]
[DEBUG] Parsed op: [GD] operand: [AB]
[DEBUG] Invalid operand: AB
[DEBUG] Operand error
[DEBUG] Terminating process 4
[DEBUG] No more processes in ready queue
System shutdown normally
```

</details>

