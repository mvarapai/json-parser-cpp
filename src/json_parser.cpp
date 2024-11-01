#include "json_parser.h"
#include "utilstr.h"

#include <iostream>

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(JSONString body);

JSONString JSONSource::GetString() { return JSONString(this, trimmedStr.data(), trimmedStr.size()); }

void isInString(const char c, bool& escape, bool& inString)
{
    // If '"' is in no escape sequence, it is either opening or closing quote.
    if (c == '"' && !escape) inString = !inString;

    // If escape is false, backslash will expect some other character.
    // If true, the escape character is simply written.
    if (c == '\\') escape = !escape;
    else if (escape) escape = false;
}

// Loop invariant: check whether the symbol should be retained.
// Arguments:
//  c : current character
//  escape : true if previous character was '\'
//  inString : true if character c is contained in string literal
// Supposed to be invoked from a while loop, with initial bool values set to false.
bool CleanTest(const char c, bool& escape, bool& inString)
{
    isInString(c, escape, inString);

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

void JSONString::PrintSyntaxMsg(std::string errorText, int msgType, size_t _Off)const
{
    // [ERROR] test1.json:2 - "X expected."
    std::string msg;

    switch (msgType)
    {
    case 0:
        msg = "\n[ERROR]";
        break;
    case 1:
        msg = "\n[WARNING]";
        break;
    default:
        msg = "\n[MESSAGE]";
    }

    msg += " ";
    msg += source->GetFilename();
    msg += ":";
    msg += std::to_string(GetSourcePos(_Off).line);
    msg += " - ";
    msg += errorText;

    std::cerr << msg << std::endl;

    // Going on with a syntax error is impossible.
    if (msgType == SYNTAX_MSG_TYPE_ERROR) 
    {
        std::cerr << "Interpretation failed." << std::endl;
        exit(0);
    }
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
    for (counter = 1; counter < size; counter++)
    {
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
        
        // Read next character as an escape sequence
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

    _Pos = counter + 1;
    return str;
}

JSONString JSONString::ScanListObjectBody(size_t& _Pos)
{
    char closing;
    if (front() == '{') closing = '}';
    else if (front() == '[') closing = ']';
    else 
    {
        PrintSyntaxMsg("Expected an object or a list.");
        return *this;
    }

    bool escape = false;
    bool inString = false;
    size_t depth = 0;
    size_t i;
    for (i = 0; i < size; i++)
    {
        const char c = at(i);
        isInString(c, escape, inString);
        
        if (!inString)
        {
            if (c == '{' || c == '[') depth++;
            if (c == '}' || c == ']') depth--;
        }

        // If all parentheses are closed
        if (depth == 0)
        {
            // Check if the closing parentheses correspond
            if (c != closing)
            {
                PrintSyntaxMsg("Parentheses mismatch.", SYNTAX_MSG_TYPE_ERROR, i);
                return *this;
            }

            _Pos = i + 1;
            return substr(0, _Pos);
        }
    }
    PrintSyntaxMsg("No closing parentheses found.", SYNTAX_MSG_TYPE_ERROR, size - 1);
    return *this;
}

JSONString JSONString::ScanLiteral(size_t& _Pos)
{
    bool escape = false;
    bool inString = false;

    // Scan till the first comma outside a string
    size_t i;
    for (i = 0; i < size; i++)
    {
        const char c = at(i);
        isInString(i, escape, inString);

        if (c == ',' && !inString)
        {
            _Pos = i;
            return substr(0, i);
        }
    }

    // i == size
    _Pos = i;
    return substr(0, i);
}

JSON::JSON(const std::string& filename)
{
    jsonSource = new JSONSource(filename);
    JSONString source = jsonSource->GetString();

    // Check for empty input
    if (source.Size() == 0)
    {
        std::string errorMsg = "JSON file does not exist or is empty. ";
        errorMsg += "An empty JSON instance is created.";
        source.PrintSyntaxMsg(errorMsg, SYNTAX_MSG_TYPE_WARNING);

        return;
    }

    // Check first important condition - global space must be an object.
    if (!utilstr::BeginsAndEndsWith(source, '{', '}'))
    {
        std::string errorMsg = "JSON file does not contain an object. ";
        errorMsg += "Correct format of the file would be: \"{..}\". ";
        errorMsg += "Empty JSON object returned.";
        source.PrintSyntaxMsg(errorMsg);

        return;
    }

    // Here, by the condition above, we are certain that the global space
    // is in fact a JSON object.
    globalSpace = static_cast<JSONObject*>(resolve_json(source));
}

JSON::JSONNode* resolve_json(JSONString body)
{	
    // A JSON object
    if (utilstr::BeginsAndEndsWith(body, '{', '}'))
    {
        // Create new JSON object and expose its members
        JSON::JSONObject* object = new JSON::JSONObject;
        body = body.substr(1, body.Size() - 2);

        if (body.Size() == 0)
        {
            body.PrintSyntaxMsg("Expected an expression.");
        }

        // Iterate through "id": value pairs
        do
        {
            // Pair to store member
            std::pair<std::string, JSON::JSONNode*> member;
            
            // First is the identifier
            size_t pos = 0;
            std::string id = body.ScanString(pos);
            member.first = id;

            // Trim the identifier
            body = body.substr(pos);

            if (body.front() != ':')
            {
                body.PrintSyntaxMsg("Expected ':'.");
                return nullptr;
            }

            // Trim the ':'
            body = body.substr(1);

            // Retrieve the body
            
            // If we are dealing with an object or a list
            if (body.front() == '{' || body.front() == '[')
            {
                JSONString objectListBody = body.ScanListObjectBody(pos);
                member.second = resolve_json(objectListBody);
            }
            else    // Some literal
            {
                JSONString literalBody = body.ScanLiteral(pos);
                member.second = resolve_json(literalBody);
            }

            object->members.insert(member);

            // Remove contents from the string.
            body = body.substr(pos);

            // If we processed the whole string, the object is processed.
            if (body.Size() == 0)
            {
                break;
            }

            if (body.front() == ',')
            {
                // Trim the comma
                body = body.substr(1);
            }
            else
            {
                // Hypothetically impossible situation, purely to avoid infinite loop.
                body.PrintSyntaxMsg("Expected ','.");
                break;
            }

        } while (true);

        return object;
    }

    // A JSON list
    else if (utilstr::BeginsAndEndsWith(body, '[', ']'))
    {
        body = body.substr(1, body.Size() - 2);
        if (body.Size() == 0) 
        {
            body.PrintSyntaxMsg("List cannot be empty.");
            return nullptr;
        }

        JSON::JSONList* list = new JSON::JSONList();
        size_t pos = 0;

        // Read elements
        do
        {
            if (body.front() == '{' || body.front() == '[')
            {
                JSONString objectListBody = body.ScanListObjectBody(pos);
                list->elements.push_back(resolve_json(objectListBody));
            }
            else    // Some literal
            {
                JSONString literalBody = body.ScanLiteral(pos);
                list->elements.push_back(resolve_json(literalBody));
            }

            // Remove everything before the current object
            body = body.substr(pos);

            // End of the list
            if (body.Size() == 0)
            {
                break;
            }

            // If comma, delete it and start another iteration
            if (body.front() == ',')
            {
                body = body.substr(1);
            }
            else
            {
                // Hypothetically impossible situation, purely to avoid infinite loop.
                body.PrintSyntaxMsg("Expected ','.");
                break;
            }

        } while (true);
        return list;
    }

    // A string literal
    else if (utilstr::BeginsAndEndsWith(body, '"'))
    {
        size_t pos = 0;
        JSON::JSONLiteral<std::string>* literalPtr = new JSON::JSONLiteral<std::string>(body.ScanString(pos), JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING);
    
        // If there are any symbols after the string
        if (body.Size() > pos)
        {
            body.PrintSyntaxMsg("Invalid characters after string literal.", SYNTAX_MSG_TYPE_ERROR, pos);
        }

        return literalPtr;
    }

    // Numeric literal or invalid
    else
    {
        body.PrintSyntaxMsg("Invalid expression.");
    }

    return new JSON::JSONObject();
}