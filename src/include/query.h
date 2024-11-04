/*****************************************************************//**
 * \file   query.h
 * \brief  Header file for processing CLI JSON queries.
 * 
 * \author Mikalai Varapai
 * \date   November 2024
 *********************************************************************/

#include <string>

#define EXPR_OP_INVALID 0
#define EXPR_OP_CONST 1
#define EXPR_OP_UNARY_MINUS 2
#define EXPR_OP_PLUS 3
#define EXPR_OP_MULTIPLY 4
#define EXPR_OP_DIVIDE 5
#define EXPR_OP_CUSTOM_UNARY 6
#define EXPR_OP_CUSTOM_BINARY 7
#define EXPR_OP_IDENTITY 8

inline unsigned int CharToOpcode(const char c)
{
	switch (c)
	{
	case '+':
		return EXPR_OP_PLUS;
	case '-':
		return EXPR_OP_UNARY_MINUS;
	case '*':
		return EXPR_OP_MULTIPLY;
	case '/':
		return EXPR_OP_DIVIDE;
	default:
		return EXPR_OP_INVALID;
	}
}

#define EITHER_INT 0
#define EITHER_DOUBLE 1

// Throughout the interpretation of expression we need some
// general type to express a numeric literal.
// The idea for such type came from functional programming and Haskell.
struct Either
{
	unsigned int Type;

	union
	{
		int NumInt;
		double NumDouble;
	};

	Either(int num) : Type(EITHER_INT), NumInt(num) { }
	Either(double num) : Type(EITHER_DOUBLE), NumDouble(num) { }
	Either() : Type(EITHER_INT), NumInt(0) { }

	std::string ToString()
	{
		if (Type == EITHER_INT)
		{
			return std::to_string(NumInt);
		}
		else
		{
			return std::to_string(NumDouble);
		}
	}

	// Whatever the actual value, convert to double
	double GetDouble()const
	{
		if (Type == EITHER_DOUBLE)
		{
			return NumDouble;
		}
		else return NumInt;
	}

	Either& operator=(Either& rhs)
	{
		Type = rhs.Type;
		if (rhs.Type == EITHER_INT)
		{
			NumInt = rhs.NumInt;
		}
		else
		{
			NumDouble = rhs.NumDouble;
		}
		return *this;
	}

	bool operator>(const Either& rhs) const;
	bool operator<(const Either& rhs) const;
	bool operator==(const Either& rhs) const;
};

// Define arithmetic for Either

Either Plus(Either a, Either b);
Either UnaryMinus(Either a);
Either Mult(Either a, Either b);
Either Div(Either a, Either b);

class JSONInterface;
bool Tokenize(std::string source, std::string& token, size_t& pos);
bool ProcessFunctions(std::string src, JSONInterface& jsonInterface, Either& output);

// a.b[a.b[1]].c
class Expr
{
	friend bool ProcessFunctions(std::string src, JSONInterface& jsonInterface, Either& output);

	unsigned int OpCode = EXPR_OP_INVALID;

	Expr* lhs = nullptr;
	Expr* rhs = nullptr;
	Either value;


public:
	Either Eval();
	Expr(std::string, JSONInterface& jsonInterface);
	Expr() = default;
};

