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

class JSONString;
// Class for dealing with JSON syntax error highlighing
class JSONSource
{
    const std::string sourceStr;	// String as in initial JSON file
    const std::string trimmedStr;	// String without whilespaces (outside strings), newlines and tabs

    const std::string filename;

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

    // Return an initial JSONString, with whole trimmed string and offset of zero
    JSONString GetString();
    std::string GetFilename() { return filename; }
};



// Augmented string type to support finding source position
class JSONString : public std::string
{
    size_t offset;		// Offset from beginning of the source string
    JSONSource* source;	// Class has no ownership of that pointer,
                        // thus the default copy constructor will do fine.

    // Instance of JSONString can only be created via:
    //	1) JSONSource::GetString()	- Initial string with offset 0
    //	2) JSONString::substr(..)	- All subsequent substrings
    //
    // It is a point of design that JSON strings are to be created only via substrings.
    // It ensures that any JSON string is an actual fragment of trimmed source JSON file.
    friend class JSONSource;
    JSONString(std::string str, JSONSource* source, size_t offset = 0)
        : std::string(str), source(source), offset(offset) { }
public:

    // Override substring method to increase offset
    const JSONString substr(size_t _Off, size_t _Count)
    {
        return JSONString(std::string::substr(_Off, _Count), source, offset += _Off);
    }

    // Access source position easily without having to care 
    JSONSource::Pos GetSourcePos(size_t _Off = 0)
    {
        return source->GetSymbolSourcePosition(offset + _Off);
    }

    // By creation, instance of JSONString cannot contain nullptr JSONSource pointer.
    JSONSource* GetSource() { return source; }

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
