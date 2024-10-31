/*****************************************************************//**
 * \file   json_parser.h
 * \brief  Defines JSON parsing functionality.
 * 
 * \author Mikalai Varapai
 * \date   October 2024
 *********************************************************************/

#include <string>
#include <unordered_map>

#define SYNTAX_MSG_TYPE_ERROR 0
#define SYNTAX_MSG_TYPE_WARNING 1
#define SYNTAX_MSG_TYPE_MESSAGE 2

// Forward declaration to use in JSONSource
class JSONString;


// Class for dealing with JSON syntax error highlighing
class JSONSource
{
    const std::string filename;

    const std::string sourceStr;	// String as in initial JSON file
    const std::string trimmedStr;	// String without whilespaces (outside strings), newlines and tabs.
                                    // This is the string we will work with from now on.

    friend class JSONString;
    const char* data() { return trimmedStr.data(); }

public:
    // Struct to represent position of a character
    struct Pos
    {
        unsigned int line = 0;
        unsigned int col = 0;

        std::string ToString();
    };

    JSONSource(std::string filename);				// Read and trim source file
    Pos GetSymbolSourcePosition(size_t trimmedPos);	// Iterate the file to find position

    // Return an initial JSONString, with offset of zero and whole size.
    // This is supposed to be the only way to get JSONString not from another instance.
    JSONString GetString();

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

    // Create a new JSONString object using string chunk
    const JSONString substr(size_t _Off, size_t _Count)
    {
        const char* pData = data;
        pData += _Off;

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

        return JSONString(source, data + _Off, _Count);
    }

    // Access source position easily without having to care 
    JSONSource::Pos GetSourcePos(size_t _Off = 0)
    {
        // If character position is outside the string, use the last char
        if (_Off >= size) _Off = size - 1;

        size_t offsetFromSource = data - source->data();
        return source->GetSymbolSourcePosition(offsetFromSource + _Off);
    }

    // By creation, instance of JSONString cannot contain nullptr JSONSource pointer.
    JSONSource* GetSource() { return source; }

    size_t Size() { return size; }

    const char at(size_t index)
    {
        if (index >= size) index = size - 1;
        return data[index];
    }

    std::string ToString() 
    {
        return std::string(data, size);
    }

    void PrintSyntaxMsg(std::string errorText, int msgType = 0, size_t _Off = 0);

    // Scan string at the beginning, bounded by \".
    std::string ScanString(size_t& _Pos);
    JSONString ScanSyntax(size_t& _Pos);
};

// Class to represent JSON syntax tree.
class JSON
{
private:
    // Create JSON from file
    JSON(const std::string& filename);

public:
    // Default copy and assignment
    JSON& operator=(const JSON& rhs) = delete;
    JSON(const JSON& other) = delete;

public:
    enum class JSON_NODE_TYPE
    {
        JSON_NODE_TYPE_INVALID = -1,
        JSON_NODE_TYPE_OBJECT = 0,
        JSON_NODE_TYPE_LIST = 1,
        JSON_NODE_TYPE_LITERAL_STRING = 2,
        JSON_NODE_TYPE_LITERAL_INT = 3,
    };

    bool isLiteral(JSON_NODE_TYPE type) { return (int)type > 1; }

    // Basic node object. Contains only type, cannot be instantiated.
    class JSONNode
    {
        // It is expected that type of the node cannot be changed during runtime.
        const JSON_NODE_TYPE type = JSON_NODE_TYPE::JSON_NODE_TYPE_INVALID;

    public:
        JSONNode(JSON_NODE_TYPE nodeType) : type(nodeType) { }
    };

    // JSON object - contains a list of identifiers and links further down the tree.
    struct JSONObject : JSONNode
    {
        JSONObject() : JSONNode(JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT) { }
        std::unordered_map<std::string, JSONNode*> members;
    };

    //	List - special structure in the tree, works parallel to JSONObject.
    struct JSONList : public JSONNode
    {
        JSONList() : JSONNode(JSON_NODE_TYPE::JSON_NODE_TYPE_LIST) { }
        std::vector<JSONObject*> elements;
    };

    // JSON literal - leaf of the tree.
    template <typename T>
    struct JSONLiteral : public JSONNode
    {
    private:
        T value;

    public:
        JSONLiteral(T literalValue, JSON_NODE_TYPE literalType) :
            value(literalValue), JSONNode(literalType) { }

        T GetValue() { return value; }
        JSON_NODE_TYPE GetType() { return type; }
    };


private:
    // Root of the JSON sytax tree, must be a JSON object.
    JSONObject* globalSpace = nullptr;
    JSONSource* jsonSource = nullptr;

    void Release()
    {
        // Go down the syntax tree and call Release on these nodes
    }

    ~JSON()
    {
        // TODO: Tree deallocation
        //delete jsonSource;
    }
};
