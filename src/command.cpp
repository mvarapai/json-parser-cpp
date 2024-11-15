
#include <iostream>
#include "command.h"
#include "json_parser.h"
#include "utilstr.h"
#include "query.h"
#include "fsm.h"

void ProcessCommand(std::string command, 
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

void ProcessCommand(std::string input, JSONInterface& jsonInterface, CommandInterface& cmdInterface)
{
	if (input.empty())
	{
		std::cout << "Expected a command." << std::endl;
		return;
	}

	CommandLineInterpreter interpreter(input);
	interpreter.Interpret();

	Command const* cmd = cmdInterface.FindCommand(interpreter.GetCommandName());

	if (!cmd)
	{
		std::cout << "Unknown command. :help for list of available commands." << std::endl;
		return;
	}

	cmd->Execute(interpreter);
}

void CommandCurrent::Execute(const CommandLineInterpreter& interpreter) const
{
	bool showValues = false;
	unsigned int maxDepth = 0;

	for (const Argument& arg : interpreter.GetArgs())
	{
		std::string argName = arg.ToString();

		if (argName == "show-values" || argName == "s") showValues = true;

		if (argName == "recursive" || argName == "r")
		{
			if (!arg.HasValue())
			{
				maxDepth = UINT32_MAX;
				continue;
			}

			if (!utilstr::IsNumLiteral(arg.GetValue()))
			{
				std::cout << "MAX_DEPTH must be a number." << std::endl;
				return;
			}

			maxDepth = std::stoi(arg.GetValue());
		}
	}

	json.ListMembers(showValues, maxDepth);
}

void CommandSelect::Execute(const CommandLineInterpreter& interpreter) const
{
	if (interpreter.GetTokens().size() < 1)
	{
		std::cout << "Enter an object to select." << std::endl;
		return;
	}

	Token token = interpreter.GetTokens().at(0);

	json.Select(token.GetValue());
}

void CommandBack::Execute(const CommandLineInterpreter& interpreter) const
{
	unsigned int stepsBack = 1;

	for (const Token& t : interpreter.GetTokens())
	{
		if (utilstr::IsNumLiteral(t.GetValue()))
		{
			stepsBack = std::stoi(t.GetValue());
			break;
		}
	}

	for (const Argument& arg : interpreter.GetArgs())
	{
		if (arg == ArgumentAlias("root", "r"))
		{
			stepsBack = UINT32_MAX;
			break;
		}
	}

	json.Back(stepsBack);
}
