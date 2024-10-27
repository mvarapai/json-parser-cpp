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
    std::cout << "Pos: " << src.GetSymbolSourcePosition(12).ToString() << std::endl;

    return 0;
}