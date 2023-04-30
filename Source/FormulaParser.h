#pragma once
#include <functional>
#include "juce_core/juce_core.h"

#define DebugFormulaParser false

namespace fx
{
	using String = juce::String;
	using Func = std::function<float(float)>;
	using Func2 = std::function<float(float, float)>;
	using Char = juce::juce_wchar;
	using Random = juce::Random;

	bool isDigit(Char) noexcept;

	unsigned int getDigit(Char) noexcept;

	/* txt, idx */
	template<typename Float>
	Float getNumber(const String&, int&) noexcept;

	enum class ParserErrorType
	{
		NoError,
		EmptyString,
		InvalidOperator,
		CouldNotTokenize,
		MismatchedParenthesis,
		UnknownToken,
		WroteAmountOfArguments,
		NumTypes
	};

	String toString(ParserErrorType);

	enum class Operator
	{
		Plus,
		Minus,
		Multiply,
		Divide,
		Modulo,
		Power,
		Asinh,
		Acosh,
		Atanh,
		Floor,
		Log10,
		Noise,
		Asin,
		Acos,
		Atan,
		Ceil,
		Cosh,
		Log2,
		Sinh,
		Sign,
		Sqrt,
		Tanh,
		Abs,
		Cos,
		Exp,
		Sin,
		Tan,
		Log,
		Ln,
		NumOperators
	};
	static constexpr int NumOperators = static_cast<int>(Operator::NumOperators);

	int getPrecedence(Operator);

	int getAssociativity(Operator);

	int getNumArguments(Operator) noexcept;

	Operator getOperator(const String&);

	String toString(Operator);

	/* txt, idx */
	String getOperator(const String&, int&);

	Func getFunc(Operator);

	Func2 getFunc2(Operator);

	struct Token
	{
		enum class Type
		{
			Number,
			X,
			Operator,
			ParenthesisLeft,
			ParenthesisRight,
			NumTypes
		};

		Token(Type, const String & = "");

		Token(Type, const Char);

		Token(Operator);

		const Type type;
		const float value;
		const Operator op;
		const int precedence, associativity, numArguments;
		const Func func;
		const Func2 func2;
	};

	String toString(const Token&);

	using Tokens = std::vector<Token>;

	String toString(const Tokens&);

	/* tokens, numbr */
	void addNumberToTokens(Tokens&, float);

	Operator getRandomOperator(Random&) noexcept;

	/* tokens, rand, likelyX, numMin, numMax */
	void addRandomNumber(Tokens&, Random&, float, float, float);

	/* postfix, numElements, likelyX, numMin, numMax */
	void generateTerm(Tokens&, int, float, float, float);

	struct Parser
	{
		Parser();

		bool operator()(const String&);

		/* postfix */
		bool operator()(const Tokens&);

		/* x */
		float operator()(float = 0.f) const noexcept;

		ParserErrorType errorType;
	protected:
		Func func;
	};
}

// src:
// https://www.youtube.com/watch?v=PAceaOSnxQs
// https://stackoverflow.com/questions/789847/postfix-notation-validation
// http://mathcenter.oxford.emory.edu/site/cs171/postfixExpressions/

#undef DebugFormulaParser