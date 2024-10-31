#include "json_parser.h"
#include "utilstr.h"

#include <iostream>

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(JSONString body);

JSONString JSONSource::GetString() { return JSONString(this, trimmedStr.data(), trimmedStr.size()); }

// Loop invariant: check whether the symbol should be retained.
// Arguments:
//  c : current character
//  escape : true if previous character was '\'
//  inString : true if character c is contained in string literal
// Supposed to be invoked from a while loop, with initial bool values set to false.
bool CleanTest(const char c, bool& escape, bool& inString)
{
    // If '"' is in no escape sequence, it is either opening or closing quote.
    if (c == '"' && !escape) inString = !inString;

    // If escape is false, backslash will expect some other character.
    // If true, the escape character is simply written.
    if (c == '\\') escape = !escape;
    else if (escape) escape = false;

    // Choose whether to drop the character

    // Tabs and newlines are removed either way
    if (c == '\t' || c == '\n') return false;

    // Remove whitespaces only outside strings
    if (c == ' ' && !inString) return false;

    return true;
}

// Function to transform initial raw character position from trimmed string,
// to the line and col position in original string, understandable to the developer.
// If trimmedOffset is larger than the length of the string, position of last character
// in trimmed string is returned.
JSONSource::Pos JSONSource::GetSymbolSourcePosition(size_t trimmedOffset)
{
    size_t sourcePos = 0;
    size_t trimmedPos = 0;	// Counts all symbols except whitespaces, tabs and newlines
    size_t lineOffset = 0;		// Counts all symbols up to last newline, according to source
    size_t line = 1;			// Number of whole lines

    bool escape = false;
    bool inString = false;

    for (sourcePos = 0; sourcePos < sourceStr.size(); sourcePos++)
    {
        char c = sourceStr.at(sourcePos);

        if (c == '\n')
        {
            line++;
            lineOffset = sourcePos + 1;
        }

        if (CleanTest(c, escape, inString)) trimmedPos++;
        if (trimmedPos == trimmedOffset + 1) break;

        // If the end of trimmed string reached
        if (trimmedPos == trimmedStr.size()) break;
    }

    Pos query(line, sourcePos - lineOffset + 1);
    return query;
}

// Translate Pos object to string
// with format (line:col)
std::string JSONSource::Pos::ToString()
{
    std::string str = "(";
    str += std::to_string(line);
    str += ";";
    str += std::to_string(col);
    str += ")";
    return str;
}

//  Helper function to remove all characters that do not
//  contibute to JSON syntax, namely:
//      1. Tabs
//      2. Newlines
//      3. Whitespaces (except for string literals)
std::string CleanJSON(std::string source)
{
    std::string result;

    bool escape = false;    // Check whether previous symbol was '\'.
    bool inString = false;  // Check if currently processed symbol is part of a string literal.

    for (char& c : source)
    {
        if (CleanTest(c, escape, inString)) result += c;
    }
    return result;
}

JSONSource::JSONSource(std::string filename) 
    : filename(filename), 
    sourceStr(utilstr::ReadFromFile(filename)), 
    trimmedStr(CleanJSON(sourceStr)) { }

JSON::JSON(const std::string& filename)
{
    jsonSource = new JSONSource(filename);
    JSONString source = jsonSource->GetString();

    // Check for empty input
    if (source.Size() == 0)
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

void JSONString::PrintSyntaxMsg(std::string errorText, int msgType, size_t _Off)const
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
    for (counter = 0; counter < size; counter++)
    {
        if (counter == 1) continue;

        char c = at(counter);

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
            case '\"':
                str += '\"';
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

// TODO
JSONString JSONString::ScanSyntax(size_t& _Pos)
{
    char open = at(0);
    if (open != '{' && open != '[')
    {
        PrintSyntaxMsg("'{' or '[' expected.");
        return *this;
    }

    char close;
    if (open == '{') close = '}';
    if (open == '[') close = ']';

    size_t level = 0;
    size_t counter = 0;
    for (counter = 0; counter < size; counter++)
    {
        char c = at(counter);

        if (counter == 1) continue;

        if (c == close && level == 0) break;
    }
    return *this;
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
        body = body.substr(1, body.Size() - 2);

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
            for (counter = 0; counter < body.Size(); counter++)
            {
                char c = body.at(counter);
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
    /*else if (utilstr::BeginsAndEndsWith(body, '"'))
    {
        return new JSON::JSONLiteral<std::string>(utilstr::TrimOneChar(body),
            JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING);
    }*/

    // Numeric literal or invalid
    else
    {

    }

    return new JSON::JSONObject();
}

int add(int a, int b)
{
    return a + b;
}
