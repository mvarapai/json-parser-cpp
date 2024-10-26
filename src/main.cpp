#include <iostream>

#include "utilstr.h"

// Entry point to the program
int main()
{
    std::string jsonPath = "json\\";
    std::string filename = "test1.json";
    std::cout << utilstr::ReadFromFile(jsonPath + filename) << std::endl;

    return 0;
}