// MIT License
// 
// Copyright (c) 2018 Lasse Oorni
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

#include <stdlib.h>

#include "Emulator.h"
#include "Screen.h"
#include "MOS6502.h"
#include "RAM64K.h"
#include "VIC2.h"

Emulator::Emulator()
{
    InitScreen();

    _ram = new RAM64K(*this);
    _processor = new MOS6502(*_ram, *this);
    _vic2 = new VIC2(*_ram);
    _ram->WriteIO(0xd020, 7);
}

Emulator::~Emulator()
{
    delete _processor;
    delete _ram;
    delete _vic2;
}

void Emulator::Start()
{
    InitScreen();
}

void Emulator::Update()
{
    RunFrame();
    UpdateScreen(_vic2->Pixels());
}

void Emulator::InitMemory()
{
}

void Emulator::RunFrame()
{
    const int frameCycles = CYCLES_PER_LINE * NUM_LINES;

    _processor->SetCycles(0);

    for (unsigned i = 0; i < FIRST_VISIBLE_LINE; ++i)
        ExecuteLine(i, false);

    _vic2->BeginFrame();

    for (unsigned i = FIRST_VISIBLE_LINE; i < FIRST_INVISIBLE_LINE; ++i)
        ExecuteLine(i, true);

    for (unsigned i = FIRST_INVISIBLE_LINE; i < NUM_LINES; ++i)
        ExecuteLine(i, false);
}

void Emulator::ExecuteLine(int lineNum, bool visible)
{
    int targetCycles = CYCLES_PER_LINE * lineNum;

    while (_processor->Cycles() < targetCycles && !_processor->Jam())
        _processor->Process();

    if (visible)
        _vic2->RenderNextLine();
}

void Emulator::KernalTrap(unsigned short address)
{
}

unsigned char Emulator::IORead(unsigned short address, bool& handled)
{
    handled = false;
    return 0;
}

void Emulator::IOWrite(unsigned short address)
{
}
