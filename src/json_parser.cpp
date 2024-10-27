#include "json_parser.h"
#include "utilstr.h"

#include <iostream>

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(std::string body);

JSONSource::Pos JSONSource::GetSymbolSourcePosition(size_t trimmedPos)
{
	std::cout << "Symbol: " << trimmedStr.at(trimmedPos) << std::endl;

	size_t sourceOffset = 0;	// Counts all symbols except newlines
	size_t trimmedOffset = 0;	// Counts all symbols except whitespaces, tabs and newlines
	size_t lineOffset = 0;		// Counts all symbols up to last newline, according to source
	size_t line = 1;			// Number of whole lines

	for (char& c : sourceStr)
	{
		if (trimmedOffset == trimmedPos + 1) break;

		sourceOffset++;

		switch (c)
		{
		case ' ':
		case '\t':
			break;
		case '\n':
			line++;
			lineOffset = sourceOffset;
			break;
		default:
			trimmedOffset++;
		}

	}

	Pos query;
	query.line = line;
	query.col = sourceOffset - lineOffset;

	return query;
}

std::string JSONSource::Pos::ToString()
{
	std::string str = "(";
	str += std::to_string(line);
	str += ";";
	str += std::to_string(col);
	str += ")";
	return str;
}

JSONSource::JSONSource(std::string filename)
{
	sourceStr = utilstr::ReadFromFile(filename);

	// Remove all spaces, tabs and newlines from the string
	trimmedStr = sourceStr;
	utilstr::ReplaceAllChars(trimmedStr, " \n\t", "");
}

JSON JSON::ReadFromFile(const std::string& filename)
{
	return JSON::ReadFromString(utilstr::ReadFromFile(filename));
}

JSON JSON::ReadFromString(std::string source)
{
	

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
	// Example JSON:
	// {"menu":{"id":"file","value":"File"}}

	// A JSON object
	if (utilstr::BeginsAndEndsWith(body, '{', '}'))
	{
		JSON::JSONObject* object = new JSON::JSONObject;
		body = utilstr::TrimOneChar(body);
		// "menu":{"id":"file","value":"File"}
		
		// Iterate through "id": value pairs
		while (true)
		{
			size_t beginIdentifier, endIdentifier;
			size_t beginBody, endBody;

			if (body.at(0) != '"')
			{
				std::string errorMsg = "[ERROR] '\"' expected at";
			}
			
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
