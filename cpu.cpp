#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <windows.h>

using namespace std;

using byte = unsigned char;
using sbyte = signed char;
using word = unsigned short;
using u32 = unsigned int;


struct Memory {
    static const u32 MAX_MEM = 1024 * 64 + 1;
    byte data[MAX_MEM] = { 0 };

    void init() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            data[i] = 0;
        }
    }

    int loadROM(const char* fileName) {
        FILE* pFile;
        fopen_s(&pFile, fileName, "rb");
        fseek(pFile, 0L, SEEK_END);
        static const size_t size = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);
        if (pFile != NULL) {
            for (int i = 0x0; i < 0x10; i++) {
                fgetc(pFile);
            }
            int i = 0x0;
            do {
                data[0x8000 + i] = data[0xC000 + i] = fgetc(pFile);
                i++;
            } while (0xC000 + i < 0xFFFF);
            fclose(pFile);
        }
        return 0;
    }

    void dumpMem(word start, word end) {
        start = start - (start % 16);
        end = end + (15 - (end % 16));
        for (int addr = start; addr <= end; addr++) {
            if (addr % 16 == 0) {
                printf("\n$%04X  ", addr);
            }
            printf("%02X  ", data[addr]);
        }
        printf("\n");
    }

    //read or write byte
    byte& operator[](word address) {
        return data[address];
    }
};

struct CPU {

    word PC;        //program counter
    byte SP;        //stack pointer
    byte AC;        //accumulator
    byte X;         //index register x
    byte Y;         //index register y
    byte C : 1;     //carry flag
    byte Z : 1;     //zero flag
    byte I : 1;     //interupt disable
    byte D : 1;     //decimal mode
    byte B : 1;     //break command
    byte V : 1;     //overflow flag
    byte N : 1;     //negative flag

                          
    //high byte is instruction, mid byte is addressing mode, low byte is the number of cycles
    //FF**** - invalid opcode
    //**FE** - implied addressing mode
    //                                 0         1         2         3         4         5         6         7          8        9         A          B         C        D          E        F
    const u32 opcodeTable[256] = {  0x0AFE07, 0x220606, 0xFFFFFF, 0x3E0608, 0x210903, 0x220903, 0x020905, 0x3E0905, 0x24FE03, 0x220402, 0x020002, 0xFFFFFF, 0x210104, 0x220104, 0x020106, 0x3E0106,
                                    0x090802, 0x220705, 0xFFFFFF, 0x3E0F08, 0x210A04, 0x220A04, 0x020A06, 0x3E0A06, 0x0DFE02, 0x220304, 0x21FE02, 0x3E0E07, 0x210204, 0x220204, 0x020D07, 0x3E0D07,
                                    0x1C0106, 0x010606, 0xFFFFFF, 0x3B0608, 0x060903, 0x010903, 0x270905, 0x3B0905, 0x26FE04, 0x010402, 0x270002, 0xFFFFFF, 0x060104, 0x010104, 0x270106, 0x3B0106,
                                    0x070802, 0x010705, 0xFFFFFF, 0x3B0F08, 0x210A04, 0x010A04, 0x270A06, 0x3B0A06, 0x2CFE02, 0x010304, 0x21FE02, 0x3B0E07, 0x210204, 0x010204, 0x270D07, 0x3B0D07,
                                    0x29FE06, 0x170606, 0xFFFFFF, 0x3F0608, 0x210903, 0x170903, 0x200905, 0x3F0905, 0x23FE03, 0x170402, 0x200002, 0xFFFFFF, 0x1B0103, 0x170104, 0x200106, 0x3F0106,
                                    0x0B0802, 0x170705, 0xFFFFFF, 0x3F0F08, 0x210A04, 0x170A04, 0x200A06, 0x3F0A06, 0x0FFE02, 0x170304, 0x21FE02, 0x3F0E07, 0x210204, 0x170204, 0x200D07, 0x3F0D07,
                                    0x2AFE06, 0x000606, 0xFFFFFF, 0x3C0608, 0x210903, 0x000903, 0x280905, 0x3C0905, 0x25FE04, 0x000402, 0x280002, 0xFFFFFF, 0x1B0C05, 0x000104, 0x280106, 0x3C0106,
                                    0x0C0802, 0x000705, 0xFFFFFF, 0x3C0F08, 0x210A04, 0x000A04, 0x280A06, 0x3C0A06, 0x2EFE02, 0x000304, 0x21FE02, 0x3C0E07, 0x210204, 0x000204, 0x280D07, 0x3C0D07,
                                    0x210402, 0x2F0606, 0x210402, 0x3D0606, 0x310903, 0x2F0903, 0x300903, 0x3D0903, 0x16FE02, 0x210402, 0x35FE02, 0xFFFFFF, 0x310104, 0x2F0104, 0x300104, 0x3D0104,
                                    0x030802, 0x2F0706, 0xFFFFFF, 0xFFFFFF, 0x310A04, 0x2F0A04, 0x300B04, 0x3D0B04, 0x37FE02, 0x2F0E05, 0x36FE02, 0xFFFFFF, 0xFFFFFF, 0x2F0D05, 0xFFFFFF, 0xFFFFFF,
                                    0x1F0402, 0x1D0606, 0x1E0402, 0x3A0606, 0x1F0903, 0x1D0903, 0x1E0903, 0x3A0903, 0x33FE02, 0x1D0402, 0x32FE02, 0xFFFFFF, 0x1F0104, 0x1D0104, 0x1E0104, 0x3A0104,
                                    0x040802, 0x1D0705, 0xFFFFFF, 0x3A0705, 0x1F0A04, 0x1D0A04, 0x1E0B04, 0x3A0B04, 0x10FE02, 0x1D0304, 0x34FE02, 0xFFFFFF, 0x1F0204, 0x1D0204, 0x1E0304, 0x3A0304,
                                    0x130402, 0x110606, 0x210402, 0x380608, 0x130903, 0x110903, 0x140905, 0x380905, 0x1AFE02, 0x110402, 0x15FE02, 0xFFFFFF, 0x130104, 0x110104, 0x140106, 0x380106,
                                    0x080802, 0x110705, 0xFFFFFF, 0x380F08, 0x210A04, 0x110A04, 0x140A06, 0x380A06, 0x0EFE02, 0x110304, 0x21FE02, 0x380E07, 0x210204, 0x110204, 0x140D07, 0x380D07,
                                    0x120402, 0x2B0606, 0x210402, 0x390608, 0x120903, 0x2B0903, 0x180905, 0x390905, 0x19FE02, 0x2B0402, 0x21FE02, 0x400402, 0x120104, 0x2B0104, 0x180106, 0x390106,
                                    0x050802, 0x2B0705, 0xFFFFFF, 0x390F08, 0x210A04, 0x2B0A04, 0x180A06, 0x390A06, 0x2DFE02, 0x2B0304, 0x21FE02, 0x390E07, 0x210204, 0x2B0204, 0x180D07, 0x390D07};
    //DEBUG
    void dumpReg() {
        printf("\n---------------\nPC = 0x%04X\n\n", PC);
        printf("AC = 0x%02X  ", AC);
        printf("SP = 0x%02X\n", SP);
        printf("X  = 0x%02X  ", X);
        printf("Y  = 0x%02X\n\n", Y);
        printf("P  = 0x%02X\n", getStatusReg());
        printf("C = %d     ", C);
        printf("Z = %d\n", Z);
        printf("I = %d     ", I);
        printf("D = %d\n", D);
        printf("B = %d     ", B);
        printf("V = %d\n", V);
        printf("N = %d\n", N);
        printf("---------------\n");
    }

    void briefStatus() {
        printf(" A:%02X X:%02X Y:%02X P:%02X SP:%02X ", AC, X, Y, getStatusReg(), SP);
    }

    //resets CPU and memory
    void reset(Memory& mem) {
        PC = readByte(mem, 0xFFFC) | (readByte(mem, 0xFFFD) << 8);
        SP = 0xFD;
        C = Z = D = B = V = N = 0;
        I = 1;
        AC = X = Y = 0;
        mem.init();
    }
    //READ AND WRITES
    // 
    // 
    //returns byte at PC in 1 cycle, increments PC
    byte fetchByte(Memory& mem) {
        byte value = mem[PC];
        PC++;
        return value;
    }

    //returns 2 bytes at PC in 2 cycle, increments PC +2
    word fetchWord(Memory& mem) {
        byte low = mem[PC];
        byte high = mem[PC + 1];
        PC += 2;
        return low | (high << 8);
    }

    //returns byte at address in 1 cycle
    byte readByte(Memory& mem, word address) {
        return mem[address];
    }

    //writes byte at address in 1 cycle
    void writeByte(Memory& mem, word address, byte value) {
        mem[address] = value;
    }

    //returns 2 bytes at address in 2 cycles
    word readWord(Memory& mem, word address) {
        byte low = mem[address];
        byte high = mem[address + 1];
        return low | (high << 8);
    }

    //writes 2 bytes at address in 2 cycles
    void writeWord(Memory& mem, word address, word value) {
        mem[address] = value & 0xFF;
        mem[address + 1] = value >> 8;
    }

    void pushByte(Memory& mem, byte value) {
        writeByte(mem, 0x0100 | SP--, value);
    }

    void pushWord(Memory& mem, word value) {
        pushByte(mem, value >> 8);
        pushByte(mem, value & 0xFF);
    }

    byte pullByte(Memory& mem) {
        return readByte(mem, 0x0100 | ++SP);
    }

    word pullWord(Memory& mem) {
        byte low = pullByte(mem);
        byte high = pullByte(mem);
        return low | (high << 8);
    }


    //HELPER FUNCTIONS

    void updateZNFlags(byte reg) {
        Z = (reg == 0);
        N = (reg & 0b10000000) > 0;
    }

    byte getStatusReg() {
        byte sr = 0;;
        sr += N;
        sr = sr << 1;
        sr += V;
        sr = sr << 1;
        sr += 1;
        sr = sr << 1;
        sr += B;
        sr = sr << 1;
        sr += D;
        sr = sr << 1;
        sr += I;
        sr = sr << 1;
        sr += Z;
        sr = sr << 1;
        sr += C;
        return sr;
    }

    void setStatusReg(byte reg) {
        C = (reg & 0b00000001) > 0;
        Z = (reg & 0b00000010) > 0;
        I = (reg & 0b00000100) > 0;
        D = (reg & 0b00001000) > 0;
        //B = (reg & 0b00100000) > 0;
        V = (reg & 0b01000000) > 0;
        N = (reg & 0b10000000) > 0;
    }
    //ADDRESSING MODE FUNCTIONS --------------------------------------------------

    word accumulator(Memory& mem, u32& cycles) {
        return AC;
    }

    word absolute(Memory& mem, u32& cycles) {
        return fetchWord(mem);
    }

    word absoluteX(Memory& mem, u32& cycles) {
        word address = fetchWord(mem);
        cycles += ((address & 0xFF) > 0xFF - X);   
        address = address + X;
        return address;
    }

    word absoluteY(Memory& mem, u32& cycles) {
        word address = fetchWord(mem);
        cycles += ((address & 0xFF) > 0xFF - Y);
        address = address + Y;
        return address;
    }

    word immediate(Memory& mem, u32& cycles) {
        return PC++;
    }

    word indirect(Memory& mem, u32& cycles) {
        word address = fetchWord(mem);
        printf("[INDIRECT DEBUG] address1 = %04X\n", address);
        printf("[INDIRECT DEBUG] address2 = %04X\n", readWord(mem, address));
        return readWord(mem, address);
    }

    word Xindirect(Memory& mem, u32& cycles) {
        word address = (fetchByte(mem) + X) & 0xFF;
        printf("[XINDIRECT DEBUG] ind. address = %04X\n", address);
        byte low = readByte(mem, address);
        byte high = readByte(mem, (address + 1) & 0xFF);
        printf("[XINDIRECT DEBUG] eff. address = %04X\n", low | (high << 8));
        return low | (high << 8);
    }

    word indirectY(Memory& mem, u32& cycles) {
        word address = fetchByte(mem);
        cycles += ((readWord(mem, address) & 0xFF) > 0xFF - Y);
        printf("[INDIRECT Y DEBUG] ind. address = %04X\n", address);
        byte low = readByte(mem, address);
        byte high = readByte(mem, (address + 1) & 0xFF);
        word effAddress = (low | (high << 8)) + Y;
        printf("[INDIRECT Y DEBUG] eff. address = %04X\n", readWord(mem, address) + Y);
        return effAddress;
    }

    word relative(Memory& mem, u32& cycles) {
        sbyte offset = (sbyte)(fetchByte(mem));
        return PC + offset;
    }

    word zeropage(Memory& mem, u32& cycles) {
        return fetchByte(mem);
    }

    word zeropageX(Memory& mem, u32& cycles) {
        word address = fetchByte(mem);
        return (address + X) & 0xFF;
    }

    word zeropageY(Memory& mem, u32& cycles) {
        word address = fetchByte(mem);
        return (address + Y) & 0xFF;
    }

    //varitions for certain instructions

    word jmpIndirect(Memory& mem, u32& cycles) {
        word address = fetchWord(mem);
        byte low = readByte(mem, address);
        byte high = readByte(mem, (address + 1));
        //if over page special condition
        if ((address & 0x00FF) == 0x00FF) {
            printf("[JMP INDIRECT DEBUG] Page crossed.\n");
            printf("[JMP INDIRECT DEBUG] replacing high = %02X\n", high);
            high = readByte(mem, (address - 0xFF));
            printf("[JMP INDIRECT DEBUG] replacing high = %02X ($%04X)\n", high, address - 0xFF);
        }
        word eAddress = (low | (high << 8));
        printf("[JMP INDIRECT DEBUG] high = %02X, low = %02X\n", high, low);
        printf("[JMP INDIRECT DEBUG] address1 = %04X\n", address);
        printf("[JMP INDIRECT DEBUG] address2 = %04X\n", eAddress);
        return eAddress;
    }

    word absoluteXStaticCyc(Memory& mem, u32& cycles) {
        word address = fetchWord(mem);
        address = address + X;
        return address;
    }

    word absoluteYStaticCyc(Memory& mem, u32& cycles) {
        word address = fetchWord(mem);
        address = address + Y;
        return address;
    }

    word indirectYStaticCyc(Memory& mem, u32& cycles) {
        word address = fetchByte(mem);
        printf("[INDIRECT Y DEBUG] ind. address = %04X\n", address);
        byte low = readByte(mem, address);
        byte high = readByte(mem, (address + 1) & 0xFF);
        word effAddress = (low | (high << 8)) + Y;
        printf("[INDIRECT Y DEBUG] eff. address = %04X\n", readWord(mem, address) + Y);
        return effAddress;
    }


    //acc:00, abs:01, abx*:02, aby*:03, imm:04, ind:05, indx:06, yind*:07, 
    //rel:08, zpg:09, zpx:0A, zpy:0B, jind:0C, absSX:0D, absSY:0E
    typedef word (CPU::*addrFunctionPointer)(Memory&, u32&);
    addrFunctionPointer addrPointers[16] = {&CPU::accumulator, &CPU::absolute, &CPU::absoluteX, &CPU::absoluteY,
                                                    &CPU::immediate, &CPU::indirect, &CPU::Xindirect, &CPU::indirectY,
                                                    &CPU::relative, &CPU::zeropage, &CPU::zeropageX, &CPU::zeropageY, 
                                                    &CPU::jmpIndirect, &CPU::absoluteXStaticCyc, &CPU::absoluteYStaticCyc,
                                                    &CPU::indirectYStaticCyc};

    //--------------------------------------------------------------------------------
    //INSTRUCTIONS
    void ADC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        byte sum = AC + value + C;
        word sum2 = AC + value + C;
        C = (sum2 > 0xFF);
        V = ((AC < 0 && value < 0 && sum >= 0) || (AC >= 0 && value >= 0 && sum < 0) || ((sum ^ AC) & (sum ^ value)) & 0x80);
        AC = sum;
        updateZNFlags(AC);
    }

    void AND(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        AC = AC & readByte(mem, address);
        updateZNFlags(AC);
    }

    void ASL(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (!address) {  //AC mode
            C = (AC & 0b10000000) > 0;
            AC = AC << 1;
            updateZNFlags(AC);
        }
        else {          //memory mode
            byte value = readByte(mem, address);
            C = (value & 0b10000000) > 0;
            writeByte(mem, address, value << 1);
            updateZNFlags(value << 1);
        }
    }

    void BCC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (!C) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }

    void BCS(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (C) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }

    void BEQ(Memory& mem, word address, u32& cycles) {
        printf("BEQ DEBUG CYC %d\n", cycles);
        printf("Instruction %s\n", __func__);
        if (Z) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
        
    }

    void BIT(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        Z = !((AC & value) > 0);
        N = (value & 0b10000000) > 0;
        V = (value & 0b01000000) > 0;
    }

    void BMI(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (N) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }
    
    void BNE(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (!Z) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }

    void BPL(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (!N) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }

    void BRK(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        I = 1;
        pushWord(mem, PC + 2);
        pushByte(mem, getStatusReg());
        PC = readByte(mem, 0xFFFE) | (readByte(mem, 0xFFFF) << 8);
    }

    void BVC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (!V) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }

    void BVS(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (V) {
            cycles++;
            if ((PC & 0xFF00) != (address & 0xFF00)) {
                cycles++;
            }
            PC = address;
        }
    }

    void CLC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        C = 0;
    }

    void CLD(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        D = 0;
    }

    void CLI(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        I = 0;
    }

    void CLV(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        V = 0;
    }

    void CMP(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = AC - readByte(mem, address);
        updateZNFlags(value);
        C = value <= AC;
    }

    void CPX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = X - readByte(mem, address);
        updateZNFlags(value);
        C = value <= X;
    }

    void CPY(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = Y - readByte(mem, address);
        updateZNFlags(value);
        C = value <= Y;
    }

    void DEC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        value--;
        writeByte(mem, address, value);
        updateZNFlags(value);
    }
        
    void DEX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        X--;
        updateZNFlags(X);
    }

    void DEY(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        Y--;
        updateZNFlags(Y);
    }

    void EOR(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        AC = value ^ AC;
        updateZNFlags(AC);
    }

    void INC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        value++;
        writeByte(mem, address, value);
        updateZNFlags(value);
    }

    void INX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        X++;
        updateZNFlags(X);
    }

    void INY(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        Y++;
        updateZNFlags(Y);
    }

    void JMP(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        printf("[JMP DEBUG] address = %04X", address);
        PC = address;
    }

    void JSR(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        pushWord(mem, PC - 1);
        PC = address;
    }

    void LDA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        printf("[LDA DEBUG] address = %04X\n", address);
        AC = readByte(mem, address);
        updateZNFlags(AC);
    }

    void LDX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        X = readByte(mem, address);
        updateZNFlags(X);
    }

    void LDY(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        Y = readByte(mem, address);
        updateZNFlags(Y);
    }

    void LSR(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        if (!address) {  //AC mode
            C = (AC & 0b00000001) > 0;
            AC = AC >> 1;
            updateZNFlags(AC);
        }
        else {          //memory mode
            byte value = readByte(mem, address);
            C = (value & 0b00000001) > 0;
            writeByte(mem, address, value >> 1);
            updateZNFlags(value >> 1);
        }
    }

    void NOP(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
    }

    void ORA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        AC = value | AC;
        updateZNFlags(AC);
    }

    void PHA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        pushByte(mem, AC);
    }

    void PHP(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte temp = B;
        B = 1;
        byte sp = getStatusReg();
        pushByte(mem, sp);
        B = temp;
    }

    void PLA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        AC = pullByte(mem);
        updateZNFlags(AC);
    }

    void PLP(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte sp = pullByte(mem);
        setStatusReg(sp);
    }

    void ROL(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte C_old = C;
        if (!address) {  //AC mode
            C = (AC & 0b10000000) > 0;
            AC = (AC << 1) | C_old;
            updateZNFlags(AC);
        }
        else {          //memory mode
            byte value = readByte(mem, address);
            C = (value & 0b10000000) > 0;
            value = (value << 1) | C_old;
            updateZNFlags(value);
            writeByte(mem, address, value);
        }
    }

    void ROR(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte C_old = C;
        if (!address) {  //AC mode
            C = (AC & 1) > 0;
            AC = (AC >> 1) | (C_old << 7);
            updateZNFlags(AC);
        }
        else {          //memory mode
            byte value = readByte(mem, address);
            C = (value & 1) > 0;
            value = (value >> 1) | (C_old << 7);
            updateZNFlags(value);
            writeByte(mem, address, value);
        }
    }

    void RTI(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        setStatusReg(pullByte(mem));
        PC = pullWord(mem);
    }

    void RTS(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        PC = pullWord(mem) + 1;
    }

    void SBC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        byte value = readByte(mem, address);
        byte diff = AC - value - !C;
        C = (AC >= value + !C);
        V = ((AC ^ diff) & 0x80) && ((AC ^ value) & 0x80);
        AC = diff & 0xFF;
        updateZNFlags(AC);
    }

    void SEC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        C = 1;
    }

    void SED(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        D = 1;
    }

    void SEI(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        I = 1;
    }

    void STA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        writeByte(mem, address, AC);
    }

    void STX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        writeByte(mem, address, X);
    }

    void STY(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        writeByte(mem, address, Y);
    }

    void TAX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        X = AC;
        updateZNFlags(X);
    }

    void TAY(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s", __func__);
        Y = AC;
        updateZNFlags(Y);
    }

    void TSX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s", __func__);
        X = SP;
        updateZNFlags(X);
    }

    void TXA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s", __func__);
        AC = X;
        updateZNFlags(AC);
    }

    void TXS(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s", __func__);
        SP = X;
    }

    void TYA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s", __func__);
        AC = Y;
        updateZNFlags(AC);
    }

    //ILLEGAL OPCODES

    void DCP(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        DEC(mem, address, cycles);
        CMP(mem, address, cycles);
    }

    void ISB(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        INC(mem, address, cycles);
        SBC(mem, address, cycles);
    }

    void LAX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        LDA(mem, address, cycles);
        LDX(mem, address, cycles);
    }

    void RLA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        ROL(mem, address, cycles);
        AND(mem, address, cycles);
    }

    void RRA(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        ROR(mem, address, cycles);
        ADC(mem, address, cycles);
    }

    void SAX(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        writeByte(mem, address, AC & X);
    }

    void SLO(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        ASL(mem, address, cycles);
        ORA(mem, address, cycles);
    }

    void SRE(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        LSR(mem, address, cycles);
        EOR(mem, address, cycles);
    }

    void USBC(Memory& mem, word address, u32& cycles) {
        printf("Instruction %s\n", __func__);
        return SBC(mem, address, cycles);
    }


    //ADC:00, AND:01, ASL:02, BCC:03, BCS:04, BEQ:05, BIT:06, BMI:07, BNE:08, BPL:09, BRK:0A, BVC:0B, BVS:0C, CLC:0D, CLD:0E, CLI:0F
    //CLV:10, CMP:11, CPX:12, CPY:13, DEC:14, DEX,15, DEY:16, EOR:17, INC:18, INX:19, too lazy to keep going

    //Illegal Opcodes
    //DCP:38, ISB:39, LAX:3A, RLA:3B, RRA:3C, SAX:3D, SLO:3E, SRE:3F, USBC:40, *NOP:23

    typedef void (CPU::*insFunctionPointer)(Memory&, word, u32&);
    insFunctionPointer insPointers[65] = {&CPU::ADC, &CPU::AND, &CPU::ASL, &CPU::BCC,
                                                                   &CPU::BCS, &CPU::BEQ, &CPU::BIT, &CPU::BMI,
                                                                   &CPU::BNE, &CPU::BPL, &CPU::BRK, &CPU::BVC,
                                                                   &CPU::BVS, &CPU::CLC, &CPU::CLD, &CPU::CLI,
                                                                   &CPU::CLV, &CPU::CMP, &CPU::CPX, &CPU::CPY,
                                                                   &CPU::DEC, &CPU::DEX, &CPU::DEY, &CPU::EOR,
                                                                   &CPU::INC, &CPU::INX, &CPU::INY, &CPU::JMP,
                                                                   &CPU::JSR, &CPU::LDA, &CPU::LDX, &CPU::LDY,
                                                                   &CPU::LSR, &CPU::NOP, &CPU::ORA, &CPU::PHA,
                                                                   &CPU::PHP, &CPU::PLA, &CPU::PLP, &CPU::ROL,
                                                                   &CPU::ROR, &CPU::RTI, &CPU::RTS, &CPU::SBC,
                                                                   &CPU::SEC, &CPU::SED, &CPU::SEI, &CPU::STA,
                                                                   &CPU::STX, &CPU::STY, &CPU::TAX, &CPU::TAY,
                                                                   &CPU::TSX, &CPU::TXA, &CPU::TXS, &CPU::TYA,
                                                                   &CPU::DCP, &CPU::ISB, &CPU::LAX, &CPU::RLA,
                                                                   &CPU::RRA, &CPU::SAX, &CPU::SLO, &CPU::SRE,
                                                                   &CPU::USBC};


    //EXECUTE--------------------------------------------------------------------------------
    u32 step(Memory& mem) {
        byte opcode = fetchByte(mem);

        //obtain number of cycles, address mode and type of instruction
        u32 cycles = opcodeTable[opcode] & 0xFF;
        byte addressMode = opcodeTable[opcode] >> 8;
        byte instruction = opcodeTable[opcode] >> 16;

        word eAddress;
        if (addressMode == 0xFE) {            //implied address
            eAddress = NULL;
        }
        else if (addressMode == 0x00) {        //accumulator
            eAddress = NULL;
        }
        else if (addressMode == 0xFF) {     //invalid opcode
            printf("Invalid opcode: %02X\n\n", opcode);
            return 0;
        }
        else {                              //get address from other mode
            eAddress = (this->*addrPointers[addressMode])(mem, cycles);
        }

        //execute instruction with effective address
        (this->*insPointers[instruction])(mem, eAddress, cycles);

        //debug
        printf("Opcode: %02X,   Cycles: %02X,   Address Mode: %02X,   Instruction: %02X,    Effective Address: %04X\n",
            opcode, cycles, addressMode, instruction, eAddress);

        return cycles;
    }

};

int main() {
    CPU cpu;
    Memory mem;

    //mem[0xFFFC] = 0x20;
    //mem[0xFFFD] = 0x40;
    cpu.reset(mem);
    cpu.PC = 0xC000;
    mem.loadROM("nestest.nes");
    cpu.dumpReg();

    //mem.dumpMem(0x0000, 0xFFFF);

    int i = 0;
    int cycles = 7;
    while (i < 1000000) {
        printf("===============================================\n");
        printf("%02X  ", cpu.PC);
        cpu.briefStatus();
        printf("CYC: %d\n", cycles);
        cycles += cpu.step(mem);
        if (cycles >= 26561) {
            break;
        }
        //cpu.dumpReg();
        i++;
        if (cycles > 26500) {
            system("pause");
        }
    }


    //end inline program
    //mem.dumpMem(0x0100, 0x01FF);    //view stack
    cpu.dumpReg();
    printf("Error code: ");
    mem.dumpMem(0x0, 0x08);
    system("pause");
}

//good log for nestest https://www.qmtpro.com/~nes/misc/nestest.log
//Successfully made it to 14000 something, encountered illegal opcodes and various NOP, 
//just need to work out cycles and the few illegal opcodes


//Illegal opcodes to implement:
// op   add   cyc
//DCP:38, ISB:39, LAX:3A, RLA:3B, 
//RRA:3C, SAX:3D, SLO:3E, SRE:3F, 
//USBC:40, *NOP:23
// 
// address modes
//acc:00, abs:01, abx*:02, aby*:03, 
//imm:04, ind:05, indx:06, yind*:07, 
//rel:08, zpg:09, zpx:0A, zpy:0B, 
//jind:0C, absSX:0D, absSY:0E, yindS:0F
// ! - means done
//DCP code 38
// C7 - zp - 5!
// D7 - zpx - 6!
// CF - abs - 6!
// DF - absx - 7!
// DB - absy - 7!
// C3 - indx - 8!
// D3 - yind - 8!

//ISB code 39
// E7 - zp - 5!
// F7 - zpx - 6!
// EF - abs - 6!
// FF - absx - 7!
// FB - absy - 7!
// E3 - indx - 8!
// F3 - yind - 8!

//LAX code 3A
// A7 - zp - 3!
// B7 - zpy - 4!
// AF - abs - 4!
// BF - absy - 4*!
// A3 - indx - 6!
// B3 - yind - 5*!

//RLA code 3B
// 27 - zp - 5!
// 37 - zpx - 6!
// 2F - abs - 6!
// 3F - absx - 7!
// 3B - absy - 7!
// 23 - indx - 8!
// 33 - yind - 8!

//RRA code 3C
// 67 - zp - 5!
// 77 - zpx - 6!
// 6F - abs - 6!
// 7F - absx - 7!
// 7B - absy - 7!
// 63 - indx - 8!
// 73 - yind - 8!

//SAX code 3D
// 87 - zp - 3!
// 97 - zpy - 4!
// 8F - abs - 4!
// 83 - indx - 6!
//
// address modes
//acc:00, abs:01, abx*:02, aby*:03, 
//imm:04, ind:05, indx:06, yind*:07, 
//rel:08, zpg:09, zpx:0A, zpy:0B, 
//jind:0C, absSX:0D, absSY:0E, yindS:0F
// 
//SLO code 3E
// 07 - zp - 5!
// 1F - zpx - 6!
// 0F - abs - 6!
// 1F - absx - 7!
// 1B - absy - 7!
// 03 - indx - 8!
// 13 - yind - 8!

//SRE code 3F
// 47 - zp - 5
// 57 - zpx - 6
// 4F - abs - 6
// 5F - absx - 7
// 5B - absy - 7
// 43 - indx - 8
// 53 - yind - 8

//USBC code 40
// EB - imm - 2 same as normal SBC (E9)

//NOP* code 21 (just uses address modes but ignores operands)
// addr   opcs   bytes    cycles 
//(FE) imp 1A, 3A, 5A, 7A, DA, FA, 1, 2 	21FE02
//(04) 80, 82, 89, C2, E2,     2, 2		210402
//(09) zpg 04, 44, 64,             2, 3		210903
//(0A) zpX 14, 34, 54, 74, D4, F4, 2, 4		210A04
//(01) abs 0C                      3, 4		210104
//(02) abx 1C, 3C, 5C, 7C, DC, FC, 3, 4*	210204
//DONE ALL