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

#pragma once

#include <map>
#include <set>
#include "DiskImage.h"

class MOS6502;
class RAM64K;
class VIC2;
class SID;

class Emulator
{
public:
    Emulator(const std::string& imageName);
    ~Emulator();

    void Update();
    void QueueAudio();

    void KernalTrap(unsigned short address);
    unsigned char IORead(unsigned short address, bool& handled);
    void IOWrite(unsigned short address, unsigned char value);
    void HandleKey(unsigned keyCode, bool down);

private:
    void InitMemory();
    void BootGame();
    void RunFrame();
    void ExecuteLine(int lineNum, bool visible);
    void UpdateLineCounterAndIRQ(int lineNum);
    bool IsKeyDown(unsigned keyCode);

    RAM64K* _ram;
    MOS6502* _processor;
    VIC2* _vic2;
    SID* _sid;
    DiskImage* _disk;
    FileHandle _fileHandle;
    std::vector<unsigned char> _fileName;
    std::set<unsigned> _keysDown;
    std::map<unsigned, unsigned char> _keyMappings;
    int _lineCounter;
    int _audioCycles;
    int _timer;
    bool _timerIRQEnable;
    bool _timerIRQFlag;
    unsigned char _keyMatrix[8];
};
