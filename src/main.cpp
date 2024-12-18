/*****************************************************************//**
 * \file   main.cpp
 * \brief  Entry point to JSON parser command line interface.
 * 
 * \author Mikalai Varapai
 * \date   November 2024
 *********************************************************************/

#include <iostream>

// JSON parser library
#include <json_parser.h>

#include "command.h"
#include "fsm.h"

// Entry point to the program
int main(int argc, char* argv[])
{
    // Validate the arguments
    if (argc < 2)
    {
        std::cout << "Enter the file name. Correct syntax:\n./json_eval <filename>\n";
        return 0;
    }

    // argv[1] is guaranteed to be a valid pointer
    std::string path = argv[1];
    JSON json(path);
    
    // Here, JSON file is guaranteed to be valid, otherwise it would have exited.

    JSONInterface interface = json.CreateInterface();

    std::string welcome_msg = "Welcome to JSON Parser v1.0 by Mikalai Varapai!\n";
    welcome_msg += "The list of available commands can be accessed with \":h\" or \":help\".\n";
    welcome_msg += "Current file: " + path;
    std::cout << welcome_msg << std::endl;

    CommandInterface cmdInterface;
    cmdInterface.RegisterCommand(new CommandHelp(cmdInterface));
    cmdInterface.RegisterCommand(new CommandQuit());
    cmdInterface.RegisterCommand(new CommandCurrent(interface));
    cmdInterface.RegisterCommand(new CommandSelect(interface));
    cmdInterface.RegisterCommand(new CommandBack(interface));

    std::string command;

    while (true)
    {
        std::cout << "json_eval>";
        std::getline(std::cin, command);
        ProcessInput(command, interface, cmdInterface);
    }

    return 0;
}