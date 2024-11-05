#include "cypheri/errors.hpp"
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
	auto tk_res = cypheri::tokenize(source, name_table);
	if (tk_res.error.has_value()) {
		std::cout << std::format("Error: \n{}", tk_res.error.value())
				  << std::endl;
		return 0;
	}

	// Print tokens
	for (auto &tk : tk_res.tokens) {
		std::cout << std::format("{}:\t{{ type=\"{}\"", tk.loc,
								 cypheri::TOKEN_TYPE_NAMES[tk.type]);

		switch (tk.type) {
		case cypheri::TK("(integer)"):
			std::cout << ", value=" << tk.integer;
			break;
		case cypheri::TK("(number)"):
			std::cout << ", value=" << tk.num;
			break;
		case cypheri::TK("(string)"):
			std::cout << std::format(", value=\"{}\"",
									 tk_res.str_literals[tk.str_idx]);
			break;
		case cypheri::TK("(identifier)"):
			std::cout << std::format(", value=\"{}\"({})",
									 name_table.get_name(tk.id), tk.id);
			break;
		default:
			break;
		}

		std::cout << " }" << std::endl;
	}

	return 0;
}
