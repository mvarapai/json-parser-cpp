#include "fsm.h"
#include "utilstr.h"

bool CommandLineInterpreter::Interpret()
{
	// Some checks
	if (contents.empty())
	{
		std::cout << "Command cannot be empty." << std::endl;
		return false;
	}

	size_t pos = 0;


	// 1. Interpret command name

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

	// 2. Interpret arguments and tokens
	// Loop invariant: no whitespaces in front.
	unsigned int tokenIndex = 0;
	while (pos < contents.size())
	{
		if (utilstr::BeginsWith(contents, "--", pos))
		{
			pos += 2;

			// Read full argument

			// 1. Read main part

			std::string argBody;
			size_t prevPos = pos;
			pos = contents.find_first_of(" =", pos);

			// Argument is at the line end
			if (pos == std::string::npos) 
			{
				pos = contents.size();
				argBody = contents.substr(prevPos, pos - prevPos);

				// Push new argument
				arguments.push_back(Argument(argBody));
				break;
			}

			argBody = contents.substr(prevPos, pos - prevPos);

			if (contents.at(pos) == '=')
			{
				// Read argument value
				prevPos = pos + 1;
				pos = utilstr::FindFirstOfOutsideString(contents, " ", pos);
				std::string val = contents.substr(prevPos, pos - prevPos);

				arguments.push_back(Argument(argBody, true, val));
			}

			else
			{
				arguments.push_back(Argument(argBody));
			}
		}
		else if (utilstr::BeginsWith(contents, "-", pos))
		{
			pos += 1;

			// Read aliased argument

			size_t prevPos = pos;
			pos = contents.find_first_of(" =", pos);

			std::string argBody;

			// Aliased arguments at the end of the string
			if (pos == std::string::npos)
			{
				pos = contents.size();
				argBody = contents.substr(prevPos, pos - prevPos);

				for (const char& c : argBody)
				{
					arguments.push_back(Argument(std::string(1, c)));
				}

				break;
			}

			argBody = contents.substr(prevPos, pos - prevPos);

			if (contents.at(pos) == '=')
			{
				// Last argument has a value
				prevPos = pos + 1;
				pos = utilstr::FindFirstOfOutsideString(contents, " ", pos);
				std::string val = contents.substr(prevPos, pos - prevPos);

				// All other arguments are without values
				for (size_t i = 0; i < argBody.size() - 1; i++)
				{
					char c = argBody.at(i);
					arguments.push_back(Argument(std::string(1, c)));
				}

				// Except for the last
				arguments.push_back(Argument(std::string(1, argBody.back()), true, val));
			}

			else
			{
				for (const char& c : argBody)
					arguments.push_back(Argument(std::string(1, c)));
			}
		}
		else
		{
			// Read a token
			size_t prevPos = pos;
			pos = utilstr::FindFirstOfOutsideString(contents, " ", pos);

			tokens.push_back(Token(contents.substr(prevPos, pos - prevPos), tokenIndex));

			tokenIndex++;
		}

		pos = contents.find_first_not_of(' ', pos);
	}
	return true;
}
