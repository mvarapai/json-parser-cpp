
#include <iostream>
#include "command.h"
#include "json_parser.h"
#include "utilstr.h"
#include "query.h"

std::string ProcessCommand(std::string command, 
	JSONInterface& jsonInterface, 
	CommandInterface& cmdInterface);

void ProcessInput(std::string input, JSONInterface& jsonInterface, CommandInterface& cmdInterface)
{
	if (input.empty())
	{
		std::cout << "Enter an expression." << std::endl;
		return;
	}

	if (input.front() == ':')
	{
		input.erase(0, 1);
		ProcessCommand(input, jsonInterface, cmdInterface);
		return;
	}

	// Else, we are dealing with queries

	utilstr::ReplaceAllChars(input, " \t\n", "");

	if (utilstr::Contains(input, '+')
		|| utilstr::Contains(input, '-')
		|| utilstr::Contains(input, '*')
		|| utilstr::Contains(input, '/')
		|| utilstr::Contains(input, '(')
		|| isdigit(input[0]))
	{
		Expr expr = Expr(input, jsonInterface);
		std::cout << expr.Eval().ToString() << std::endl;
	}
	// If arithmetic is not used, simple data query can be used.
	else
	{
		JSON::JSONNode* node = jsonInterface.tree_walk(input);
		if (!node) return;

		if (!JSON::isLiteral(node->GetType()))
		{
			std::cout << "To view an object or a list, use :current." << std::endl;
			return;
		}

		std::cout << getLiteralValue(node) << std::endl;
		return;
	}
}

std::string ProcessCommand(std::string input, JSONInterface& jsonInterface, CommandInterface& cmdInterface)
{
	std::vector<std::string> args;

	std::string substring;
	size_t pos = 0;
	while (utilstr::Split(input, ' ', substring, pos))
	{
		args.push_back(substring);
	}

	std::string command = args[0];

	if (command == "quit" || command == "q")
	{
		std::cout << "Exiting.." << std::endl;
		exit(0);
	}
	
	if (command == "help" || command == "h")
	{
		ConsoleTable<2> table({ 7, 9}, 0);

		table.PrintLine({ "List of commands:", "" });
		std::cout << std::endl;
		table.PrintLine({ ":help", "Display the list of commands." });
		table.PrintLine({ ":quit", "Exit the CLI." });
		std::cout << std::endl;

		table.PrintLine({ ":current (--recursive=MAX_DEPTH) (--show-values)", "Displays info about current object." });
		table.PrintLine({ ":select <EXPR>", "Select object member. Must also be an object." });
		table.PrintLine({ ":back (<NUM_STEPS>) (--root)", "Move up the hierarchy." });

		return "";
	}

	if (command == "current" || command == "c")
	{
		bool showValues = false;
		unsigned int maxDepth = 0;

		// Some additional arguments supplied
		if (args.size() > 1)
		{
			for (int i = 1; i < args.size(); i++)
			{

				// --recursive
				std::string arg = args[i];
				if (arg.substr(0, 11) == "--recursive" || arg.substr(0, 2) == "-r")
				{
					if (utilstr::Contains(arg, '='))
					{
						size_t readInt = 0;

						if (arg.substr(0, 12) == "--recursive=")
						{
							readInt = 12;
						}

						else if (arg.substr(0, 3) == "-r=")
						{
							readInt = 3;
						}

						std::string numStr = arg.substr(readInt);

						if (!utilstr::IsNumLiteral(numStr))
						{
							return "Enter valid MAX_DEPTH";
						}

						maxDepth = std::stoi(numStr);
					}			
					else
					{
						maxDepth = UINT32_MAX;
					}
				}

				// --show-values
				if (arg == "--show-values" || arg == "-s")
				{
					showValues = true;
				}

			}
		}

		return jsonInterface.ListMembers(showValues, maxDepth);
	}

	if (command == "select" || command == "s")
	{
		if (args.size() < 2)
		{
			return "Provide an expression.\n";
		}
		return jsonInterface.Select(args[1]);
	}

	if (command == "back" || command == "b")
	{
		unsigned int stepsBack = 1;

		if (args.size() > 1)
		{
			std::string arg = args[1];

			// If entered a number of steps
			if (utilstr::IsNumLiteral(arg))
			{
				stepsBack = std::stoi(arg);
			}

			// If want to return to root
			if (arg == "--root" || arg == "-r")
			{
				stepsBack = UINT32_MAX;
			}
		}

		jsonInterface.Back(stepsBack);
		return "";
	}

	return "Unknown command. :help (:h) for help.";
}
