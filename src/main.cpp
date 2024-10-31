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

    str = str.substr(5, 10);
    str.PrintSyntaxMsg("Some error.", 0, 5);

    std::cout << "Str: " << str.ToString() << std::endl;

    return 0;
}