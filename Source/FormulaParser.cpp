#include "FormulaParser.h"
#include <random>

namespace fx
{
	using MersenneTwister = std::mt19937;
	using RandDistribution = std::uniform_real_distribution<float>;

	bool isDigit(Char chr) noexcept
	{
		return chr >= '0' && chr <= '9';
	}

	unsigned int getDigit(Char chr) noexcept
	{
		return chr - '0';
	}

	template<typename Float>
	Float getNumber(const String& txt, int& i) noexcept
	{
		auto num = static_cast<Float>(0);
		if (txt[i] != '.')
		{
			num = static_cast<Float>(getDigit(txt[i]));
			while (i + 1 < txt.length() && isDigit(txt[i + 1]))
				num = num * static_cast<Float>(10) + static_cast<Float>(getDigit(txt[++i]));
		}
		else
			--i;
		if (txt[i + 1] == '.')
		{
			++i;
			auto frac = static_cast<Float>(.1);
			while (i + 1 < txt.length() && isDigit(txt[i + 1]))
			{
				num += frac * static_cast<Float>(getDigit(txt[++i]));
				frac *= .1f;
			}
		}
		return num;
	}

	template float getNumber<float>(const String& txt, int& i) noexcept;
	template double getNumber<double>(const String& txt, int& i) noexcept;

	String toString(ParserErrorType p)
	{
		switch (p)
		{
		case ParserErrorType::NoError:
			return "No Error.";
		case ParserErrorType::EmptyString:
			return "Empty String.";
		case ParserErrorType::InvalidOperator:
			return "Invalid Operator.";
		case ParserErrorType::CouldNotTokenize:
			return "Unable To Tokenize!";
		case ParserErrorType::MismatchedParenthesis:
			return "Mismatched Parenthesis.";
		case ParserErrorType::UnknownToken:
			return "Unknown Token.";
		case ParserErrorType::WroteAmountOfArguments:
			return "Wrote Amount Of Arguments.";
		default: return "Unknown Error.";
		}
	}

	int getPrecedence(Operator op)
	{
		switch (op)
		{
		case Operator::Plus:
		case Operator::Minus:
			return 1;
		case Operator::Multiply:
		case Operator::Divide:
		case Operator::Modulo:
			return 2;
		case Operator::Power:
			return 3;
		case Operator::Asinh:
		case Operator::Acosh:
		case Operator::Atanh:
		case Operator::Floor:
		case Operator::Log10:
		case Operator::Noise:
		case Operator::Asin:
		case Operator::Acos:
		case Operator::Atan:
		case Operator::Ceil:
		case Operator::Cosh:
		case Operator::Log2:
		case Operator::Sinh:
		case Operator::Sign:
		case Operator::Sqrt:
		case Operator::Tanh:
		case Operator::Abs:
		case Operator::Cos:
		case Operator::Exp:
		case Operator::Sin:
		case Operator::Tan:
		case Operator::Log:
		case Operator::Ln:
			return 4;
		default:
			return 0;
		}
	}

	int getAssociativity(Operator op)
	{
		switch (op)
		{
		case Operator::Plus:
		case Operator::Minus:
		case Operator::Multiply:
		case Operator::Divide:
		case Operator::Modulo:
		case Operator::Asinh:
		case Operator::Acosh:
		case Operator::Atanh:
		case Operator::Floor:
		case Operator::Log10:
		case Operator::Noise:
		case Operator::Asin:
		case Operator::Acos:
		case Operator::Atan:
		case Operator::Ceil:
		case Operator::Cosh:
		case Operator::Log2:
		case Operator::Sinh:
		case Operator::Sign:
		case Operator::Sqrt:
		case Operator::Tanh:
		case Operator::Abs:
		case Operator::Cos:
		case Operator::Exp:
		case Operator::Sin:
		case Operator::Tan:
		case Operator::Log:
		case Operator::Ln:
			return 1;
		case Operator::Power:
			return 2;
		default:
			return 0;
		}
	}

	int getNumArguments(Operator op) noexcept
	{
		switch (op)
		{
		case Operator::Plus:
		case Operator::Minus:
		case Operator::Multiply:
		case Operator::Divide:
		case Operator::Modulo:
		case Operator::Power:
			return 2;
		case Operator::Asinh:
		case Operator::Acosh:
		case Operator::Atanh:
		case Operator::Floor:
		case Operator::Log10:
		case Operator::Noise:
		case Operator::Asin:
		case Operator::Acos:
		case Operator::Atan:
		case Operator::Ceil:
		case Operator::Cosh:
		case Operator::Log2:
		case Operator::Sinh:
		case Operator::Sign:
		case Operator::Sqrt:
		case Operator::Tanh:
		case Operator::Abs:
		case Operator::Cos:
		case Operator::Exp:
		case Operator::Sin:
		case Operator::Tan:
		case Operator::Log:
		case Operator::Ln:
			return 1;
		default:
			return 0;
		}
	}

	Operator getOperator(const String& str)
	{
		if (str == "+")
			return Operator::Plus;
		else if (str == "-")
			return Operator::Minus;
		else if (str == "*")
			return Operator::Multiply;
		else if (str == "/")
			return Operator::Divide;
		else if (str == "%")
			return Operator::Modulo;
		else if (str == "^")
			return Operator::Power;
		else if (str == "asinh")
			return Operator::Asinh;
		else if (str == "acosh")
			return Operator::Acosh;
		else if (str == "atanh")
			return Operator::Atanh;
		else if (str == "floor")
			return Operator::Floor;
		else if (str == "log10")
			return Operator::Log10;
		else if (str == "noise")
			return Operator::Noise;
		else if (str == "asin")
			return Operator::Asin;
		else if (str == "acos")
			return Operator::Acos;
		else if (str == "atan")
			return Operator::Atan;
		else if (str == "ceil")
			return Operator::Ceil;
		else if (str == "cosh")
			return Operator::Cosh;
		else if (str == "log2")
			return Operator::Log2;
		else if (str == "sinh")
			return Operator::Sinh;
		else if (str == "sign")
			return Operator::Sign;
		else if (str == "sqrt")
			return Operator::Sqrt;
		else if (str == "tanh")
			return Operator::Tanh;
		else if (str == "abs")
			return Operator::Abs;
		else if (str == "cos")
			return Operator::Cos;
		else if (str == "exp")
			return Operator::Exp;
		else if (str == "sin")
			return Operator::Sin;
		else if (str == "tan")
			return Operator::Tan;
		else if (str == "log")
			return Operator::Log;
		else if (str == "ln")
			return Operator::Ln;
		else
			return Operator::NumOperators;
	}

	String toString(Operator o)
	{
		switch (o)
		{
		case Operator::Plus:
			return "+";
		case Operator::Minus:
			return "-";
		case Operator::Multiply:
			return "*";
		case Operator::Divide:
			return "/";
		case Operator::Modulo:
			return "%";
		case Operator::Power:
			return "^";
		case Operator::Asinh:
			return "asinh";
		case Operator::Acosh:
			return "acosh";
		case Operator::Atanh:
			return "atanh";
		case Operator::Floor:
			return "floor";
		case Operator::Log10:
			return "log10";
		case Operator::Noise:
			return "noise";
		case Operator::Asin:
			return "asin";
		case Operator::Acos:
			return "acos";
		case Operator::Atan:
			return "atan";
		case Operator::Ceil:
			return "ceil";
		case Operator::Cosh:
			return "cosh";
		case Operator::Log2:
			return "log2";
		case Operator::Sinh:
			return "sinh";
		case Operator::Sign:
			return "sign";
		case Operator::Sqrt:
			return "sqrt";
		case Operator::Tanh:
			return "tanh";
		case Operator::Abs:
			return "abs";
		case Operator::Cos:
			return "cos";
		case Operator::Exp:
			return "exp";
		case Operator::Sin:
			return "sin";
		case Operator::Tan:
			return "tan";
		case Operator::Log:
			return "log";
		case Operator::Ln:
			return "ln";
		default: return "Unknown Operator.";
		}
	}

	String getOperator(const String& txt, int& i)
	{
		for (auto o = 0; o < NumOperators; ++o)
		{
			const auto op = static_cast<Operator>(o);
			const auto opStr = toString(op);
			if (txt.substring(i, i + opStr.length()) == opStr)
			{
				i += opStr.length() - 1;
				return opStr;
			}
		}
		return "";
	}

	Func getFunc(Operator o)
	{
		switch (o)
		{
		case Operator::Asinh:
			return [](float v) { return std::asinh(v); };
		case Operator::Acosh:
			return [](float v) { return std::acosh(v); };
		case Operator::Atanh:
			return [](float v) { return std::atanh(v); };
		case Operator::Floor:
			return [](float v) { return std::floor(v); };
		case Operator::Log10:
			return [](float v) { return std::log10(v); };
		case Operator::Noise:
			return [](float seed)
			{
				MersenneTwister mt(static_cast<unsigned int>(seed));
				RandDistribution dist(-1.f, 1.f);

				return dist(mt) * 2.f - 1.f;
			};
		case Operator::Asin:
			return [](float v) { return std::asin(v); };
		case Operator::Acos:
			return [](float v) { return std::acos(v); };
		case Operator::Atan:
			return [](float v) { return std::atan(v); };
		case Operator::Ceil:
			return [](float v) { return std::ceil(v); };
		case Operator::Cosh:
			return [](float v) { return std::cosh(v); };
		case Operator::Log2:
			return [](float v) { return std::log2(v); };
		case Operator::Sinh:
			return [](float v) { return std::sinh(v); };
		case Operator::Sign:
			return [](float v) { return std::signbit(v) ? -1.f : 1.f; };
		case Operator::Sqrt:
			return [](float v) { return std::sqrt(v); };
		case Operator::Tanh:
			return [](float v) { return std::tanh(v); };
		case Operator::Abs:
			return [](float v) { return std::abs(v); };
		case Operator::Cos:
			return [](float v) { return std::cos(v); };
		case Operator::Exp:
			return [](float v) { return std::exp(v); };
		case Operator::Sin:
			return [](float v) { return std::sin(v); };
		case Operator::Tan:
			return [](float v) { return std::tan(v); };
		case Operator::Log:
			return [](float v) { return std::log(v); };
		case Operator::Ln:
			return [](float v) { return std::log(v); };
		default:
			return nullptr;
		}
	}

	Func2 getFunc2(Operator o)
	{
		switch (o)
		{
		case Operator::Plus:
			return [](float a, float b) { return a + b; };
		case Operator::Minus:
			return [](float a, float b) { return a - b; };
		case Operator::Multiply:
			return [](float a, float b) { return a * b; };
		case Operator::Divide:
			return [](float a, float b)
			{
				if (b == 0.f)
					b = std::numeric_limits<float>::min();
				return a / b;
			};
		case Operator::Modulo:
			return [](float a, float b)
			{
				if (b == 0.f)
					b = std::numeric_limits<float>::min();
				return std::fmod(a, b);
			};
		case Operator::Power:
			return [](float a, float b)
			{
				if (a == 0.f)
					if (b < 0.f)
						a = std::numeric_limits<float>::min();
				return std::pow(a, b);
			};
		default:
			return nullptr;
		}
	}

	// Token

	Token::Token(Type _type, const String& text) :
		type(_type),
		value((type == Type::Number) || (type == Type::X) ? text.getFloatValue() : 0.f),
		op(getOperator(text)),
		precedence(getPrecedence(op)),
		associativity(getAssociativity(op)),
		numArguments(getNumArguments(op)),
		func(getFunc(op)),
		func2(getFunc2(op))
	{}

	Token::Token(Type _type, const Char _char) :
		type(_type),
		value((type == Type::Number) || (type == Type::X) ? static_cast<float>(_char - '0') : 0.f),
		op(getOperator(String::charToString(_char))),
		precedence(getPrecedence(op)),
		associativity(getAssociativity(op)),
		numArguments(getNumArguments(op)),
		func(getFunc(op)),
		func2(getFunc2(op))
	{}

	Token::Token(Operator _op) :
		type(Type::Operator),
		value(0.f),
		op(_op),
		precedence(getPrecedence(op)),
		associativity(getAssociativity(op)),
		numArguments(getNumArguments(op)),
		func(getFunc(op)),
		func2(getFunc2(op))
	{}

	//

	String toString(const Token& t)
	{
		switch (t.type)
		{
		case Token::Type::Number:
			return String(t.value);
		case Token::Type::X:
			return String((t.value == -1.f ? "-" : "")) + "X";
		case Token::Type::Operator:
			return toString(t.op);
		case Token::Type::ParenthesisLeft:
			return "(";
		case Token::Type::ParenthesisRight:
			return ")";
		default:
			return "Unknown Token";
		}
	}

	String toString(const Tokens& t)
	{
		String s;
		for (const auto& token : t)
			s += toString(token) + "\n";
		return s;
	}

	void addNumberToTokens(Tokens& tokens, float numbr)
	{
		auto mult = 1.f;
		if (!tokens.empty())
			if (tokens.back().type == Token::Type::Number ||
				tokens.back().type == Token::Type::X ||
				tokens.back().type == Token::Type::ParenthesisRight)
			{
				tokens.push_back({ Token::Type::Operator, "*" });
			}
			else if (tokens.back().type == Token::Type::Operator)
			{
				if (tokens.back().op == Operator::Minus)
				{
					mult = -1.f;
					tokens.pop_back();
				}
				else if (tokens.back().op == Operator::Plus)
				{
					tokens.pop_back();
				}
			}
		tokens.push_back(Token(Token::Type::Number, String(numbr * mult)));
	}

	Operator getRandomOperator(Random& rand) noexcept
	{
		return static_cast<Operator>(rand.nextInt(static_cast<int>(Operator::NumOperators)));
	}

	void addRandomNumber(Tokens& tokens, Random& rand, float likelyX, float numMin, float numMax)
	{
		bool addX = !(rand.nextFloat() > likelyX);
		if (addX)
			tokens.emplace_back(Token(Token::Type::X, (rand.nextBool() ? "1" : "-1")));
		else
		{
			const auto range = numMax - numMin;
			const auto numbr = numMin + rand.nextFloat() * range;
			tokens.push_back(Token(Token::Type::Number, String(numbr)));
		}
	}

	void generateTerm(Tokens& postfix, int numElements, float likelyX, float numMin, float numMax)
	{
		postfix.clear();
		postfix.reserve(numElements);
		Random rand;
		addRandomNumber(postfix, rand, likelyX, numMin, numMax);
		auto numArguments = 1;

		for (int i = 1; i < numElements; ++i)
		{
			bool addOperator = numArguments == 2 || rand.nextFloat() > .75f;
			if (addOperator)
			{
				while (true)
				{
					auto op = getRandomOperator(rand);
					auto numArgs = getNumArguments(op);
					if (numArgs == numArguments)
					{
						postfix.push_back(Token(op));
						numArguments = 1;
						break;
					}
				}
			}
			else
			{
				addRandomNumber(postfix, rand, likelyX, numMin, numMax);
				++numArguments;
			}
		}

		if (postfix.back().type == Token::Type::Number || postfix.back().type == Token::Type::X)
			while (numArguments != 0)
			{
				auto op = getRandomOperator(rand);
				auto args = getNumArguments(op);
				if (args == numArguments)
				{
					postfix.push_back({ op });
					return;
				}
			}
	}

	// Parser

	Parser::Parser() :
		errorType(ParserErrorType::NoError),
		func([](float) { return 0.f; })
	{
	}

	bool Parser::operator()(const String& text)
	{
		if (text.isEmpty())
		{
			errorType = ParserErrorType::EmptyString;
			return false;
		}

		auto txt = text.toLowerCase();

		// TOKENIZE
		Tokens tokens;
		for (auto i = 0; i < txt.length(); ++i)
		{
			auto chr = txt[i];
			// CHECK FOR OPERATORS
			const auto opStr = getOperator(txt, i);
			if (opStr.isNotEmpty())
			{
				Token op(Token::Type::Operator, opStr);

				if (!tokens.empty())
				{
					if (tokens.back().type == Token::Type::Number ||
						tokens.back().type == Token::Type::X ||
						tokens.back().type == Token::Type::ParenthesisRight)
					{
						if (op.numArguments != 2)
							tokens.push_back({ Token::Type::Operator, '*' });
					}
				}
				tokens.push_back(op);
			}
			// CHECK FOR NUMBERS
			else if (txt.substring(i, i + 3) == "tau")
			{
				addNumberToTokens(tokens, 6.28318530718f);
				i += 2;
			}
			else if (txt.substring(i, i + 2) == "pi")
			{
				addNumberToTokens(tokens, 3.14159265359f);
				++i;
			}
			else if (chr >= '0' && chr <= '9' || chr == '.')
			{
				const auto numbr = getNumber<float>(txt, i);
				addNumberToTokens(tokens, numbr);
			}
			// CHECK FOR X
			else if (chr == 'x')
			{
				float mult = 1.f;
				if (!tokens.empty())
					if (tokens.back().type == Token::Type::Number ||
						tokens.back().type == Token::Type::X ||
						tokens.back().type == Token::Type::ParenthesisRight)
					{
						tokens.push_back({ Token::Type::Operator, "*" });
					}
					else if (tokens.back().type == Token::Type::Operator)
					{
						if (tokens.back().op == Operator::Minus)
						{
							mult = -1.f;
							tokens.pop_back();
						}
						else if (tokens.back().op == Operator::Plus)
						{
							tokens.pop_back();
						}
					}
				tokens.push_back(Token(Token::Type::X, String(mult)));
			}
			// CHECK FOR PARANTHESIS
			else if (chr == '(')
			{
				tokens.push_back(Token(Token::Type::ParenthesisLeft));
			}
			else if (chr == ')')
			{
				tokens.push_back(Token(Token::Type::ParenthesisRight));
			}
			else if (chr == ' ')
			{
				continue;
			}
			// ERROR HANDLING
			else
			{
				errorType = ParserErrorType::CouldNotTokenize;
				return false;
			}
		}

#if JUCE_DEBUG && DebugFormularParser
		DBG("infix:");
		DBG(toString(tokens));
#endif

		// TO POSTFIX
		Tokens postfix;
		{
			Tokens stack;

			for (const auto& t : tokens)
			{
				switch (t.type)
				{
				case Token::Type::Number:
				case Token::Type::X:
					postfix.push_back(t);
					break;
				case Token::Type::Operator:
					while (!stack.empty() && stack.back().type == Token::Type::Operator &&
						((t.associativity == 0 && t.precedence <= stack.back().precedence) ||
							(t.associativity == 1 && t.precedence < stack.back().precedence)))
					{
						postfix.push_back(stack.back());
						stack.pop_back();
					}
					stack.push_back(t);
					break;
				case Token::Type::ParenthesisLeft:
					stack.push_back(t);
					break;
				case Token::Type::ParenthesisRight:
					while (!stack.empty() && stack.back().type != Token::Type::ParenthesisLeft)
					{
						postfix.push_back(stack.back());
						stack.pop_back();
					}
					if (stack.empty())
					{
						errorType = ParserErrorType::MismatchedParenthesis;
						return false;
					}
					stack.pop_back();
					if (!stack.empty())
						if (stack.back().type == Token::Type::Operator)
						{
							postfix.push_back(stack.back());
							stack.pop_back();
						}
					break;
				default:
					errorType = ParserErrorType::UnknownToken;
					return false;
				}
			}

			for (auto i = static_cast<int>(stack.size()) - 1; i >= 0; --i)
			{
				const auto& t = stack[i];
				if (t.type == Token::Type::ParenthesisLeft || t.type == Token::Type::ParenthesisRight)
				{
					errorType = ParserErrorType::MismatchedParenthesis;
					return false;
				}
				postfix.push_back(t);
			}
		}

		return operator()(postfix);
	}

	bool Parser::operator()(const Tokens& postfix)
	{
#if JUCE_DEBUG && DebugFormularParser
		DBG("postfix:");
		DBG(toString(postfix));
#endif

		// CREATE FUNCTION
		func = [this, postfix](float x)
		{
			Tokens stack;
			float y = 0.f;
			for (const auto& p : postfix)
			{
				switch (p.type)
				{
				case Token::Type::Number:
					y = p.value;
					stack.push_back(p);
					break;
				case Token::Type::X:
					y = x * p.value;
					stack.push_back({ Token::Type::Number, String(y) });
					break;
				case Token::Type::Operator:
					if (stack.size() < p.numArguments)
					{
						errorType = ParserErrorType::WroteAmountOfArguments;
						return 0.f;
					}

					if (p.numArguments == 1)
						y = p.func(stack.back().value);
					else if (p.numArguments == 2)
						y = p.func2(stack[stack.size() - 2].value, stack.back().value);

					for (auto i = 0; i < p.numArguments; ++i)
						stack.pop_back();

					stack.push_back({ Token::Type::Number, String(y) });
					break;
				default:
					errorType = ParserErrorType::UnknownToken;
					return 0.f;
				}
			}

			if (std::isnan(y) || std::isinf(y))
				return 0.f;
			return y;
		};

		errorType = ParserErrorType::NoError;
#if JUCE_DEBUG && DebugFormularParser
		DBG("\nerr: " << toString(errorType));
#endif
		return true;
	}

	float Parser::operator()(float x) const noexcept
	{
		return func(x);
	}

}