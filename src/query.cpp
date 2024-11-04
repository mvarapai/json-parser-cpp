
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

Expr::Expr(std::string body, JSONInterface& jsonInterface)
{
	std::cout << "Received expression \"" << body << "\";" << std::endl;

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

		
	//	// Function
	//	else
	//	{
	//		size_t argsBegin = 0;
	//		argsBegin = body.find_first_of('(');

	//		if (argsBegin == std::string::npos)
	//		{
	//			std::cout << "Invalid function syntax" << std::endl;
	//			return;
	//		}

	//		std::string function = body.substr(0, argsBegin);

	//		std::string args = body.substr(argsBegin);
	//		args = utilstr::TrimOneChar(args);

	//		std::vector<std::string> arguments;

	//		std::string argument;
	//		size_t _pos;
	//		while (utilstr::Split(args, ',', argument, _pos))
	//		{
	//			arguments.push_back(argument);
	//		}

	//		
	//	}
	//	

	//	return;
	//}
	
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
