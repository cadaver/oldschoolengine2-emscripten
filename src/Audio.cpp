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

#include <AL/al.h>
#include <AL/alc.h>
#include <vector>
#include <stdio.h>
#include "Audio.h"

ALCdevice* dev;
ALCcontext* ctx;
ALuint source;
std::vector<ALuint> freeBuffers;

void Audio::Init(int numBuffers)
{
    dev = alcOpenDevice(nullptr);
    ctx = alcCreateContext(dev, nullptr);
    alcMakeContextCurrent(ctx);
    freeBuffers.resize(numBuffers);
    alGenBuffers(numBuffers, &freeBuffers[0]);
    alGenSources(1, &source);
}

int Audio::NumFreeBuffers()
{
    ALint numProcessed;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &numProcessed);
    while (numProcessed--)
    {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
        freeBuffers.push_back(buffer);
    }

    return freeBuffers.size();
}

bool Audio::QueueBuffer(short* samples, int numSamples)
{
    if (!NumFreeBuffers())
        return false;

    ALuint buffer = freeBuffers[0];
    freeBuffers.erase(freeBuffers.begin());
    alBufferData(buffer, AL_FORMAT_MONO16, samples, numSamples * sizeof(short), 44100);
    alSourceQueueBuffers(source, 1, &buffer);

    // Start playback if not playing yet or underrun happened
    ALenum state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
        alSourcePlay(source);

    return true;
}