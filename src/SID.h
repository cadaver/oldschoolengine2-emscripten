// MIT License
// 
// Copyright (c) 2018-2024 Lasse Oorni
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

// Waveform generation and filter implementation based on jsSID
//
// jsSID by Hermit (Mihaly Horvath) : a javascript SID emulator and player for the Web Audio API
// (Year 2016) http://hermit.sidrip.com

#pragma once

#include <vector>

class RAM64K;

enum ADSRState
{
    Attack = 0,
    Decay,
    Release
};

class SIDChannel
{
public:
    SIDChannel();
    void Clock(int cycles);
    void ResetAccumulator();
    float GetOutput();
    unsigned Triangle();
    unsigned Sawtooth();
    unsigned Pulse();
    unsigned Noise();

    SIDChannel* syncTarget;
    SIDChannel* syncSource;

    unsigned short frequency;
    unsigned char ad;
    unsigned char sr;
    unsigned short pulse;
    unsigned char waveform;
    bool doSync = false;

    ADSRState state;
    unsigned accumulator;
    unsigned noiseGenerator;
    unsigned short adsrCounter;
    unsigned char adsrExpCounter;
    unsigned char volumeLevel;
};

class SID
{
public:
    SID(RAM64K& ram);
    void BufferSamples(int cpuCycles);
    
    std::vector<short> samples;

private:
    RAM64K& _ram;
    SIDChannel _channels[3];

    float _cutoffRatio;
    float _cyclesPerSample;
    float _cycleAccumulator;
    float _prevBandPass;
    float _prevLowPass;
};