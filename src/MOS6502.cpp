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

#include "MOS6502.h"
#include "RAM64K.h"
#include "Emulator.h"

MOS6502::MOS6502(RAM64K& ram, Emulator& emulator) :
    _ram(ram),
    _emulator(emulator)
{
    Reset();
}

void MOS6502::CountCycle(int cycles)
{
    _cycles += cycles;
}

void MOS6502::Jump(unsigned short address)
{
    _pc = _ram.Read16(address);
}

unsigned short MOS6502::Combine(unsigned char a, unsigned char b)
{
    unsigned short value = b;
    value <<= 8;
    value |= a;
    return value;
}

void MOS6502::Push(unsigned char data)
{
    _ram.Write((unsigned short)(_sp | 0x0100), data);
    _sp--;
}
void MOS6502::Push16(unsigned short data)
{
    Push((unsigned char)(data >> 8));
    Push((unsigned char)(data & 0xFF));
}

unsigned char MOS6502::Pop()
{
    _sp++;
    return _ram.Read((unsigned short)(_sp | 0x0100));
}
unsigned short MOS6502::Pop16()
{
    unsigned char a = Pop();
    unsigned char b = Pop();
    return Combine(a, b);
}

void MOS6502::CheckPageBoundaries(unsigned short a, unsigned short b)
{
    if ((a & 0xFF00) != (b & 0xFF00))
        CountCycle();
}

// Addressing modes
/*
unsigned short MOS6502::ZeroPage(unsigned char argA)
{
    return argA;
}
*/
unsigned short MOS6502::ZeroPageX(unsigned char address)
{
    return (unsigned short)((address + _x) & 0xFF);
}
unsigned short MOS6502::ZeroPageY(unsigned char address)
{
    return (unsigned short)((address + _y) & 0xFF);
}
/*
unsigned short MOS6502::Absolute(unsigned char argA, unsigned char argB)
{
    return Combine(argA, argB);
}
*/
unsigned short MOS6502::AbsoluteX(unsigned short address, bool checkPage)
{
    //unsigned short address = Combine(addrA, addrB);
    unsigned short trAddress = (unsigned short)(address + _x);
    if (checkPage)
        CheckPageBoundaries(address, trAddress);
    return trAddress;
}
unsigned short MOS6502::AbsoluteY(unsigned short address, bool checkPage)
{
    //unsigned short address = Combine(addrA, addrB);
    unsigned short trAddress = (unsigned short)(address + _y);
    if (checkPage)
        CheckPageBoundaries(address, trAddress);
    return trAddress;
}
unsigned short MOS6502::IndirectX(unsigned char address)
{
    return _ram.Read16((unsigned short)((address + _x) & 0xFF));
}
unsigned short MOS6502::IndirectY(unsigned char address, bool checkPage)
{
    unsigned short value = _ram.Read16(address);
    unsigned short translatedAddress = (unsigned short)(value + _y);
    if (checkPage)
        CheckPageBoundaries(value, translatedAddress);
    return translatedAddress;
}

void MOS6502::SetZN(unsigned char value)
{
    _zero = value == 0;
    _negative = (value & 0x80) != 0;
}

void MOS6502::ADC(unsigned char value)
{
    if (_decimal)
    {
        // Low nybble
        int low = (_a & 0xF) + (value & 0xF) + (_carry ? 0x1 : 0);
        bool halfCarry = (low > 0x9);

        // High nybble
        int high = (_a & 0xF0) + (value & 0xF0) + (halfCarry ? 0x10 : 0);
        _carry = (high > 0x9F);

        // Set flags on the binary result
        unsigned char binary = (unsigned char)((low & 0xF) + (high & 0xF0));
        SetZN(binary);
        _overflow = ((_a ^ binary) & (value ^ binary) & 0x80) != 0;
        //_overflow = ((_a ^ value) & 0x80) == 0 && binary > 127 && binary < 0x180;

        // Decimal adjust
        if (halfCarry)
            low += 0x6;
        if (_carry)
            high += 0x60;

        _a = (unsigned char)((low & 0xF) + (high & 0xF0));
    }
    else
    {
        int result = _a + value + (_carry ? 1 : 0);
        _overflow = ((_a ^ result) & (value ^ result) & 0x80) != 0;
        _carry = result > 0xFF;
        _a = (unsigned char)result;
        SetZN(_a);
    }
}
void MOS6502::ADC(unsigned short address)
{
    ADC(_ram.Read(address));
}

void MOS6502::AND(unsigned char value)
{
    _a = (unsigned char)(_a & value);
    SetZN(_a);
}
void MOS6502::AND(unsigned short address)
{
    MOS6502::AND(_ram.Read(address));
}

unsigned char MOS6502::ASL(unsigned char value)
{
    _carry = (value & 0x80) != 0;
    value <<= 1;
    SetZN(value);
    return value;
}
void MOS6502::ASL(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _ram.Write(address, ASL(value));
}

void MOS6502::Bxx(unsigned char value)
{
    unsigned short address = (unsigned short)(_pc + (signed char)value);
    CheckPageBoundaries(_pc, address);
    _pc = address;
    CountCycle();
}

void MOS6502::BIT(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _overflow = (value & 0x40) != 0;
    _negative = (value & 0x80) != 0;
    value &= _a;
    _zero = value == 0;
}

void MOS6502::Cxx(unsigned char value, unsigned char reg)
{
    _carry = (reg >= value);
    value = (unsigned char)(reg - value);
    SetZN(value);
}
void MOS6502::Cxx(unsigned short address, unsigned char reg)
{
    Cxx(_ram.Read(address), reg);
}

unsigned char MOS6502::Dxx(unsigned char value)
{
    value--;
    SetZN(value);
    return value;
}
void MOS6502::DEC(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _ram.Write(address, Dxx(value));
}

void MOS6502::EOR(unsigned char value)
{
    _a ^= value;
    SetZN(_a);
}
void MOS6502::EOR(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    EOR(value);
}

unsigned char MOS6502::Ixx(unsigned char value)
{
    value++;
    SetZN(value);
    return value;
}
void MOS6502::INC(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _ram.Write(address, Ixx(value));
}

void MOS6502::LDA(unsigned char value)
{
    _a = value;
    SetZN(value);
}
void MOS6502::LDA(unsigned short address)
{
    LDA(_ram.Read(address));
}
void MOS6502::LDX(unsigned char value)
{
    _x = value;
    SetZN(value);
}
void MOS6502::LDX(unsigned short address)
{
    LDX(_ram.Read(address));
}
void MOS6502::LDY(unsigned char value)
{
    _y = value;
    SetZN(value);
}
void MOS6502::LDY(unsigned short address)
{
    LDY(_ram.Read(address));
}

unsigned char MOS6502::LSR(unsigned char value)
{
    _carry = (value & 0x1) != 0;
    value >>= 1;
    SetZN(value);
    return value;
}
void MOS6502::LSR(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _ram.Write(address, LSR(value));
}

void MOS6502::ORA(unsigned char value)
{
    _a |= value;
    SetZN(_a);
}
void MOS6502::ORA(unsigned short address)
{
    ORA(_ram.Read(address));
}

unsigned char MOS6502::ROL(unsigned char value)
{
    bool oldCarry = _carry;
    _carry = (value & 0x80) != 0;
    value <<= 1;
    if (oldCarry) value |= 0x1;
    SetZN(value);
    return value;
}
void MOS6502::ROL(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _ram.Write(address, ROL(value));
}

unsigned char MOS6502::ROR(unsigned char value)
{
    bool oldCarry = _carry;
    _carry = (value & 0x1) != 0;
    value >>= 1;
    if (oldCarry) value |= 0x80;
    SetZN(value);
    return value;
}
void MOS6502::ROR(unsigned short address)
{
    unsigned char value = _ram.Read(address);
    _ram.Write(address, ROR(value));
}

void MOS6502::SBC(unsigned char value)
{
    if (_decimal)
    {
        // Low nybble
        int low = 0xF + (_a & 0xF) - (value & 0xF) + (_carry ? 0x1 : 0);
        bool halfCarry = (low > 0xF);

        // High nybble
        int high = 0xF0 + (_a & 0xF0) - (value & 0xF0) + (halfCarry ? 0x10 : 0);
        _carry = (high > 0xFF);

        // Set flags on the binary result
        unsigned char binary = (unsigned char)((low & 0xF) + (high & 0xF0));
        SetZN(binary);
        _overflow = ((_a ^ binary) & (~value ^ binary) & 0x80) != 0;
        //_overflow = ((_a ^ value) & 0x80) != 0 && result >= 0x80 && result < 0x180;

        // Decimal adjust
        if (!halfCarry)
            low -= 0x6;
        if (!_carry)
            high -= 0x60;

        _a = (unsigned char)((low & 0xF) + (high & 0xF0));
    }
    else
    {
        int result = 0xFF + _a - value + (_carry ? 1 : 0);
        _overflow = ((_a ^ result) & (~value ^ result) & 0x80) != 0;
        _carry = result > 0xFF;
        _a = (unsigned char)result;
        SetZN(_a);
    }
}
void MOS6502::SBC(unsigned short address)
{
    MOS6502::SBC(_ram.Read(address));
}

void MOS6502::SetNMI()
{
    _nmi = true;
}

void MOS6502::SetIRQ()
{
    _irq = true;
}

/// <summary>
/// Resets the CPU.
/// </summary>
void MOS6502::Reset()
{
    _reset = true;
}

/// <summary>
/// Execute the next opcode.
/// </summary>
void MOS6502::Process()
{
    if (_reset)
    {
        // Writes are ignored at reset, so don't push anything but do modify the stack pointer
        _sp -= 3;
        _interrupt = true;
        _pc = _ram.Read16(0xFFFC);
        _cycles = 0;
        CountCycle(7);
        _reset = false;
        _jam = false;
        _nmi = false;
        _irq = false;
        return;
    }

    if (_jam)
    {
        _nmi = false;
        _irq = false;
        return;
    }

    if (_nmi)
    {
        Push16(_pc);
        Push((unsigned char)(Status() & 0xEF)); // Mask off break flag
        _interrupt = true;
        _pc = _ram.Read16(0xFFFA);
        CountCycle(7);
        _nmi = false;
        _irq = false;
        return;
    }
    else if (_irq && !_interrupt)
    {
        Push16(_pc);
        Push((unsigned char)(Status() & 0xEF)); // Mask off break flag
        _interrupt = true;
        _pc = _ram.Read16(0xFFFE);
        // HACK for MW4 scorepanel: do not waste cycles before IRQ
        //CountCycle(7);
        _irq = false;
        return;
    }

    _opcode = _ram.Read(_pc);

    // HACK: do not care of banking to simplify $01 handling for ingame IRQs
    // There is no game code in $ff00 - $ffff region
    if (_pc >= 0xff00)
    {
        _emulator.KernalTrap(_pc);
        _opcode = 0x60; // RTS, return from the Kernal routine
    }

    _data = _ram.Read((unsigned short)(_pc + 1));
    _address = Combine(_data, _ram.Read((unsigned short)(_pc + 2)));

    switch (_opcode)
    {
        // ADC
        case (0x69): // Immediate
            _pc += 2;
            ADC(_data);
            CountCycle(2);
            break;
        case (0x65): // Zero Page
            _pc += 2;
            ADC((unsigned short)_data);
            CountCycle(3);
            break;
        case (0x75): // Zero Page,X
            _pc += 2;
            ADC(ZeroPageX(_data));
            CountCycle(4);
            break;
        case (0x6D): // Absolute
            _pc += 3;
            ADC(_address);
            CountCycle(4);
            break;
        case (0x7D): // Absolute,X
            _pc += 3;
            ADC(AbsoluteX(_address, true));
            CountCycle(4);
            break;
        case (0x79): // Absolute,Y
            _pc += 3;
            ADC(AbsoluteY(_address, true));
            CountCycle(4);
            break;
        case (0x61): // Indirect,X
            _pc += 2;
            ADC(IndirectX(_data));
            CountCycle(6);
            break;
        case (0x71): // Indirect,Y
            _pc += 2;
            ADC(IndirectY(_data, true));
            CountCycle(5);
            break;

        // AND
        case (0x29): // Immediate
            _pc += 2;
            AND(_data);
            CountCycle(2);
            break;
        case (0x25): // Zero Page
            _pc += 2;
            AND((unsigned short)_data);
            CountCycle(2);
            break;
        case (0x35): // Zero Page X
            _pc += 2;
            AND(ZeroPageX(_data));
            CountCycle(3);
            break;
        case (0x2D): // Absolute
            _pc += 3;
            AND(_address);
            CountCycle(4);
            break;
        case (0x3D): // Absolute,X
            _pc += 3;
            AND(AbsoluteX(_address, true));
            CountCycle(4);
            break;
        case (0x39): // Absolute,Y
            _pc += 3;
            AND(AbsoluteY(_address, true));
            CountCycle(4);
            break;
        case (0x21): // Indirect,X
            _pc += 2;
            AND(IndirectX(_data));
            CountCycle(6);
            break;
        case (0x31): // Indirect,Y
            _pc += 2;
            AND(IndirectY(_data, true));
            CountCycle(5);
            break;

        case (0x0A): // Accumulator
            _pc += 1;
            _a = ASL(_a);
            CountCycle(2);
            break;
        case (0x06): // Zero Page
            _pc += 2;
            ASL((unsigned short)_data);
            CountCycle(5);
            break;
        case (0x16): // Zero Page,X
            _pc += 2;
            ASL(ZeroPageX(_data));
            CountCycle(6);
            break;
        case (0x0E): // Absolute
            _pc += 3;
            ASL(_address);
            CountCycle(6);
            break;
        case (0x1E): // Absolute,X
            _pc += 3;
            ASL(AbsoluteX(_address));
            CountCycle(7);
            break;

        // BCC
        case (0x90): // Relative
            _pc += 2;
            if (!_carry)
                Bxx(_data);
            CountCycle(2);
            break;

        // BCS
        case (0xB0): // Relative
            _pc += 2;
            if (_carry)
                Bxx(_data);
            CountCycle(2);
            break;

        // BEQ
        case (0xF0): // Relative
            _pc += 2;
            if (_zero)
                Bxx(_data);
            CountCycle(2);
            break;

        // BIT
        case (0x24): // Zero Page
            _pc += 2;
            BIT((unsigned short)_data);
            CountCycle(3);
            break;
        case (0x2C): // Absolute
            _pc += 3;
            BIT(_address);
            CountCycle(4);
            break;

        // BMI
        case (0x30): // Relative
            _pc += 2;
            if (_negative)
                Bxx(_data);
            CountCycle(2);
            break;

        // BNE
        case (0xD0): // Relative
            _pc += 2;
            if (!_zero)
                Bxx(_data);
            CountCycle(2);
            break;

        // BPL
        case (0x10): // Relative
            _pc += 2;
            if (!_negative)
                Bxx(_data);
            CountCycle(2);
            break;
            
        // BRK
        case (0x00): // Implied
            _pc += 2;
#if BUG
            if (_irq || _nmi || _reset) break; // Emulate the interrupt bug
#endif
            Push16(_pc);
            Push(Status());
            _interrupt = true;
            _pc = _ram.Read16(0xFFFE);
            CountCycle(7);
            break;
            
        // BVC
        case (0x50): // Relative
            _pc += 2;
            if (!_overflow)
                Bxx(_data);
            CountCycle(2);
            break;

        // BVS
        case (0x70): // Relative
            _pc += 2;
            if (_overflow)
                Bxx(_data);
            CountCycle(2);
            break;

        // CLC
        case (0x18): // Implied
            _pc += 1;
            _carry = false;
            CountCycle(2);
            break;
        
        // CLD
        case (0xD8): // Implied
            _pc += 1;
            _decimal = false;
            CountCycle(2);
            break;
            
        // CLI
        case (0x58): // Implied
            _pc += 1;
            _interrupt = false;
            CountCycle(2);
            break;
            
        // CLV
        case (0xB8): // Implied
            _pc += 1;
            _overflow = false;
            CountCycle(2);
            break;
            
        // CMP
        case (0xC9): // Immediate
            _pc += 2;
            Cxx(_data, _a);
            CountCycle(2);
            break;
        case (0xC5): // Zero Page
            _pc += 2;
            Cxx((unsigned short)_data, _a);
            CountCycle(3);
            break;
        case (0xD5): // Zero Page,X
            _pc += 2;
            Cxx(ZeroPageX(_data), _a);
            CountCycle(4);
            break;
        case (0xCD): // Absolute
            _pc += 3;
            Cxx(_address, _a);
            CountCycle(4);
            break;
        case (0xDD): // Absolute,X
            _pc += 3;
            Cxx(AbsoluteX(_address, true), _a);
            CountCycle(4);
            break;
        case (0xD9): // Absolute,Y
            _pc += 3;
            Cxx(AbsoluteY(_address, true), _a);
            CountCycle(4);
            break;
        case (0xC1): // Indirect,X
            _pc += 2;
            Cxx(IndirectX(_data), _a);
            CountCycle(6);
            break;
        case (0xD1): // Indirect,Y
            _pc += 2;
            Cxx(IndirectY(_data, true), _a);
            CountCycle(5);
            break;
            
        // CPX
        case (0xE0): // Immediate
            _pc += 2;
            Cxx(_data, _x);
            CountCycle(2);
            break;
        case (0xE4): // Zero Page
            _pc += 2;
            Cxx((unsigned short)_data, _x);
            CountCycle(3);
            break;
        case (0xEC): // Absolute
            _pc += 3;
            Cxx(_address, _x);
            CountCycle(4);
            break;

        // CPY
        case (0xC0): // Immediate
            _pc += 2;
            Cxx(_data, _y);
            CountCycle(2);
            break;
        case (0xC4): // Zero Page
            _pc += 2;
            Cxx((unsigned short)_data, _y);
            CountCycle(3);
            break;
        case (0xCC): // Absolute
            _pc += 3;
            Cxx(_address, _y);
            CountCycle(4);
            break;

        // DEC
        case (0xC6): // Zero Page
            _pc += 2;
            DEC((unsigned short)_data);
            CountCycle(5);
            break;
        case (0xD6): // Zero Page,X
            _pc += 2;
            DEC(ZeroPageX(_data));
            CountCycle(6);
            break;
        case (0xCE): // Absolute
            _pc += 3;
            DEC(_address);
            CountCycle(6);
            break;
        case (0xDE): // Absolute,X
            _pc += 3;
            DEC(AbsoluteX(_address));
            CountCycle(7);
            break;

        // DEX
        case (0xCA): // Implied
            _pc += 1;
            _x = Dxx(_x);
            CountCycle(2);
            break;

        // DEY
        case (0x88): // Implied
            _pc += 1;
            _y = Dxx(_y);
            CountCycle(2);
            break;

        // EOR
        case (0x49): // Immediate
            _pc += 2;
            EOR(_data);
            CountCycle(2);
            break;
        case (0x45): // Zero Page
            _pc += 2;
            EOR((unsigned short)_data);
            CountCycle(3);
            break;
        case (0x55): // Zero Page,X
            _pc += 2;
            EOR(ZeroPageX(_data));
            CountCycle(4);
            break;
        case (0x4D): // Absolute
            _pc += 3;
            EOR(_address);
            CountCycle(4);
            break;
        case (0x5D): // Absolute,X
            _pc += 3;
            EOR(AbsoluteX(_address, true));
            CountCycle(4);
            break;
        case (0x59): // Absolute,Y
            _pc += 3;
            EOR(AbsoluteY(_address, true));
            CountCycle(4);
            break;
        case (0x41): // Indirect,X
            _pc += 2;
            EOR(IndirectX(_data));
            CountCycle(6);
            break;
        case (0x51): // Indirect,Y
            _pc += 2;
            EOR(IndirectY(_data, true));
            CountCycle(5);
            break;

        // INC
        case (0xE6): // Zero Page
            _pc += 2;
            INC((unsigned short)_data);
            CountCycle(5);
            break;
        case (0xF6): // Zero Page,X
            _pc += 2;
            INC(ZeroPageX(_data));
            CountCycle(6);
            break;
        case (0xEE): // Absolute
            _pc += 3;
            INC(_address);
            CountCycle(6);
            break;
        case (0xFE): // Absolute,X
            _pc += 3;
            INC(AbsoluteX(_address));
            CountCycle(7);
            break;

        // INX
        case (0xE8): // Implied
            _pc += 1;
            _x = Ixx(_x);
            CountCycle(2);
            break;

        // INY
        case (0xC8): // Implied
            _pc += 1;
            _y = Ixx(_y);
            CountCycle(2);
            break;

        // JMP
        case (0x4C): // Absolute
            _pc = _address;
            CountCycle(3);
            break;

        case (0x6C): // Indirect
            {
                unsigned short address = _address;
#if BUG
                if ((address & 0x00FF) == 0x00FF) // Emulate the indirect jump bug
                    _pc = (unsigned short)((_ram.Read((unsigned short)(address & 0xFF00)) << 8) | _ram.Read(address));
                else
#endif
                _pc = _ram.Read16(address);
                CountCycle(5);
            }
            break;

        // JSR
        case (0x20): // Absolute
            Push16((unsigned short)(_pc + 2)); // + 3 - 1
            _pc = _address;
            CountCycle(6);
            break;

        // LDA
        case (0xA9): // Immediate
            _pc += 2;
            LDA(_data);
            CountCycle(2);
            break;
        case (0xA5): // Zero Page
            _pc += 2;
            LDA((unsigned short)_data);
            CountCycle(3);
            break;
        case (0xB5): // Zero Page,X
            _pc += 2;
            LDA(ZeroPageX(_data));
            CountCycle(4);
            break;
        case (0xAD): // Absolute
            _pc += 3;
            LDA(_address);
            CountCycle(4);
            break;
        case (0xBD): // Absolute,X
            _pc += 3;
            LDA(AbsoluteX(_address, true));
            CountCycle(4);
            break;
        case (0xB9): // Absolute,Y
            _pc += 3;
            LDA(AbsoluteY(_address, true));
            CountCycle(4);
            break;
        case (0xA1): // Indirect,X
            _pc += 2;
            LDA(IndirectX(_data));
            CountCycle(6);
            break;
        case (0xB1): // Indirect,Y
            _pc += 2;
            LDA(IndirectY(_data, true));
            CountCycle(5);
            break;

        // LDX
        case (0xA2): // Immediate
            _pc += 2;
            LDX(_data);
            CountCycle(2);
            break;
        case (0xA6): // Zero Page
            _pc += 2;
            LDX((unsigned short)_data);
            CountCycle(3);
            break;
        case (0xB6): // Zero Page,Y
            _pc += 2;
            LDX(ZeroPageY(_data));
            CountCycle(4);
            break;
        case (0xAE): // Absolute
            _pc += 3;
            LDX(_address);
            CountCycle(4);
            break;
        case (0xBE): // Absolute,Y
            _pc += 3;
            LDX(AbsoluteY(_address, true));
            CountCycle(4);
            break;
            
        // LDY
        case (0xA0): // Immediate
            _pc += 2;
            LDY(_data);
            CountCycle(2);
            break;
        case (0xA4): // Zero Page
            _pc += 2;
            LDY((unsigned short)_data);
            CountCycle(3);
            break;
        case (0xB4): // Zero Page,X
            _pc += 2;
            LDY(ZeroPageX(_data));
            CountCycle(4);
            break;
        case (0xAC): // Absolute
            _pc += 3;
            LDY(_address);
            CountCycle(4);
            break;
        case (0xBC): // Absolute,X
            _pc += 3;
            LDY(AbsoluteX(_address, true));
            CountCycle(4);
            break;
            
        // LSR
        case (0x4A): // Accumulator
            _pc += 1;
            _a = LSR(_a);
            CountCycle(2);
            break;
        case (0x46): // Zero Page
            _pc += 2;
            LSR((unsigned short)_data);
            CountCycle(5);
            break;
        case (0x56): // Zero Page,X
            _pc += 2;
            LSR(ZeroPageX(_data));
            CountCycle(6);
            break;
        case (0x4E): // Absolute
            _pc += 3;
            LSR(_address);
            CountCycle(6);
            break;
        case (0x5E): // Absolute,X
            _pc += 3;
            LSR(AbsoluteX(_address));
            CountCycle(7);
            break;

        // NOP
        case(0xEA): // Implied
            _pc += 1;
            CountCycle(2);
            break;
            
        // ORA
        case (0x09): // Immediate
            _pc += 2;
            ORA(_data);
            CountCycle(2);
            break;
        case (0x05): // Zero Page
            _pc += 2;
            ORA((unsigned short)_data);
            CountCycle(3);
            break;
        case (0x15): // Zero Page,X
            _pc += 2;
            ORA(ZeroPageX(_data));
            CountCycle(4);
            break;
        case (0x0D): // Absolute
            _pc += 3;
            ORA(_address);
            CountCycle(4);
            break;
        case (0x1D): // Absolute,X
            _pc += 3;
            ORA(AbsoluteX(_address, true));
            CountCycle(4);
            break;
        case (0x19): // Absolute,Y
            _pc += 3;
            ORA(AbsoluteY(_address, true));
            CountCycle(4);
            break;
        case (0x01): // Indirect,X
            _pc += 2;
            ORA(IndirectX(_data));
            CountCycle(6);
            break;
        case (0x11): // Indirect,Y
            _pc += 2;
            ORA(IndirectY(_data, true));
            CountCycle(5);
            break;

        // PHA
        case (0x48): // Implied
            _pc += 1;
            Push(_a);
            CountCycle(3);
            break;

        // PHP
        case (0x08): // Implied
            _pc += 1;
            Push(Status());
            CountCycle(3);
            break;

        // PLA
        case (0x68): // Implied
            _pc += 1;
            _a = Pop();
            SetZN(_a);
            CountCycle(4);
            break;

        // PLP
        case (0x28): // Implied
            _pc += 1;
            SetStatus(Pop());
            CountCycle(4);
            break;

        // ROL
        case (0x2A): // Accumulator
            _pc += 1;
            _a = ROL(_a);
            CountCycle(2);
            break;
        case (0x26): // Zero Page
            _pc += 2;
            ROL((unsigned short)_data);
            CountCycle(5);
            break;
        case (0x36): // Zero Page,X
            _pc += 2;
            ROL(ZeroPageX(_data));
            CountCycle(6);
            break;
        case (0x2E): // Absolute
            _pc += 3;
            ROL(_address);
            CountCycle(6);
            break;
        case (0x3E): // Absolute,X
            _pc += 3;
            ROL(AbsoluteX(_address));
            CountCycle(7);
            break;

        // ROR
        case (0x6A): // Accumulator
            _pc += 1;
            _a = ROR(_a);
            CountCycle(2);
            break;
        case (0x66): // Zero Page
            _pc += 2;
            ROR((unsigned short)_data);
            CountCycle(5);
            break;
        case (0x76): // Zero Page,X
            _pc += 2;
            ROR(ZeroPageX(_data));
            CountCycle(6);
            break;
        case (0x6E): // Absolute
            _pc += 3;
            ROR(_address);
            CountCycle(6);
            break;
        case (0x7E): // Absolute,X
            _pc += 3;
            ROR(AbsoluteX(_address));
            CountCycle(7);
            break;

        // RTI
        case (0x40): // Implied
            SetStatus(Pop());
            _pc = Pop16();
            CountCycle(6);
            break;

        // RTS
        case (0x60): // Implied
            _pc = Pop16();
            _pc += 1;
            CountCycle(6);
            break;
            
        // SBC
        case (0xE9): // Immediate
            _pc += 2;
            SBC(_data);
            CountCycle(2);
            break;
        case (0xE5): // Zero Page
            _pc += 2;
            SBC((unsigned short)_data);
            CountCycle(3);
            break;
        case (0xF5): // Zero Page,X
            _pc += 2;
            SBC(ZeroPageX(_data));
            CountCycle(4);
            break;
        case (0xED): // Absolute
            _pc += 3;
            SBC(_address);
            CountCycle(4);
            break;
        case (0xFD): // Absolute,X
            _pc += 3;
            SBC(AbsoluteX(_address, true));
            CountCycle(4);
            break;
        case (0xF9): // Absolute,Y
            _pc += 3;
            SBC(AbsoluteY(_address, true));
            CountCycle(4);
            break;
        case (0xE1): // Indirect,X
            _pc += 2;
            SBC(IndirectX(_data));
            CountCycle(6);
            break;
        case (0xF1): // Indirect,Y
            _pc += 2;
            SBC(IndirectY(_data, true));
            CountCycle(5);
            break;

        // SEC
        case (0x38): // Implied
            _pc += 1;
            _carry = true;
            CountCycle(2);
            break;

        // SED
        case (0xF8): // Implied
            _pc += 1;
            _decimal = true;
            CountCycle(2);
            break;

        // SEI
        case (0x78): // Implied
            _pc += 1;
            _interrupt = true;
            CountCycle(2);
            break;

        // STA
        case (0x85): // Zero Page
            _pc += 2;
            _ram.Write((unsigned short)_data, _a);
            CountCycle(3);
            break;
        case (0x95): // Zero Page,X
            _pc += 2;
            _ram.Write(ZeroPageX(_data), _a);
            CountCycle(4);
            break;
        case (0x8D): // Absolute
            _pc += 3;
            _ram.Write(_address, _a);
            CountCycle(4);
            break;
        case (0x9D): // Absolute,X
            _pc += 3;
            _ram.Write(AbsoluteX(_address), _a);
            CountCycle(5);
            break;
        case (0x99): // Absolute,Y
            _pc += 3;
            _ram.Write(AbsoluteY(_address), _a);
            CountCycle(5);
            break;
        case (0x81): // Indirect,X
            _pc += 2;
            _ram.Write(IndirectX(_data), _a);
            CountCycle(6);
            break;
        case (0x91): // Indirect,Y
            _pc += 2;
            _ram.Write(IndirectY(_data), _a);
            CountCycle(6);
            break;

        // STX
        case (0x86): // Zero Page
            _pc += 2;
            _ram.Write((unsigned short)_data, _x);
            CountCycle(3);
            break;
        case (0x96): // Zero Page,Y
            _pc += 2;
            _ram.Write(ZeroPageY(_data), _x);
            CountCycle(4);
            break;
        case (0x8E): // Absolute
            _pc += 3;
            _ram.Write(_address, _x);
            CountCycle(4);
            break;

        // STY
        case (0x84): // Zero Page
            _pc += 2;
            _ram.Write((unsigned short)_data, _y);
            CountCycle(3);
            break;
        case (0x94): // Zero Page,X
            _pc += 2;
            _ram.Write(ZeroPageX(_data), _y);
            CountCycle(4);
            break;
        case (0x8C): // Absolute
            _pc += 3;
            _ram.Write(_address, _y);
            CountCycle(4);
            break;

        // TAX
        case (0xAA): // Implied
            _pc += 1;
            _x = _a;
            SetZN(_x);
            CountCycle(2);
            break;

        // TAY
        case (0xA8): // Implied
            _pc += 1;
            _y = _a;
            SetZN(_y);
            CountCycle(2);
            break;

        // TSX
        case (0xBA): // Implied
            _pc += 1;
            _x = _sp;
            SetZN(_x);
            CountCycle(2);
            break;
            
        // TXA
        case (0x8A): // Implied
            _pc += 1;
            _a = _x;
            SetZN(_a);
            CountCycle(2);
            break;

        // TXS
        case (0x9A): // Implied
            _pc += 1;
            _sp = _x;
            CountCycle(2);
            break;

        // TYA
        case (0x98): // Implied
            _pc += 1;
            _a = _y;
            SetZN(_a);
            CountCycle(2);
            break;

        /////////////////////
        // Illegal opcodes //
        /////////////////////
        
        // ANC
        case (0x0B): // Immediate
        case (0x2B):
            _pc += 2;
            AND(_data);
            _carry = _negative;
            CountCycle(2);
            break;

        // KIL
        case (0x02):
        case (0x12):
        case (0x22):
        case (0x32):
        case (0x42):
        case (0x52):
        case (0x62):
        case (0x72):
        case (0x92):
        case (0xB2):
        case (0xD2):
        case (0xF2):
            _jam = true;
            break;
        
        //TODO: Count extra NOP cycles and double/triple unsigned chars
        /*
        // NOP
        case (0x80):
        case (0x82):
        case (0xC2):
        case (0xE2):
        case (0x04):
        case (0x44):
        case (0x64):
        case (0x89):
        case (0x0C):
        case (0x14):
        case (0x34):
        case (0x54):
        case (0x74):
        case (0xD4):
        case (0xF4):
        case (0x1A):
        case (0x3A):
        case (0x5A):
        case (0x7A):
        case (0xDA):
        case (0xFA):
        case (0x1C):
        case (0x3C):
        case (0x5C):
        case (0x7C):
        case (0xDC):
        case (0xFC):
            goto case (0xEA);
            */
        default:
            _pc += 1;
            CountCycle(2);
            break;
    }
}
