# LC-3 Virtual Machine (Basic) with Assembler and Sample Programs

This repository contains a very basic LC-3 virtual machine (VM) implementation with two variants (`C` and `C++14`), along with a lightweight LC-3 assembler framework and several example LC-3 assembly programs for testing. It also includes the LC3 roguelike tunnel generator program sourced from the public repository `justinmeiners/lc3-rogue` for experimentation and homework exploration.

## Features
- Minimal LC-3 emulator implementing the 15-instruction ISA and traps.
- Two VM builds:
  - `vm` (C version, `vm.c`)
  - `vm-alt` (C++14 version, `vm-alt.cpp`)
- Included assembler: `lc3as.py/lc3as.py` to assemble LC-3 assembly into `.obj` binary images.
- Example assembly programs to validate the VM and demonstrate I/O via traps.
- A copy of the LC3 rogue tunnel generator assembly for a larger test case.

## Repository Layout
- `vm.c` — Basic LC-3 VM (C).
- `vm-alt.cpp` — Alternate LC-3 VM (C++14, table-driven style).
- `Makefile` — Builds both VM binaries.
- `lc3as.py/` — Python-based LC-3 assembler and examples.
  - `lc3as.py/lc3as.py` — Assembler (produces `.obj`).
  - `lc3as.py/examples/` — Small sample `.asm` files.
  - Additional example files at the root of `lc3as.py/` like `hello.asm`, `sum.asm`.
- `lc3-rogue-master/rogue.asm` — Roguelike tunnel generator assembly (attribution below).

## Prerequisites
- Linux or WSL (Windows Subsystem for Linux). The VM uses UNIX `termios` APIs for input; run it in a Linux/WSL terminal.
- `gcc` and `g++` (or compatible toolchain)
- `make`
- `python3` (for the assembler `lc3as.py`)

On Ubuntu/WSL you can install the basics with:
```bash
sudo apt update
sudo apt install build-essential python3
```

## Build
From the repository root `virtual_machine/`:
```bash
make
```
This produces two executables:
- `./vm`
- `./vm-alt`

Clean build outputs:
```bash
make clean
```

## Assembling LC-3 Programs
Use the included Python assembler to convert `.asm` to `.obj` (LC-3 object image). The assembler does not require a specific extension, but this repo uses `.asm` for source and `.obj` for output.

Examples:
```bash
# Assemble a simple hello program
python3 lc3as.py/lc3as.py lc3as.py/hello.asm   # creates lc3as.py/hello.obj

# Assemble sum/add examples
python3 lc3as.py/lc3as.py lc3as.py/sum.asm     # creates lc3as.py/sum.obj
python3 lc3as.py/lc3as.py lc3as.py/add_two.asm # creates lc3as.py/add_two.obj

# Assemble the rogue example (two equivalent sources exist)
python3 lc3as.py/lc3as.py lc3-rogue-master/rogue.asm # outputs lc3-rogue-master/rogue.obj
# or
python3 lc3as.py/lc3as.py lc3as.py/rogue.asm          # outputs lc3as.py/rogue.obj
```

Notes:
- Ensure your assembly begins with an origin matching the VM start address, e.g. `.ORIG x3000`.
- The assembler’s usage is also documented in `lc3as.py/README.md`.

## Running Programs on the VM
The VM expects one or more LC-3 image files (`.obj`, binary) and starts execution at `PC = x3000`.

General usage:
```bash
./vm <image1.obj> [image2.obj ...]
# or
./vm-alt <image1.obj> [image2.obj ...]
```

Examples:
```bash
# Hello
./vm lc3as.py/hello.obj

# Sum
./vm lc3as.py/sum.obj

# Rogue (depending on where you assembled it)
./vm lc3-rogue-master/rogue.obj
# or
./vm lc3as.py/rogue.obj
```

Tips:
- Input is taken from the terminal (GETC/IN traps). Output uses OUT/PUTS/PUTSP traps.
- Press `Ctrl+C` to interrupt; the VM restores terminal settings on exit.

## Sample Programs Included
- `lc3as.py/examples/hello.asm`, `lc3as.py/hello.asm` — Prints a greeting using traps.
- `lc3as.py/sum.asm` — Demonstrates arithmetic and memory operations.
- `lc3as.py/add_two.asm` — Simple arithmetic.
- `lc3-rogue-master/rogue.asm` and `lc3as.py/rogue.asm` — Roguelike tunnel generator.

You can browse more tiny examples in `lc3as.py/examples/`.

## How It Works (Brief)
- The VM allocates 65,536 16-bit words of memory and maintains LC-3 registers `R0..R7`, `PC`, and `COND`.
- It implements all core opcodes: `BR, ADD, LD, ST, JSR/JSRR, AND, LDR, STR, RTI (unused), NOT, LDI, STI, JMP/RET, RES (unused), LEA, TRAP`.
- Keyboard is mapped via memory-mapped registers `KBSR` (`xFE00`) and `KBDR` (`xFE02`).
- The loader reads the origin word from the object file and loads subsequent words into memory, byte-swapping from big-endian to host order.

For details, see `vm.c` and `vm-alt.cpp`.

## Troubleshooting
- If the VM prints usage like `lc3 [ image-file]....`, you likely didn’t pass any `.obj` files.
- If nothing appears on screen, verify your program uses traps to output text (e.g., `PUTS` or `OUT`).
- If input echoing or line buffering seems odd after a crash, run `reset` in your terminal to restore settings (the VM restores them on normal exit and on `SIGINT`).
- Ensure `.ORIG x3000` in your assembly to match the VM’s start address.

## Attribution
- LC3 Rogue example is from: `justinmeiners/lc3-rogue` — Roguelike tunnel generator in LC3 assembly. See `lc3-rogue-master/README.md` for its MIT license.
- Assembler `lc3as.py` by Jason Pepas (MIT). See `lc3as.py/README.md` for usage and references.
- The LC-3 architecture is described in Patt & Patel, “Introduction to Computing Systems,” and the ISA in Appendix A (links in `lc3as.py/README.md`).

## License
This repository includes third-party components under their respective licenses as noted above. If you add your own license for this VM, document it here.

