#include <iostream>

#include "utilstr.h"
#include "json_parser.h"

// Entry point to the program
int main()
{
    std::string jsonPath = "json\\";
    std::string filename = "test1.json";
    
    std::string path = jsonPath + filename;

    JSONSource src(path);
    JSONString str = src.GetString();
    size_t trimmedPos = 300;
    JSONSource::Pos pos = str.GetSourcePos(trimmedPos);
    //std::string string = str.ScanString(pos);
    std::cout << "Str: " << str << std::endl;
    std::cout << "TrimmedPos: " << trimmedPos << "\nPos: " << pos.ToString() << std::endl;

    return 0;
}