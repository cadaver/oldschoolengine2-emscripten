﻿// MIT License
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

#include <math.h>
#include <stdio.h>
#include "SID.h"
#include "VIC2.h"
#include "RAM64K.h"

#define min(a,b) ((a)<(b)?(a):(b))

unsigned short adsrRateTable[] = { 9, 32, 63, 95, 149, 220, 267, 313, 392, 977, 1954, 3126, 3907, 11720, 19532, 31251 };
unsigned char sustainLevels[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
unsigned char expTargetTable[] = { 1,30,30,30,30,30,16,16,16,16,16,16,16,16,8,8,8,8,8,8,8,8,8,8,8,8,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};

SIDChannel::SIDChannel() :
    state(Release),
    noiseGenerator(0x7ffff8),
    adsrCounter(0),
    adsrExpCounter(0),
    volumeLevel(0)
{
}

void SIDChannel::Clock(int cycles)
{
    if ((waveform & 0x1) != 0)
    {
        if (state == Release)
            state = Attack;
    }
    else
        state = Release;

    int adsrCycles = cycles;

    while (adsrCycles > 0)
    {
        // Calculate how long can run until ADSR counter reaches target
        unsigned short adsrTarget = (state == Attack) ? adsrRateTable[ad >> 4] : ((state == Decay) ? adsrRateTable[ad & 0xf] : adsrRateTable[sr & 0xf]);
        int adsrCyclesNow = min(adsrCycles, adsrCounter < adsrTarget ? adsrTarget - adsrCounter : 0x8000 + adsrTarget - adsrCounter);

        adsrCounter += adsrCyclesNow;
        adsrCounter &= 0x7fff;

        if (adsrCounter == adsrTarget)
        {
            adsrCounter = 0;
            
            switch (state)
            {
            case Attack:
                {
                    adsrExpCounter = 0;
                    ++volumeLevel;
                    if (volumeLevel == 0xff)
                        state = Decay;
                }
                break;

            case Decay:
                {
                    unsigned char adsrExpTarget = volumeLevel < 0x5d ? expTargetTable[volumeLevel] : 1;
                    ++adsrExpCounter;
                    if (adsrExpCounter >= adsrExpTarget)
                    {
                        adsrExpCounter = 0;
                        if (volumeLevel > sustainLevels[sr >> 4])
                            --volumeLevel;
                    }
                }
                break;

            case Release:
                {
                    if (volumeLevel > 0)
                    {
                        unsigned char adsrExpTarget = volumeLevel < 0x5d ? expTargetTable[volumeLevel] : 1;
                        ++adsrExpCounter;
                        if (adsrExpCounter >= adsrExpTarget)
                        {
                            adsrExpCounter = 0;
                            --volumeLevel;
                        }
                    }
                }
                break;
            }
        }

        adsrCycles -= adsrCyclesNow;
    }

    // Testbit
    if ((waveform & 0x8) != 0)
    {
        accumulator = 0;
        // Hack for testbit noise sounding mechanical under this crude emulation
        //noiseGenerator = 0x7ffff8;
        return;
    }

    // If frequency 0, no-op
    if (frequency == 0)
        return;

    // If no noise and no sync target, can use fast clocking
    if ((waveform & 0x80) == 0 && (syncTarget->waveform & 0x2) == 0)
    {
        accumulator += frequency * cycles;
        accumulator &= 0xffffff;
    }
    else
    {
        // Else calculate how long until next noise generator step or sync
        int accumulatorCycles = cycles;

        while (accumulatorCycles > 0)
        {
            int accumulatorCyclesNow = accumulatorCycles;

            if ((waveform & 0x80) != 0)
            {
                if ((accumulator & 0xfffff) < 0x80000)
                    accumulatorCyclesNow = min(accumulatorCyclesNow, (int)(0x80000 - (accumulator & 0xfffff)) / frequency + 1);
                else
                    accumulatorCyclesNow = min(accumulatorCyclesNow, (int)(0x180000 - (accumulator & 0xfffff)) / frequency + 1);
            }

            if ((syncTarget->waveform & 0x2) != 0)
            {
                if (accumulator < 0x800000)
                    accumulatorCyclesNow = min(accumulatorCyclesNow, (int)(0x800000 - accumulator) / frequency + 1);
                else
                    accumulatorCyclesNow = min(accumulatorCyclesNow, (int)(0x1800000 - accumulator) / frequency + 1);
            }

            unsigned lastAccumulator = accumulator;

            accumulator += frequency * accumulatorCyclesNow;
            accumulator &= 0xffffff;

            if ((waveform & 0x80) != 0 && (lastAccumulator & 0x80000) == 0 && (accumulator & 0x80000) != 0)
            {
                unsigned temp = noiseGenerator;
                unsigned step = (temp & 0x400000) ^ ((temp & 0x20000) << 5);
                temp <<= 1;
                if (step > 0)
                    temp |= 1;
                noiseGenerator = temp & 0x7fffff;
            }

            // Sync
            doSync = ((lastAccumulator & 0x800000) == 0 && (accumulator & 0x800000) != 0);

            accumulatorCycles -= accumulatorCyclesNow;
        }
    }
}

void SIDChannel::ResetAccumulator()
{
    accumulator = 0;
}

float SIDChannel::GetOutput()
{
    if (volumeLevel == 0)
        return 0.f;

    unsigned waveOutput = 0;

    switch (waveform & 0xf0)
    {
        case 0x10:
            waveOutput = Triangle();
            break;
        case 0x20:
            waveOutput = Sawtooth();
            break;
        case 0x40:
            waveOutput = Pulse();
            break;
        case 0x50:
            {
                unsigned triangle = Triangle();
                unsigned square = Pulse();
                waveOutput = ((square & triangle & (triangle >> 1)) & (triangle << 1)) << 1;
                if (waveOutput > 0xffff)
                    waveOutput = 0xffff;
            }
            break;
        case 0x60:
            {
                unsigned saw = Sawtooth();
                unsigned square = Pulse();
                waveOutput = ((square & saw & (saw >> 1)) & (saw << 1)) << 1;
                if (waveOutput > 0xffff)
                    waveOutput = 0xffff;
            }
            break;
        case 0x70:
            {
                unsigned triSaw = Triangle() & Sawtooth();
                unsigned square = Pulse();
                waveOutput = ((square & triSaw & (triSaw >> 1)) & (triSaw << 1)) << 1;
                if (waveOutput > 0xffff)
                    waveOutput = 0xffff;
            }
            break;
        case 0x80:
            waveOutput = Noise();
            break;
    }

    return ((int)waveOutput - 0x8000) * volumeLevel / 16777216.f;
}

unsigned SIDChannel::Triangle()
{
    unsigned temp = accumulator ^ ((waveform & 0x4) != 0 ? syncSource->accumulator : 0);
    return ((temp >= 0x800000 ? (accumulator ^ 0xffffff) : accumulator) >> 7) & 0xffff;
}

unsigned SIDChannel::Sawtooth()
{
    return accumulator >> 8;
}

unsigned SIDChannel::Pulse()
{
    return (unsigned)((accumulator >> 12) >= (pulse & 0xfff) ? 0xffff : 0x0);
}

unsigned SIDChannel::Noise()
{
    return ((noiseGenerator & 0x100000) >> 5) + ((noiseGenerator & 0x40000) >> 4) + ((noiseGenerator & 0x4000) >> 1) + 
           ((noiseGenerator & 0x800) << 1) + ((noiseGenerator & 0x200) << 2) + ((noiseGenerator & 0x20) << 5) + 
           ((noiseGenerator & 0x04) << 7) + ((noiseGenerator & 0x01) << 8);
}

SID::SID(RAM64K& ram) :
    _ram(ram),
    _cycleAccumulator(0.f),
    _prevBandPass(0.f),
    _prevLowPass(0.f)
{
    _channels[0].syncTarget = &_channels[1];
    _channels[1].syncTarget = &_channels[2];
    _channels[2].syncTarget = &_channels[0];
    _channels[0].syncSource = &_channels[2];
    _channels[1].syncSource = &_channels[0];
    _channels[2].syncSource = &_channels[1];

    _cyclesPerSample = (63.f*312.f*50.f) / 44100.f;
    _cutoffRatio = -2.f * 3.14f * (18000. / 256.f) / 44100.f; // 6581 filter
}

void SID::BufferSamples(int cpuCycles)
{
    if (cpuCycles == 0)
        return;

    // Adjust amount of cycles to render based on buffer fill
    float multiplier = 1.f + (2048 - (int)samples.size()) / 8192.f;
    // Let multiplier remain at 1 when we're executing the playroutine, to make sure ADSR behavior is accurate
    if (cpuCycles <= CYCLES_PER_LINE*2)
        multiplier = 1.f;
    cpuCycles = (int)(multiplier * cpuCycles);

    for (int i = 0; i < 3; ++i)
    {
        unsigned short ioBase = (unsigned short)(0xd400 + i * 7);
        _channels[i].frequency = (unsigned short)(_ram.ReadIO(ioBase) | (_ram.ReadIO((unsigned short)(ioBase + 1)) << 8));
        _channels[i].pulse = (unsigned short)(_ram.ReadIO((unsigned short)(ioBase + 2)) | (_ram.ReadIO((unsigned short)(ioBase + 3)) << 8));
        _channels[i].waveform = _ram.ReadIO((unsigned short)(ioBase + 4));
        _channels[i].ad = _ram.ReadIO((unsigned short)(ioBase + 5));
        _channels[i].sr = _ram.ReadIO((unsigned short)(ioBase + 6));
    }

    float masterVol = (_ram.ReadIO(0xd418) & 0xf) / 22.5f;
    unsigned char filterSelect = (unsigned char)(_ram.ReadIO(0xd418) & 0x70);
    unsigned char filterCtrl = _ram.ReadIO(0xd417);

    // Filter cutoff & resonance
    // Adjusted to be slightly darker than jsSID
    float cutoff = 0.05f + 0.85f * (sinf((_ram.ReadIO(0xd416) / 255.0f - 0.5f) * M_PI) * 0.5f + 0.5f);
    cutoff = powf(cutoff, 1.3f);
    float resonance = (_ram.ReadIO(0xd417) > 0x3f) ? 7.0f / (_ram.ReadIO(0xd417) >> 4) : 1.75f;

    while (cpuCycles > 0)
    {
        int cyclesToRun = min(cpuCycles, (int)ceilf(_cyclesPerSample - _cycleAccumulator));

        for (int j = 0; j < 3; ++j)
            _channels[j].Clock(cyclesToRun);
        for (int j = 0; j < 3; ++j)
        {
            if (_channels[j].doSync && (_channels[j].syncTarget->waveform & 0x2) != 0)
                _channels[j].syncTarget->ResetAccumulator();
        }
    
        _cycleAccumulator += cyclesToRun;

        if (_cycleAccumulator >= _cyclesPerSample)
        {
            _cycleAccumulator -= _cyclesPerSample;

            float output = 0.f;
            float filterInput = 0.f;

            if ((filterCtrl & 1) == 0)
                output += _channels[0].GetOutput();
            else
                filterInput += _channels[0].GetOutput();

            if ((filterCtrl & 2) == 0)
                output += _channels[1].GetOutput();
            else
                filterInput += _channels[1].GetOutput();

            if ((filterCtrl & 4) == 0)
                output += _channels[2].GetOutput();
            else
                filterInput += _channels[2].GetOutput();

            // Highpass
            float temp = filterInput + _prevBandPass * resonance + _prevLowPass;
            if ((filterSelect & 0x40) != 0)
                output -= temp;
            // Bandpass
            temp = _prevBandPass - temp * cutoff;
            _prevBandPass = temp;
            if ((filterSelect & 0x20) != 0)
                output -= temp;
            // Lowpass
            temp = _prevLowPass + temp * cutoff;
            _prevLowPass = temp;
            if ((filterSelect & 0x10) != 0)
                output += temp;

            output *= masterVol;
            if (output < -1.f)
                output = -1.f;
            if (output > 1.f)
                output = 1.f;
            samples.push_back((short)(output * 32767));
        }
        
        cpuCycles -= cyclesToRun;
    }
}