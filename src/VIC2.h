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

const int NUM_LINES = 312;
const int FIRST_VISIBLE_LINE = 50;
const int FIRST_INVISIBLE_LINE = 250;
const int CYCLES_PER_LINE = 63;

class RAM64K;

class VIC2
{
public:
    VIC2(RAM64K& ram);
    void BeginFrame();
    void RenderNextLine();
    unsigned* Pixels() { return &_pixels[0]; }

    static unsigned char bitValues[8];

private:
    void DoBadLine(int yScroll);

    RAM64K& _ram;
    static unsigned _palette[16];
    int _lineNum;
    int _nextBadlineLineNum;
    int _currentCharRow;
    int _charRow;
    int _bitmapRow;
    bool _idleState;
    bool _spriteActive[8];
    unsigned char _spriteRow[8];
    unsigned char _lineChars[40];
    unsigned char _lineColors[40];
    unsigned _pixels[320*200];
};