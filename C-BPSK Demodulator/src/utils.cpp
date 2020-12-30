#include "utils.h"
#include <fstream>

// Return filesize
size_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::size_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

int8_t clamp(float x)
{
    if (x < -128.0)
        return -127;
    if (x > 127.0)
        return 127;
    return x;
}