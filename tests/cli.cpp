#include "cypheri/token.hpp"
#include <string>
#include <iostream>

int main() {
	std::string source, line;
	// Read until EOF
	while (std::getline(std::cin, line)) {
		source += line + "\n";
	}

	// Tokenize
	auto tokens = cypheri::tokenize(source);

	bool error = false;
	for (auto& token : tokens) {
		if (token.type == cypheri::TK("(error)")) {
			std::cerr << "Syntax error at " << token.loc.line << ":" << token.loc.column << ": " << std::get<std::string>(token.value) << std::endl;
			error = true;
		}
	}

	if (!error) {
		for (auto& token : tokens) {
			std::cout << token.loc.line << ":" << token.loc.column << " " 
				<< cypheri::TOKEN_TYPE_NAMES[token.type];
			if (std::holds_alternative<std::string>(token.value)) {
				std::cout << "(" << std::get<std::string>(token.value) << ")";
			} else if (std::holds_alternative<uint64_t>(token.value)) {
				std::cout << "(" << std::get<uint64_t>(token.value) << ")";
			}
			std::cout << std::endl;
		}
	}

	return 0;
}
