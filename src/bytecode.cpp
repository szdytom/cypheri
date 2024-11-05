#include "cypheri/bytecode.hpp"

namespace cypheri {

BytecodeInstruction::BytecodeInstruction(InstructionType type, int n) noexcept
	: type(type), n(n) {}

BytecodeInstruction::BytecodeInstruction(InstructionType type,
										 uint64_t i_lit) noexcept
	: type(type), i_lit(i_lit) {}

BytecodeInstruction::BytecodeInstruction(InstructionType type,
										 double f_lit) noexcept
	: type(type), f_lit(f_lit) {}

BytecodeInstruction::BytecodeInstruction(InstructionType type, bool b) noexcept
	: type(type), i_lit(b) {}

BytecodeInstruction::BytecodeInstruction(InstructionType type,
										 NameIdType name) noexcept
	: type(type), i_lit(name) {}

size_t BytecodeInstruction::idx() const noexcept {
	return i_lit;
}

size_t &BytecodeInstruction::idx() noexcept {
	return i_lit;
}

} // namespace cypheri
