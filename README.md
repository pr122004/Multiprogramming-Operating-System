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
