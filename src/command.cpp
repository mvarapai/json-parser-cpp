
#include <iostream>
#include "command.h"
#include "json_parser.h"
#include "utilstr.h"

std::string ProcessCommand(std::string command, JSONInterface& interface);

std::string ProcessInput(std::string input, JSONInterface& interface)
{
	if (input.empty())
	{
		return "Enter an expression.";
	}

	if (input.front() == ':')
	{
		input.erase(0, 1);
		return ProcessCommand(input, interface);
	}

	return "Invalid expression.";
}

std::string ProcessCommand(std::string input, JSONInterface& interface)
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
		table.PrintLine({ ":help (:h)", "Display the list of commands." });
		table.PrintLine({ ":quit (:q)", "Exit the CLI." });
		std::cout << std::endl;

		table.PrintLine({ ":current (--recursive=MAX_DEPTH) (--show-values)", "Displays info about current object." });
		table.PrintLine({ ":select (:s) <expr>", "Select object member. Must also be an object." });
		table.PrintLine({ ":back (--root)", "Move one object up the hierarchy." });

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

						if (!utilstr::IsIntLiteral(numStr))
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

		return interface.ListMembers(showValues, maxDepth);
	}

	if (command == "select" || command == "s")
	{
		if (args.size() < 2)
		{
			return "Provide an expression.\n";
		}
		return interface.Select(args[1]);
	}

	if (command == "back" || command == "b")
	{
		unsigned int stepsBack = 1;

		if (args.size() > 1)
		{
			std::string arg = args[1];

			// If entered a number of steps
			if (utilstr::IsIntLiteral(arg))
			{
				stepsBack = std::stoi(arg);
			}

			// If want to return to root
			if (arg == "--root" || arg == "-r")
			{
				stepsBack = UINT32_MAX;
			}
		}

		interface.Back(stepsBack);
		return "";
	}

	return "Unknown command. :help (:h) for help.";
}
