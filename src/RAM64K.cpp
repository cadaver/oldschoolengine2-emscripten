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

#include "RAM64K.h"
#include "Emulator.h"

RAM64K::RAM64K(Emulator& emulator) :
    _emulator(emulator)
{
    for (unsigned i = 0; i < sizeof(_ram); ++i)
        _ram[i] = 0x0;
    for (unsigned i = 0; i < sizeof(_ioRam); ++i)
        _ioRam[i] = 0x0;
}

unsigned char RAM64K::Read(unsigned short address)
{
    if ((_ram[0x01] & 0x3) == 0 || address < 0xd000 || address >= 0xe000)
        return ReadRAM(address);
    else
        return ReadIO(address);
}

unsigned char RAM64K::ReadRAM(unsigned short address)
{
    return _ram[address];
}

unsigned char RAM64K::ReadIO(unsigned short address, bool readInput)
{
    if (address >= 0xd000 && address < 0xe000)
    {
        if (readInput)
        {
            bool handled = false;
            unsigned char ret = _emulator.IORead(address, handled);
            if (handled)
                return ret;
        }

        return _ioRam[address - 0xd000];
    }
    else
        return _ram[address];
}

unsigned short RAM64K::Read16(unsigned short address)
{
    unsigned char a = Read(address);
    unsigned char b = Read(++address);
    return (unsigned short)((b << 8) | a);
    // Let's keep things readable shall we
    //return (unsigned short)((Read(++address) << 8) | Read(--address));
}

void RAM64K::Write(unsigned short address, unsigned char value)
{
    if ((_ram[0x01] & 0x3) == 0 || address < 0xd000 || address >= 0xe000)
        WriteRAM(address, value);
    else
        WriteIO(address, value);
}

void RAM64K::WriteRAM(unsigned short address, unsigned char value)
{
    _ram[address] = value;
}

void RAM64K::WriteIO(unsigned short address, unsigned char value)
{
    if (address >= 0xd000 && address < 0xe000)
    {
        // Hook before the value changes
        _emulator.IOWrite(address);
        _ioRam[address - 0xd000] = value;
    }
    else
        _ram[address] = value;
}

void RAM64K::Write16(unsigned short address, unsigned short value)
{
    Write(address, (unsigned char)(value & 0xFF));
    Write(++address, (unsigned char)(value >> 8));
}