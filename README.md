# LC-3-Virtual-Machine
A fully functional LC-3 Virtual Machine implemented from scratch in C, capable of loading and executing LC-3 .obj binaries.
This project simulates the LC-3 architecture including registers, memory, instruction execution cycle, condition flags, and memory-mapped I/O, allowing real LC-3 machine code to run exactly as on hardware.

## ✨ Features
- Complete LC-3 instruction set implementation
- Memory-mapped I/O (keyboard status & keyboard data registers)
- Full support for TRAP routines (GETC, OUT, IN, PUTS, PUTSP, HALT)
- Handles sign extension, branching, subroutines (JSR/JSRR), PC-offset addressing, and indirect addressing
- Executes compiled .obj LC-3 binaries
- Includes sample program to print "Hello World" with TRAP output

## 📦 Building & Running
  gcc vm.c -o lc3  
./lc3 hello.obj

## 📂 Project Structure
├── lc3vm.c       # Source code for LC-3 Virtual Machine  
├── hello.asm     # Assembly file which I assembled using an online LC-3 assembler
└── hello.obj     # Sample program that prints "Hello World!"

## 🧠 How It Works
- Simulates the LC-3’s 16-bit registers and 65,536-word memory
- Fetch → Decode → Execute loop
- Sign-extension and flag updates for realistic execution
- Loads programs starting at memory location 0x3000

## 🤝 Contributions
Suggestions, issues, and improvements are welcome!  
Feel free to fork and submit a PR.

## 📜 License
MIT License — free to use, improve, and share!
