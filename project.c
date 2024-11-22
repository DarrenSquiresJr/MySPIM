#include "spimcore.h"

#define MEMSIZE (65536 >> 2) // Define MEMSIZE explicitly here

/* ALU */
void ALU(unsigned A, unsigned B, char ALUControl, unsigned *ALUresult, char *Zero) {
    switch (ALUControl) {
        case 0: *ALUresult = A + B; break;               // Add
        case 1: *ALUresult = A - B; break;               // Subtract
        case 2: *ALUresult = (A < B) ? 1 : 0; break;     // Set less than (signed)
        case 3: *ALUresult = (A < B) ? 1 : 0; break;     // Set less than (unsigned)
        case 4: *ALUresult = A & B; break;               // AND
        case 5: *ALUresult = A | B; break;               // OR
        case 6: *ALUresult = B << 16; break;             // Shift left 16
        case 7: *ALUresult = ~A; break;                  // NOT
        default: *ALUresult = 0; break;
    }
    *Zero = (*ALUresult == 0);
}

/* Instruction Fetch */
int instruction_fetch(unsigned PC, unsigned *Mem, unsigned *instruction) {
    if (PC % 4 != 0 || PC >= MEMSIZE * 4) return 1;  // Halt if PC is misaligned or out of bounds
    *instruction = MEM(PC);
    return 0;  // Continue execution
}

/* Instruction Partition */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1, unsigned *r2,
                           unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec) {
    *op = (instruction >> 26) & 0x3F;
    *r1 = (instruction >> 21) & 0x1F;
    *r2 = (instruction >> 16) & 0x1F;
    *r3 = (instruction >> 11) & 0x1F;
    *funct = instruction & 0x3F;
    *offset = instruction & 0xFFFF;
    *jsec = instruction & 0x3FFFFFF;
}

/* Instruction Decode */
int instruction_decode(unsigned op, struct_controls *controls) {
    memset(controls, 0, sizeof(struct_controls));  // Default all control signals to 0

    switch (op) {
        case 0:  // R-Type
            controls->RegDst = 1;
            controls->RegWrite = 1;
            controls->ALUOp = 7;
            break;
        case 8:  // ADDI
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            break;
        case 35:  // LW
            controls->ALUSrc = 1;
            controls->MemtoReg = 1;
            controls->RegWrite = 1;
            controls->MemRead = 1;
            break;
        case 43:  // SW
            controls->ALUSrc = 1;
            controls->MemWrite = 1;
            break;
        case 4:  // BEQ
            controls->Branch = 1;
            controls->ALUOp = 1;
            break;
        case 2:  // J
            controls->Jump = 1;
            break;
        default:
            return 1;  // Halt on unsupported opcode
    }
    return 0;  // Continue execution
}

/* Read Register */
void read_register(unsigned r1, unsigned r2, unsigned *Reg, unsigned *data1, unsigned *data2) {
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}

/* Sign Extend */
void sign_extend(unsigned offset, unsigned *extended_value) {
    if (offset & 0x8000)  // If the sign bit is set
        *extended_value = offset | 0xFFFF0000;
    else
        *extended_value = offset & 0x0000FFFF;
}

/* ALU Operations */
int ALU_operations(unsigned data1, unsigned data2, unsigned extended_value, unsigned funct,
                   char ALUOp, char ALUSrc, unsigned *ALUresult, char *Zero) {
    unsigned B = (ALUSrc) ? extended_value : data2;
    char control = ALUOp;

    if (ALUOp == 7) {  // R-Type
        switch (funct) {
            case 32: control = 0; break;  // ADD
            case 34: control = 1; break;  // SUB
            case 42: control = 2; break;  // SLT
            case 43: control = 3; break;  // SLTU
            case 36: control = 4; break;  // AND
            case 37: control = 5; break;  // OR
            default: return 1;            // Halt on unsupported funct
        }
    }

    ALU(data1, B, control, ALUresult, Zero);
    return 0;  // Continue execution
}

/* Read/Write Memory */
int rw_memory(unsigned ALUresult, unsigned data2, char MemWrite, char MemRead, unsigned *memdata,
              unsigned *Mem) {
    if (MemRead) {
        if (ALUresult % 4 != 0 || ALUresult >= MEMSIZE * 4) return 1;  // Halt on invalid memory access
        *memdata = MEM(ALUresult);
    }
    if (MemWrite) {
        if (ALUresult % 4 != 0 || ALUresult >= MEMSIZE * 4) return 1;  // Halt on invalid memory access
        MEM(ALUresult) = data2;
    }
    return 0;  // Continue execution
}

/* Write Register */
void write_register(unsigned r2, unsigned r3, unsigned memdata, unsigned ALUresult, char RegWrite,
                    char RegDst, char MemtoReg, unsigned *Reg) {
    if (RegWrite) {
        unsigned dest = (RegDst) ? r3 : r2;
        unsigned data = (MemtoReg) ? memdata : ALUresult;
        Reg[dest] = data;
    }
}

/* PC Update */
void PC_update(unsigned jsec, unsigned extended_value, char Branch, char Jump, char Zero,
               unsigned *PC) {
    *PC += 4;
    if (Branch && Zero) *PC += (extended_value << 2);
    if (Jump) *PC = (jsec << 2);
}
