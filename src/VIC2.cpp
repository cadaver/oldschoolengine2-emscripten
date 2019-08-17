// MIT License
// 
// Copyright (c) 2018-2019 Lasse Oorni
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
#include "VIC2.h"
#include "RAM64K.h"

unsigned char VIC2::bitValues[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
unsigned VIC2::_palette[] = { 0xff000000, 0xffffffff, 0xff2b3768, 0xffb2a470, 0xff863d6f, 0xff438d58, 0xff792835, 0xff6fc7b8,
                       0xff254f6f, 0xff003943, 0xff59679a, 0xff444444, 0xff6c6c6c, 0xff84d29a, 0xffb55e6c, 0xff959595 };

VIC2::VIC2(RAM64K& ram) :
    _ram(ram)
{
}

void VIC2::BeginFrame()
{
    // Should be called just before visible line
    _lineNum = 0;
    _nextBadlineLineNum = 0;
    _charRow = 0;
    _currentCharRow = 0;
    _bitmapRow = 0;
    _idleState = true;
    for (int i = 0; i < 8; ++i)
        _spriteActive[i] = false;
}

void VIC2::DoBadLine(int yScroll)
{
    _currentCharRow = _charRow;

    if (_charRow >= 25)
    {
        _idleState = true;
        return;
    }

    _idleState = false;
    _bitmapRow = _charRow;
    _nextBadlineLineNum = (_charRow + 1) * 8 + yScroll - 3;
    ++_charRow;
}

void VIC2::RenderNextLine()
{
    if (_lineNum >= 200)
        return;

    int pixelStart = (199 - _lineNum) * 320;

    unsigned black = _palette[0];
    unsigned bgColor = _palette[_ram.ReadIO(0xd021) & 0xf];
    unsigned borderColor = _palette[_ram.ReadIO(0xd020) & 0xf];

    unsigned char control = _ram.ReadIO(0xd011);
    int yScroll = control & 0x7;
    int xScroll = _ram.ReadIO(0xd016) & 0x7;
    bool hBorders = (_ram.ReadIO(0xd016) & 0x8) == 0;
    bool vBorders = (control & 0x8) == 0;
    bool displayEnable = (control & 0x10) != 0;
    bool bitmapMode = (control & 0x20) != 0;
    bool multiColor = (_ram.ReadIO(0xd016) & 0x10) != 0;
    bool ebcMode = (control & 0x40) != 0;

    unsigned short videoBank = (0xc000 - (_ram.ReadIO(0xdd00) & 0x3) * 0x4000);
    unsigned short charData = (videoBank + (_ram.ReadIO(0xd018) & 0xe) * 0x400);
    unsigned short bitmapData = (videoBank + (_ram.ReadIO(0xd018) & 0x8) * 0x400);
    unsigned short screenAddress = (videoBank + (_ram.ReadIO(0xd018) & 0xf0) * 0x40);

    unsigned mc1 = _palette[_ram.ReadIO(0xd022) & 0xf];
    unsigned mc2 = _palette[_ram.ReadIO(0xd023) & 0xf];
    unsigned mc3 = _palette[_ram.ReadIO(0xd024) & 0xf];

    if ((_lineNum == 0 && ((_lineNum + 3) & 0x7) >= yScroll) || (((_lineNum + 3) & 0x7) == yScroll && _lineNum >= _nextBadlineLineNum))
        DoBadLine(yScroll);

    // HACK for Hessian scrolling: actually get chars & colors every line
    if (!_idleState)
    {
        for (int i = 0; i < 40; ++i)
        {
            _lineChars[i] = _ram.ReadRAM((screenAddress + _currentCharRow * 40 + i));
            _lineColors[i] = (unsigned char)(_ram.ReadIO((0xd800 + _currentCharRow * 40 + i)) & 0xf);
        }
    }

    int charIndex = 0;
    int charRow = (_lineNum + 3 - yScroll) & 0x7;
    int bit = 0x80 << xScroll;

    bool renderSprites = true;

    // V-border or display off
    if (!displayEnable || (vBorders && (_lineNum < 4 || _lineNum >= 196)))
    {
        for (int i = 0; i < 320; ++i)
            _pixels[pixelStart + i] = borderColor;
        renderSprites = false;
    }
    else
    // Idle state or illegal mode (render just black)
    if (_idleState || (ebcMode && multiColor))
    {
        for (int i = 0; i < 320; ++i)
            _pixels[pixelStart + i] = black;
    }
    // Charmode
    else if (!bitmapMode)
    {
        unsigned char charByte = ebcMode ? _ram.ReadRAM((charData + (_lineChars[charIndex] & 0x3f) * 8 + charRow)) :
            _ram.ReadRAM((charData + _lineChars[charIndex] * 8 + charRow));

        // Singlecolor
        if (!multiColor)
        {
            for (int i = 0; i < 320; ++i)
            {
                if (hBorders && (i < 7 || i >= 311))
                    _pixels[pixelStart + i] = borderColor;
                else
                {
                    if (bit > 0x80 || charByte == 0 || (charByte & bit) == 0)
                    {
                        if (!ebcMode)
                            _pixels[pixelStart + i] = bgColor;
                        else
                        {
                            switch (_lineChars[charIndex] >> 6)
                            {
                                case 0:
                                    _pixels[pixelStart + i] = bgColor;
                                    break;
                                case 1:
                                    _pixels[pixelStart + i] = mc1;
                                    break;
                                case 2:
                                    _pixels[pixelStart + i] = mc2;
                                    break;
                                case 3:
                                    _pixels[pixelStart + i] = mc3;
                                    break;
                            }
                        }
                    }
                    else
                        _pixels[pixelStart + i] = _palette[_lineColors[charIndex]];
                }

                bit >>= 1;
                if (bit == 0)
                {
                    bit = 0x80;
                    ++charIndex;
                    if (charIndex < 40)
                    {
                        charByte = ebcMode ? _ram.ReadRAM((charData + (_lineChars[charIndex] & 0x3f) * 8 + charRow)) :
                            _ram.ReadRAM((charData + _lineChars[charIndex] * 8 + charRow));
                    }
                }
            }
        }
        // Multicolor
        else
        {
            int bitPairShift = 0x7 + xScroll;

            for (int i = 0; i < 320; ++i)
            {
                if (hBorders && (i < 7 || i >= 311))
                    _pixels[pixelStart + i] = borderColor;
                else
                {
                    if (bit > 0x80 || charByte == 0)
                        _pixels[pixelStart + i] = bgColor;
                    else
                    {
                        if (_lineColors[charIndex] < 0x8)
                        {
                            if ((charByte & bit) != 0)
                                _pixels[pixelStart + i] = _palette[_lineColors[charIndex]];
                            else
                                _pixels[pixelStart + i] = bgColor;
                        }
                        else
                        {
                            unsigned char bitPair = (unsigned char)((charByte >> (bitPairShift & 0x6)) & 0x3);
                            switch (bitPair)
                            {
                                case 0:
                                    _pixels[pixelStart + i] = bgColor;
                                    break;
                                case 1:
                                    _pixels[pixelStart + i] = mc1;
                                    break;
                                case 2:
                                    _pixels[pixelStart + i] = mc2;
                                    break;
                                case 3:
                                    _pixels[pixelStart + i] = _palette[_lineColors[charIndex] & 0x7];
                                    break;
                            }
                        }
                    }
                }

                bit >>= 1;
                --bitPairShift;
                if (bit == 0)
                {
                    bit = 0x80;
                    bitPairShift = 0x7;
                    ++charIndex;
                    if (charIndex < 40)
                        charByte = _ram.ReadRAM((charData + _lineChars[charIndex] * 8 + charRow));
                }
            }
        }
    }
    else if (bitmapMode)
    {
        unsigned char charByte = _ram.ReadRAM((bitmapData + _bitmapRow * 320 + charIndex * 8 + charRow));

        // Singlecolor
        if (!multiColor)
        {
            for (int i = 0; i < 320; ++i)
            {
                if (hBorders && (i < 7 || i >= 311))
                    _pixels[pixelStart + i] = borderColor;
                else
                {
                    if (bit > 0x80 || charByte == 0)
                        _pixels[pixelStart + i] = bgColor;
                    else
                    {
                        if ((charByte & bit) != 0)
                            _pixels[pixelStart + i] = _palette[_lineChars[charIndex] >> 4];
                        else
                            _pixels[pixelStart + i] = _palette[_lineChars[charIndex] & 0xf];
                    }
                }

                bit >>= 1;
                if (bit == 0)
                {
                    bit = 0x80;
                    ++charIndex;
                    if (charIndex < 40)
                        charByte = _ram.ReadRAM((bitmapData + _bitmapRow * 320 + charIndex * 8 + charRow));
                }
            }
        }
        // Multicolor
        else
        {
            int bitPairShift = 0x7 + xScroll;

            for (int i = 0; i < 320; ++i)
            {
                if (hBorders && (i < 7 || i >= 311))
                    _pixels[pixelStart + i] = borderColor;
                else
                {
                    if (bit > 0x80 || charByte == 0)
                        _pixels[pixelStart + i] = bgColor;
                    else
                    {
                        unsigned char bitPair = (unsigned char)((charByte >> (bitPairShift & 0x6)) & 0x3);
                        switch (bitPair)
                        {
                            case 0:
                                _pixels[pixelStart + i] = bgColor;
                                break;
                            case 1:
                                _pixels[pixelStart + i] = _palette[_lineChars[charIndex] >> 4];
                                break;
                            case 2:
                                _pixels[pixelStart + i] = _palette[_lineChars[charIndex] & 0xf];
                                break;
                            case 3:
                                _pixels[pixelStart + i] = _palette[_lineColors[charIndex] & 0xf];
                                break;
                        }
                    }
                }

                bit >>= 1;
                --bitPairShift;
                if (bit == 0)
                {
                    bit = 0x80;
                    bitPairShift = 0x7;
                    ++charIndex;
                    if (charIndex < 40)
                        charByte = _ram.ReadRAM((bitmapData + _bitmapRow * 320 + charIndex * 8 + charRow));
                }
            }
        }
    }

    unsigned char spriteFlags = _ram.ReadIO(0xd015);
    unsigned char spriteMCFlags = _ram.ReadIO(0xd01c);
    unsigned char spriteXMSBFlags = _ram.ReadIO(0xd010);
    unsigned char spriteXExpandFlags = _ram.ReadIO(0xd01d);

    unsigned sprMc1 = _palette[_ram.ReadIO(0xd025) & 0xf];
    unsigned sprMc2 = _palette[_ram.ReadIO(0xd026) & 0xf];

    for (int i = 7; i >= 0; --i)
    {
        unsigned char spriteY = _ram.ReadIO((0xd001 + i * 2));
        if (!_spriteActive[i] && (spriteFlags & bitValues[i]) != 0)
        {
            if (_lineNum == spriteY - 50 || (_lineNum == 0 && spriteY >= 30 && spriteY < 50))
            {
                _spriteActive[i] = true;
                _spriteRow[i] = (unsigned char)(_lineNum + 50 - spriteY);
            }
        }

        if (_spriteActive[i])
        {
            // TODO: Y expansion, background priority
            if (renderSprites)
            {
                int startX = _ram.ReadIO((0xd000 + i * 2));
                if ((spriteXMSBFlags & bitValues[i]) != 0)
                    startX += 256;
                bool xExpand = (spriteXExpandFlags & bitValues[i]) != 0;
                if (xExpand && startX >= 480 && startX < 504)
                    startX -= 504;

                unsigned short spriteData = (videoBank + _ram.ReadRAM((screenAddress + 0x3f8 + i)) * 0x40 + _spriteRow[i] * 3);
                unsigned spriteColor = _palette[_ram.ReadIO((0xd027 + i)) & 0xf];

                for (int j = 0; j < 24; ++j)
                {
                    int k = xExpand ? (startX + j * 2 - 24) : (startX + j - 24);
                    unsigned char spriteByte = _ram.ReadRAM((spriteData + (j >> 3)));

                    for (int l = 0; l < (xExpand ? 2 : 1); ++l)
                    {
                        if (k >= 0 && k <= 320 && (!hBorders || (k >= 7 && k < 311)) && spriteByte != 0)
                        {
                            if ((spriteMCFlags & bitValues[i]) != 0)
                            {
                                unsigned char bitPair = ((spriteByte >> (6 - (j & 0x6))) & 0x3);
                                switch (bitPair)
                                {
                                    case 1:
                                        _pixels[pixelStart + k] = sprMc1;
                                        break;
                                    case 2:
                                        _pixels[pixelStart + k] = spriteColor;
                                        break;
                                    case 3:
                                        _pixels[pixelStart + k] = sprMc2;
                                        break;
                                }
                            }
                            else
                            {
                                if ((spriteByte & bitValues[7 - (j & 0x7)]) != 0)
                                    _pixels[pixelStart + k] = spriteColor;
                            }
                        }
                        ++k;
                    }
                }
            }

            ++_spriteRow[i];
            if (_spriteRow[i] >= 21)
                _spriteActive[i] = false;
        }
    }

    // Done, increment linecount
    ++_lineNum;
}