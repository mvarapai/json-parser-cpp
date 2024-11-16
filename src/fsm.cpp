#include "fsm.h"
#include "utilstr.h"

// Main function for interpreting the command.
// Starts by identifying the command name, and proceeds
// with scanning all command arguments with the following rules:
// 
//	1. --arg(=..) is a full (non-aliased) argument with potential value;
//	2. -xy(=..) is an aliased list of arguments. The value is assigned to the last one.
//	3. General arguments without -/-- are read as tokens and stored in order.
//	   It is up to the caller on how to interpret these tokens.
// 
// At the end, two lists are formed: the list of arguments, containing
// the argument name (aliased or not) and its possible value, and the list
// of tokens, stored in order.
// 
// Designed to be as tolerant and robust as possible, with ignoring invalid input.
// The rest of robustness is provided by the caller, which can assess the validity of values.
bool CommandLineInterpreter::Interpret()
{
	// Some checks
	if (contents.empty())
	{
		std::cout << "Command cannot be empty." << std::endl;
		return false;
	}

	size_t pos = 0;

	// Interpret command name
	std::string commandName;
	while (pos < contents.size())
	{
		// Read characters and break if whitespace found.
		char c = contents.at(pos);
		if (c == ' ') break;

		// If illegal character, return.
		if (!std::isalnum(c) && c != '_' && c != '-')
		{
			std::cout << "Invalid command name." << std::endl;
			return false;
		}

		// If all checks passed, increment the position.
		pos++;
	}

	// Two outcomes:
	// 1) Whitespace found -> pos of whitespace is the length.
	// 2) pos == contents.size() -> pos is the length.
	this->commandName = contents.substr(0, pos);
	pos = contents.find_first_not_of(' ', pos);

	// Interpret arguments and tokens
	// Loop invariant: no whitespaces in front.
	unsigned int tokenIndex = 0;
	while (pos < contents.size())
	{
		// Handle the full argument notation.
		if (utilstr::BeginsWith(contents, "--", pos))
		{
			// Offset the position by "--"
			pos += 2;

			// Read argument name

			std::string argBody;
			size_t prevPos = pos;
			pos = contents.find_first_of(" =", pos);

			// Clamp the position to string boundaries.
			if (pos == std::string::npos) pos = contents.size();

			// For all conditions, the argument body is bound by pos and prevPos.
			argBody = contents.substr(prevPos, pos - prevPos);

			// Make sure that argument is not empty (e.g. no "--=.." entered).
			// Ignore it and move to the next argument.
			if (argBody.empty());

			// For all arguments without a value, we can push them right away.
			else if (pos == contents.size() || contents.at(pos) == ' ')
			{
				arguments.push_back(Argument(argBody, false));
			}

			// Process the case in which there is a value.
			else
			{
				// Read argument value
				prevPos = pos + 1;

				// Handle the cases where the user has not entered the value.
				// Makes use of C++ short-circuit evaluation: if size condition is true,
				// the character at this position is checked, preventing the error.
				if (prevPos == contents.size() || contents.at(prevPos) == ' ')
				{
					std::cout << "Enter the value." << std::endl;
					return false;
				}

				// Otherwise, we have a value. The utility function will return
				// string size if no character found, thus giving us the string
				// from '=' up to the next whitespace/string end.
				pos = utilstr::FindFirstOfOutsideString(contents, " ", pos);
				std::string val = contents.substr(prevPos, pos - prevPos);

				// Finally, add the argument to the list.
				// Here, we must specify that the argument is not aliased
				// for better search in the command processing code.
				arguments.push_back(Argument(argBody, false, true, val));
			}
		}

		// Consider the case of aliased arguments
		else if (utilstr::BeginsWith(contents, "-", pos))
		{
			// Increment position to account for "-".
			pos += 1;

			// Try to find '=' or ' '.
			size_t prevPos = pos;
			pos = contents.find_first_of(" =", pos);
			std::string argBody;

			// Clamp pos to the string size.
			if (pos == std::string::npos) pos = contents.size();

			// For any of the cases, we retrieve the list of arguments.
			argBody = contents.substr(prevPos, pos - prevPos);

			// Guarantee that there is no input like "-=..".
			// Ignore it and move on to the next argument.
			if (argBody.empty());

			// If there is no value, push arguments right away.
			else if (pos == contents.size() || contents.at(pos) == ' ')
			{
				for (const char& c : argBody)
					arguments.push_back(Argument(std::string(1, c), true));
			}

			// Handle the case where value for the last argument was provided.
			else
			{
				// Last argument has a value
				prevPos = pos + 1;

				// Handle the situation where there is no value entered
				if (prevPos == contents.size() || contents.at(prevPos) == ' ')
				{
					std::cout << "Enter the value." << std::endl;
					return false;
				}

				// Find the next whitespace. If end of the string, returns the size.
				pos = utilstr::FindFirstOfOutsideString(contents, " ", pos);
				std::string val = contents.substr(prevPos, pos - prevPos);

				// All other arguments are without values
				for (size_t i = 0; i < argBody.size() - 1; i++)
				{
					char c = argBody.at(i);
					arguments.push_back(Argument(std::string(1, c), true));
				}

				// Except for the last
				arguments.push_back(Argument(std::string(1, argBody.back()), true, true, val));
			}
		}

		// Otherwise, input is a token.
		else
		{
			// Read a token.
			size_t prevPos = pos;
			pos = utilstr::FindFirstOfOutsideString(contents, " ", pos);

			tokens.push_back(Token(contents.substr(prevPos, pos - prevPos), tokenIndex));
			tokenIndex++;
		}

		// Positioned at the end so that the loop condition will be checked at
		// next interation, and if we reached the string end, will exit it.
		pos = contents.find_first_not_of(' ', pos);
	}

	// If no error message shown, return true.
	return true;
}
