#include "cypheri/errors.hpp"
#include "cypheri/parse.hpp"
#include "cypheri/token.hpp"
#include <format>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
	if (argc >= 2) {
		freopen(argv[1], "r", stdin);
	}

	if (argc >= 3) {
		freopen(argv[2], "w", stdout);
	}

	std::string source, line;
	// Read until EOF
	while (std::getline(std::cin, line)) {
		source += line + "\n";
	}

	// Tokenize
	cypheri::NameTable name_table;

	// Parse
	auto parse_res =
		cypheri::parse(cypheri::tokenize(source, name_table), name_table);

	if (auto err = std::get_if<cypheri::SyntaxError>(&parse_res)) {
		std::cout << std::format("Error: \n{}", *err) << std::endl;
		return 0;
	}

	auto bc = std::get<cypheri::BytecodeModule>(std::move(parse_res));
	for (auto &[name, func] : bc.functions) {
		std::cout << std::format("Function {}(args = {}, locals = {}):",
								 name_table.get_name(name), func.arg_count,
								 func.local_count)
				  << std::endl;

		for (size_t i = 0; i < func.instructions.size(); i++) {
			const auto &inst = func.instructions[i];
			std::cout << std::format("\t+{:0>4d}: {}", i, inst.type);
			switch (inst.type) {
			case cypheri::InstructionType::LII:
				std::cout << std::format("\t{}", inst.i_lit);
				break;
			case cypheri::InstructionType::LIN:
				std::cout << std::format("\t{}", inst.f_lit);
				break;
			case cypheri::InstructionType::LIBOOL:
				std::cout << std::format("\t{}", static_cast<bool>(inst.i_lit));
				break;
			case cypheri::InstructionType::LISTR:
				std::cout << std::format("\t\"{}\"", bc.str_lits[inst.i_lit]);
				break;
			case cypheri::InstructionType::LDLOCAL:
			case cypheri::InstructionType::STLOCAL:
			case cypheri::InstructionType::JMP:
			case cypheri::InstructionType::JZ:
			case cypheri::InstructionType::JNZ:
				std::cout << std::format("\t{}", inst.idx());
				break;
			case cypheri::InstructionType::LDGLOBAL:
			case cypheri::InstructionType::STGLOBAL:
				std::cout << std::format("\t{}",
										 name_table.get_name(inst.idx()));
				break;
			case cypheri::InstructionType::CALL:
			case cypheri::InstructionType::POPN:
				std::cout << std::format("\t{}", inst.n);
				break;
			default:
				break;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	return 0;
}
