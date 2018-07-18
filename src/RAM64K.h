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

class Emulator;

class RAM64K
{
public:
    RAM64K(Emulator& emulator);

    unsigned char Read(unsigned short address);
    unsigned char ReadRAM(unsigned short address);
    unsigned char ReadIO(unsigned short address, bool readInput = true);
    unsigned short Read16(unsigned short address);
    void Write(unsigned short address, unsigned char value);
    void WriteRAM(unsigned short address, unsigned char value);
    void WriteIO(unsigned short address, unsigned char value);
    void Write16(unsigned short address, unsigned short value);

private:
    Emulator& _emulator;
    unsigned char _ram[65536];
    unsigned char _ioRam[4096];
};