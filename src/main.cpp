#include <iostream>

#include "utilstr.h"
#include "json_parser.h"

// Entry point to the program
int main()
{
    std::string jsonPath = "json\\";
    std::string filename = "test2.json";
    
    std::string path = jsonPath + filename;

    JSON json(path);
    
    // Do stuff with JSON..

    return 0;
}