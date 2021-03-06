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

#include <emscripten.h>
#include <emscripten/html5.h>
#include "Emulator.h"

const double frameTime = 1000.0 / 50.0;

Emulator* emulator = nullptr;
double lastTime;
double timeAccumulator;

void FrameCallback();
EM_BOOL KeyCallback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);

int main(int argc, char** argv)
{
    // Load old saves
    EM_ASM(
        FS.mkdir('/savedata');
        FS.mount(IDBFS,{},'/savedata');
        FS.syncfs(true, function(err) {
            if (!err) {
                Module.print('Savefiles initialized');
            }
        });
    );

    std::string diskImageName;
    for (int i = 0; i < argc; ++i)
    {
        std::string argument(argv[i]);
        if (argument.find("diskimage=") == 0)
            diskImageName = argument.substr(10);
    }

    emulator = new Emulator(diskImageName);
    
    emscripten_set_main_loop(FrameCallback, 0, 0);
    emscripten_set_keydown_callback(0, 0, 1, KeyCallback);
    emscripten_set_keyup_callback(0, 0, 1, KeyCallback);
    lastTime = emscripten_get_now();
}

void FrameCallback()
{
    double currentTime = emscripten_get_now();
    timeAccumulator += (currentTime - lastTime);
    lastTime = currentTime;

    // Clamp time accumulator in case emulator was paused
    if (timeAccumulator > 2 * frameTime)
        timeAccumulator = 2 * frameTime;
        
    if (timeAccumulator >= frameTime)
    {
        timeAccumulator -= frameTime;
        emulator->Update();
    }

    emulator->QueueAudio();
}

EM_BOOL KeyCallback(int eventType, const EmscriptenKeyboardEvent *e, void * /*userData*/)
{
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN)
        emulator->HandleKey(e->keyCode, true);
    else if (eventType == EMSCRIPTEN_EVENT_KEYUP)
        emulator->HandleKey(e->keyCode, false);
    return 0;
}