#include "token.hpp"
#include <cctype>
#include <cstdlib>

namespace cypheri {

Token::Token(TokenType type, SourceLocation loc) noexcept : type(type), loc(loc) {}
Token::Token(TokenType type, SourceLocation loc, uint64_t value) noexcept : type(type), loc(loc), value(value) {}
Token::Token(TokenType type, SourceLocation loc, double value) noexcept : type(type), loc(loc), value(value) {}
Token::Token(TokenType type, SourceLocation loc, std::string_view str) noexcept : type(type), loc(loc), value(std::string(str)) {}
Token::Token(TokenType type, SourceLocation loc, std::string&& str) noexcept : type(type), loc(loc), value(std::move(str)) {}
Token::Token(TokenType type, SourceLocation loc, const char *str) noexcept : type(type), loc(loc), value(std::string(str)) {}

class SourceStream {
public:
	SourceStream(std::string_view source) noexcept : source(source), pos{0}, cur_loc{1, 1} {}

	char peek() const noexcept {
		return source[pos];
	}

	char consume() noexcept {
		char c = source[pos];
		if (c == '\n') {
			cur_loc.line++;
			cur_loc.column = 1;
		} else {
			cur_loc.column++;
		}
		pos++;
		return c;
	}

	bool match(char expected) noexcept {
		if (peek() == expected) {
			consume();
			return true;
		}
		return false;
	}

	bool eof() const noexcept {
		return pos >= source.size();
	}

	SourceLocation location() const noexcept {
		return cur_loc;
	}

	void skip_whitespace() noexcept {
		while (!eof() && std::isspace(peek())) {
			consume();
		}
	}

	std::string_view consumeIdentifier() noexcept {
		pos -= 1; // consume() already advanced the position
		size_t len = 0, begin = pos;
		char c = peek();
		while (!eof() && (std::isalnum(c) || c == '_')) {
			len++;
			consume();
			c = peek();
		}
		return source.substr(begin, len);
	}

	std::string consumeString() noexcept {
	    // The opening quote is already consumed
		std::string res;
		bool escaped = false;
		while (!eof()) {
			char c = consume();
			if (escaped) {
			    switch (c) {
			    case 'n':
				    res += '\n';
				    break;
			    case 't':
				    res += '\t';
				    break;
			    case 'r':
				    res += '\r';
				    break;
			    case 'b':
				    res += '\b';
				    break;
			    case 'f':
				    res += '\f';
				    break;
			    case '"':
				    res += '"';
				    break;
			    case '\'':
				    res += '\'';
				    break;
			    case '\\':
				    res += '\\';
				    break;
				// TODO: \0, \x, \u
			    default:
					// unknown escape sequence, keep it as is
					res += c;
					break;
			    }
			    escaped = false;
			} else {
				switch (c) {
				case '"':
					return res;
				case '\\':
					escaped = true;
					break;
				default:
					res += c;
					break;
				}
			}
		}

		// unterminated string
		// keep it as is for now
		// TODO: error handling
		return res;
	}

private:
	std::string_view source;
	size_t pos;
	SourceLocation cur_loc;
};

// NOTE: this is not a full-fledged hex parser
// assumes that the char is a valid hex number! (0-9, a-f, A-F)
int hex2int(char c) noexcept {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return c - 'A' + 10;
}

TokenType match_keyword(std::string_view str) noexcept {
	// TODO: BuiltinXXX

	switch (str[0]) {
	case 'B':
		if (str == "Break") {
			return TK("Break");
		}
		break;
	case 'C':
		if (str == "Class") {
			return TK("Class");
		} else if (str == "Continue") {
			return TK("Continue");
		} else if (str == "Catch") {
			return TK("Catch");
		}
		break;
	case 'D':
		if (str == "Declare") {
			return TK("Declare");
		} else if (str == "Do") {
			return TK("Do");
		}
		break;
	case 'E':
		if (str == "End") {
			return TK("End");
		} else if (str == "Else") {
			return TK("Else");
		} else if (str == "ElseIf") {
			return TK("ElseIf");
		}
		break;
	case 'F':
		if (str == "Function") {
			return TK("Function");
		} else if (str == "For") {
			return TK("For");
		}
		break;
	case 'I':
		if (str == "If") {
			return TK("If");
		} else if (str == "Import") {
			return TK("Import");
		}
		break;
	case 'L':
		if (str == "Lambda") {
			return TK("Lambda");
		}
		break;
	case 'M':
		if (str == "Module") {
			return TK("Module");
		}
		break;
	case 'N':
		if (str == "New") {
			return TK("New");
		}
		break;
	case 'R':
		if (str == "Return") {
			return TK("Return");
		}
		break;
	case 'W':
		if (str == "While") {
			return TK("While");
		}
		break;
	case 'T':
		if (str == "Then") {
			return TK("Then");
		} else if (str == "Throw") {
			return TK("Throw");
		} else if (str == "Try") {
			return TK("Try");
		} else if (str == "Typeof") {
			return TK("Typeof");
		}
		break;
	case '_':
		if (str == "_Yield") {
			return TK("_Yield");
		}
		break;
	default:
		break;
	}
	return TK("(error)");
}

std::vector<Token> tokenize(std::string_view source) noexcept {
	std::vector<Token> tokens;
	SourceStream stream(source);

	stream.skip_whitespace();
	while (!stream.eof()) {
		auto loc = stream.location();
		char c = stream.consume();

		switch (c) {
		case '+':
			if (stream.match('=')) {
				tokens.emplace_back(TK("+="), loc);
			} else {
				tokens.emplace_back(TK("+"), loc);
			}
			break;
		case '-':
			if (stream.match('=')) {
				tokens.emplace_back(TK("-="), loc);
			} else {
				tokens.emplace_back(TK("-"), loc);
			}
			break;
		case '*':
			if (stream.match('=')) {
				tokens.emplace_back(TK("*="), loc);
			} else if (stream.match('*')) {
				if (stream.match('=')) {
					tokens.emplace_back(TK("**="), loc);
				} else {
					tokens.emplace_back(TK("**"), loc);
				}
			} else {
				tokens.emplace_back(TK("*"), loc);
			}
			break;
		case '/':
			if (stream.match('=')) {
				tokens.emplace_back(TK("/="), loc);
			} else if (stream.match('/')) {
				if (stream.match('=')) {
					tokens.emplace_back(TK("//="), loc);
				} else {
					tokens.emplace_back(TK("//"), loc);
				}
			} else {
				tokens.emplace_back(TK("/"), loc);
			}
			break;
		case '%':
			if (stream.match('=')) {
				tokens.emplace_back(TK("%="), loc);
			} else {
				tokens.emplace_back(TK("%"), loc);
			}
			break;
		case '^':
			if (stream.match('=')) {
				tokens.emplace_back(TK("^="), loc);
			} else {
				tokens.emplace_back(TK("^"), loc);
			}
			break;
		case '=':
			if (stream.match('=')) {
				tokens.emplace_back(TK("=="), loc);
			} else {
				tokens.emplace_back(TK("="), loc);
			}
			break;
		case '!':
			if (stream.match('=')) {
				tokens.emplace_back(TK("!="), loc);
			} else {
				tokens.emplace_back(TK("!"), loc);
			}
			break;
		case '<':
			if (stream.match('=')) {
				tokens.emplace_back(TK("<="), loc);
			} else if (stream.match('<')) {
				if (stream.match('=')) {
					tokens.emplace_back(TK("<<="), loc);
				} else {
					tokens.emplace_back(TK("<<"), loc);
				}
			} else {
				tokens.emplace_back(TK("<"), loc);
			}
			break;
		case '>':
			if (stream.match('=')) {
				tokens.emplace_back(TK(">="), loc);
			} else if (stream.match('>')) {
				if (stream.match('=')) {
					tokens.emplace_back(TK(">>="), loc);
				} else {
					tokens.emplace_back(TK(">>"), loc);
				}
			} else {
				tokens.emplace_back(TK(">"), loc);
			}
			break;
		case '&':
			if (stream.match('&')) {
				tokens.emplace_back(TK("&&"), loc);
			} else if (stream.match('=')) {
				tokens.emplace_back(TK("&="), loc);
			} else {
				tokens.emplace_back(TK("&"), loc);
			}
			break;
		case '|':
			if (stream.match('|')) {
				tokens.emplace_back(TK("||"), loc);
			} else if (stream.match('=')) {
				tokens.emplace_back(TK("|="), loc);
			} else {
				tokens.emplace_back(TK("|"), loc);
			}
			break;
		case ';':
			tokens.emplace_back(TK(";"), loc);
			break;
		case '(':
			tokens.emplace_back(TK("("), loc);
			break;
		case ')':
			tokens.emplace_back(TK(")"), loc);
			break;
		case '{':
			tokens.emplace_back(TK("{"), loc);
			break;
		case '}':	
			tokens.emplace_back(TK("}"), loc);
			break;
		case ',':
			tokens.emplace_back(TK(","), loc);
			break;
		case '[':
			tokens.emplace_back(TK("["), loc);
			break;
		case ']':
			tokens.emplace_back(TK("]"), loc);
			break;
		case ':':
			if (!stream.match(':')) {
				tokens.emplace_back(TK("(error)"), loc, "Unexpected ':'");
			} else {
				tokens.emplace_back(TK("::"), loc);
			}
			break;
		case '"':
			tokens.emplace_back(TK("(string)"), loc, stream.consumeString());
			break;
		default:
			if (std::isdigit(c)) {
				// TODO: handle hex, oct, and binary numbers, as well as floats
				uint64_t val = 0;
				bool overflow = false;
				do {
					if (val > std::numeric_limits<uint64_t>::max() / 10) {
						overflow = true;
						break;
					}
					val = val * 10 + (c - '0');
					if (stream.eof())
						break;
					c = stream.consume();
				} while (std::isdigit(c));
				if (overflow) {
					tokens.emplace_back(TK("(error)"), loc, "Integer literal overflow");
				} else {
					tokens.emplace_back(TK("(integer)"), loc, val);
				}
			} else if (std::isalpha(c) || c == '_') {
				std::string_view id = stream.consumeIdentifier();
				TokenType keyword = match_keyword(id);
				if (keyword != TK("(error)")) {
					tokens.emplace_back(keyword, loc);
				} else {
					tokens.emplace_back(TK("(identifier)"), loc, id);
				}
			} else {
				tokens.emplace_back(TK("(error)"), loc, "Unexpected character");
			}
		}
		stream.skip_whitespace();
	}

	tokens.emplace_back(TK("(eof)"), stream.location());
	return tokens;
}

} // namespace cypheri
