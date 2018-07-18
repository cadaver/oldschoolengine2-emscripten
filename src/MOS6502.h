// MIT License
// 
// Copyright (c) 2016 Yve Verstrepen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Modified by Lasse Oorni for OldschoolEngine2

#pragma once

class RAM64K;
class Emulator;

class MOS6502
{
public:
    MOS6502(RAM64K& ram, Emulator& emulator);
    void Jump(unsigned short address);
    void SetNMI();
    void SetIRQ();
    void Reset();
    void Process();
    void SetCycles(int value) { _cycles = value; }
    unsigned char A() const { return _a; }
    unsigned char X() const { return _x; }
    unsigned char Y() const { return _y; }
    int Cycles() const { return _cycles; }
    bool Jam() const { return _jam; }

private:
    void CountCycle(int cycles = 1);
    unsigned short Combine(unsigned char a, unsigned char b);
    void Push(unsigned char data);
    void Push16(unsigned short data);
    unsigned char Pop();
    unsigned short Pop16();
    void CheckPageBoundaries(unsigned short a, unsigned short b);
    unsigned short ZeroPageX(unsigned char address);
    unsigned short ZeroPageY(unsigned char address);
    unsigned short Absolute(unsigned char argA, unsigned char argB);
    unsigned short AbsoluteX(unsigned short address, bool checkPage = false);
    unsigned short AbsoluteY(unsigned short address, bool checkPage = false);
    unsigned short IndirectX(unsigned char address);
    unsigned short IndirectY(unsigned char address, bool checkPage = false);
    void SetZN(unsigned char value);
    void ADC(unsigned char value);
    void ADC(unsigned short address);
    void AND(unsigned char value);
    void AND(unsigned short address);
    unsigned char ASL(unsigned char value);
    void ASL(unsigned short address);
    void Bxx(unsigned char value);
    void BIT(unsigned short address);
    void Cxx(unsigned char value, unsigned char reg);
    void Cxx(unsigned short address, unsigned char reg);
    unsigned char Dxx(unsigned char value);
    void DEC(unsigned short address);
    void EOR(unsigned char value);
    void EOR(unsigned short address);
    unsigned char Ixx(unsigned char value);
    void INC(unsigned short address);
    void LDA(unsigned char value);
    void LDA(unsigned short address);
    void LDX(unsigned char value);
    void LDX(unsigned short address);
    void LDY(unsigned char value);
    void LDY(unsigned short address);
    unsigned char LSR(unsigned char value);
    void LSR(unsigned short address);
    void ORA(unsigned char value);
    void ORA(unsigned short address);
    unsigned char ROL(unsigned char value);
    void ROL(unsigned short address);
    unsigned char ROR(unsigned char value);
    void ROR(unsigned short address);
    void SBC(unsigned char value);
    void SBC(unsigned short address);

    unsigned char Status()
    {
        return (unsigned char)
            ((_carry ? 0x1 : 0) |
            (_zero ? 0x2 : 0) |
            (_interrupt ? 0x4 : 0) |
            (_decimal ? 0x8 : 0) |
            0x10 | //(_break ? 0x10 : 0) |
            0x20 |
            (_overflow ? 0x40 : 0) |
            (_negative ? 0x80 : 0));
    }

    void SetStatus(unsigned char value)
    {
        _carry = (value & 0x1) != 0;
        _zero = (value & 0x2) != 0;
        _interrupt = (value & 0x4) != 0;
        _decimal = (value & 0x8) != 0;
        //_break = (value & 0x10) != 0;
        _overflow = (value & 0x40) != 0;
        _negative = (value & 0x80) != 0;
    }

    RAM64K& _ram;
    Emulator& _emulator;

    unsigned char _opcode;
    unsigned char _data;
    unsigned short _address;
    unsigned char _a;
    unsigned char _x;
    unsigned char _y;
    unsigned char _sp;
    unsigned short _pc;

    bool _carry;    //0x1
    bool _zero;     //0x2
    bool _interrupt;//0x4
    bool _decimal;  //0x8
    //bool _break;  //0x10 only exists on stack
    //bool _unused; //0x20
    bool _overflow; //0x40
    bool _negative; //0x80

    bool _nmi;
    bool _irq;
    bool _reset;
    bool _jam;
    int _cycles;
};