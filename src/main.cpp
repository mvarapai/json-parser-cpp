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
    cmdInterface.RegisterCommand(CommandHelp(cmdInterface));
    cmdInterface.RegisterCommand(CommandQuit());
    cmdInterface.RegisterCommand(CommandCurrent());

    std::string command;

    while (true)
    {
        std::cout << "json_eval>";
        std::getline(std::cin, command);

        CommandLineInterpreter cli(command.substr(1));
        cli.Interpret();

        for (const Argument& a : cli.GetArgs())
        {
            std::cout << "Name: " << a.ToString() << ", Value: " << a.GetValue() << std::endl;
        }

        for (const Token& t : cli.GetTokens())
        {
            std::cout << "Token value: " << t.GetValue() << ", Index: " << t.GetIndex() << std::endl;
        }
        //ProcessInput(command, interface, cmdInterface);
    }

    return 0;
}