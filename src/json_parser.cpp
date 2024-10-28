#include "json_parser.h"
#include "utilstr.h"

#include <iostream>

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(JSONString body);

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

JSONSource::JSONSource(std::string filename) : filename(filename)
{
    sourceStr = utilstr::ReadFromFile(filename);

    // Remove all spaces, tabs and newlines from the string
    trimmedStr = sourceStr;
    utilstr::ReplaceAllChars(trimmedStr, " \n\t", "");
}

JSON::JSON(const std::string& filename)
{
    jsonSource = new JSONSource(filename);
    JSONString source = jsonSource->GetString();

    // Check for empty input
    if (source.size() == 0)
    {
        std::string errorMsg =  "JSON file does not exist or is empty. ";
        errorMsg +=             "An empty JSON instance is created.";
        source.PrintSyntaxMsg(errorMsg, SYNTAX_MSG_TYPE_WARNING);

        return;
    }
    
    // Check first important condition - global space must be an object.
    if (!utilstr::BeginsAndEndsWith(source, '{', '}'))
    {
        std::string errorMsg =  "JSON file does not contain an object. ";
        errorMsg +=             "Correct format of the file would be: \"{..}\". ";
        errorMsg +=             "Empty JSON object returned.";
        source.PrintSyntaxMsg(errorMsg);

        return;
    }

    // Here, by the condition above, we are certain that the global space
    // is in fact a JSON object.
    globalSpace = static_cast<JSONObject*>(resolve_json(source));
}

void JSONString::PrintSyntaxMsg(std::string errorText, int msgType = 0, size_t _Off = 0)
{
    // [ERROR] test1.json:2 - "X expected."
    std::string msg;

    switch (msgType)
    {
    case 0:
        msg = "[ERROR]";
        break;
    case 1:
        msg = "[WARNING]";
        break;
    default:
        msg = "[MESSAGE]";
    }

    msg += " ";
    msg += source->GetFilename();
    msg += ":";
    msg += std::to_string(GetSourcePos(_Off).line);
    msg += " - ";
    msg += errorText;

    std::cerr << msg << std::endl;
}

// Input: " \"TEXT\".. "
// Output: TEXT, with
//  \\ -> backslash
//  \n -> newline
//  \t -> tab
//  \" -> "
// Ignores anything after closing '"', returns position of
// first character after it.
// Invalid string: return "".
std::string JSONString::ScanString(size_t& _Pos)
{
    if (at(0) != '"')
    {
        PrintSyntaxMsg("'\"' expected.");
        return "";
    }

    bool escape = false;
    bool closingFound = false;
    std::string str;

    size_t counter = 0;
    for (char& c : *this)
    {
        counter++;

        if (counter == 1) continue;

        // If this character follows \, it is an escape sequence
        if (escape)
        {
            escape = false;
            switch (c)
            {
            case '\\':
                str += '\\';
                break;
            case 'n':
                str += '\n';
                break;
            case 't':
                str += '\t';
                break;
            default:
                PrintSyntaxMsg("Valid escape sequence expected.", SYNTAX_MSG_TYPE_WARNING, counter);
            }
            continue;
        }
        
        if (c == '\\')
        {
            escape = true;
            continue;
        }

        // Closing '"' found!
        if (c == '"')
        {
            closingFound = true;
            break;
        }

        // Any other character
        str += c;
    }

    // Analyze why loop was teminated
    if (!closingFound)
    {
        PrintSyntaxMsg("'\"' expected.", SYNTAX_MSG_TYPE_ERROR, counter - 1);
        return "";
    }
    _Pos = counter;
    return str;
}

JSON::JSONNode* resolve_json(JSONString body)
{	
    // Example JSON:
    // {"menu":{"id":"file","value":"File"}}

    // A JSON object
    if (utilstr::BeginsAndEndsWith(body, '{', '}'))
    {
        // Create new JSON object and expose its members
        JSON::JSONObject* object = new JSON::JSONObject;
        body = body.substr(1, body.size() - 2);

        // "menu":{"id":"file","value":"File"}
        
        // Iterate through "id": value pairs
        while (true)
        {
            // Expect '"'
            if (body.at(0) != '"')
            {
                body.PrintSyntaxMsg("'\"' expected.");
                break;
            }

            // Find non-escaped '"'
            size_t counter = 0;
            bool escape = false;
            std::string id;
            for (char& c : body)
            {
                if (counter == 0)
                {
                    counter++;
                    continue;       // We are looking for closing '"'
                }

                if (escape)
                {
                    switch (c)
                    {
                    
                                                
                    }
                }
                if (c == '\\') escape = true;
                
                if (c == '"') break;
                counter++;
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
