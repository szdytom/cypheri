#ifndef CYPHERI_TOKEN_HPP
#define CYPHERI_TOKEN_HPP

#include <cstdint>
#include <string_view>
#include <vector>
#include <optional>
#include <string>
#include <format>
#include <exception>
#include "cypheri/errors.hpp"
#include "cypheri/nametable.hpp"

namespace cypheri {

constexpr const char * TOKEN_TYPE_NAMES[] = {
	// special tokens
	"(eof)", "(error)", "(identifier)",
	// literal tokens
	"(integer)", "(number)", "(string)", "(symbol)",
	// arithmetic operators
	"+", "-", "*", "/", "%", "**", "//"
	"+=", "-=", "*=", "/=", "%=", "**=", "//=",
	"^", "&", "|", "~", "<<", ">>",
	"^=", "&=", "|=", "~=", "<<=", ">>=",
	// comparison operators
	"==", "!=", "<", ">", "<=", ">=",
	// logical operators
	"&&", "||", "!",
	// other operators
	"(", ")", "[", "]", "{", "}",
	".", ",", ";", "::", "=",
	// keywords
	"Break",
	"Class", "Continue", "Catch",
	"Declare", "Do",
	"End", "Else", "ElseIf",
	"Function", "For"
	"If", "Import",
	"Lambda",
	"Module",
	"New",
	"Return",
	"While",
	"Then", "Throw", "Typeof", "Try"
	"_Yield",
	// operators under builtin
	"BuiltinPopcnt", "BuiltinCtz", "BuiltinClz",
	"BuiltinAbs", "BuiltinCeil", "BuiltinFloor", "BuiltinRound",
	"BuiltinSwap",
	// end
	"(guard)"
};

using TokenType = std::uint8_t;
const TokenType TOKEN_COUNT = sizeof(TOKEN_TYPE_NAMES) / sizeof(TOKEN_TYPE_NAMES[0]);

constexpr TokenType TK(const char name[]) {
	for (int i = 0; i < TOKEN_COUNT; ++i) {
		const char *x = TOKEN_TYPE_NAMES[i];
		const char *y = name;
		while (*x && *y && *x == *y) {
			++x;
			++y;
		}

		if (*x == 0 && *y == 0)
			return i;
	}
	throw std::runtime_error(std::format("unknown token type: {}", name));
}

static_assert(TOKEN_COUNT == TK("(guard)") + 1, "TOKEN_COUNT is not correct");

class Token {
public:
	TokenType type;
	SourceLocation loc;
	union {
		uint64_t integer;
		double num;
		NameIdType id;
		size_t str_idx;
	};

	Token(TokenType type, SourceLocation loc) noexcept; // monostate tokens

	static Token from_integer(SourceLocation loc, uint64_t integer) noexcept;
	static Token from_number(SourceLocation loc, double num) noexcept;
	static Token from_identifier(SourceLocation loc, NameIdType id) noexcept;
	static Token from_string(SourceLocation loc, size_t str_idx) noexcept;
};

struct TokenizeResult {
	std::vector<Token> tokens;
	std::vector<std::string> str_literals;
	std::optional<SyntaxError> error;

	static TokenizeResult from_error(SourceLocation loc, const char *msg) noexcept;
};

TokenizeResult tokenize(std::string_view source, NameTable& name_table) noexcept;

} // namespace cypheri

#endif // CYPHERI_TOKEN_HPP