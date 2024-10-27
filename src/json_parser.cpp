#include "json_parser.h"
#include "utilstr.h"

#include <iostream>

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(std::string body);

inline bool isStringLiteral(const std::string& str)
{
	if (!utilstr::BeginsAndEndsWith(str, '"'))
	{
		std::string errorMsg =	"[ERROR] Object member identifier/string literal ";
		errorMsg +=				"is not of string format. Member ignored.";
		std::cerr << errorMsg << std::endl;
		return false;
	}
	return true;
}

// Assumes string input.
inline std::string getStringContents(std::string str)
{
	// Output error message if input is not string
	isStringLiteral(str);
	return utilstr::TrimOneChar(str);
}

inline std::string jsonTrim(std::string str)
{
	return utilstr::Trim(str, " \n\t");
}

JSON::JSON()
{
	std::cout << "Created JSON!" << std::endl;
}

JSON::JSON(const JSON& other)
{
	std::cout << "Copied JSON!" << std::endl;
}

JSON JSON::ReadFromFile(const std::string& filename)
{
	return JSON::ReadFromString(utilstr::ReadFromFile(filename));
}

JSON JSON::ReadFromString(std::string source)
{
	source = jsonTrim(source);

	// Check for empty input
	if (source.size() == 0)
	{
		std::string errorMsg = "[WARNING] JSON file/string does not exist or is empty. ";
		errorMsg += "An empty JSON instance is created.";
		std::cerr << errorMsg << std::endl;

		return JSON();
	}
	std::cout << source << std::endl;
	// Check first important condition - global space must be an object.
	if (!utilstr::BeginsAndEndsWith(source, '{', '}'))
	{
		std::string errorMsg = "[ERROR] JSON file/string does not contain an object. ";
		errorMsg += "Correct format of the file/string would be: \"{..}\". ";
		errorMsg += "Empty JSON object returned.";
		std::cerr << errorMsg << std::endl;

		return JSON();
	}

	// Here, by the condition above, we are certain that the global space
	// is in fact a JSON object.
	JSON json;
	json.globalSpace = static_cast<JSONObject*>(resolve_json(source));
	
	return json;
}

JSON::JSONNode* resolve_json(std::string body)
{	
	body = jsonTrim(body);

	// A JSON object
	if (utilstr::BeginsAndEndsWith(body, '{', '}'))
	{
		JSON::JSONObject* object = new JSON::JSONObject;
		body = utilstr::TrimOneChar(body);
		std::cout << "Trimmed body:\n" << body << std::endl;

		// Split body with commas to find individual class members
		size_t prevPosComma = 0;
		std::string member;
		while (utilstr::Split(body, ',', member, prevPosComma))
		{
			// PROBLEM: commas are also inside object body
			std::cout << "Comma-separated value:\n" << member << std::endl;

			// Substring format:
			// "..": (..) - (..) can be any JSON node.
			std::string memberIdentifier;
			std::string memberBody;

			// Split member source string with colon
			size_t prevPosSemicolon = 0;
			std::string substring;
			int counter = 0;
			while (utilstr::Split(member, ':', substring, prevPosSemicolon))
			{
				substring = jsonTrim(substring);

				// Member identifier
				if (counter == 0)
				{
					// If string is invalid, ignore this member
					if (!isStringLiteral(substring)) break;

					memberIdentifier = getStringContents(substring);
				}

				// JSON node source
				else if (counter == 1)
				{
					// No error checking, this is responsibility of subsequent resolve_json calls.

					memberBody = substring;
				}

				// JSON Error - called if several colon characters were found
				else break;

				counter++;
			}

			// If member was dropped, move to next member
			if (memberIdentifier.empty() || memberBody.empty()) continue;

			std::cout << "Entry: " << memberIdentifier << ": " << memberBody << std::endl;

			// Otherwise, add member to a tree
			JSON::JSONNode* childNodePtr = resolve_json(memberBody);	// Recursive call
			std::pair<std::string, JSON::JSONNode*> memberEntry(memberIdentifier, childNodePtr);
			object->members.insert(memberEntry);
		}

		return object;
	}

	// A JSON list
	else if (utilstr::BeginsAndEndsWith(body, '[', ']'))
	{
		std::cout << "List not implemented yet." << std::endl;
	}

	// A string literal
	else if (utilstr::BeginsAndEndsWith(body, '"'))
	{
		return new JSON::JSONLiteral<std::string>(utilstr::TrimOneChar(body),
			JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING);
	}

	// Numeric literal or invalid
	else
	{

	}

	return new JSON::JSONObject();
}
