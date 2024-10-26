/*****************************************************************//**
 * \file	utilstr.h
 * \brief   Header file that defines several functions that can be used
 *			for more efficient text processing.
 * 
 * \author	Mikalai Varapai
 * \date	October 2024
 *********************************************************************/

#pragma once

#include <string>

namespace utilstr
{
    //  Replaces all given substrings
    std::string ReplaceAll(std::string& str, std::string from, std::string to);

    //  Utility function to return trimmed string.
    //  Changes the reference and returns the copy.
    std::string Trim(std::string& str, const char c);

    //  Trim a sequence of characters.
    std::string Trim(std::string& str, const std::string& cs);

    //  Checks whether given char is unique in a string
    bool UniqueChar(const std::string& str, const char c);

    //  Checks whether a string contains given char
    bool Contains(const std::string& str, const char c);

    //  Returns std::string object of the file "filename"
    std::string ReadFromFile(const std::string& filename);

    //  Finds whether string begins and ends with the same character.
    //  If string contains less than two characters, returns false.
    bool BeginsAndEndsWith(const std::string& str, const char begins, const char ends);
    bool BeginsAndEndsWith(const std::string& str, const char c);

    //  Trims any character from the beginning and the end of the string.
    //  If length < 2, returns empty string.
    std::string TrimOneChar(std::string str);
};