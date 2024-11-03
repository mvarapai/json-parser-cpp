/*****************************************************************//**
 * \file   command.h
 * \brief  Defines commands available in the CLI.
 * 
 * \author Mikalai Varapai
 * \date   November 2024
 *********************************************************************/

#include <string>
#include <iostream>
#include <array>

class JSONInterface;

std::string ProcessInput(std::string input, JSONInterface& interface);

template <int N>
class ConsoleTable
{
    const int TabSize = 8;

    const std::array<int, N> columnWidths;
    const int tabOffset;

public:
    ConsoleTable(std::array<int, N> columnWidths, int tabOffset) 
        : columnWidths(columnWidths), tabOffset(tabOffset) { }

    void PrintLine(std::array<std::string, N> elements)
    {

        size_t maxNumLines = 0;
        int i = 0;
        for (std::string s : elements)
        {
            int columnSize = columnWidths[i] * TabSize - 1;
            int numLines = s.size() / columnSize;
            if (s.size() % columnSize > 0) numLines++;

            if (numLines > maxNumLines) maxNumLines = numLines;
            i++;
        }

        std::vector<std::string> lines(maxNumLines);

        // Iterate through columns
        for (int i = 0; i < N; i++)
        {
            std::string currentString = elements[i];
            int columnSize = columnWidths[i] * TabSize - 1;

            // Get the amout of lines for the string
            int numLines = currentString.size() / columnSize;
            if (currentString.size() % columnSize > 0) numLines++;

            size_t pos = 0;

            // Loop with whole lines
            int j;
            for (j = 0; j < numLines - 1; j++)
            {
                lines[j] += currentString.substr(pos, columnSize);
                lines[j] += '\t';
                pos += columnSize;
            }

            // Process the last line
            std::string lastElement = currentString.substr(pos, columnSize);
            lines[j] += lastElement;

            // Get and append the amount of tabs
            size_t tabAmount = lastElement.size() / TabSize;
            tabAmount = columnWidths[i] - tabAmount;
            lines[j] += std::string(tabAmount, '\t');

            // If there are other lines, fill them with tabs
            j++;
            for (j; j < maxNumLines; j++)
            {
                lines[j] += std::string(columnWidths[i], '\t');
            }
        }

        // Add offset and print the lines

        for (std::string line : lines)
        {
            std::cout << std::string(tabOffset, '\t') << line << std::endl;
        }
    }
};
