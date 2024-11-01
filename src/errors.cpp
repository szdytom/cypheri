#include "cypheri/errors.hpp"

namespace cypheri {

SyntaxError::SyntaxError(const std::string &message, SourceLocation location) noexcept : message(message), location(location) {}

} // namespace cypheri
