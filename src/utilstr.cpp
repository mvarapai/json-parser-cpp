//          utilstr.cpp
//
//  Provides definitions for string utility functions.
//
//  (c) Mikalai Varapai, 2024

#include "json_parser.h"
#include "utilstr.h"

#include <fstream>
#include <iostream>

//  Replaces all given substrings
std::string utilstr::ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

//  Replace all characters in a sequence
std::string utilstr::ReplaceAllChars(std::string& str, const std::string& charArrayFrom, const std::string& to)
{
    for (const char c : charArrayFrom)
    {
        ReplaceAll(str, std::string(1, c), to);
    }
    return str;
}

//  Utility function to return trimmed string.
//  Changes the reference and returns the copy.
std::string utilstr::Trim(std::string& str, const char c)
{
    str.erase(0, str.find_first_not_of(c));
    str.erase(str.find_last_not_of(c) + 1, str.size());
    return str;
}

//  Trim a sequence of characters.
std::string utilstr::Trim(std::string& str, const std::string& cs)
{
    str.erase(0, str.find_first_not_of(cs));
    str.erase(str.find_last_not_of(cs) + 1, str.size());
    return str;
}

//  Checks whether given char is unique in a string
bool utilstr::UniqueChar(const std::string& str, const char c)
{
    return str.find_first_of(c) == str.find_last_of(c);
}

//  Checks whether a string contains given char
bool utilstr::Contains(const std::string& str, const char c)
{
    return str.find(c) != std::string::npos;
}

std::string utilstr::ReadFromFile(const std::string& filename)
{
    // Try to open the file
    std::ifstream t(filename);

    if (!t.is_open())
    {
        return "";
    }

    // Get file size
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();

    // Allocate and read to a string
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);

    t.close();

    return buffer;
}

bool utilstr::BeginsAndEndsWith(const std::string& str, const char begins, const char ends)
{
    if (str.size() < 2) return false;
    return (str.front() == begins && str.back() == ends);
}

bool utilstr::BeginsAndEndsWith(const std::string& str, const char c)
{
    return BeginsAndEndsWith(str, c, c);
}

bool utilstr::BeginsAndEndsWith(JSONString str, const char begins, const char ends)
{
    if (str.Size() < 2) return false;
    return (str.at(0) == begins && str.at(str.Size() - 1));
}

bool utilstr::BeginsAndEndsWith(JSONString str, const char c)
{
    return BeginsAndEndsWith(str, c, c);
}

std::string utilstr::TrimOneChar(std::string str)
{
    if (str.size() < 2) return "";
    str.erase(0, 1);
    str.erase(str.size() - 1, str.size());

    return str;
}

bool utilstr::Split(std::string src, char delimiter, std::string& substring, size_t& prevPos)
{
    if (prevPos > src.size()) return false;

    size_t pos = src.find(delimiter, prevPos);
    if (pos == std::string::npos) pos = src.size();

    substring = src.substr(prevPos, (pos - prevPos));
    prevPos = pos + 1;

    return true;
}

std::string utilstr::ScanIndex(std::string source, size_t& pos)
{
    pos = source.find('[', pos);
    if (pos == source.npos)
    {
        std::string errorMsg = "[ERROR] No '[' found.\n";
        std::cout << errorMsg;
        return "";
    }

    size_t depth = 0;
    size_t startPos = pos + 1;

    // Iterate through the rest of string, startng with '['
    for (size_t i = pos; i < source.size(); i++)
    {
        const char c = source.at(i);

        if (c == '[') depth++;
        if (c == ']') depth--;

        if (depth == 0)
        {
            pos = i + 1;
            return source.substr(startPos, i - startPos);
        }
    }

    std::cout << "[ERROR] No closing parenthesis." << std::endl;
    return "";
}

bool utilstr::IsIntLiteral(std::string str)
{
    for (char c : str)
    {
        if (!isdigit(c)) return false;
    }
    return true;
}
