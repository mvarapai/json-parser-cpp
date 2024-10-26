/*****************************************************************//**
 * \file   json_parser.h
 * \brief  Defines JSON parsing functionality.
 * 
 * \author Mikalai Varapai
 * \date   October 2024
 *********************************************************************/

#include <string>
#include <unordered_map>

class JSON
{
private:
	// Due to several ways to intialize a JSON,
	// contructor will be private and empty.
	JSON() { }

public:
	JSON& operator=(const JSON& rhs) = default;
	JSON(const JSON& other);

public:
	static JSON ReadFromFile(const std::string& filename);
	static JSON ReadFromString(const std::string& source);

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
	virtual class JSONNode
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
	JSONObject* globalSpace;
};


