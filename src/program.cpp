#include "program.h"

namespace IR::program {
	using namespace std_alias;

	std::pair<A_type, int64_t> str_to_a_type(const std::string& str) {
		static const std::map<std::string, A_type> stringToTypeMap = {
			{"int64", A_type::Int64},
			{"code", A_type::Code},
			{"tuple", A_type::Tuple},
			{"void", A_type::Void}
		};
		int transitionIndex = -1;
		for (int i = 0; i < str.size(); ++i) {
			if (str[i] == '[') {
				transitionIndex = i;
				break;
			}
		}
		A_type sol_type;
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
	std::string to_string(A_type t) {
		switch (t) {
			case A_type::Int64: return "int64";
			case A_type::Code: return "code";
			case A_type::Tuple: return "tuple";
			case A_type::Void: return "void";
			default: return "unknown";
		}
	}
	
	Type::Type(const std::string& str){
		auto[a_type, num_dim] = str_to_a_type(str);
		this->a_type = a_type;
		this->num_dim = num_dim;
	}
	std::string Type::to_string() const {
		std::string sol = "";
		switch (this->a_type) {
			case A_type::Int64: sol += "int64";
			case A_type::Code: sol += "code";
			case A_type::Tuple: sol += "tuple";
			case A_type::Void: sol += "void";
			default: 
				std::cerr << "detected unknown type: " << std::endl;
				exit(-1);
		}

		for(int i = 0; i < this->num_dim; i++) {
			sol += "[]";
		}
		return sol;
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
	
	template<> std::string ItemRef<Variable>::to_string() const {
		std::string result = "%" + this->get_ref_name();
		if (!this->referent_nullable) {
			result += "?";
		}
		return result;
	}
	template<> void ItemRef<Variable>::bind_to_scope(AggregateScope &agg_scope){
		agg_scope.variable_scope.add_ref(*this);
	}
	template<> std::string ItemRef<BasicBlock>::to_string() const {
		std::string result = ":" + this->get_ref_name();
		if (!this->referent_nullable) {
			result += "?";
		}
		return result;
	}
	template<> void ItemRef<BasicBlock>::bind_to_scope(AggregateScope &agg_scope){
		agg_scope.basic_block_scope.add_ref(*this);
	}
	template<> std::string ItemRef<IRFunction>::to_string() const {
		std::string result = "@" + this->get_ref_name();
		if (!this->referent_nullable) {
			result += "?";
		}
		return result;
	}
	template<> void ItemRef<IRFunction>::bind_to_scope(AggregateScope &agg_scope){
		agg_scope.ir_function_scope.add_ref(*this);
	}
	template<> std::string ItemRef<ExternalFunction>::to_string() const {
		std::string result = this->get_ref_name();
		if (!this->referent_nullable) {
			result += "?";
		}
		return result;
	}
	template<> void ItemRef<ExternalFunction>::bind_to_scope(AggregateScope &agg_scope){
		agg_scope.external_function_scope.add_ref(*this);
	}

	std::string Variable::to_string() const {
		return "%" + this->get_name();
	}

	std::string BinaryOperation::to_string() const {
		return this->lhs->to_string()
			+ " " + program::to_string(this->op)
			+ " " + this->rhs->to_string();
	}
	void BinaryOperation::bind_to_scope(AggregateScope &agg_scope) {
		this->lhs->bind_to_scope(agg_scope);
		this->rhs->bind_to_scope(agg_scope);
	}
	std::string FunctionCall::to_string() const {
		std::string result = "call " + this->callee->to_string() + "(";
		for (const Uptr<Expr> &argument : this->arguments) {
			result += argument->to_string() + ", ";
		}
		result += ")";
		return result;
	}
	void FunctionCall::bind_to_scope(AggregateScope &agg_scope) {
		this->callee->bind_to_scope(agg_scope);
		for (Uptr<Expr> &arg : this->arguments) {
			arg->bind_to_scope(agg_scope);
		}
	}
	std::string MemoryLocation::to_string() const {
		std::string sol = "" + this->base->to_string();
		for (const auto &expr : this->dimensions) {
			sol += "[" + expr->to_string() + "]";
		}
		return sol;
	}
	void MemoryLocation::bind_to_scope(AggregateScope &agg_scope) {
		this->base->bind_to_scope(agg_scope);
		for (const auto &expr : this->dimensions) {
			expr->bind_to_scope(agg_scope);
		}
	}
	std::string ArrayDeclaration::to_string() const {
		std::string sol = "new Array (";
		for (const auto &arg : this->args) {
			sol += arg->to_string() + ", ";
		}
		sol += ")";
		return sol;
	}
	void ArrayDeclaration::bind_to_scope(AggregateScope &agg_scope) {
		for (const auto &arg : this->args) {
			arg->bind_to_scope(agg_scope);
		}
	}
	std::string Length::to_string() const {
		std::string sol = "Length " + this->var->to_string();
		if (this->dimension.has_value()){
			sol += " " + this->var->to_string();
		}
		return sol;
	}
	void Length::bind_to_scope(AggregateScope &agg_scope) {
		this->var->bind_to_scope(agg_scope);
	}

	std::string InstructionAssignment::to_string() const {
		std::string sol = "";
		if (this->maybe_dest.has_value()) {
			sol += this->maybe_dest.value()->to_string();
			sol += " <- ";
		}
		sol += this->source->to_string();
		return sol;
	}
	void InstructionAssignment::bind_to_scope(AggregateScope &agg_scope){
		if (this->maybe_dest.has_value()) {
			this->maybe_dest.value()->bind_to_scope(agg_scope);
		}
		this->source->bind_to_scope(agg_scope);
	}
	std::string InstructionDeclaration::to_string() const {
		return this->t.to_string() + " " + this->base->get_ref_name();
	}
	void InstructionDeclaration::bind_to_scope(AggregateScope &agg_scope){
		this->base->bind_to_scope(agg_scope);
	}
	std::string InstructionStore::to_string() const {
		return this->dest->to_string() + " <- " + this-> source->to_string();
	}
	void InstructionStore::bind_to_scope(AggregateScope &agg_scope) {
		this->dest->bind_to_scope(agg_scope);
		this->source->bind_to_scope(agg_scope);
	}

	void TerminatorBranchOne::bind_to_scope(AggregateScope &agg_scope) {
		this->bb_ref->bind_to_scope(agg_scope);
	}
	std::string TerminatorBranchOne::to_string() const {
		return "br" + this->bb_ref->to_string();
	}
	void TerminatorBranchTwo::bind_to_scope(AggregateScope &agg_scope) {
		this->condition->bind_to_scope(agg_scope);
		this->branchTrue->bind_to_scope(agg_scope);
		this->branchFalse->bind_to_scope(agg_scope);
	}
	std::string TerminatorBranchTwo::to_string() const {
		std::string sol = "br" + this->condition->to_string();
		sol += " " + this->branchTrue->to_string();
		sol += " " + this->branchFalse->to_string()  + "\n";
		return sol;
	}
	void TerminatorReturnVar::bind_to_scope(AggregateScope &agg_scope) {
		this->ret_var->bind_to_scope(agg_scope);
	}
	std::string TerminatorReturnVar::to_string() const {
		return "return" + this->ret_var->to_string();
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
		this->inst.push_back(mv(inst));
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
		return sol;
	}
	void BasicBlock::bind_to_scope(AggregateScope &scope){
		for (auto const& inst: this->inst) {
			inst->bind_to_scope(scope);
		}
		this->te->bind_to_scope(scope);
	}

	void AggregateScope::set_parent(AggregateScope &parent) {
		this->variable_scope.set_parent(parent.variable_scope);
		this->basic_block_scope.set_parent(parent.basic_block_scope);
		this->ir_function_scope.set_parent(parent.ir_function_scope);
		this->external_function_scope.set_parent(parent.external_function_scope);
	}

	Uptr<IRFunction> IRFunction::Builder::get_result() {
		for (std::string name : this->agg_scope.variable_scope.get_free_names()) {
			Uptr<Variable> var_ptr = mkuptr<Variable>(name);
			this->agg_scope.variable_scope.resolve_item(mv(name), var_ptr.get());
			this->vars.emplace_back(mv(var_ptr));
		}
		return Uptr<IRFunction>(new IRFunction(
			mv(this->name),
			mv(this->ret_type),
			mv(this->basic_blocks),
			mv(this->vars),
			mv(this->parameter_vars),
			mv(this->agg_scope)
		));
	}
	void IRFunction::Builder::add_name(std::string name) {
		this->name = mv(name);
	}
	void IRFunction::Builder::add_ret_type(Type t){
		this->ret_type = mv(t);
	}
	void IRFunction::Builder::add_block(Uptr<BasicBlock> &&bb){
		bb->bind_to_scope(this->agg_scope);
		this->agg_scope.basic_block_scope.resolve_item(bb->get_name(), bb.get());
		this->basic_blocks.push_back(mv(bb));
	}
	void IRFunction::Builder::add_parameter(Type type, std::string var_name){
		Uptr<Variable> var_ptr = mkuptr<Variable>(var_name);
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
	}
	void Program::Builder::add_ir_function(Uptr<IRFunction> &&function){
		function->get_scope().set_parent(this->agg_scope);
		this->agg_scope.ir_function_scope.resolve_item(function->get_name(), function.get());
		this->ir_functions.push_back(mv(function));
	}
	Uptr<Program> Program::Builder::get_result(){
		return Uptr<Program>(new Program(
			mv(this->ir_functions),
			mv(this->external_functions)
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
		for (const Uptr<IRFunction> &function : this->ir_functions) {
			result += function->to_string() + "\n";
		}
		return result;
	}
}