#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
/* unix only */
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>
// #include <filename.h>

// Memory Storage
#define MEMORY_MAX  (1 << 16)
uint16_t memory[MEMORY_MAX]; /* 65536 Locations */

// Registers
enum register_e {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};

// Register Storage
uint16_t reg[R_COUNT];

// Opcodes
enum opcode_e {
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP,   /* execute trap */
    OP_OUT
};

// Condition Flags
enum condition_flags_e {
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

// Memory Mapped Registers
enum {
    MR_KBSR = 0xFE00, /* Keyboard Status */
    MR_KBDR = 0xFE02, /* Keyboard Data */
};

enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if(address == MR_KBSR)
    {
        if(check_key())
        {
            memory[MR_KBSR] = ( 1 << 15 );
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ( (x >> (bit_count -1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

void update_flags(uint16_t r)
{
    if(reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if(reg[r] >> 15)
    {
        reg[R_COND] = FL_NEG;
    }
    else{
        reg[R_COND] = FL_POS;
    }
}

void vm_Add(uint16_t instr)
{
    /* destination register */
    uint16_t r0 = (instr >> 9) & 0x7;
    /* first operand */
    uint16_t r1 = (instr >> 7) & 0x7;
    /* weather we are in immediate mode */
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] + reg[r2];
    }
    update_flags(r0);
}

void vm_LDI(uint16_t instr)
{
    /* destination register */
    uint16_t r0 = (instr >> 9) & 0x7;
    /* PC Offset 9 */
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    /* Add PC Offset to the current PC and load the data to R0 */
    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
    update_flags(r0);
}

void vm_And(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag)
    {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] & imm5;
    }
    else
    {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] & reg[r2];
    }
    update_flags(r0);
}

void vm_Not(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;

    reg[r0] = ~reg[r1];
    update_flags(r0);
}

void vm_Br(uint16_t instr)
{
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    uint16_t cond_flag = (instr >> 9) & 0x7;
    if (cond_flag & reg[R_COND])
    {
        reg[R_PC] += pc_offset;
    }
}

void vm_Jmp(uint16_t instr)
{
    /* Also handles RET */
    uint16_t r1 = (instr >> 6) & 0x7;
    reg[R_PC] = reg[r1];
}

void vm_JmpReg(uint16_t instr)
{
    uint16_t long_flag = (instr >> 11) & 1;
    reg[R_R7] = reg[R_PC];
    if (long_flag)
    {
        uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
        reg[R_PC] += long_pc_offset;  /* JSR */
    }
    else
    {
        uint16_t r1 = (instr >> 6) & 0x7;
        reg[R_PC] = reg[r1]; /* JSRR */
    }
}

void vm_LD(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    reg[r0] = mem_read(reg[R_PC] + pc_offset);
    update_flags(r0);
}

void vm_LDR(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3F, 6);
    reg[r0] = mem_read(reg[r1] + offset);
    update_flags(r0);
}

void vm_LDEA(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    reg[r0] = reg[R_PC] + pc_offset;
    update_flags(r0);
}

void vm_ST(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    mem_write(reg[R_PC] + pc_offset, reg[r0]);
}

void vm_STI(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
}

void vm_STR(uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3F, 6);
    mem_write(reg[r1] + offset, reg[r0]);
}

void vm_TRAP(uint16_t instr, int* running)
{
    reg[R_R7] = reg[R_PC];
    printf("instrTrap : %d\n", (instr & 0xFF));       
    switch (instr & 0xFF)
    {
        case TRAP_GETC:
            /* read a single ASCII char */
            reg[R_R0] = (uint16_t)getchar();
            update_flags(R_R0);
            break;
        case TRAP_OUT:
            printf("%x\n",(char)reg[R_R0]);
            putc((char)reg[R_R0], stdout);
            fflush(stdout);
            break;
        case TRAP_PUTS:
            {
                /* one char per word */
                uint16_t* c = memory + reg[R_R0];
                while (*c)
                {
                    putc((char)*c, stdout);
                    ++c;
                }
                fflush(stdout);
            }
            break;
        case TRAP_IN:
            {
                printf("Enter a character: ");
                char c = getchar();
                putc(c, stdout);
                fflush(stdout);
                reg[R_R0] = (uint16_t)c;
                update_flags(R_R0);
            }
            break;
        case TRAP_PUTSP:
            {
                /* one char per byte (two bytes per word)
                    here we need to swap back to
                    big endian format */
                uint16_t* c = memory + reg[R_R0];
                while (*c)
                {
                    char char1 = (*c) & 0xFF;
                    putc(char1, stdout);
                    char char2 = (*c) >> 8;
                    if (char2) putc(char2, stdout);
                    ++c;
                }
                fflush(stdout);
            }
            break;
        case TRAP_HALT:
            puts("HALT");
            fflush(stdout);
            *running = 0;
            break;
    }
}

void read_image_file( FILE* file)
{
    /* the origin tells us where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size, so we need only one fread */
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    while(read--)
    {
        *p = swap16(*p);
        p++;
    }
}

int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file)
        return 0;

    read_image_file(file);
    fclose(file);
    return 1;
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        /* show usage string */
        printf("lc3 [ image-file]....\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("Failed to load image : %s\n", argv[j]);
            exit(1);
        }
    }
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    reg[R_COND] = FL_ZRO;

    enum { PC_START = 0x3000 };

    reg[R_PC] = PC_START;

    int running = 1;

    while(running)
    {
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;
        printf("instr : %d\n", op);
        switch (op)
        {
            case OP_BR:
                vm_Br(instr);
                break;
            case OP_ADD:
                vm_Add(instr);
                break;
            case OP_LD:
                vm_LD(instr);
                break;
            case OP_ST:
                vm_ST(instr);
                break;
            case OP_JSR:
                vm_JmpReg(instr);
                break;
            case OP_AND:
                vm_And(instr);
                break;
            case OP_LDR:
                vm_LDR(instr);
                break;
            case OP_STR:
                vm_STR(instr);
                break;
            case OP_RTI:
                //@ {};
                break;
            case OP_NOT:
                vm_Not(instr);
                break;
            case OP_LDI:
                vm_LDI(instr);
                break;
            case OP_STI:
                vm_STI(instr);
                break;
            case OP_JMP:
                vm_Jmp(instr);
                break;
            case OP_RES:
                //@ {};
                break;
            case OP_LEA:
                vm_LDEA(instr);
                break;
            case OP_TRAP:
                vm_TRAP(instr, &running);
                break;
            default:
                //@ {Bad opcode}
                break;
        }
    }
    restore_input_buffering();
}