//          utilstr.cpp
//
//  Provides definitions for string utility functions.
//
//  (c) Mikalai Varapai, 2024

#include "json_parser.h"
#include "utilstr.h"
#include "query.h"

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

bool utilstr::IsNumLiteral(std::string str)
{
    for (char c : str)
    {
        if (!isdigit(c) && c != '.') return false;
    }
    return true;
}

bool utilstr::GetNumLiteralValue(std::string src, Either& result)
{
    bool fractional = false;    // true if '.' was detected
    bool exponent = false;      // true if 'e/E' was detected

    int exponentSign = 1;       // E(+/-)123
    int sign = 1;               // Sign of the number
    int part = 0;               // Start with whole part (0), and go in order
    std::string number_str[3];  // 0 -> whole part, 1 -> fraction part, 2 -> exponent

    // Special state where we want to retrieve the (optional) exponent sign.
    bool waitForExponentSign = false;

    // Start iterating through the string
    for (size_t i = 0; i < src.size(); i++)
    {
        const char c = src.at(i);

        // For any digit that we discover
        if (isdigit(c))
        {
            // Add the numerical to corresponding string
            number_str[part] += c;
            // No exponent sign specified, using '+' as default.
            if (waitForExponentSign) waitForExponentSign = false;
        }

        // Unary minus
        else if (c == '-' && i == 0)
        {
            sign = -1;
        }

        // Unary plus
        else if (c == '+' && i == 0)
        {
            sign = 1;
        }

        // If previous element was 'e/E', we wait for the sign.
        // By previous conditions, this brach assumes that the character
        // is non-numeric, thus throwing an error if it is neither plus or minus.
        else if (waitForExponentSign)
        {
            if (c == '+')
            {
                exponentSign = 1;
            }
            else if (c == '-')
            {
                exponentSign = -1;
            }
            else
            {
                return false;
            }
            // Exit the state of waiting for the sign
            waitForExponentSign = false;
        }

        // Point discovered. Further action depends on the current state.
        else if (c == '.')
        {
            // To this moment, no other point or exponent were discovered.
            if (fractional || exponent)
            {
                return false;
            }
            // The state is changed to writing fraction value.
            fractional = true;
            part = 1;
        }

        // Exponent discovered. Further action depends on current state.
        else if (c == 'e' || c == 'E')
        {
            // To be valid, we must not have discovered another exponent.
            // It does not matter whether there is fractional part.
            if (exponent)
            {
                return false;
            }
            // The state is changed to waiting for exponent sign.
            // Afterwards, even if no sign provided, we read exponent value.
            exponent = true;
            waitForExponentSign = true;
            part = 2;
        }
        // Invalid symbol detected.
        else
        {
            return false;
        }
    }

    // Validation:
    if (number_str[0].empty()                       // Whole part cannot be empty;
        || (fractional && number_str[1].empty())    // If discovered '.', this part cannot be empty;
        || (exponent && number_str[2].empty()))     // If discovered 'e/E', this part cannot be empty;
    {
        return false;
    }

    // At this point, we have:
    //  1) A non-empty string with whole part, with sign stored in bool negative;
    //  2) If fractional == true, non-empty string with fractional part;
    //  3) If exponent == true, non-empty string with exponent, with sign stored in int exponentSign;

    // Now, we need to do type inference:
    //  Has negative exponent -> DOUBLE since we cannot guarantee that the result will be INT
    //  Has fractional part -> DOUBLE
    //  Otherwise -> INT

    // DOUBLE
    if (fractional || (exponent && exponentSign == -1))
    {
        // This is guaranteed to be non-empty
        double num = std::stoi(number_str[0]);

        // Add the fractional part if there is one.
        if (fractional)
        {
            double frac = std::stoi(number_str[1]);
            num += frac / (pow(10, (double)number_str[1].size()));
        }

        // Apply the sign
        num *= (float)sign;

        // If we use an exponent
        if (exponent)
        {
            double exp = std::stoi(number_str[2]);
            exp *= (float)exponentSign;
            num *= pow(10.0, exp);
        }

        result = Either(num);
        return true;
    }

    // INT
    else
    {
        // This is guaranteed to be non-empty.
        int num = std::stoi(number_str[0]);
        num *= sign;

        // If we use an exponent
        if (exponent)
        {
            // Here, exponent sign is guaranteed to be positive,
            // so the exponent is calculated this way, since we
            // want to avoid dealing with double in this case.
            int exp = std::stoi(number_str[2]);
            for (int i = 0; i < exp; i++)
                num *= 10;
        }
        result = Either(num);
        return true;
    }
}
