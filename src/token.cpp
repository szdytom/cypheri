#include "cypheri/token.hpp"
#include <cctype>
#include <cstdlib>
#include <limits>

namespace cypheri {

Token::Token(TokenType type, SourceLocation loc) noexcept
	: type(type), loc(loc) {}

Token Token::from_integer(SourceLocation loc, uint64_t value) noexcept {
	Token token(TK("(integer)"), loc);
	token.integer = value;
	return token;
}

Token Token::from_number(SourceLocation loc, double value) noexcept {
	Token token(TK("(number)"), loc);
	token.num = value;
	return token;
}

Token Token::from_identifier(SourceLocation loc, NameIdType id) noexcept {
	Token token(TK("(identifier)"), loc);
	token.id = id;
	return token;
}

Token Token::from_string(SourceLocation loc, size_t idx) noexcept {
	Token token(TK("(string)"), loc);
	token.str_idx = idx;
	return token;
}

TokenizeResult TokenizeResult::from_error(SourceLocation loc,
										  const char *msg) noexcept {
	return TokenizeResult{.error = SyntaxError(msg, loc)};
}

namespace {

class SourceStream {
public:
	SourceStream(std::string_view source) noexcept
		: source(source), pos{0}, cur_loc{1, 1} {}

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
		// consume() already advanced the position, rollback
		pos -= 1;
		cur_loc.column -= 1;

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
		} else if (str == "FALSE") {
			return TK("FALSE");
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
		} else if (str == "NULL") {
			return TK("NULL");
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
		} else if (str == "TRUE") {
			return TK("TRUE");
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
	return TK("(identifier)");
}

} // namespace

TokenizeResult tokenize(std::string_view source,
						NameTable &name_table) noexcept {
	TokenizeResult res;
	auto &tokens = res.tokens;
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
				return TokenizeResult::from_error(loc, "Expected '::'");
			} else {
				tokens.emplace_back(TK("::"), loc);
			}
			break;
		case '"':
			res.str_literals.push_back(stream.consumeString());
			tokens.push_back(
				Token::from_string(loc, res.str_literals.size() - 1));
			break;
		default:
			if (std::isdigit(c)) {
				// TODO: handle hex, oct, and binary numbers, as well as floats
				uint64_t val = c - '0';
				bool overflow = false;
				char x = stream.peek();
				while (std::isdigit(x)) {
					if (val > std::numeric_limits<uint64_t>::max() / 10) {
						overflow = true;
						break;
					}
					val = val * 10 + (x - '0');
					stream.consume();
					if (stream.eof()) {
						break;
					}
					x = stream.peek();
				}
				if (overflow) {
					return TokenizeResult::from_error(
						loc, "Integer literal overflow");
				} else {
					tokens.push_back(Token::from_integer(loc, val));
				}
			} else if (std::isalpha(c) || c == '_') {
				std::string_view id = stream.consumeIdentifier();
				TokenType keyword = match_keyword(id);
				if (keyword != TK("(identifier)")) {
					tokens.emplace_back(keyword, loc);
				} else {
					tokens.push_back(Token::from_identifier(
						loc, name_table.get_id_or_insert(id)));
				}
			} else {
				return TokenizeResult::from_error(loc, "Unexpected character");
			}
		}
		stream.skip_whitespace();
	}

	tokens.emplace_back(TK("(eof)"), stream.location());
	return res;
}

} // namespace cypheri
