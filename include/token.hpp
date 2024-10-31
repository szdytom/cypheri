#ifndef CYPHERI_TOKEN_HPP
#define CYPHERI_TOKEN_HPP

#include <cstdint>
#include <string_view>
#include <vector>
#include <variant>
#include <string>

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

constexpr TokenType TK(const char name[]) noexcept {
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
	return -1;
}

static_assert(TOKEN_COUNT == TK("(guard)") + 1, "TOKEN_COUNT is not correct");

struct SourceLocation {
	uint32_t line;
	uint32_t column;
};

class Token {
public:
	TokenType type;
	SourceLocation loc;
	std::variant<std::monostate, uint64_t, double, std::string> value;

	Token(TokenType type, SourceLocation loc) noexcept;
	Token(TokenType type, SourceLocation loc, uint64_t integer) noexcept;
	Token(TokenType type, SourceLocation loc, double num) noexcept;
	Token(TokenType type, SourceLocation loc, std::string_view str) noexcept;
	Token(TokenType type, SourceLocation loc, std::string&& str) noexcept;
	Token(TokenType type, SourceLocation loc, const char *str) noexcept;
};

std::vector<Token> tokenize(std::string_view source) noexcept;

} // namespace cypheri

#endif // CYPHERI_TOKEN_HPP