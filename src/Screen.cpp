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

#include <stdio.h>
#include <memory.h>
#include <GLES2/gl2.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "Screen.h"

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
GLuint quadBuffer;
GLuint matPos;
GLuint screenTexture;
GLuint quadProgram;

GLuint CompileShader(GLenum shaderType, const char* src)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    return shader;
}

GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader)
{
   GLuint program = glCreateProgram();
   glAttachShader(program, vertexShader);
   glAttachShader(program, fragmentShader);
   glBindAttribLocation(program, 0, "pos");
   glLinkProgram(program);
   glUseProgram(program);
   return program;
}

GLuint CreateTexture()
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

GLuint CreateVertexBuffer(int size, const float* data)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);
    return buffer;
}

void Screen::Init()
{
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = 0;
    glContext = emscripten_webgl_create_context(0, &attrs);
    emscripten_webgl_make_context_current(glContext);

    static const char vertexShader[] =
        "attribute vec4 pos;"
        "varying vec2 uv;"
        "uniform mat4 mat;"
        "void main(){"
            "uv = pos.xy;"
            "gl_Position = mat * pos;"
        "}";
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);

    static const char fragmentShader[] =
        "precision lowp float;"
        "uniform sampler2D tex;"
        "varying vec2 uv;"
        "void main(){"
            "gl_FragColor = fract(uv.y * 200.0) >= 0.5 ? texture2D(tex,uv) : texture2D(tex,uv) * 0.5;"
        "}";
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    quadProgram = CreateProgram(vs, fs);
    matPos = glGetUniformLocation(quadProgram, "mat");

    screenTexture = CreateTexture();
    unsigned initialScreenData[320 * 200];
    memset(initialScreenData, 0, sizeof initialScreenData);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, &initialScreenData);

    const float quadBufferData[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
    quadBuffer = CreateVertexBuffer(8, quadBufferData);
}

void Screen::Redraw(unsigned* screenData)
{
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 320, 200, GL_RGBA, GL_UNSIGNED_BYTE, screenData);

    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;

    float mat[16] = { w*2.f, 0, 0, 0, 0, h*2.f, 0, 0, 0, 0, 1, 0, x*2.f-1.f, y*2.f-1.f, 0, 1};
    glUniformMatrix4fv(matPos, 1, 0, mat);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glUseProgram(quadProgram);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}
