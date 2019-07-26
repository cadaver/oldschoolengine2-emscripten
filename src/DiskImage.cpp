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

#include <stdio.h>
#include <emscripten.h>
#include "DiskImage.h"

FileHandle::FileHandle() :
    track(0),
    sector(0),
    offset(0),
    length(0),
    reader(nullptr),
    writer(nullptr)
{
}

bool FileHandle::IsOpen() const
{
    return track != 0 || reader != nullptr || writer != nullptr;
}

void FileHandle::Close()
{
    if (reader)
    {
        fclose(reader);
        reader = nullptr;
    }
    if (writer)
    {
        fclose(writer);
        writer = nullptr;

        // When a file written to is closed, begin sync to persistent file system
        EM_ASM(
            FS.syncfs(false, function(err) {
            });
        );
    }
    track = 0;
}

int DiskImage::_d64SectorsPerTrack[] = {
    0, 21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
    19,19,19,19,19,19,19,
    18,18,18,18,18,18,
    17,17,17,17,17
};

DiskImage::DiskImage(const std::string& name)
{
    std::string imagePath = "diskimages/" + name;

    FILE* imageFile = fopen(imagePath.c_str(), "rb");
    if (imageFile)
    {
        fseek(imageFile, 0, SEEK_END);
        int length = ftell(imageFile);
        _data.resize(length);
        fseek(imageFile, 0, SEEK_SET);
        fread(&_data[0], length, 1, imageFile);
        fclose(imageFile);

        _name = name;
        _type = length == 174848 ? D64 : D81;
        printf("Opened disk image %s, size %d\n", name.c_str(), length);
    }
    else
        printf("Failed to open disk image %s\n", name.c_str());

    MakeSectorTable();
}

int DiskImage::GetSectorOffset(int track, int sector)
{
    return _sectorOffsets[track][sector];
}

FileHandle DiskImage::OpenFileForWrite(const std::vector<unsigned char>& fileName)
{
    FILE* saveFile = fopen(GetSaveFileName(fileName).c_str(), "wb");
    FileHandle ret;
    ret.writer = saveFile;

    return ret;
}

FileHandle DiskImage::OpenFile(const std::vector<unsigned char>& fileName)
{
    // Check for savefile
    FILE* saveFile = fopen(GetSaveFileName(fileName).c_str(), "rb");
    if (saveFile)
    {
        FileHandle ret;
        ret.reader = saveFile;
        fseek(saveFile, 0, SEEK_END);
        ret.length = ftell(saveFile);
        fseek(saveFile, 0, SEEK_SET);
        return ret;
    }

    int dirTrack = (_type == D64) ? 18 : 40;
    int dirSector = (_type == D64) ? 1 : 3;

    while (dirTrack > 0)
    {
        int offset = GetSectorOffset(dirTrack, dirSector);
        for (int d = 2; d < 256; d += 32)
        {
            if (_data[offset + d] == 0x82)
            {
                bool match = true;

                // If no filename specified, open any file
                for (unsigned e = 0; e < fileName.size(); ++e)
                {
                    if (_data[offset + d + 3 + e] != fileName[e])
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    FileHandle ret;
                    ret.track = _data[offset + d + 1];
                    ret.sector = _data[offset + d + 2];
                    ret.offset = 2;
                    return ret;
                }
            }
        }

        // Next sector
        dirTrack = _data[offset];
        dirSector = _data[offset + 1];
    }

    return FileHandle();
}

unsigned char DiskImage::ReadByte(FileHandle& handle)
{
    if (!handle.IsOpen())
        return 0;

    unsigned char ret;

    if (handle.reader)
    {
        ret = fgetc(handle.reader);
        // Need to know EOF in advance
        if (ftell(handle.reader) >= handle.length)
            handle.Close();
        return ret;
    }

    int sectorStart = GetSectorOffset(handle.track, handle.sector);
    ret = _data[sectorStart + handle.offset];

    // Last sector?
    if (_data[sectorStart] == 0)
    {
        // Last byte read?
        if (handle.offset >= _data[sectorStart + 1])
            handle.track = 0;
        else
            ++handle.offset;
    }
    else
    {
        ++handle.offset;
        if (handle.offset >= 256)
        {
            handle.track = _data[sectorStart];
            handle.sector = _data[sectorStart + 1];
            handle.offset = 2;
        }
    }

    return ret;
}

void DiskImage::WriteByte(FileHandle& handle, unsigned char value)
{
    if (handle.writer)
        fputc(value, handle.writer);
}

std::string DiskImage::GetSaveFileName(const std::vector<unsigned char>& fileName)
{
    std::string filePath = "/savedata/" + _name;
    for (unsigned i = 0; i < fileName.size(); ++i)
        filePath += (char)fileName[i];
    return filePath;
}

void DiskImage::MakeSectorTable()
{
    int offset = 0;

    if (_type == D64)
    {
        for (int c = 1; c <= MAX_D64_TRACK; ++c)
        {
            for (int d = 0; d < _d64SectorsPerTrack[c]; ++d)
            {
                _sectorOffsets[c][d] = offset;
                offset += 256;
            }
        }
    }
    else
    {
        for (int c = 1; c <= MAX_TRACK; ++c)
        {
            for (int d = 0; d < MAX_SECTOR; ++d)
            {
                _sectorOffsets[c][d] = offset;
                offset += 256;
            }
        }
    }
}