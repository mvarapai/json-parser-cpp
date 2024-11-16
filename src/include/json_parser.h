/*****************************************************************//**
 * \file   json_parser.h
 * \brief  Defines JSON parsing functionality.
 * 
 * \author Mikalai Varapai
 * \date   October 2024
 *********************************************************************/

#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>

#define SYNTAX_MSG_TYPE_ERROR 0
#define SYNTAX_MSG_TYPE_WARNING 1
#define SYNTAX_MSG_TYPE_MESSAGE 2

// Forward declaration to use in JSONSource
class JSONString;

// JSONSource - class providing underlying data for JSONString's and used
// for error reporting, where each JSONString knows its offset from the source start.
// The class provides functionality to find a line and column of given character offset.
class JSONSource
{
    const std::string filename;

    const std::string sourceStr;	// String as in initial JSON file
    const std::string trimmedStr;	// String without whilespaces (outside strings), newlines and tabs.
                                    // This is the string we will work with from now on.

    // JSONString class has to know about data pointer, while we want to hide it from everyone else.
    friend class JSONString;
    const char* data() { return trimmedStr.data(); }

public:
    // Struct to represent position of a character
    struct Pos
    {
        size_t line = 0;
        size_t col = 0;

        std::string ToString();

        Pos(size_t line, size_t col) : line(line), col(col) { }

        // Pos comparison operator, used in testing.
        bool operator==(const Pos& other) const
        {
            return (line == other.line && col == other.col);
        }
    };

    JSONSource(std::string filename);				// Read and trim source fi
    Pos GetSymbolSourcePosition(size_t trimmedPos);	// Iterate the file to find position

    // Return an initial JSONString, with offset of zero and whole size.
    // This is supposed to be the only way to get JSONString not from another instance.
    JSONString GetString();

    // Getter for file name, used in debugging.
    std::string GetFilename() { return filename; }
};



// An interface to access JSON string.
//
// The rationale behind using C-strings is that all types of JSONString are interfaces to
// substrings from the same JSONSource, which stores std::string with all data. Thus,
// we can avoid having to copy the same char sequence, which would otherwise be wasteful.
// Performance gain from using such interface is especially noticeable since each recursive call
// to process JSON would need a string to work on, which would otherwise be copied each time.
//
// Specification:
//  1. data MUST be inside JSONSource string
//  2. data + size MUST be inside JSONSource string
class JSONString
{
private:
    size_t size;        // Size of the char sequence
    const char* data;   // C-style string (no ownership);
                        // WARNING: not null-terminated

    JSONSource* source;	// Class has no ownership of that pointer,
                        // thus the default copy constructor will do fine.

    // Instance of JSONString can only be created via:
    //	1) JSONSource::GetString()	- Initial string with offset 0
    //	2) JSONString::substr(..)	- All subsequent substrings
    //
    // It is a point of design that JSON strings are to be created only via substrings.
    // It ensures that any JSON string is an actual fragment of trimmed source JSON file.
    friend class JSONSource;
    JSONString(JSONSource* pSource, const char* pData, size_t size)
        : source(pSource), data(pData), size(size) { }
public:

    // Create a new JSONString object using string chunk.
    // This function gives a no-throw guarantee as out-of-bound
    // arguments are trimmed to the string size where possible.
    // If substring is empty, original string is returned, with the
    // exception of when _Count == 0, where just an empty string is returned.
    const JSONString substr(size_t _Off, size_t _Count)
    {
        // Specification check for the object
        if (_Off + _Count >= size)
        {
            // If completely outside boundaries, copy this string.
            if (_Off >= size)
            {
                return *this;
            }
            // Otherwise, trim it at the end
            return JSONString(source, data + _Off, size - _Off);
        }
        // Regular substring
        return JSONString(source, data + _Off, _Count);
    }

    // Substring from _Off inclusive till the end
    const JSONString substr(size_t _Off)
    {
        if (_Off > size)
        {
            return *this;
        }
        return JSONString(source, data + _Off, size - _Off);
    }

    // Access source position easily without having to care 
    JSONSource::Pos GetSourcePos(size_t _Off = 0) const
    {
        // If character position is outside the string, use the last char
        if (_Off > size) _Off = size - 1;

        size_t offsetFromSource = data - source->data();
        return source->GetSymbolSourcePosition(offsetFromSource + _Off);
    }

    // By creation, instance of JSONString cannot contain nullptr JSONSource pointer.
    JSONSource* GetSource() const { return source; }


    // Some common string functionality

    size_t Size() const { return size; }

    const char at(size_t index) const
    {
        if (index >= size) index = size - 1;
        return data[index];
    }

    const char front() const
    {
        return at(0);
    }

    std::string ToString() const
    {
        return std::string(data, size);
    }

    // String-specific message handling
    void PrintSyntaxMsg(std::string errorText, int msgType = 0, size_t _Off = 0) const;

    // Scan string at the beginning, bounded by '"'.
    std::string ScanString(size_t& _Pos);

    // Scan {..} or [..]
    JSONString ScanListObjectBody(size_t& _Pos);

    JSONString ScanLiteral(size_t& _Pos);
};

class JSONInterface;
class Expr;

// Class to represent JSON syntax tree.
class JSON
{
public:
    // Create JSON from file
    JSON(const std::string& filename);

    // Forbid copying (potentially to be implemented later)
    JSON& operator=(const JSON& rhs) = delete;
    JSON(const JSON& other) = delete;

public:
    enum class JSON_NODE_TYPE
    {
        JSON_NODE_TYPE_LITERAL_NULL = -1,
        JSON_NODE_TYPE_OBJECT = 0,
        JSON_NODE_TYPE_LIST = 1,
        JSON_NODE_TYPE_LITERAL_STRING = 2,
        JSON_NODE_TYPE_LITERAL_INT = 3,
        JSON_NODE_TYPE_LITERAL_DOUBLE = 4,
        JSON_NODE_TYPE_LITERAL_BOOL = 5,
    };

    static std::string ToString(JSON_NODE_TYPE type)
    {
        switch (type) {
        case JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_NULL:
            return "NULL";
        case JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT:
            return "OBJECT";
        case JSON_NODE_TYPE::JSON_NODE_TYPE_LIST:
            return "LIST";
        case JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING:
            return "STRING";
        case JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT:
            return "INT";
        case JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_DOUBLE:
            return "DOUBLE";
        case JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_BOOL:
            return "BOOL";
        default:
            return "UNKNOWN";
        }
    }

    static bool isLiteral(JSON_NODE_TYPE type) { return (int)type > 1; }

    static bool isNumericLiteral(JSON_NODE_TYPE type) 
    {
        return type == JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT
            || type == JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_DOUBLE;
    }



    // Basic node object. Contains only type, cannot be instantiated.
    // All elements of JSON syntax tree are of that object.
    class JSONNode
    {
        JSONNode* parent = nullptr;     // Can only be nullptr if it is the root object
        // It is expected that type of the node cannot be changed during runtime.
        const JSON_NODE_TYPE type = JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_NULL;

    public:
        JSONNode(JSON_NODE_TYPE nodeType, JSONNode* parent) : type(nodeType), parent(parent) { }

        JSON_NODE_TYPE GetType() const { return type; }

        // Literals do not need to implement this function
        virtual ~JSONNode() 
        {
        }

        JSONNode* GetParent() { return parent; }

        friend class JSONInterface;
    };

   

    // JSON object - contains a list of identifiers and links further down the tree.
    class JSONObject : public JSONNode
    {
    public:
        std::unordered_map<std::string, JSONNode*> members;

        JSONObject(JSONNode* parent) : JSONNode(JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT, parent) 
        {
        }

        void ListMembers(bool showValue = false, unsigned int depth = 0,
            unsigned int maxDepth = UINT32_MAX);

        JSONNode* Find(std::string identifier)
        {
            // Cannot find this member
            if (members.find(identifier) == members.end())
            {
                std::string errorMsg = "[ERROR] Cannot find member \"";
                errorMsg += identifier;
                errorMsg += "\" of the object.";
                std::cout << errorMsg << std::endl;
                return nullptr;
            }

            // Node is an object, and given identifier exists
            return members.at(identifier);
        }

        ~JSONObject() override
        {
            for (std::pair member : members)
            {
                if (member.second) delete member.second;
                member.second = nullptr;
            }
        }
    };


    //	List - special structure in the tree, works in parallel to JSONObject.
    class JSONList : public JSONNode
    {
    public:
        std::vector<JSONNode*> elements;

        JSONList(JSONNode* parent) : JSONNode(JSON_NODE_TYPE::JSON_NODE_TYPE_LIST, parent) 
        {
        }

        JSONNode* Find(size_t index)
        {
            if (index >= elements.size())
            {
                std::cout << "[ERROR] Tried to access an out-of-bound index." << std::endl;
                return nullptr;
            }
            return elements.at(index);
        }

        void ListMembers(bool showValues = false,
            unsigned int depth = 0, unsigned int maxDepth = UINT32_MAX);

        ~JSONList() override
        {
            for (JSONNode* e : elements)
            {
                if (e) delete e;
                e = nullptr;
            }
        }
    };


    // Special node type to handle nullable values
    class JSONNull : public JSONNode
    {
    public:
        JSONNull(JSONNode* parent) : JSONNode(JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_NULL, parent)
        {
        }
    };


    // JSON literal - leaf of the tree.
    template <typename T>
    class JSONLiteral : public JSONNode
    {
    private:
        T value;

    public:
        JSONLiteral(T literalValue, JSON_NODE_TYPE literalType, JSONNode* parent) :
            value(literalValue), JSONNode(literalType, parent) 
        {
        }

        T GetValue() { return value; }
        JSON_NODE_TYPE GetType() { return type; }

        ~JSONLiteral()
        {
        }
    };

private:
    // Root of the JSON sytax tree, must be a JSON object.
    JSONObject* globalSpace = nullptr;
    JSONSource* jsonSource = nullptr;

public:

    ~JSON()
    {
        if (globalSpace) delete globalSpace;
        globalSpace = nullptr;
    }

    friend class JSONInterface;
    JSONInterface CreateInterface();
};

std::string getLiteralValue(JSON::JSONNode* node);

struct Either;
class CommandInterface;

// Class used to traverse JSON syntax tree
class JSONInterface
{
     JSON::JSONObject* currentObject;
     std::string currentObjectName = "~";

     friend void ProcessInput(std::string, JSONInterface&, CommandInterface&);
     friend bool ProcessFunctions(std::string src, JSONInterface& jsonInterface, Either& output);
     friend class Expr;
     JSON::JSONNode* tree_walk(std::string request);

public:

    JSONInterface(JSON::JSONObject* object) : currentObject(object) 
    {
    }

    std::string ListMembers(bool showValues, unsigned int maxDepth)
    {
        currentObject->ListMembers(showValues, 0, maxDepth);
        return "";
    }

    std::string Select(std::string request);

    void Back(unsigned int steps);

    // WARNING: trying to get a literal of another type will
    // give unpredictable result.
    // Always check the type beforehand.
    template <typename T>
    bool GetValue(JSON::JSONNode* node, T& value)
    {
        if (!node)
        {
            return false;
        }

        if (JSON::isLiteral(node->GetType()))
        {
            JSON::JSONLiteral<T>* literal = (JSON::JSONLiteral<T>*)node;
            value = literal->GetValue();
            return true;
        }
        return false;
    }

    bool GetValue(JSON::JSONNode* node, Either& value);

    
};