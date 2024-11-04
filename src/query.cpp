
#include <iostream>

#include "query.h"
#include "utilstr.h"
#include "json_parser.h"

// Input: expression of the form "(A.B[2] - 1.65) * 6 + C.D[A.B[3]]"
// Tokenization:	1) "(A.B[2] - 1.65) * (6)" ++ "..."
//					2) Expr(EXPR_OP_MULTIPLY, Expr("A.B[2] - 1.65"), Expr("6")) + C.D[A.B[3]]
//					3) Expr(EXPR_OP_PLUS, 
//						Expr(EXPR_OP_MULTIPLY, 
//							Expr(EXPR_OP_PLUS, A.B[2], Expr("-1.65")), 
//							Expr(EXPR_OP_CONST, 6))
//						Expr("C.D[A.B[3]]"))

bool Tokenize(std::string source, std::string& token, size_t& pos)
{
	size_t depth = 0;
	for (size_t i = pos; i < source.size(); i++)
	{
		const char c = source.at(i);

		if (c == '(') depth++;
		if (c == ')') depth--;

		// Make sure we ignore the unary minus and leave it
		// to the caller
		if (depth == 0 && i > pos)
		{
			if (c == '+' || c == '-' || c == '/' || c == '*')
			{
				token = source.substr(pos, i - pos);
				pos = i;
				return true;
			}
		}
	}

	token = source.substr(pos);
	
	if (depth != 0)
	{
		std::cout << "Missing parentheses." << std::endl;
		token = "";
	}

	pos = source.size();
	return false;
}

bool ScanFunction(std::string src, std::string& functionName, std::vector<std::string>& args)
{
	size_t argsBegin = 0;
	argsBegin = src.find_first_of('(');

	if (argsBegin == std::string::npos)
	{
		return false;
	}

	functionName = src.substr(0, argsBegin);

	if (src.at(src.size() - 1) != ')')
	{
		std::cout << "Invalid function syntax." << std::endl;
		return false;
	}

	std::string argsStr = src.substr(argsBegin);
	argsStr = utilstr::TrimOneChar(argsStr);


	std::string argument;
	size_t _pos = 0;
	while (utilstr::Split(argsStr, ',', argument, _pos))
	{
		args.push_back(argument);
	}

	return true;
}

bool ProcessFunctions(std::string src, JSONInterface& jsonInterface, Either& output)
{
	std::vector<std::string> args;
	std::string function;

	// Not a function
	if (src.find_first_of('(') == src.npos)
	{
		return false;
	}
	
	if (!ScanFunction(src, function, args)) return false;

	// Now, we have a vector with arguments and are ready to process functions.

	if (function == "min" || function == "max")
	{
		// With one argument, we are processing a list.
		if (args.size() == 1)
		{
			JSON::JSONNode* node = jsonInterface.tree_walk(args[0]);
			if (!node) return true;
			if (node->GetType() != JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LIST)
			{
				std::cout << "Expected a list." << std::endl;
				return true;
			}
			JSON::JSONList* list = (JSON::JSONList*)node;

			std::vector<Either> numericElements;

			for (JSON::JSONNode* e : list->elements)
			{
				if (!JSON::isNumericLiteral(e->GetType())) continue;

				Either val;
				jsonInterface.GetValue(e, val);
				numericElements.push_back(val);
			}

			if (numericElements.size() == 0)
			{
				std::cout << "List empty or does not contain any numeric values." << std::endl;
				return true;
			}

			if (function == "min")
			{
				Either minElement = numericElements[0];
				for (Either val : numericElements)
				{
					if (val < minElement) minElement = val;
				}
				output = minElement;
				return true;
			}

			if (function == "max")
			{
				Either maxElement = numericElements[0];
				for (Either val : numericElements)
				{
					if (val > maxElement) maxElement = val;
				}
				output = maxElement;
				return true;
			}

			return true;
		}

		// With more arguments, we are processing literals
		else
		{
			std::vector<Either> elements;

			for (std::string arg : args)
			{
				Expr expr(arg, jsonInterface);
				elements.push_back(expr.Eval());
			}

			if (function == "min")
			{
				Either minValue = elements[0];

				for (Either e : elements)
				{
					if (e < minValue) minValue = e;
				}
				output = minValue;
				return true;
			}

			if (function == "max")
			{
				Either maxValue = elements[0];

				for (Either e : elements)
				{
					if (e > maxValue) maxValue = e;
				}
				output = maxValue;
				return true;
			}

			return true;
		}
	}
	if (function == "size")
	{
		if (args.size() < 1)
		{
			std::cout << "Provide an object, list or string." << std::endl;
			return true;
		}

		JSON::JSONNode* node = jsonInterface.tree_walk(args[0]);
		if (!node) return true;

		if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_STRING)
		{
			std::string val;
			jsonInterface.GetValue<std::string>(node, val);

			output = Either((int)val.size());
			return true;
		}
		if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_OBJECT)
		{
			JSON::JSONObject* obj = (JSON::JSONObject*)node;
			output = Either((int)obj->members.size());
			return true;
		}
		if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LIST)
		{
			JSON::JSONList* list = (JSON::JSONList*)node;
			output = Either((int)list->elements.size());
			return true;
		}
		else
		{
			std::cout << "Expected an object, list or string." << std::endl;
			return true;
		}

	}
	return false;
}

Expr::Expr(std::string body, JSONInterface& jsonInterface)
{
	// Trim all parentheses
	while (utilstr::BeginsAndEndsWith(body, '(', ')'))
	{
		body = utilstr::TrimOneChar(body);
	}

	std::string token;
	size_t pos = 0;

	// Get the first token
	bool literalToken = !Tokenize(body, token, pos);

	
	if (token == "")
	{
		std::cout << "Invalid token." << std::endl;
		return;
	}

	// No other operator detecter after given token
	if (literalToken)
	{
		// Process unary minus
		if (token.at(0) == '-')
		{
			lhs = new Expr(token.substr(1), jsonInterface);
			OpCode = EXPR_OP_UNARY_MINUS;
			return;
		}
		 
		// Process in-terminal numeric literals
		if (isdigit(token.at(0)))
		{
			if (!utilstr::GetNumLiteralValue(token, value))
			{
				std::cout << "Invalid numeric literal." << std::endl;
				return;
			}
			OpCode = EXPR_OP_CONST;
			return;
		}

		Either val;
		if (ProcessFunctions(body, jsonInterface, val))
		{
			OpCode = EXPR_OP_CONST;
			value = val;
			return;
		}

		// Process numeric JSON queries
		else
		{
			JSON::JSONNode* node = jsonInterface.tree_walk(token);

			if (!node)
			{
				std::cout << "Element not found." << std::endl;
				return;
			}


			if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_INT)
			{
				JSON::JSONLiteral<int>* literalInt = (JSON::JSONLiteral<int>*)node;
				value = Either(literalInt->GetValue());
				OpCode = EXPR_OP_CONST;
				return;
			}

			else if (node->GetType() == JSON::JSON_NODE_TYPE::JSON_NODE_TYPE_LITERAL_DOUBLE)
			{
				JSON::JSONLiteral<double>* literalDouble = (JSON::JSONLiteral<double>*)node;
				value = Either(literalDouble->GetValue());
				OpCode = EXPR_OP_CONST;
				return;
			}
			else
			{
				std::cout << "Cannot perform operations on non-numeric literals." << std::endl;
				return;
			}
		}
		return;
	}
	
	// If the program has reached this point, there are some
	// binary operators to process.
	char op = body.at(pos);


	// A*B/C+E
	// ((A*B)/C)+..

	// LHS is by default the token itself,
	// can only be changed with multiplication or division
	// due to operator precedence rules.
	Expr* prevExpr = new Expr(token, jsonInterface);

	// Logic of '*':
	// Form an expression with the next token.
	if (op == '*' || op == '/')	// token = "A"
	{
		pos++;

		// Initial LHS is the expression constructed recursively
		// with a token. Expression tree is, however, constructed later on
		// by appending nodes upwards, i.e. using previous node to create current.
		

		unsigned int prevOpCode = CharToOpcode(op);

		bool terminateLoop = false;
		// Iterate till an operator other that '*' or '/' is found,
		// or an end of expression
		while (!terminateLoop)
		{
			Tokenize(body, token, pos);

			// Termination conditions
			if (pos == body.size())
			{
				// Terminate at next iteration
				terminateLoop = true;
			}
			else if (body.at(pos) != '*' && body.at(pos) != '/')
			{
				// Terminate at next iteration
				terminateLoop = true;
				op = body.at(pos);
			}


			Expr* expr = new Expr();
			expr->lhs = prevExpr;
			expr->rhs = new Expr(token, jsonInterface);
			expr->OpCode = prevOpCode;

			prevExpr = expr;

			if (!terminateLoop) prevOpCode = CharToOpcode(body.at(pos));
			if (!terminateLoop) pos++;
		}

		
	}

	lhs = prevExpr;

	// If neither '+' nor '-' detected AFTER
	// multiplication was done
	if (op == '*' || op == '/')
	{
		OpCode = EXPR_OP_IDENTITY;
		return;
	}


	// Logic of '+': 
	// Create an expression with LHS being the token and RHS everything else.
	if (op == '+')
	{
		rhs = new Expr(body.substr(pos + 1), jsonInterface);
		OpCode = EXPR_OP_PLUS;
		return;
	}

	// Logic of '-':
	// Same as with '+', but leave unary minus in the RHS.
	if (op == '-')
	{
		rhs = new Expr(body.substr(pos), jsonInterface);
		OpCode = EXPR_OP_PLUS;
		return;
	}

	std::cout << "Invalid operator \"" << op << "\"." << std::endl;
	OpCode = EXPR_OP_INVALID;
	return;
}

Either Expr::Eval()
{
	switch (OpCode)
	{
	case EXPR_OP_CONST:
		return value;
	case EXPR_OP_PLUS:
		return Plus(lhs->Eval(), rhs->Eval());
	case EXPR_OP_UNARY_MINUS:
		return UnaryMinus(lhs->Eval());
	case EXPR_OP_MULTIPLY:
		return Mult(lhs->Eval(), rhs->Eval());
	case EXPR_OP_DIVIDE:
		return Div(lhs->Eval(), rhs->Eval());
	case EXPR_OP_IDENTITY:
		return lhs->Eval();
	default:
		return Either(0);
	}
}

Either Plus(Either a, Either b)
{
	if (a.Type == EITHER_DOUBLE || b.Type == EITHER_DOUBLE)
	{
		return Either(a.GetDouble() + b.GetDouble());
	}
	// None of a or b are doubles
	return Either(a.NumInt + b.NumInt);
}

Either UnaryMinus(Either a)
{
	if (a.Type == EITHER_INT)
	{
		return Either(-a.NumInt);
	}
	else return Either(-a.NumDouble);
}

Either Mult(Either a, Either b)
{
	if (a.Type == EITHER_DOUBLE || b.Type == EITHER_DOUBLE)
	{
		return Either(a.GetDouble() * b.GetDouble());
	}
	// None of a or b are doubles
	return Either(a.NumInt * b.NumInt);
}

Either Div(Either a, Either b)
{
	if (a.Type == EITHER_DOUBLE || b.Type == EITHER_DOUBLE)
	{
		return Either(a.GetDouble() / b.GetDouble());
	}
	// None of a or b are doubles
	return Either(a.NumInt / b.NumInt);
}

bool Either::operator>(const Either& rhs) const
{
	if (Type == EITHER_DOUBLE || rhs.Type == EITHER_DOUBLE) return GetDouble() > rhs.GetDouble();
	return NumInt > rhs.NumInt;
}

bool Either::operator<(const Either& rhs) const
{
	if (Type == EITHER_DOUBLE || rhs.Type == EITHER_DOUBLE) return GetDouble() < rhs.GetDouble();
	return NumInt < rhs.NumInt;
}

bool Either::operator==(const Either& rhs) const
{
	if (Type == EITHER_DOUBLE || rhs.Type == EITHER_DOUBLE) return GetDouble() == rhs.GetDouble();
	return NumInt == rhs.NumInt;
}
