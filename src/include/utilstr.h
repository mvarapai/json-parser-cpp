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
    inline std::string Trim(std::string& str, const char c);

    //  Trim a sequence of characters.
    inline std::string Trim(std::string& str, const std::string& cs);

    //  Checks whether given char is unique in a string
    inline bool UniqueChar(const std::string& str, const char c);

    //  Checks whether a string contains given char
    inline bool Contains(const std::string& str, const char c);

    //  Returns std::string object of the file "filename"
    std::string ReadFromFile(const std::string& filename);
};