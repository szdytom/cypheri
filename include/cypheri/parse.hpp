#ifndef CYPHERI_PARSE_HPP
#define CYPHERI_PARSE_HPP

#include "cypheri/bytecode.hpp"
#include "cypheri/errors.hpp"
#include "cypheri/token.hpp"
#include <string>
#include <variant>
#include <vector>

namespace cypheri {

std::variant<BytecodeModule, SyntaxError> parse(TokenizeResult &&tk_res,
												NameTable &name_table) noexcept;

} // namespace cypheri

#endif // CYPHERI_PARSE_HPP
