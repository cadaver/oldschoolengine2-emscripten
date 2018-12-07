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
#include "Audio.h"
#include "MOS6502.h"
#include "RAM64K.h"
#include "VIC2.h"
#include "SID.h"

std::string diskImageName = "steelrangerdemo";

Emulator::Emulator(const std::string& imageName) :
    _ram(nullptr),
    _processor(nullptr),
    _vic2(nullptr),
    _sid(nullptr),
    _disk(nullptr),
    _timer(0),
    _timerIRQEnable(false),
    _timerIRQFlag(false)
{
    if (imageName.length())
        diskImageName = imageName;

    _ram = new RAM64K(*this);
    _processor = new MOS6502(*_ram, *this);
    _vic2 = new VIC2(*_ram);
    _sid = new SID(*_ram);
    for (int i = 0; i < 8; ++i)
        _keyMatrix[i] = 0xff;

    // TODO: more keymappings
    _keyMappings[27] = 63;
    _keyMappings[32] = 60;
    _keyMappings[188] = 47;
    _keyMappings[190] = 44;
    _keyMappings[13] = 1;
    _keyMappings[8] = 0;
    _keyMappings['3'] = 8;
    _keyMappings['W'] = 9;
    _keyMappings['A'] = 10;
    _keyMappings['4'] = 11;
    _keyMappings['Z'] = 12;
    _keyMappings['S'] = 13;
    _keyMappings['E'] = 14;
    _keyMappings['5'] = 16;
    _keyMappings['R'] = 17;
    _keyMappings['D'] = 18;
    _keyMappings['6'] = 19;
    _keyMappings['C'] = 20;
    _keyMappings['F'] = 21;
    _keyMappings['T'] = 22;
    _keyMappings['X'] = 23;
    _keyMappings['7'] = 24;
    _keyMappings['Y'] = 25;
    _keyMappings['G'] = 26;
    _keyMappings['8'] = 27;
    _keyMappings['B'] = 28;
    _keyMappings['H'] = 29;
    _keyMappings['U'] = 30;
    _keyMappings['V'] = 31;
    _keyMappings['9'] = 32;
    _keyMappings['I'] = 33;
    _keyMappings['J'] = 34;
    _keyMappings['0'] = 35;
    _keyMappings['M'] = 36;
    _keyMappings['K'] = 37;
    _keyMappings['O'] = 38;
    _keyMappings['N'] = 39;
    _keyMappings['P'] = 41;
    _keyMappings['L'] = 42;
    _keyMappings['1'] = 56;
    _keyMappings['2'] = 59;
    _keyMappings['Q'] = 62;

    Screen::Init();
    Audio::Init(4);
    InitMemory();
    BootGame();
}

Emulator::~Emulator()
{
    delete _processor;
    delete _ram;
    delete _vic2;
    delete _sid;
}

void Emulator::Update()
{
    RunFrame();
    Screen::Redraw(_vic2->Pixels());
}

void Emulator::InitMemory()
{
    _ram->WriteRAM(0x01, 0x37);
    _ram->WriteIO(0xd018, 0x14);
    _ram->WriteIO(0xd011, 27);
    _ram->WriteIO(0xd016, 24);
    _ram->WriteIO(0xdd00, 0x3);
    _ram->WriteIO(0xd030, 0xff);
    _ram->WriteIO(0xd0bc, 0xff);
    _ram->WriteIO(0xdc00, 0xff);
}

void Emulator::BootGame()
{
    _disk = new DiskImage(diskImageName);

    // No filename, open first file in directory
    FileHandle bootFile = _disk->OpenFile(std::vector<unsigned char>());
    if (bootFile.IsOpen())
    {
        unsigned short loadAddress = (_disk->ReadByte(bootFile) + _disk->ReadByte(bootFile) * 256);
        unsigned short address = loadAddress;
        while (bootFile.IsOpen())
            _ram->WriteRAM(address++, _disk->ReadByte(bootFile));

        // Set RESET vector to CLALL (autostart), otherwise assume sys2061
        if (loadAddress <= 0x32c)
        {
            _ram->WriteRAM(0xfffc, _ram->ReadRAM(0x32c));
            _ram->WriteRAM(0xfffd, _ram->ReadRAM(0x32d));
        }
        else
        {
            _ram->WriteRAM(0xfffc, 0xd);
            _ram->WriteRAM(0xfffd, 0x8);
        }
    }
}

void Emulator::RunFrame()
{
    const int frameCycles = CYCLES_PER_LINE * NUM_LINES;
    _audioCycles = 0;

    _processor->SetCycles(0);

    for (unsigned i = 0; i < FIRST_VISIBLE_LINE; ++i)
        ExecuteLine(i, false);

    _vic2->BeginFrame();

    for (unsigned i = FIRST_VISIBLE_LINE; i < FIRST_INVISIBLE_LINE; ++i)
        ExecuteLine(i, true);

    for (unsigned i = FIRST_INVISIBLE_LINE; i < NUM_LINES; ++i)
        ExecuteLine(i, false);

    // Render rest of audio until end of frame
    if (_audioCycles < frameCycles)
    {
        _sid->BufferSamples(frameCycles - _audioCycles);
        _audioCycles = frameCycles;
    }
}

void Emulator::QueueAudio()
{
    unsigned frameSamples = 44100 / 50;

    while (_sid->samples.size() > frameSamples && Audio::NumFreeBuffers() > 0)
    {
        Audio::QueueBuffer(&_sid->samples[0], frameSamples);
        _sid->samples.erase(_sid->samples.begin(), _sid->samples.begin() + frameSamples);
    }
}

void Emulator::ExecuteLine(int lineNum, bool visible)
{
    UpdateLineCounterAndIRQ(lineNum);

    int targetCycles = CYCLES_PER_LINE * lineNum;

    while (_processor->Cycles() < targetCycles && !_processor->Jam())
        _processor->Process();

    if (visible)
        _vic2->RenderNextLine();
}

void Emulator::UpdateLineCounterAndIRQ(int lineNum)
{
    _lineCounter = lineNum;
    if ((_ram->ReadIO(0xd01a, false) & 0x1) > 0)
    {
        int targetLineNum = (_ram->ReadIO(0xd011, false) & 0x80) * 2 + _ram->ReadIO(0xd012, false);
        if (_lineCounter == targetLineNum)
            _processor->SetIRQ();
    }
    if (_timer > 0 && (_ram->ReadIO(0xdc0e, false) & 0x1) > 0)
    {
        _timer -= CYCLES_PER_LINE;
        if (_timer <= 0)
        {
            _timer = 0;
            if (_timerIRQEnable)
            {
                _timerIRQFlag = true;
                _processor->SetIRQ();
            }
        }
    }
}

void Emulator::IOWrite(unsigned short address, unsigned char value)
{
    // Render audio up to the current point on each SID write
    if (address >= 0xd400 && address <= 0xd418)
    {
        _sid->BufferSamples(_processor->Cycles() - _audioCycles);
        _audioCycles = _processor->Cycles();
    }
    if (address == 0xdc0d)
    {
        if ((value & 0x81) == 0x81)
            _timerIRQEnable = true;
        if ((value & 0x81) == 0x1)
            _timerIRQEnable = false;
    }
    if (address == 0xdc0e)
    {
        if ((value & 0x10) > 0)
            _timer = _ram->ReadIO(0xdc04, false) | (_ram->ReadIO(0xdc05, false) << 8);
    }
}

unsigned char Emulator::IORead(unsigned short address, bool& handled)
{
    handled = false;

    if (address == 0xdc00)
    {
        handled = true;
        unsigned char joystick = 0x00;
        if (IsKeyDown(38)) joystick |= 0x1;
        if (IsKeyDown(40)) joystick |= 0x2;
        if (IsKeyDown(37)) joystick |= 0x4;
        if (IsKeyDown(39)) joystick |= 0x8;
        if (IsKeyDown(17)) joystick |= 0x10;
        return joystick ^ 0xff;
    }
    else if (address == 0xdc01)
    {
        handled = true;
        int matrixRow = -1;
        for (int i = 0; i < 8; ++i)
        {
            if ((_ram->ReadIO(0xdc00, false) & (1 << i)) == 0)
            {
                matrixRow = i;
                break;
            }
        }
        return matrixRow >= 0 ? _keyMatrix[matrixRow] : 0xff;
    }
    else if (address == 0xd011)
    {
        handled = true;
        return (_lineCounter >= 0x100 ? 0x80 : 0x00) | (_ram->ReadIO(0xd011, false) & 0x7f);
    }
    else if (address == 0xd012)
    {
        handled = true;
        return _lineCounter & 0xff;
    }
    else if (address == 0xdc0d)
    {
        handled = true;
        unsigned char ret = 0x0;
        if (_timerIRQEnable) 
            ret |= 0x1;
        if (_timerIRQFlag)
        {
            _timerIRQFlag = false;
            ret |= 0x80;
        }
        return ret;
    }
    else
    {
        handled = false;
        return 0;
    }
}

void Emulator::KernalTrap(unsigned short address)
{
    // SETNAM
    if (address == 0xffbd)
    {
        _fileName.resize(_processor->A());
        unsigned short fileNameAddress = (_processor->Y() * 256 + _processor->X());
        for (unsigned i = 0; i < _fileName.size(); ++i)
            _fileName[i] = _ram->ReadRAM(fileNameAddress++);
    }
    // CHKIN (actually open the file stream)
    else if (address == 0xffc6)
    {
        _fileHandle.Close();
        _fileHandle = _disk->OpenFile(_fileName);
        if (!_fileHandle.IsOpen())
        {
            printf("File %c%c not found\n", _fileName[0], _fileName[1]);
        }
    }
    // CHRIN
    else if (address == 0xffcf)
    {
        if (_fileHandle.IsOpen())
        {
            _processor->SetA(_disk->ReadByte(_fileHandle));
            _ram->WriteRAM(0x90, (_fileHandle.IsOpen() ? 0x00 : 0x40));
        }
        else
            _ram->WriteRAM(0x90, 0x42); // EOF, file not found
    }
    // CHKOUT
    else if (address == 0xffc9)
    {
        _fileHandle.Close();
        _fileHandle = _disk->OpenFileForWrite(_fileName);
        if (!_fileHandle.IsOpen())
        {
            printf("File %c%c failed to open for write\n", _fileName[0], _fileName[1]);
        }
    }
    // CHROUT
    else if (address == 0xffd2)
    {
        if (_fileHandle.IsOpen())
            _disk->WriteByte(_fileHandle, _processor->A());
    }
    // CLOSE
    else if (address == 0xffc3)
    {
         _fileHandle.Close();
    }
    // CIOUT - loader detection
    else if (address == 0xffa8)
    {
        _ram->WriteRAM(0x90, 0x80); // Error, prevent fastloader from running
    }
}

void Emulator::HandleKey(unsigned keyCode, bool down)
{
    if (down)
    {
        _keysDown.insert(keyCode);
        if (_keyMappings.find(keyCode) != _keyMappings.end())
        {
            unsigned char matrixSlot = _keyMappings[keyCode];
            _keyMatrix[matrixSlot >> 3] &= (1 << (matrixSlot & 0x7)) ^ 0xff;
        }
    }
    else
    {
        _keysDown.erase(keyCode);
        if (_keyMappings.find(keyCode) != _keyMappings.end())
        {
            unsigned char matrixSlot = _keyMappings[keyCode];
            _keyMatrix[matrixSlot >> 3] |= (1 << (matrixSlot & 0x7));
        }
    }
}

bool Emulator::IsKeyDown(unsigned keyCode)
{
    return _keysDown.find(keyCode) != _keysDown.end();
}