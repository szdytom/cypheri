#include "cypheri/parse.hpp"

#include <array>
#include <cassert>
#include <format>
#include <limits>
#include <memory>
#include <stack>

namespace cypheri {

namespace {

class ScopedLocalNameTable {
public:
	static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

	size_t get(NameIdType name) const noexcept {
		auto it = local_names.find(name);
		if (it == local_names.end()) {
			return INVALID_ID;
		}

		return it->second.top();
	}

	size_t add(NameIdType name) noexcept {
		auto &stk = local_names[name];
		stk.push(next_id);
		return next_id++;
	}

	void enter_scope() noexcept {
		scopes.emplace();
	}

	void leave_scope() noexcept {
		for (auto name : scopes.top()) {
			auto &stk = local_names[name];
			stk.pop();
			if (stk.empty()) {
				local_names.erase(name);
			}
		}
		scopes.pop();
	}

	size_t size() const noexcept {
		return next_id;
	}

private:
	size_t next_id = 0;
	std::stack<std::vector<NameIdType>> scopes;
	SpraseNameArray<std::stack<size_t>> local_names;
};

enum class LvalaueType {
	NONE,	  // Not a lvalue
	SIMPLE,	  // Simple lvalue
	COMPOUND, // Compound lvalue
};

struct ExprTreeNode {
	virtual ~ExprTreeNode() = default;
	virtual void emit(BytecodeFunction &func) const = 0;
	virtual LvalaueType lvalue_type() const noexcept {
		return LvalaueType::NONE;
	}

	virtual void emit_store(BytecodeFunction &func) const {}
};

using ExprTreeNodePtr = std::unique_ptr<ExprTreeNode>;

struct ExprTreeSimpleLeaf : ExprTreeNode {
	ExprTreeSimpleLeaf(InstructionType type) : type(type) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(type);
	}

	InstructionType type;
};

struct ExprTreeLitInt : ExprTreeNode {
	ExprTreeLitInt(uint64_t val) : val(val) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::LII, val);
	}

	uint64_t val;
};

struct ExprTreeLitNum : ExprTreeNode {
	ExprTreeLitNum(double val) : val(val) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::LIN, val);
	}

	double val;
};

struct ExprTreeLitStr : ExprTreeNode {
	ExprTreeLitStr(size_t str_id) : str_id(str_id) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::LISTR, str_id);
	}

	size_t str_id;
};

struct ExprTreeLitBool : ExprTreeNode {
	ExprTreeLitBool(bool val) : val(val) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::LIBOOL, val);
	}

	bool val;
};

struct ExprTreeLocal : ExprTreeNode {
	ExprTreeLocal(size_t local_id) : local_id(local_id) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::LDLOCAL, local_id);
	}

	LvalaueType lvalue_type() const noexcept override {
		return LvalaueType::SIMPLE;
	}

	void emit_store(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::STLOCAL, local_id);
	}

	size_t local_id;
};

struct ExprTreeGlobal : ExprTreeNode {
	ExprTreeGlobal(NameIdType name) : name(name) {}

	void emit(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::LDGLOBAL, name);
	}

	LvalaueType lvalue_type() const noexcept override {
		return LvalaueType::SIMPLE;
	}

	void emit_store(BytecodeFunction &func) const override {
		func.instructions.emplace_back(InstructionType::STGLOBAL, name);
	}

	NameIdType name;
};

struct ExprTreeUnOp : ExprTreeNode {
	ExprTreeUnOp(ExprTreeNodePtr expr, InstructionType type)
		: expr(std::move(expr)), type(type) {}

	void emit(BytecodeFunction &func) const override {
		expr->emit(func);
		func.instructions.emplace_back(type);
	}

	ExprTreeNodePtr expr;
	InstructionType type;
};

// Not for assignment, always left associative
struct ExprTreeBinOp : ExprTreeNode {
	ExprTreeBinOp(ExprTreeNodePtr lhs, ExprTreeNodePtr rhs,
				  InstructionType type)
		: lhs(std::move(lhs)), rhs(std::move(rhs)), type(type) {}

	void emit(BytecodeFunction &func) const override {
		lhs->emit(func);
		rhs->emit(func);
		func.instructions.emplace_back(type);
	}

	ExprTreeNodePtr lhs, rhs;
	InstructionType type;
};

struct ExprTreeCall : ExprTreeNode {
	ExprTreeCall(ExprTreeNodePtr func, std::vector<ExprTreeNodePtr> &&args)
		: func(std::move(func)), args(std::move(args)) {}

	void emit(BytecodeFunction &bcfunc) const override {
		for (auto &arg : args) {
			arg->emit(bcfunc);
		}
		func->emit(bcfunc);
		bcfunc.instructions.emplace_back(InstructionType::CALL, args.size());
	}

	ExprTreeNodePtr func;
	std::vector<ExprTreeNodePtr> args;
};

consteval std::array<int, TOKEN_COUNT> make_op_precedence_table() noexcept {
	std::array<int, TOKEN_COUNT> tb;
	tb.fill(-1);

	// larger numbers have higher precedence
	// TODO: check this table

	// logical operators
	tb[TK("||")] = 40;
	tb[TK("&&")] = 40;

	// bitwise operators
	tb[TK("|")] = 50;
	tb[TK("^")] = 51;
	tb[(TK("&"))] = 52;

	// comparison operators
	tb[TK("==")] = tb[TK("!=")] = 60;
	tb[TK("<")] = tb[TK(">")] = tb[TK("<=")] = tb[TK(">=")] = 65;

	// shift operators
	tb[TK("<<")] = tb[TK(">>")] = 70;

	// arithmetic operators
	tb[TK("+")] = tb[TK("-")] = 80;
	tb[TK("*")] = tb[TK("/")] = tb[TK("//")] = tb[TK("%")] = 90;
	tb[TK("**")] = 95;

	// for dynamic indexing and function calls
	// e.g. a.b.c[0](1, 2, 3)[5]
	tb[TK("[")] = tb[TK("(")] = 100;

	// member access
	tb[TK(".")] = 110;

	return tb;
}

// Binary operators preceedence table
// -1 means not a binary operator
// Larger numbers have higher precedence, which should be evaluated first
constexpr std::array<int, TOKEN_COUNT> OP_PRECEDENCE_TABLE =
	make_op_precedence_table();

consteval std::array<InstructionType, TOKEN_COUNT>
make_op_to_instr_table() noexcept {
	std::array<InstructionType, TOKEN_COUNT> tb;
	tb.fill(InstructionType::INVALID);

	tb[TK("+")] = tb[TK("+=")] = InstructionType::ADD;
	tb[TK("-")] = tb[TK("-=")] = InstructionType::SUB;
	tb[TK("*")] = tb[TK("*=")] = InstructionType::MUL;
	tb[TK("/")] = tb[TK("/=")] = InstructionType::DIV;
	tb[TK("//")] = tb[TK("//=")] = InstructionType::IDIV;
	tb[TK("%")] = tb[TK("%=")] = InstructionType::MOD;
	tb[TK("**")] = tb[TK("**=")] = InstructionType::POW;
	tb[TK("<<")] = tb[TK("<<=")] = InstructionType::SHL;
	tb[TK(">>")] = tb[TK(">>=")] = InstructionType::SHR;
	tb[TK("&")] = tb[TK("&=")] = InstructionType::BAND;
	tb[TK("|")] = tb[TK("|=")] = InstructionType::BOR;
	tb[TK("^")] = tb[TK("^=")] = InstructionType::BXOR;
	tb[TK("~")] = InstructionType::BNOT;
	tb[TK("==")] = InstructionType::EQ;
	tb[TK("!=")] = InstructionType::NE;
	tb[TK("<")] = InstructionType::LT;
	tb[TK(">")] = InstructionType::GT;
	tb[TK("<=")] = InstructionType::LE;
	tb[TK(">=")] = InstructionType::GE;
	tb[TK("&&")] = InstructionType::AND;
	tb[TK("||")] = InstructionType::OR;
	tb[TK("!")] = InstructionType::NOT;
	return tb;
}

// Operator to instruction table
constexpr std::array<InstructionType, TOKEN_COUNT> OP_TO_INSTR_TABLE =
	make_op_to_instr_table();

consteval std::array<bool, TOKEN_COUNT> make_op_is_assignment_table() noexcept {
	std::array<bool, TOKEN_COUNT> tb;
	for (int i = 0; i < TOKEN_COUNT; i++) {
		tb[i] = false;
	}

	tb[TK("=")] = tb[TK("+=")] = tb[TK("-=")] = tb[TK("*=")] = tb[TK("/=")] =
		tb[TK("//=")] = tb[TK("%=")] = tb[TK("**=")] = tb[TK("<<=")] =
			tb[TK(">>=")] = tb[TK("&=")] = tb[TK("|=")] = tb[TK("^=")] = true;

	return tb;
}

// Operator is assignment table
constexpr std::array<bool, TOKEN_COUNT> OP_IS_ASSIGNMENT_TABLE =
	make_op_is_assignment_table();

class Parser {
public:
	Parser(TokenizeResult &&tk_res, NameTable *nt) noexcept;

	bool has_error() const noexcept;
	std::optional<SyntaxError> consume_error() noexcept;

	std::optional<BytecodeModule> parse() noexcept;

private:
	std::vector<Token> tokens;
	size_t pos = 0;
	std::optional<SyntaxError> error;
	std::vector<std::string> str_lits;
	NameTable *name_table;
	ScopedLocalNameTable local_names;

	bool eof() const noexcept;
	const Token &peek(size_t offset = 0) const noexcept;
	Token &consume() noexcept;
	bool match(TokenType type) noexcept;
	Token &expect(TokenType type) noexcept;

	std::optional<BytecodeFunction> parse_function() noexcept;
	bool parse_block(BytecodeFunction &func, bool if_block = false) noexcept;
	bool parse_statement(BytecodeFunction &func) noexcept;
	bool parse_declare(BytecodeFunction &func) noexcept;

	bool parse_if_else(BytecodeFunction &func) noexcept;
	bool parse_if_cond(BytecodeFunction &func, std::vector<size_t> &then_jmps,
					   std::vector<size_t> &else_jmps) noexcept;

	bool parse_assign(BytecodeFunction &func) noexcept;
	bool parse_expr(BytecodeFunction &func, int precedence = 0) noexcept;

	ExprTreeNodePtr parse_expr_et(int precedence = 0) noexcept;
	ExprTreeNodePtr parse_expr_bin(int precedence = 0) noexcept;
	ExprTreeNodePtr parse_expr_un() noexcept;
	ExprTreeNodePtr parse_expr_primary() noexcept;
	bool parse_value_list(std::vector<ExprTreeNodePtr> &values,
						  TokenType term = TK(")")) noexcept;
};

Parser::Parser(TokenizeResult &&tk_res, NameTable *nt) noexcept
	: tokens(std::move(tk_res.tokens)), error(std::move(tk_res.error)),
	  str_lits(tk_res.str_literals), name_table(nt) {}

bool Parser::eof() const noexcept {
	return tokens[pos].type == TK("(eof)");
}

const Token &Parser::peek(size_t offset) const noexcept {
	return tokens[pos + offset];
}

Token &Parser::consume() noexcept {
	return eof() ? tokens[pos] : tokens[pos++];
}

bool Parser::match(TokenType type) noexcept {
	if (peek().type == type) {
		consume();
		return true;
	}
	return false;
}

Token &Parser::expect(TokenType type) noexcept {
	auto &token = consume();
	if (token.type != type) {
		error.emplace(std::format("expected {}, got {}", TOKEN_TYPE_NAMES[type],
								  TOKEN_TYPE_NAMES[token.type]),
					  token.loc);
	}
	return token;
}

bool Parser::has_error() const noexcept {
	return error.has_value();
}

std::optional<SyntaxError> Parser::consume_error() noexcept {
	if (error.has_value()) {
		return std::move(error);
	}
	return std::nullopt;
}

std::optional<BytecodeModule> Parser::parse() noexcept {
	if (has_error()) {
		return std::nullopt;
	}

	BytecodeModule mod;

	while (!eof()) {
		const auto &tk = peek();
		switch (tk.type) {
		case TK("Function"):
			if (auto func = parse_function()) {
				// copy name, or it will be use-after-move
				auto name = func->name;
				mod.functions[name] = std::move(*func);
			} else {
				return std::nullopt;
			}
			break;
		case TK("Declare"):
			// TODO: parse global variable declarations
			error.emplace("global variable declarations not implemented yet",
						  tk.loc);
			return std::nullopt;
		case TK("Import"):
			// TODO: parse imports
			error.emplace("imports not implemented yet", tk.loc);
			return std::nullopt;
		default:
			error.emplace(
				std::format("{} can not appear at the top-level of a module",
							TOKEN_TYPE_NAMES[tk.type]),
				tk.loc);
			return std::nullopt;
		}
	}

	mod.str_lits = std::move(str_lits);
	return mod;
}

std::optional<BytecodeFunction> Parser::parse_function() noexcept {
	if (has_error()) {
		return std::nullopt;
	}
	local_names = ScopedLocalNameTable();

	BytecodeFunction func;
	expect(TK("Function"));
	if (has_error()) {
		return std::nullopt;
	}

	func.name = expect(TK("(identifier)")).id;
	if (has_error()) {
		return std::nullopt;
	}

	expect(TK("("));
	if (has_error()) {
		return std::nullopt;
	}

	if (!match(TK(")"))) {
		while (true) {
			auto &token = expect(TK("(identifier)"));
			if (has_error()) {
				return std::nullopt;
			}

			auto id = token.id;
			if (local_names.get(id) != ScopedLocalNameTable::INVALID_ID) {
				error.emplace(std::format("duplicate local name {}",
										  name_table->get_name(id)),
							  token.loc);
				return std::nullopt;
			}
			local_names.add(id);
			func.arg_count++;
			func.local_count++;

			if (!match(TK(")"))) {
				expect(TK(","));
				if (has_error()) {
					return std::nullopt;
				}
			} else {
				break;
			}
		}
	}

	if (!parse_block(func)) {
		return std::nullopt;
	}

	return func;
}

bool Parser::parse_block(BytecodeFunction &func, bool if_block) noexcept {
	local_names.enter_scope();
	while (true) {
		if (eof()) {
			error.emplace("unexpected end of file", peek().loc);
			return false;
		}

		// if we're in an if block, we need to break out when we see an Else,
		// ElseIf or End, we are not consuming the tokens here, parse_if_else
		// will need to identify what kind of block come next.
		if (if_block &&
			(peek().type == TK("Else") || peek().type == TK("ElseIf") ||
			 peek().type == TK("End"))) {
			break;
		} else if (match(TK("End"))) {
			break;
		}

		if (!parse_statement(func)) {
			return false;
		}
	}

	local_names.leave_scope();
	return true;
}

bool Parser::parse_statement(BytecodeFunction &func) noexcept {
	switch (peek().type) {
	case TK("Declare"):
		return parse_declare(func);
	case TK("If"):
		return parse_if_else(func);
	case TK("Return"):
		consume();
		if (peek().type == TK(";")) {
			// return NULL
			func.instructions.emplace_back(InstructionType::RETNULL);
		} else {
			if (!parse_expr(func)) {
				return false;
			}
			func.instructions.emplace_back(InstructionType::RET);
		}
		expect(TK(";"));
		return !has_error();
	default:
		return parse_assign(func);
	}
}

bool Parser::parse_assign(BytecodeFunction &func) noexcept {
	auto lhs = parse_expr_et();
	if (!lhs) {
		return false;
	}

	if (match(TK(";"))) {
		// this is not an assignment, just an expression
		lhs->emit(func);
		func.instructions.emplace_back(InstructionType::POPN, 1);
		return true;
	}

	if (!OP_IS_ASSIGNMENT_TABLE[peek().type]) {
		error.emplace("unexpected token", peek().loc);
		return false;
	}

	auto &tk = consume();
	auto type = lhs->lvalue_type();
	if (type == LvalaueType::NONE) {
		error.emplace("cannot assign to rvalue", tk.loc);
		return false;
	}

	if (!parse_expr(func)) {
		return false;
	}

	if (type == LvalaueType::SIMPLE) {
		if (tk.type == TK("=")) {
			lhs->emit_store(func);
		} else {
			lhs->emit(func);
			func.instructions.emplace_back(InstructionType::SWP);
			func.instructions.emplace_back(OP_TO_INSTR_TABLE[tk.type]);
			lhs->emit_store(func);
		}
	} else {
		// TODO: implement this
		error.emplace("TDOD: assign to member", tk.loc);
		return false;
	}

	expect(TK(";"));
	if (has_error()) {
		return false;
	}
	return true;
}

bool Parser::parse_declare(BytecodeFunction &func) noexcept {
	expect(TK("Declare")); // already checked, but just in case
	if (has_error()) {
		return false;
	}

	while (true) {
		auto &token = expect(TK("(identifier)"));
		if (has_error()) {
			return false;
		}

		auto id = token.id;
		if (local_names.get(id) != ScopedLocalNameTable::INVALID_ID) {
			error.emplace(std::format("variable {} already declared",
									  name_table->get_name(id)),
						  token.loc);
			return false;
		}

		local_names.add(id);
		func.local_count++;

		if (match(TK("="))) {
			if (!parse_expr(func)) {
				return false;
			}
			func.instructions.emplace_back(InstructionType::LDLOCAL,
										   func.local_count - 1);
		}

		if (!match(TK(";"))) {
			expect(TK(","));
			if (has_error()) {
				return false;
			}
		} else {
			break;
		}
	}
	return true;
}

bool Parser::parse_if_else(BytecodeFunction &func) noexcept {
	expect(TK("If")); // already checked, but just in case
	if (has_error()) {
		return false;
	}

	std::vector<size_t> then_jumps, else_jumps;
	if (!parse_if_cond(func, then_jumps, else_jumps)) {
		return false;
	}
	expect(TK("Then"));
	if (has_error()) {
		return false;
	}

	for (auto jump : then_jumps) {
		func.instructions[jump].idx() = func.instructions.size();
	}

	if (!parse_block(func, true)) {
		return false;
	}

	std::vector<size_t> end_jumps;
	if (peek().type == TK("ElseIf") || peek().type == TK("Else")) {
		func.instructions.emplace_back(InstructionType::JMP);
		end_jumps.push_back(func.instructions.size() - 1);
	}
	for (auto jump : else_jumps) {
		func.instructions[jump].idx() = func.instructions.size();
	}

	while (match(TK("ElseIf"))) {
		std::vector<size_t> ei_then_jumps, ei_else_jumps;
		if (!parse_if_cond(func, ei_then_jumps, ei_else_jumps)) {
			return false;
		}
		expect(TK("Then"));
		if (has_error()) {
			return false;
		}

		for (auto jump : ei_then_jumps) {
			func.instructions[jump].idx() = func.instructions.size();
		}

		if (!parse_block(func, true)) {
			return false;
		}
		if (peek().type == TK("ElseIf") || peek().type == TK("Else")) {
			func.instructions.emplace_back(InstructionType::JMP);
			end_jumps.push_back(func.instructions.size() - 1);
		}

		for (auto jump : ei_else_jumps) {
			func.instructions[jump].idx() = func.instructions.size();
		}
	}

	if (match(TK("Else"))) {
		// It's end here, so don't accept Else orElseIf as terminator
		if (!parse_block(func)) {
			return false;
		}
		// End already consumed in parse_block
	} else {
		expect(TK("End"));
		if (has_error()) {
			return false;
		}
	}

	for (auto jump : end_jumps) {
		func.instructions[jump].idx() = func.instructions.size();
	}
	return true;
}

bool Parser::parse_if_cond(BytecodeFunction &func,
						   std::vector<size_t> &then_jmps,
						   std::vector<size_t> &else_jmps) noexcept {
	do {
		// parse expr without || and &&
		if (!parse_expr(func, OP_PRECEDENCE_TABLE[TK("||")] + 1)) {
			return false;
		}

		if (match(TK("||"))) {
			func.instructions.emplace_back(InstructionType::JNZ);
			then_jmps.push_back(func.instructions.size() - 1);
		} else if (match(TK("&&"))) {
			func.instructions.emplace_back(InstructionType::JZ);
			else_jmps.push_back(func.instructions.size() - 1);
		}
	} while (peek().type != TK("Then"));

	// last branch
	func.instructions.emplace_back(InstructionType::JZ);
	else_jmps.push_back(func.instructions.size() - 1);

	return true;
}

bool Parser::parse_expr(BytecodeFunction &func, int precedence) noexcept {
	ExprTreeNodePtr expr = parse_expr_et(precedence);
	if (!expr) {
		return false;
	}
	expr->emit(func);
	return true;
}

ExprTreeNodePtr Parser::parse_expr_et(int precedence) noexcept {
	return parse_expr_bin(precedence); // For now, only binary operators
}

ExprTreeNodePtr Parser::parse_expr_bin(int precedence) noexcept {
	auto left = parse_expr_un();
	if (!left) {
		return nullptr;
	}

	while (OP_PRECEDENCE_TABLE[peek().type] >= precedence) {
		auto &op = consume();
		if (op.type == TK("(")) {
			// function call
			std::vector<ExprTreeNodePtr> args;
			if (!parse_value_list(args)) {
				return nullptr;
			}

			// closing parenthesis already consumed by parse_value_list
			left = std::make_unique<ExprTreeCall>(std::move(left),
												  std::move(args));
		} else {
			auto right = parse_expr_bin(OP_PRECEDENCE_TABLE[op.type]);
			if (!right) {
				return nullptr;
			}
			left = std::make_unique<ExprTreeBinOp>(
				std::move(left), std::move(right), OP_TO_INSTR_TABLE[op.type]);
		}
	}
	return left;
}

ExprTreeNodePtr Parser::parse_expr_un() noexcept {
	switch (peek().type) {
	case TK("-"):
		return std::make_unique<ExprTreeUnOp>(parse_expr_un(),
											  InstructionType::NEG);
	case TK("!"):
	case TK("~"):
		return std::make_unique<ExprTreeUnOp>(parse_expr_un(),
											  OP_TO_INSTR_TABLE[peek().type]);
	default:
		return parse_expr_primary();
	}
}

bool Parser::parse_value_list(std::vector<ExprTreeNodePtr> &values,
							  TokenType term) noexcept {
	while (!match(term)) {
		auto arg = parse_expr_et();
		if (!arg) {
			return false;
		}
		values.push_back(std::move(arg));

		// we accept trailing commas
		if (peek().type != term) {
			expect(TK(","));
			if (has_error()) {
				return false;
			}
		}
	}
	return true;
}

ExprTreeNodePtr Parser::parse_expr_primary() noexcept {
	switch (peek().type) {
	case TK("("): {
		// ( expr )
		consume();
		auto expr = parse_expr_et(0);
		if (!expr) {
			return nullptr;
		}
		expect(TK(")"));
		if (has_error()) {
			return nullptr;
		}
		return expr;
	}
	case TK("(identifier)"): {
		auto &tk = consume();
		auto local_id = local_names.get(tk.id);
		if (local_id == ScopedLocalNameTable::INVALID_ID) {
			return std::make_unique<ExprTreeGlobal>(tk.id);
		}
		return std::make_unique<ExprTreeLocal>(local_id);
	}
	case TK("TRUE"):
		consume();
		return std::make_unique<ExprTreeLitBool>(true);
	case TK("FALSE"):
		consume();
		return std::make_unique<ExprTreeLitBool>(false);
	case TK("NULL"):
		consume();
		return std::make_unique<ExprTreeSimpleLeaf>(InstructionType::LINULL);
	case TK("(integer)"):
		return std::make_unique<ExprTreeLitInt>(consume().integer);
	case TK("(number)"):
		return std::make_unique<ExprTreeLitNum>(consume().num);
	case TK("(string)"):
		return std::make_unique<ExprTreeLitStr>(consume().str_idx);
	default:
		break;
	}
	error.emplace("primary expression expected", peek().loc);
	return nullptr;
}

} // namespace

std::variant<BytecodeModule, SyntaxError>
parse(TokenizeResult &&tk_res, NameTable &name_table) noexcept {
	Parser parser(std::move(tk_res), &name_table);
	if (auto m = parser.parse()) {
		return std::move(*m);
	} else {
		return std::move(*parser.consume_error());
	}
}

} // namespace cypheri
