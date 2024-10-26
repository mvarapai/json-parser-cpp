#include "json_parser.h"
#include "utilstr.h"

#include <iostream>

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(std::string body);

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

JSON JSON::ReadFromString(const std::string& source)
{
	// Check for empty input
	if (source.size() == 0)
	{
		std::string errorMsg = "[WARNING] JSON file/string does not exist or is empty. ";
		errorMsg += "An empty JSON instance is created.";
		std::cerr << errorMsg << std::endl;

		return JSON();
	}

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
	// Trim the string: whitespace, newline and tab
	body = utilstr::Trim(body, " \n\t");

	// Now, we do a case distinction

	// A JSON object
	if (utilstr::BeginsAndEndsWith(body, '{', '}'))
	{
		// Trim the brackets
		body = utilstr::TrimOneChar(body);

		JSON::JSONObject* object = new JSON::JSONObject;

		std::unordered_map<std::string, std::string> membersSrc;

		// TODO: split by ","

		for (std::pair<std::string, std::string> member : membersSrc)
		{
			std::pair<std::string, JSON::JSONNode*> memberEntry(member.first, resolve_json(member.second));
			object->members.insert(memberEntry);
		}

		return object;
	}
	// A JSON list
	else if (utilstr::BeginsAndEndsWith(body, '[', ']'))
	{

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
