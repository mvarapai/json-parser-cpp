#include <iostream>

#include "utilstr.h"
#include "json_parser.h"

// Entry point to the program
int main()
{
    std::string jsonPath = "json\\";
    std::string filename = "test1.json";
    
    JSON json = JSON::ReadFromFile(jsonPath + filename);

    

    return 0;
}