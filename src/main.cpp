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

    str = str.substr(96, 1000);

    size_t pos = 0;

    std::cout << "Str: " << str.ToString() << std::endl;
    std::cout << str.ScanListObjectBody(pos).ToString() << std::endl;
    std::cout << "Pos: " << pos << std::endl;

    return 0;
}