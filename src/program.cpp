#include "program.h"

namespace L3::program {
	using namespace std_alias;

	std::pair<Type, int64_t> str_to_type(const std::string& str) {
		static const std::map<std::string, Type> stringToTypeMap = {
			{"int64", Type::Int64},
			{"code", Type::Code},
			{"tuple", Type::Tuple},
			{"void", Type::Void}
		};
		int transitionIndex = -1;
		for (int i = 0; i < str.size(); ++i) {
			if (str[i] == '[') {
				transitionIndex = i;
				break;
			}
		}
		Type sol_type;
		if (transitionIndex == -1) {
			sol_type = stringToTypeMap.find(str)->second;
		} else {
			sol_type = stringToTypeMap.find(str.substr(0, transitionIndex + 1))->second;
		}
		
		int bracketPairs = 0;
		for (int i = transitionIndex; i < str.size(); ++i) {
			if (str[i] == '[' && i + 1 < str.size() && str[i + 1] == ']') {
				++bracketPairs;
				++i; // Skip the next character as it is part of the counted pair
			}
		}
		return std::make_pair(sol_type, bracketPairs);
	}
	std::string to_string(Type t) {
		switch (t) {
			case Type::Int64: return "int64";
			case Type::Code: return "code";
			case Type::Tuple: return "tuple";
			case Type::Void: return "void";
			default: return "unknown";
		}
	}

	Operator str_to_op(std::string str) {
		static const Map<std::string, Operator> map {
			{ "<", Operator::lt },
			{ "<=", Operator::le },
			{ "=", Operator::eq },
			{ ">=", Operator::ge },
			{ ">", Operator::gt },
			{ "+", Operator::plus },
			{ "-", Operator::minus },
			{ "*", Operator::times },
			{ "&", Operator::bitwise_and },
			{ "<<", Operator::lshift },
			{ ">>", Operator::rshift }
		};
		return map.find(str)->second;
	}
	std::string to_string(Operator op) {
		static const std::string map[] = {
			"<", "<=", "=", ">=", ">", "+", "-", "*", "&", "<<", ">>"
		};
		return map[static_cast<int>(op)];
	}
	Opt<Operator> flip_operator(Operator op) {
		switch (op) {
			// operators that are commutative
			case Operator::eq:
			case Operator::plus:
			case Operator::times:
			case Operator::bitwise_and:
				return op;

			// operators that flip
			case Operator::lt: return Operator::gt;
			case Operator::le: return Operator::ge;
			case Operator::gt: return Operator::lt;
			case Operator::ge: return Operator::le;

			// operators that can't be flipped
			default: return {};
		}
	}
	

	void TerminatorBranchOne::bind_to_scope(AggregateScope &agg_scope) {
		this->bb_ref->bind_to_scope(agg_scope);
	}
	std::string TerminatorBranchOne::to_string() {
		return "br" + this->label->to_string();
	}
	
	Uptr<BasicBlock> BasicBlock::Builder::get_result() {
		return Uptr<BasicBlock>( new BasicBlock(
			mv(this->name),
			mv(this->inst),
			mv(this->te)
		));
	}
	void BasicBlock::Builder::add_name(std::string name){
		this->name = mv(name);
	}
	void BasicBlock::Builder::add_instruction(Uptr<Instruction> &&inst) {
		this->inst.push_back(inst.get());
	}
	void BasicBlock::Builder::add_terminator(Uptr<Terminator> &&te) {
		this->te = mv(te);
	}
	std::string BasicBlock::to_string() const {
		std::string sol = ":" + this->name + "\n";
		for (const Uptr<Instruction> &inst : this->inst) {
			sol += inst->to_string();
		}
		sol += this->te->to_string() + "\n";
	}
	void BasicBlock::bind_to_scope(AggregateScope &scope){
		for (auto const& inst: this->inst) {
			inst->bind_to_scope(scope);
		}
		this->te->bind_to_scope(scope);
	}

	Uptr<IRFunction> IRFunction::Builder::get_result() {
		return Uptr<IRFunction>(new IRFunction(
			mv(this->name),
			mv(this->basic_blocks),
			mv(this->vars),
			mv(this->parameter_vars),
			mv(this->agg_scope)
		));
	}
	void IRFunction::Builder::add_name(std::string name) {
		this->name = mv(name);
	}
	void IRFunction::Builder::add_block(Uptr<BasicBlock> &&bb){
		bb->bind_to_scope(this->agg_scope);
		this->agg_scope.label_scope.resolve_item(mv(bb), bb.get());
		this->basic_blocks.push_back(bb.get());
	}
	void IRFunction::Builder::add_parameter(std::string type, std::string var_name){
		auto [t, dim] = str_to_type(type);
		Uptr<Variable> var_ptr = mkuptr<Variable>(var_name, t, dim)
		this->agg_scope.variable_scope.resolve_item(mv(var_name), var_ptr.get());
		this->parameter_vars.push_back(var_ptr.get());
		this->vars.emplace_back(mv(var_ptr));
	}
	std::string IRFunction::to_string() const {
		std::string result = "define @" + this->name + "(";
		for (const Variable *var : this->parameter_vars) {
			result += "%" + var->get_name() + ", ";
		}
		result += ") {\n";
		for (const Uptr<BasicBlock> &block : this->blocks) {
			result += block->to_string();
		}
		result += "}";
		return result;
	}

	std::string ExternalFunction::to_string() const {
		return "[[function std::" + this->name + "]]";
	}

	Program::Builder::Builder(){
		for (Uptr<ExternalFunction> &function_ptr : generate_std_functions()) {
			this->agg_scope.external_function_scope.resolve_item(
				function_ptr->get_name(),
				function_ptr.get()
			);
			this->external_functions.emplace_back(mv(function_ptr));
		}
		this->agg_scope.l3_function_scope.add_ref(*this->main_function_ref);
	}
	void Program::Builder::add_ir_function(Uptr<IRFunction> &&function){
		function.get_scope().set_parent(this->agg_scope);
		this->agg_scope.ir_function_scope.resolve_item(function->get_name(), function.get());
		this->ir_functions.push_back(mv(function))
	}
	Uptr<Program> Program::Builder::get_result(){
		return Uptr<Program>(new Program(
			mv(this->l3_functions),
			mv(this->external_functions),
			mv(this->main_function_ref)
		));
	}
	Vec<Uptr<ExternalFunction>> generate_std_functions() {
		Vec<Uptr<ExternalFunction>> result;
		result.push_back(mkuptr<ExternalFunction>("input", Vec<int> { 0 }));
		result.push_back(mkuptr<ExternalFunction>("print", Vec<int> { 1 }));
		result.push_back(mkuptr<ExternalFunction>("allocate", Vec<int> { 2 }));
		result.push_back(mkuptr<ExternalFunction>("tuple-error", Vec<int> { 3 }));
		result.push_back(mkuptr<ExternalFunction>("tensor-error", Vec<int> { 1, 3, 4 }));
		return result;
	}
	std::string Program::to_string() const {
		std::string result;
		for (const Uptr<L3Function> &function : this->l3_functions) {
			result += function->to_string() + "\n";
		}
		return result;
	}
}