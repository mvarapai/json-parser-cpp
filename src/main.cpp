#include <iostream>

#include "utilstr.h"
#include "json_parser.h"

// Entry point to the program
int main()
{
    std::string jsonPath = "json\\";
    std::string filename = "test2.json";
    
    //JSON json = JSON::ReadFromFile(jsonPath + filename);

    std::cout << utilstr::TrimOneChar("alloha") << std::endl;

    return 0;
}