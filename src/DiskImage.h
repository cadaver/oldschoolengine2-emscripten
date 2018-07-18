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

#include <string>
#include <vector>

const int MAX_D64_TRACK = 35;
const int MAX_D64_SECTOR = 21;
const int MAX_TRACK = 80;
const int MAX_SECTOR = 40;

enum DiskType
{
    D64 = 0,
    D81
};

class FileHandle
{
public:
    FileHandle();
    bool IsOpen() const;
    void Close();

    int track;
    int sector;
    int offset;
    FILE* reader;
    FILE* writer;
};

class DiskImage
{
public:
    DiskImage(const std::string& name);
    FileHandle OpenFileForWrite(const std::vector<unsigned char>& fileName);
    FileHandle OpenFile(const std::vector<unsigned char>& fileName);
    unsigned char ReadByte(FileHandle& handle);
    void WriteByte(FileHandle& handle, unsigned char value);

private:
    void MakeSectorTable();
    int GetSectorOffset(int track, int sector);
    std::string GetSaveFileName(const std::vector<unsigned char>& fileName);

    static int _d64SectorsPerTrack[];

    std::string _name;
    DiskType _type;
    int _sectorOffsets[MAX_TRACK+1][MAX_SECTOR];
    std::vector<unsigned char> _data;
};