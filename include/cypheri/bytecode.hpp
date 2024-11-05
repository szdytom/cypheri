#ifndef CYPHERI_IR_HPP
#define CYPHERI_IR_HPP

#include "cypheri/nametable.hpp"
#include <array>
#include <cstdint>
#include <format>
#include <vector>

namespace cypheri {

enum class InstructionType : uint8_t {
	// Miscellaneous Instructions
	NOP,
	INVALID,

	// Arithmetic Instructions
	ADD,  // Add
	SUB,  // Subtract
	MUL,  // Multiply
	DIV,  // Divide
	MOD,  // Modulo
	POW,  // Power
	IDIV, // Integer Divide
	NEG,  // Negate
	BXOR, // Bitwise XOR
	BAND, // Bitwise AND
	BOR,  // Bitwise OR
	BNOT, // Bitwise NOT
	SHL,  // Shift Left
	SHR,  // Shift Right
	EQ,	  // Equal
	NE,	  // Not Equal
	LT,	  // Less Than
	LE,	  // Less Than or Equal
	GT,	  // Greater Than
	GE,	  // Greater Than or Equal
	AND,  // Logical AND
	OR,	  // Logical OR
	NOT,  // Logical NOT

	// Stack Instructions
	LII,	  // Load Immediate Integer
	LIN,	  // Load Immediate Number
	LINULL,	  // Load Immediate Null
	LIBOOL,	  // Load Immediate Boolean
	LISTR,	  // Load Immediate String
	LIARR,	  // Load Immediate Array
	LIOBJ,	  // Load Immediate Object
	LILAMBDA, // Load Immediate Lambda
	LDGLOBAL, // Load Global
	LDLOCAL,  // Load Local
	STGLOBAL, // Store Global
	STLOCAL,  // Store Local
	POPN,	  // Pop
	SWP,	  // Swap Top Two Elements
	ROT3,	  // Rotate Top Three Elements: a b c -> c a b
	DUP,	  // Duplicate Top Element

	// Object Instructions
	GET,	// Get Property
	SET,	// Set Property
	GETDNY, // Get Dynamic Property
	SETDNY, // Set Dynamic Property
	NEWOBJ, // New Object

	// Control Flow Instructions
	JMP,	 // Jump
	JZ,		 // Jump If Zero
	JNZ,	 // Jump If Not Zero
	CALL,	 // Call Function
	RET,	 // Return
	RETNULL, // Return Null
	YIELD,	 // Yield Coroutine

	// Guaranteed to be last
	INSTRUCTION_COUNT,
};

constexpr int INSTRUCTION_COUNT =
	static_cast<int>(InstructionType::INSTRUCTION_COUNT);

consteval std::array<const char *, INSTRUCTION_COUNT>
make_instruction_names_table() {
	std::array<const char *, INSTRUCTION_COUNT> names;

	// Don't use names.fill here, so that compiler will report uninitialized
	// array if we forget something, hopefully.

#define CYPHERI_MAKE_INSTRUCTION_NAME(type)                                    \
	names[static_cast<int>(InstructionType::type)] = #type

	CYPHERI_MAKE_INSTRUCTION_NAME(NOP);
	CYPHERI_MAKE_INSTRUCTION_NAME(INVALID);
	CYPHERI_MAKE_INSTRUCTION_NAME(ADD);
	CYPHERI_MAKE_INSTRUCTION_NAME(SUB);
	CYPHERI_MAKE_INSTRUCTION_NAME(MUL);
	CYPHERI_MAKE_INSTRUCTION_NAME(DIV);
	CYPHERI_MAKE_INSTRUCTION_NAME(MOD);
	CYPHERI_MAKE_INSTRUCTION_NAME(POW);
	CYPHERI_MAKE_INSTRUCTION_NAME(IDIV);
	CYPHERI_MAKE_INSTRUCTION_NAME(NEG);
	CYPHERI_MAKE_INSTRUCTION_NAME(BXOR);
	CYPHERI_MAKE_INSTRUCTION_NAME(BAND);
	CYPHERI_MAKE_INSTRUCTION_NAME(BOR);
	CYPHERI_MAKE_INSTRUCTION_NAME(BNOT);
	CYPHERI_MAKE_INSTRUCTION_NAME(SHL);
	CYPHERI_MAKE_INSTRUCTION_NAME(SHR);
	CYPHERI_MAKE_INSTRUCTION_NAME(EQ);
	CYPHERI_MAKE_INSTRUCTION_NAME(NE);
	CYPHERI_MAKE_INSTRUCTION_NAME(LT);
	CYPHERI_MAKE_INSTRUCTION_NAME(LE);
	CYPHERI_MAKE_INSTRUCTION_NAME(GT);
	CYPHERI_MAKE_INSTRUCTION_NAME(GE);
	CYPHERI_MAKE_INSTRUCTION_NAME(AND);
	CYPHERI_MAKE_INSTRUCTION_NAME(OR);
	CYPHERI_MAKE_INSTRUCTION_NAME(NOT);
	CYPHERI_MAKE_INSTRUCTION_NAME(LII);
	CYPHERI_MAKE_INSTRUCTION_NAME(LIN);
	CYPHERI_MAKE_INSTRUCTION_NAME(LINULL);
	CYPHERI_MAKE_INSTRUCTION_NAME(LIBOOL);
	CYPHERI_MAKE_INSTRUCTION_NAME(LISTR);
	CYPHERI_MAKE_INSTRUCTION_NAME(LIARR);
	CYPHERI_MAKE_INSTRUCTION_NAME(LIOBJ);
	CYPHERI_MAKE_INSTRUCTION_NAME(LILAMBDA);
	CYPHERI_MAKE_INSTRUCTION_NAME(LDGLOBAL);
	CYPHERI_MAKE_INSTRUCTION_NAME(LDLOCAL);
	CYPHERI_MAKE_INSTRUCTION_NAME(STGLOBAL);
	CYPHERI_MAKE_INSTRUCTION_NAME(STLOCAL);
	CYPHERI_MAKE_INSTRUCTION_NAME(POPN);
	CYPHERI_MAKE_INSTRUCTION_NAME(SWP);
	CYPHERI_MAKE_INSTRUCTION_NAME(ROT3);
	CYPHERI_MAKE_INSTRUCTION_NAME(DUP);
	CYPHERI_MAKE_INSTRUCTION_NAME(GET);
	CYPHERI_MAKE_INSTRUCTION_NAME(SET);
	CYPHERI_MAKE_INSTRUCTION_NAME(GETDNY);
	CYPHERI_MAKE_INSTRUCTION_NAME(SETDNY);
	CYPHERI_MAKE_INSTRUCTION_NAME(NEWOBJ);
	CYPHERI_MAKE_INSTRUCTION_NAME(JMP);
	CYPHERI_MAKE_INSTRUCTION_NAME(JZ);
	CYPHERI_MAKE_INSTRUCTION_NAME(JNZ);
	CYPHERI_MAKE_INSTRUCTION_NAME(CALL);
	CYPHERI_MAKE_INSTRUCTION_NAME(RET);
	CYPHERI_MAKE_INSTRUCTION_NAME(RETNULL);
	CYPHERI_MAKE_INSTRUCTION_NAME(YIELD);
#undef CYPHERI_MAKE_INSTRUCTION_NAME

	return names;
};

constexpr std::array<const char *, INSTRUCTION_COUNT> INSTRUTION_NAMES =
	make_instruction_names_table();

class BytecodeInstruction {
public:
	BytecodeInstruction(InstructionType type, int n = 0) noexcept;
	BytecodeInstruction(InstructionType type, uint64_t i_lit) noexcept;
	BytecodeInstruction(InstructionType type, NameIdType name) noexcept;
	BytecodeInstruction(InstructionType type, double f_lit) noexcept;
	BytecodeInstruction(InstructionType type, bool b) noexcept;

	InstructionType type;
	union {
		int n;
		uint64_t i_lit;
		double f_lit;
	};

	size_t idx() const noexcept;
	size_t &idx() noexcept;
};

class BytecodeFunction {
public:
	NameIdType name;
	size_t local_count = 0, arg_count = 0;
	std::vector<BytecodeInstruction> instructions;
};

class BytecodeModule {
public:
	SpraseNameArray<BytecodeFunction> functions;
	std::vector<std::string> str_lits;

	// global variable names, function/class not included
	std::vector<NameIdType> global_names;
};

} // namespace cypheri

template <> struct std::formatter<cypheri::InstructionType> {
	bool display_name = true;

	template <typename ParseContext> constexpr auto parse(ParseContext &ctx) {
		auto it = ctx.begin();
		if (it == ctx.end()) {
			return it;
		}

		if (*it == 'n') {
			display_name = false;
			++it;
		}

		if (it != ctx.end() && *it != '}') {
			throw std::format_error("invalid format string");
		}

		return it;
	}

	template <typename FormatContext>
	auto format(cypheri::InstructionType type, FormatContext &ctx) const {
		if (display_name) {
			return std::format_to(
				ctx.out(), "{}",
				cypheri::INSTRUTION_NAMES[static_cast<size_t>(type)]);
		} else {
			return std::format_to(ctx.out(), "{}", static_cast<size_t>(type));
		}
	}
};

#endif // CYPHERI_IR_HPP
