/*****************************************************************//**
 * \file   fsm.h
 * \brief  Defines objects and functions to deal with tokenizing
 *		   strings and reading literals.
 * 
 * \author Mikalai Varapai
 * \date   November 2024
 *********************************************************************/

#include <string>
#include <vector>
#include <iostream>

// General object for command line arguments.
struct Token
{
private:
	const std::string contents;
	const unsigned int index;

public:
	Token(std::string contents, unsigned int index) : contents(contents), index(index) { }

	std::string GetValue() const { return contents; }
	unsigned int GetIndex() const { return index; }
};

struct Argument
{
private:
	// Interpret -rs as --recursive --show-values.
	// Must be two different arguments.

	std::string argument;

	bool hasValue = false;
	std::string value;

public:
	// Leave interpretation of the string to the caller.
	bool HasValue() const { return hasValue; }
	std::string GetValue() const { return value; }
	std::string ToString() const { return argument; }

	// Operator to compare argument names.
	bool operator==(const std::string& rhs) const { return argument == rhs; }

	Argument(std::string argument, bool hasValue = false, std::string value = std::string())
		: argument(argument), hasValue(hasValue), value(value) { }
};

// Stores current state of reading the string.
class CommandLineInterpreter
{
	std::string contents;

	std::string commandName;
	std::vector<Token> tokens;
	std::vector<Argument> arguments;

public:
	CommandLineInterpreter(std::string contents) : contents(contents) { }

	// Interpret the command line arguments.
	// If false is returned, error message is already written.
	bool Interpret();

	const std::string GetCommandName() const { return commandName; }
	const std::vector<Token>& GetTokens() const { return tokens; }
	const std::vector<Argument>& GetArgs() const { return arguments; }
};

