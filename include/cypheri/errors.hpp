#ifndef CYPHERI_ERRORS_HPP
#define CYPHERI_ERRORS_HPP

#include <string>
#include <format>

namespace cypheri {

struct SourceLocation {
	uint32_t line;
	uint32_t column;
};

class SyntaxError {
public:
    SyntaxError(const std::string& message, SourceLocation location) noexcept;

	std::string message;
	SourceLocation location;
};

} // namespace cypheri

template <>
struct std::formatter<cypheri::SourceLocation> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const cypheri::SourceLocation& loc, FormatContext& ctx) const {
	    return format_to(ctx.out(), "{}:{}", loc.line, loc.column);
	}
};

template <>
struct std::formatter<cypheri::SyntaxError> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const cypheri::SyntaxError& err, FormatContext& ctx) const {
	    return format_to(ctx.out(), "{}: Syntax error: {}.", err.location, err.message);
	}
};

#endif // CYPHERI_ERRORS_HPP