#include "json_parser.h"
#include "utilstr.h"
#include "command.h"
#include "query.h"

#include <iostream>
#include <cmath>

static constexpr char SeparatorChar = '-';

// Helper recursive function, assumes unprocessed strings
JSON::JSONNode* resolve_json(JSONString body, JSON::JSONNode* parent);

// Create an initial JSONString, containing the whole trimmed data.
JSONString JSONSource::GetString() { return JSONString(this, trimmedStr.data(), trimmedStr.size()); }

// A simple state machine to be called from a loop with two reference variables,
// tells whether or not current character is in a string.
// Technically, inString for opening quote would be true, and for a closing false,
// but in applications of this function it does not matter.
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

// Constructor of JSONSource - provider of underlying data to JSONString.
JSONSource::JSONSource(std::string filename) 
    : filename(filename), 
    sourceStr(utilstr::ReadFromFile(filename)), 
    trimmedStr(CleanJSON(sourceStr)) { }

// Main means for displaying a message. If message is an error, program cannot function
// correctly and it exits.
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

    // Going on with a syntax error is impossible.
    if (msgType == SYNTAX_MSG_TYPE_ERROR) 
    {
        std::cerr << "Interpretation failed." << std::endl;
        exit(0);
    }
}


// Given a JSONString starting with '"', retrieve the string literal.
// If successfully terminated, returns std::string containing the literal,
// and _Pos being equal to the position of character after closing '"'.
// Parser uses following escape sequences:
//  \\ -> backslash
//  \n -> newline
//  \t -> tab
//  \" -> "
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

    size_t i;
    for (i = 1; i < size; i++)
    {
        char c = at(i);

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
                PrintSyntaxMsg("Valid escape sequence expected.", SYNTAX_MSG_TYPE_WARNING, i);
            }
            continue;
        }
        
        // Read next character as an escape sequence
        if (c == '\\')
        {
            escape = true;
            continue;
        }

        // Closing '"' found
        if (c == '"')
        {
            _Pos = i + 1;
            return str;
        }

        // Any other character
        str += c;
    }

    // No closing '"' found.
    PrintSyntaxMsg("'\"' expected.", SYNTAX_MSG_TYPE_ERROR, i - 1);    
    _Pos = i;
    return str;
}

// Given JSONString starting with either '{' or '[', locates a
// corresponding closing parentesis and returns substring containing
// the contents including the parentheses, and position one character
// after the closing parenthesis.
// Example: input  = "{..[..]..{..{..}..}..}text"
//          output = "{..[..]..{..{..}..}..}", _Pos = position of 't' in the substring.
JSONString JSONString::ScanListObjectBody(size_t& _Pos)
{
    // Initial validity checks
    char closing;
    if (front() == '{') closing = '}';
    else if (front() == '[') closing = ']';
    else 
    {
        PrintSyntaxMsg("Expected an object or a list.");
        return *this;
    }

    // Variables used by isInString(..)
    bool escape = false;
    bool inString = false;

    // Number of opened parentheses
    size_t depth = 0;

    // Iterate over whole string 
    size_t i;
    for (i = 0; i < size; i++)
    {
        const char c = at(i);

        // This function provides all necessary string checks
        isInString(c, escape, inString);
        
        // Perform depth increment and decrement
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

            // i is the position of closing parenthesis.
            // We increment this position to include it into the substring.
            _Pos = i + 1;
            return substr(0, _Pos);
        }
    }
    PrintSyntaxMsg("No closing parentheses found.", SYNTAX_MSG_TYPE_ERROR, size - 1);
    return *this;
}

// Given JSONString, the function looks for a comma outside a string literal.
// If found, returns a substring without the comma and _Pos of the comma.
// If no comma found, returns this string and _Pos one symbol after.
JSONString JSONString::ScanLiteral(size_t& _Pos)
{
    bool escape = false;
    bool inString = false;

    // Scan till the first comma outside a string
    size_t i;
    for (i = 0; i < size; i++)
    {
        const char c = at(i);
        isInString(c, escape, inString);

        if (c == ',' && !inString)
        {
            // When substr(_Pos) is called afterwards,
            // the comma is preserved for parser to perform further reasoning.
            // substr(0, i), on the other hand, does not include that comma.
            _Pos = i;
            return substr(0, i);
        }
    }

    // Here, if no comma was detected, we return _Pos = size.
    // That is, whole string is returned, and no substring can be
    // found using _Pos, because it is the first element after the string end.
    _Pos = i;
    return substr(0, i);
}

// Entry point to creating a JSON object.
// Performs some assertions and builds recursively the JSON syntax tree.
JSON::JSON(const std::string& filename)
{
    jsonSource = new JSONSource(filename);
    JSONString source = jsonSource->GetString();

    // Check for empty input
    if (source.Size() == 0)
    {
        std::string errorMsg = "JSON file does not exist or is empty.";
        source.PrintSyntaxMsg(errorMsg, SYNTAX_MSG_TYPE_ERROR);

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
    globalSpace = static_cast<JSONObject*>(resolve_json(source, globalSpace));
}

// Forward declarations for functions used in resolve_json(..)
inline JSON::JSONNode* resolve_object(JSONString body, JSON::JSONNode* parent);
inline JSON::JSONNode* resolve_list(JSONString body, JSON::JSONNode* parent);
inline JSON::JSONNode* resolve_literal(JSONString body, JSON::JSONNode* parent);
inline JSON::JSONNode* resolve_number(JSONString body, JSON::JSONNode* parent);

// Main recursive body of the parser. Builds syntax tree by passing recursively
// contents of objects, lists and literals to the new instance of this function,
// which returns a pointer to corresponding nodes.
// Builds on the structure of JSONSource and JSONString.
JSON::JSONNode* resolve_json(JSONString body, JSON::JSONNode* parent)
{	
    // A JSON object
    if (utilstr::BeginsAndEndsWith(body, '{', '}'))
    {
        return resolve_object(body, parent);
    }

    // A JSON list
    else if (utilstr::BeginsAndEndsWith(body, '[', ']'))
    {
        return resolve_list(body, parent);
    }

    return resolve_literal(body, parent);
}

// Recursively resolve members of an object.
inline JSON::JSONNode* resolve_object(JSONString body, JSON::JSONNode* parent)
{
    // Create new JSON object and expose its members
    JSON::JSONObject* object = new JSON::JSONObject(parent);
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
        if (id.size() < 1)
        {
            body.PrintSyntaxMsg("Expected valid identifier.");
            return nullptr;
        }

        // Check for uniqueness
        if (object->members.find(id) != object->members.end())
        {
            // There already exists an object with such id.
            body.PrintSyntaxMsg("Identifier is not unique.");
            return nullptr;
        }

        // If the identifier is valid, write it
        member.first = id;

        // "..":LIT(,..) -> :LIT(,..)
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
            member.second = resolve_json(objectListBody, object);
        }
        else    // Some literal
        {
            JSONString literalBody = body.ScanLiteral(pos);
            member.second = resolve_json(literalBody, object);
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

// Resolve the contents of the list and recursively call
// the constructors of its subnodes.
inline JSON::JSONNode* resolve_list(JSONString body, JSON::JSONNode* parent)
{
    body = body.substr(1, body.Size() - 2);

    JSON::JSONList* list = new JSON::JSONList(parent);
    size_t pos = 0;

    // Return an empty list
    if (body.Size() == 0)
    {
        return list;
    }

    // Read elements
    do
    {
        if (body.front() == '{' || body.front() == '[')
        {
            JSONString objectListBody = body.ScanListObjectBody(pos);
            list->elements.push_back(resolve_json(objectListBody, list));
        }
        else    // Some literal
        {
            JSONString literalBody = body.ScanLiteral(pos);
            list->elements.push_back(resolve_json(literalBody, list));
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

// If passed string is neither list nor object, it is dealt with as a literal.
// Here, types bool and null are considered. For numerical, subfunction is called.
inline JSON::JSONNode* resolve_literal(JSONString body, JSON::JSONNode* parent)
{
    // A string literal
    if (utilstr::BeginsAndEndsWith(body, '"'))
    {
        size_t pos = 0;
        JSON::JSONLiteral<std::string>* literalPtr = new JSON::JSONLiteral<std::string>(
            body.ScanString(pos), JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING, parent);

        // If there are any symbols after the string
        if (body.Size() > pos)
        {
            body.PrintSyntaxMsg("Invalid characters after string literal.", SYNTAX_MSG_TYPE_ERROR, pos);
        }

        return literalPtr;
    }

    if (body.ToString() == "true")
    {
        return new JSON::JSONLiteral<bool>(true, JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_BOOL, parent);
    }

    if (body.ToString() == "false")
    {
        return new JSON::JSONLiteral<bool>(false, JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_BOOL, parent);
    }

    if (body.ToString() == "null")
    {
        return new JSON::JSONNull(parent);
    }

    return resolve_number(body, parent);
}

// Purpose: given a JSONString, create a node and return its address.
// Numerical is supposed to be of format 'x.xE(+/-)x', x denoting some integer.
inline JSON::JSONNode* resolve_number(JSONString body, JSON::JSONNode* parent)
{
    Either number;
    if (!utilstr::GetNumLiteralValue(body.ToString(), number))
    {
        body.PrintSyntaxMsg("Invalid literal.");
        return nullptr;
    }

    if (number.Type == EITHER_INT)
    {
        return new JSON::JSONLiteral<int>(number.NumInt, JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT, parent);
    }
    else
    {
        return new JSON::JSONLiteral<double>(number.NumDouble, JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_DOUBLE, parent);
    }
}

JSONInterface JSON::CreateInterface()
{
    return JSONInterface(globalSpace);
}

// Relative to local object, find the node at this request
// Example: A.B[A.C[2]]
std::string JSONInterface::Select(std::string request)
{
    JSON::JSONNode* node = tree_walk(request);

    if (!node)
    {
        return "Could not select an object.\n";
    }

    if (node->GetType() != JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT)
    {
        return "Can only select a node with type OBJECT.\n";
    }

    currentObject = (JSON::JSONObject*)node;
    return "Successfully selected new object.";
}

JSON::JSONNode* JSONInterface::tree_walk(std::string request)
{
    JSON::JSONNode* current = currentObject;

    size_t prevPos = 0;
    size_t pos = 0;

    while (pos < request.size())
    {
        const char c = request.at(pos);

        if (c == '[')
        {
            std::string index = utilstr::ScanIndex(request, pos);

            if (index == "") return nullptr;

            // Retrieve index value

            size_t indexNum;

            // At some point, we will encounter an int literal index
            if (utilstr::IsNumLiteral(index)) indexNum = std::stoi(index);
            else    // Do a recursive call
            {
                JSON::JSONNode* indexNode = tree_walk(index);
                if (!indexNode)
                    return nullptr;
                if (indexNode->GetType() != JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT)
                {
                    std::cout << "[ERROR] Tried to access a list using non-numeric index." << std::endl;
                    return nullptr;
                }

                JSON::JSONLiteral<int>* indexIntLiteral = (JSON::JSONLiteral<int>*)indexNode;
                indexNum = indexIntLiteral->GetValue();
            
            }

            // Here, index is known, retrieve next node
            if (current->GetType() != JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LIST)
            {
                std::cout << "[ERROR] Tried to access a non-list element with an index." << std::endl;
                return nullptr;
            }

            JSON::JSONList* list = (JSON::JSONList*)current;
            if (!(current = list->Find(indexNum)))
            {
                return nullptr;
            }
        }

        // If dot detected or at the start
        else if (c == '.' || pos == 0)
        {

            // Move object after the dot
            if (pos != 0) prevPos = pos + 1;
            pos = request.find_first_of(".[", prevPos);
            std::string identifier = request.substr(prevPos, pos - prevPos);

            // Check if current node is an object
            if (current->GetType() != JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT)
            {
                std::cout << "[ERROR] Tried to access a member \"" 
                    << identifier << "\" of non-object element" << std::endl;
                return nullptr;
            }

            JSON::JSONObject* obj = (JSON::JSONObject*)current;
            
            if (!(current = obj->Find(identifier)))
            {
                return nullptr;
            }
        }

        else
        {
            std::cout << "Invalid situation." << std::endl;
            return nullptr;
        }
    }
    return current;
}

std::string getLiteralValue(JSON::JSONNode* node)
{
    JSON::JSON_NODE_TYPE type = node->GetType();
    std::string result;

    switch (type)
    {
    case JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING:
        result += "\"";
        result += ((JSON::JSONLiteral<std::string>*)node)->GetValue();
        result += "\"";
        break;
    case JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_BOOL:
        bool valueB;
        valueB = ((JSON::JSONLiteral<bool>*)node)->GetValue();
        if (valueB) result += "true";
        else result += "false";
        break;
    case JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT:
        int valueI;
        valueI = ((JSON::JSONLiteral<int>*)node)->GetValue();
        result += std::to_string(valueI);
        break;
    case JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_DOUBLE:
        double valueD;
        valueD = ((JSON::JSONLiteral<double>*)node)->GetValue();
        result += std::to_string(valueD);
        break;
    case JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_NULL:
        result += "null";
        break;
    default:
        result += "Unknown";
    }

    return result;
}

void JSON::JSONObject::ListMembers(bool showValues,
    unsigned int depth, unsigned int maxDepth)
{
    ConsoleTable<2> table({ 2, 2 }, depth);
    table.PrintSeparator(SeparatorChar);

    for (std::pair<std::string, JSONNode*> member : members)
    {
        JSON_NODE_TYPE type = member.second->GetType();

        // Assemble first column
        
        std::string col1 = member.first;


        std::string col2 = ": ";
        col2 += ToString(type);

        // Print data to console
        table.PrintLine({ col1, col2 });

        // Print values

        if (isLiteral(type) && showValues)
        {
            table.PrintLine({ std::string("= ") + getLiteralValue(member.second), "" });
        }
        
        if (depth < maxDepth)
        {
            if (type == JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT)
            {
                JSONObject* obj = (JSONObject*)member.second;
                obj->ListMembers(showValues, depth + 1, maxDepth);
                std::cout << std::endl;
            }

            if (type == JSON_NODE_TYPE::JSON_NODE_TYPE_LIST)
            {
                JSONList* list = (JSONList*)member.second;
                list->ListMembers(showValues, depth + 1, maxDepth);
                std::cout << std::endl;
            }
        }
    }
    table.PrintSeparator(SeparatorChar);
}

void JSON::JSONList::ListMembers(bool showValues,
    unsigned int depth, unsigned int maxDepth)
{
    ConsoleTable<2> table({ 2, 2 }, depth);
    table.PrintSeparator(SeparatorChar);

    size_t index = 0;
    for (JSONNode* element : elements)
    {
        JSON_NODE_TYPE type = element->GetType();

        // Assemble first column

        std::string col1 = "[";
        col1 += std::to_string(index);
        col1 += "]";

        std::string col2 = ": ";
        col2 += ToString(type);

        // Print data to console
        table.PrintLine({ col1, col2 });

        if (isLiteral(type) && showValues)
        {
            table.PrintLine({ std::string("= ") + getLiteralValue(element), ""});
        }


        if (depth < maxDepth)
        {
            if (type == JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT)
            {
                JSONObject* obj = (JSONObject*)element;
                obj->ListMembers(showValues, depth + 1, maxDepth);
                std::cout << std::endl;
            }

            if (type == JSON_NODE_TYPE::JSON_NODE_TYPE_LIST)
            {
                JSONList* list = (JSONList*)element;
                list->ListMembers(showValues, depth + 1, maxDepth);
                std::cout << std::endl;
            }
        }
        index++;
    }
    table.PrintSeparator(SeparatorChar);
}

// Can only return JSON objects
JSON::JSONNode* recursive_back(JSON::JSONNode* ptr, unsigned int steps)
{
    JSON::JSONNode* parent = ptr->GetParent();

    // Reached the root
    if (!parent)
    {
        return ptr;
    }

    if (parent->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT)
    {
        steps--;
    }

    if (steps == 0)
    {
        return parent;
    }
    return recursive_back(parent, steps);
}

void JSONInterface::Back(unsigned int steps)
{
    currentObject = (JSON::JSONObject*)recursive_back(currentObject, steps);
}

bool JSONInterface::GetValue(JSON::JSONNode* node, Either& value)
{
    if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT)
    {
        value.Type = EITHER_INT;
        GetValue<int>(node, value.NumInt);
        return true;
    }
    if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_DOUBLE)
    {
        value.Type = EITHER_DOUBLE;
        GetValue<double>(node, value.NumDouble);
        return true;
    }
    return false;
}
