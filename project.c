
#include "spimcore.h"

#define MEMSIZE (65536 >> 2)

/* ALU */
/* 10 Points */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{
    switch (ALUControl)
    {
        case 0: // Addition
            *ALUresult = A + B;
            break;
        case 1: // Subtraction
            *ALUresult = A - B;
            break;
        case 2: // Set on Less Than (signed)
            *ALUresult = (int)A < (int)B;
            break;
        case 3: // Set on Less Than Unsigned
            *ALUresult = A < B;
            break;
        case 4: // AND
            *ALUresult = A & B;
            break;
        case 5: // OR
            *ALUresult = A | B;
            break;
        case 6: // Shift Left 16
            *ALUresult = B << 16;
            break;
        case 7: // NOT
            *ALUresult = ~A;
            break;
    }
    *Zero = (*ALUresult == 0);
}

/* instruction fetch */
/* 10 Points */
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{
    if (PC % 4 != 0 || PC >= MEMSIZE * 4) // Check alignment and bounds
        return 1; // Halt condition
    *instruction = MEM(PC);
    return 0;
}

/* instruction partition */
/* 10 Points */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
    *op = (instruction >> 26) & 0x3F;       // Bits [31-26]
    *r1 = (instruction >> 21) & 0x1F;       // Bits [25-21]
    *r2 = (instruction >> 16) & 0x1F;       // Bits [20-16]
    *r3 = (instruction >> 11) & 0x1F;       // Bits [15-11]
    *funct = instruction & 0x3F;            // Bits [5-0]
    *offset = instruction & 0xFFFF;         // Bits [15-0]
    *jsec = instruction & 0x3FFFFFF;        // Bits [25-0]
}

/* instruction decode */
/* 15 Points */
int instruction_decode(unsigned op,struct_controls *controls)
{
    memset(controls, 0, sizeof(struct_controls));
    switch (op)
    {
        case 0x00: // R-type
            controls->RegDst = 1;
            controls->ALUOp = 7;
            controls->RegWrite = 1;
            break;
        case 0x08: // ADDI
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            break;
        case 0x23: // LW
            controls->ALUSrc = 1;
            controls->MemtoReg = 1;
            controls->RegWrite = 1;
            controls->MemRead = 1;
            break;
        case 0x2B: // SW
            controls->ALUSrc = 1;
            controls->MemWrite = 1;
            break;
        case 0x04: // BEQ
            controls->Branch = 1;
            controls->ALUOp = 1;
            break;
        case 0x02: // JUMP
            controls->Jump = 1;
            break;
        default: // Illegal instruction
            return 1; // Halt
    }
    return 0;
}

/* Read Register */
/* 5 Points */
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}

/* Sign Extend */
/* 10 Points */
void sign_extend(unsigned offset,unsigned *extended_value)
{
    if (offset & 0x8000) // Check if sign bit is set
        *extended_value = offset | 0xFFFF0000; // Sign-extend to 32 bits
    else
        *extended_value = offset & 0x0000FFFF;
}

/* ALU operations */
/* 10 Points */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
    unsigned B = (ALUSrc) ? extended_value : data2;
    if (ALUOp == 7) // R-type, use funct field
    {
        switch (funct)
        {
            case 32: // ADD
                ALU(data1, B, 0, ALUresult, Zero);
                break;
            case 34: // SUB
                ALU(data1, B, 1, ALUresult, Zero);
                break;
            case 42: // SLT
                ALU(data1, B, 2, ALUresult, Zero);
                break;
            case 36: // AND
                ALU(data1, B, 4, ALUresult, Zero);
                break;
            case 37: // OR
                ALU(data1, B, 5, ALUresult, Zero);
                break;
            default: // Unsupported funct
                return 1; // Halt
        }
    }
    else // I-type
    {
        ALU(data1, B, ALUOp, ALUresult, Zero);
    }
    return 0;
}

/* Read / Write Memory */
/* 10 Points */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
    if (ALUresult % 4 != 0 || ALUresult >= MEMSIZE * 4) // Check alignment and bounds
        return 1; // Halt
    if (MemRead)
        *memdata = MEM(ALUresult);
    if (MemWrite)
        MEM(ALUresult) = data2;
    return 0;
}

/* Write Register */
/* 10 Points */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
    if (!RegWrite)
        return;
    unsigned write_data = (MemtoReg) ? memdata : ALUresult;
    unsigned reg_num = (RegDst) ? r3 : r2;
    Reg[reg_num] = write_data;
}

/* PC update */
/* 10 Points */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
    *PC += 4; // Default PC increment
    if (Jump)
        *PC = (jsec << 2) | (*PC & 0xF0000000);
    if (Branch && Zero)
        *PC += extended_value << 2;
}


