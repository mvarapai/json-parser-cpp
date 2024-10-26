//          utilstr.cpp
//
//  Provides definitions for string utility functions.
//
//  (c) Mikalai Varapai, 2024

#include "utilstr.h"

#include <fstream>
#include <iostream>

//  Replaces all given substrings
std::string utilstr::ReplaceAll(std::string& str, std::string from, std::string to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

//  Utility function to return trimmed string.
//  Changes the reference and returns the copy.
inline std::string utilstr::Trim(std::string& str, const char c)
{
    str.erase(0, str.find_first_not_of(c));
    str.erase(str.find_last_not_of(c) + 1, str.size());
    return str;
}

//  Trim a sequence of characters.
inline std::string utilstr::Trim(std::string& str, const std::string& cs)
{
    str.erase(0, str.find_first_not_of(cs));
    str.erase(str.find_last_not_of(cs) + 1, str.size());
    return str;
}

//  Checks whether given char is unique in a string
inline bool utilstr::UniqueChar(const std::string& str, const char c)
{
    return str.find_first_of(c) == str.find_last_of(c);
}

//  Checks whether a string contains given char
inline bool utilstr::Contains(const std::string& str, const char c)
{
    return str.find(c) != std::string::npos;
}

std::string utilstr::ReadFromFile(const std::string& filename)
{
    // Try to open the file
    std::ifstream t(filename);

    if (!t.is_open())
    {
        std::cerr << "Cannot open the file." << std::endl;
        return "";
    }

    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);

    return buffer;
}