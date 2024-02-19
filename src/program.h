#pragma once

#include "std_alias.h"
#include <string>
#include <string_view>
#include <iostream>
#include <typeinfo>
#include <map>
#include <optional>
#include <algorithm>

namespace IR::program {
    using namespace std_alias;

	enum class Type {
		Int64,
		Code,
		Tuple,
		Void
	};
	std::pair<Type, int64_t> str_to_type(const std::string& str);
	std::string typeToString(Type type);
	int64_t get_num_dimensions(const std::string& str);

    class Expr {
        virtual std::string to_string() const = 0;
    };

    template<typename Item>
    class ItemRef : public Expr {
		std::string free_name;
		Item *referent_nullable;

		public:

		ItemRef(std::string free_name) :
			free_name { mv(free_name) },
			referent_nullable { nullptr }
		{}
        virtual std::string to_string() const override;
		Opt<Item *> get_referent() const {
			if (this->referent_nullable) {
				return this->referent_nullable;
			} else {
				return {};
			}
		}
		const std::string &get_ref_name() const {
			if (this->referent_nullable) {
				return this->referent_nullable->get_name();
			} else {
				return this->free_name;
			}
		}
    };

    class NumberLiteral : public Expr {
		int64_t value;

        public: 
        
        NumberLiteral(int64_t value) : value { value } {}
		int64_t get_value() const { return this->value; }
        virtual std::string to_string() const override {return std::to_string(this->value);};
    };

    enum struct Operator {
		lt,
		le,
		eq,
		ge,
		gt,
		plus,
		minus,
		times,
		bitwise_and,
		lshift,
		rshift
	};
	Operator str_to_op(std::string str);
	std::string to_string(Operator op);
	Opt<Operator> flip_operator(Operator op);

    class BinaryOperation : public Expr {
        Uptr<Expr> lhs;
		Uptr<Expr> rhs;
		Operator op;

        public:

        BinaryOperation(Uptr<Expr> &&lhs, Uptr<Expr> &&rhs, Operator op) :
			lhs { mv(lhs) },
			rhs { mv(rhs) },
			op { op }
		{}
        virtual std::string to_string() const override;
        Uptr<Expr> &get_lhs() { return this->lhs; }
        Uptr<Expr> &get_rhs() { return this->rhs; }
        Operator get_operator() {return this->op; }
    };

    class FunctionCall : public Expr {
		Uptr<Expr> callee;
		Vec<Uptr<Expr>> arguments;

		public:

		FunctionCall(Uptr<Expr> &&callee, Vec<Uptr<Expr>> &&arguments) :
			callee { mv(callee) }, arguments { mv(arguments) }
		{}

		virtual std::string to_string() const override;
	};

    class Variable {
        std::string name;
		Type t;
		int64_t num_dimensions;

        public:

        Variable(std::string name, Type t, int64_t num_dimensions) : 
			name { mv(name) },
			t {t},
			num_dimensions {num_dimensions}
		{}
        const std::string &get_name() const { return this->name; }
        Type &get_type() const { return this->t; }
		int64_t &get_num_dimensions { return this->num_dimensions; }
		std::string to_string() const;
    };

    class Instruction {
        
        public:
        
        virtual std::string to_string() const = 0;
    };
    
    class Terminator {

        public:

		virtual void bind_to_scope(AggregateScope &agg_scope) = 0;
        virtual std::string to_string() const = 0;
    };

	class TerminatorBranchOne {
		Uptr<ItemRef<BasicBlock>> bb_ref;

		public:
		TerminatorBranchOne(Uptr<ItemRef<BasicBlock>> &&bb_ref): bb_ref { mv(bb_ref) }{}
		virtual void bind_to_scope(AggregateScope &agg_scope) = 0;
        virtual std::string to_string() const = 0;
	};

    class BasicBlock {
        std::string name;
        Vec<Uptr<Instruction>> inst;
        Uptr<Terminator> te;

        public:

        BasicBlock(
			std::string name,
			Vec<Uptr<Instruction>> &&inst,
			Uptr<Terminator> &&te
		) :
			name { mv(name) },
			inst { mv(inst) },
			te { mv(te) }
		{}
        std::string to_string() const;
        std::string get_name() const { return this->name; }
        Vec<Uptr<Instruction>> &get_inst() { return this->inst; }
        Uptr<Terminator> &get_terminator() { return this->te; }
		void bind_to_scope(AggregateScope &agg_scope);

		class Builder {
			std::string name;
			Vec<Uptr<Instruction>> inst;
        	Uptr<Terminator> te;

			public:

			Builder(): {}
			Uptr<BasicBlock> &get_result();
			void add_name(std::string name);
			void add_instruction(Uptr<Instruction> &&inst);
			void add_terminator(Uptr<Terminator> &&te);
		};
    };

    template<typename Item>
    class Scope {
        Opt<Scope *> parent;
		Map<std::string, Item *> dict;
		Map<std::string, Vec<ItemRef<Item> *>> free_refs;

        public:

        Scope() : parent {}, dict {}, free_refs {} {}

        Vec<Item *> get_all_items() const {
			Vec<Item *> result;
			if (this->parent) {
				result = mv(static_cast<const Scope *>(*this->parent)->get_all_items());
			}
			for (const auto &[name, item] : this->dict) {
				result.push_back(item);
			}
			return result;
		}

        void set_parent(Scope &parent) {
			if (this->parent) {
				std::cerr << "this scope already has a parent oops\n";
				exit(1);
			}

			this->parent = std::make_optional<Scope *>(&parent);

			for (auto &[name, our_free_refs_vec] : this->free_refs) {
				for (ItemRef<Item> *our_free_ref : our_free_refs_vec) {
					(*this->parent)->add_ref(*our_free_ref);
				}
			}
			this->free_refs.clear();
		}

        // returns whether free refs exist in this scope for the given name
		Vec<ItemRef<Item> *> get_free_refs() const {
			std::vector<ItemRef<Item> *> result;
			for (auto &[name, free_refs_vec] : this->free_refs) {
				result.insert(result.end(), free_refs_vec.begin(), free_refs_vec.end());
			}
			return result;
		}

        // Adds the specified item to this scope under the specified name,
		// resolving all free refs who were depending on that name. Dies if
		// there already exists an item under that name.
		void resolve_item(std::string name, Item *item) {
			auto existing_item_it = this->dict.find(name);
			if (existing_item_it != this->dict.end()) {
				std::cerr << "name conflict: " << name << std::endl;
				exit(-1);
			}

			const auto [item_it, _] = this->dict.insert(std::make_pair(name, item));
			auto free_refs_vec_it = this->free_refs.find(name);
			if (free_refs_vec_it != this->free_refs.end()) {
				for (ItemRef<Item> *item_ref_ptr : free_refs_vec_it->second) {
					item_ref_ptr->bind(item_it->second);
				}
				this->free_refs.erase(free_refs_vec_it);
			}
		}

        std::optional<Item *> get_item_maybe(std::string_view name) {
			auto item_it = this->dict.find(name);
			if (item_it != this->dict.end()) {
				return std::make_optional<Item *>(item_it->second);
			} else {
				if (this->parent) {
					return (*this->parent)->get_item_maybe(name);
				} else {
					return {};
				}
			}
		}

        // returns whether the ref was immediately bound or was left as free
		bool add_ref(ItemRef<Item> &item_ref) {
			std::string_view ref_name = item_ref.get_ref_name();

			Opt<Item *> maybe_item = this->get_item_maybe(ref_name);
			if (maybe_item) {
				// bind the ref to the item
				item_ref.bind(*maybe_item);
				return true;
			} else {
				// there is no definition of this name in the current scope
				this->push_free_ref(item_ref);
				return false;
			}
		}
    };

	struct AggregateScope {
		Scope<Variable> variable_scope;
		Scope<BasicBlock> label_scope;
		Scope<IRFunction> ir_function_scope;
		Scope<ExternalFunction> external_function_scope;

		void set_parent(AggregateScope &parent);
	};

    class Function {
        public:
        virtual const std::string &get_name() const = 0;
        virtual std::string to_string() const = 0;
    };

    class IRFunction : public Function {
        std::string name;
        Vec<Uptr<BasicBlock>> blocks;
        Vec<Uptr<Variable>> vars;
        Vec<Variable *> parameter_vars;
		AggregateScope agg_scope;

        public:

        IRFunction(
			std::string name,
			Vec<Uptr<BasicBlock>> &&blocks,
			Vec<Uptr<Variable>> &&vars,
			Vec<Variable *> parameter_vars,
			AggregateScope &agg_scope
		) :
			name { mv(name) },
			blocks { mv(blocks) },
			vars { mv(vars) },
			parameter_vars { mv(parameter_vars) },
			agg_scope {mv(agg_scope)}
		{}
        virtual const std::string &get_name() const override { return this->name; }
        Vec<Uptr<BasicBlock>> &get_blocks() { return this->blocks; }
        const Vec<Variable *> &get_parameter_vars() const { return this->parameter_vars; }
        AggregateScope &get_scope() { return this->agg_scope; }
		virtual std::string to_string() const override;

		class Builder {
			std::string name;
			Vec<BasicBlock> basic_blocks;
			Vec<Uptr<Variable>> vars;
			Vec<Variable *> parameter_vars;
			AggregateScope agg_scope;

			public:

			Builder(){}
			Uptr<IRFunction> get_result();
			AggregateScope &get_scope(){return this->agg_scope; }
			void add_name(std::string name);
			void add_block(Uptr<BasicBlock> &&bb);
			void add_parameter(std::string type, std::string name);
		};
    };

    class ExternalFunction : public Function {
		std::string name;
		Vec<int> num_arguments;

		public:

		ExternalFunction(std::string name, Vec<int> num_arguments) :
			name { mv(name) }, num_arguments { mv(num_arguments) }
		{}

		virtual const std::string &get_name() const override { return this->name; }
		virtual std::string to_string() const override;
	};

    class Program {
        Vec<Uptr<IRFunction>> IR_functions;
        Vec<Uptr<ExternalFunction>> external_functions;

        Program(
			Vec<Uptr<IRFunction>> &&IR_functions,
			Vec<Uptr<ExternalFunction>> &&external_functions,
			Uptr<ItemRef<IRFunction>> &&main_function_ref
		) :
			IR_functions { mv(IR_functions) },
			external_functions { mv(external_functions) }
		{}

        public:

        std::string to_string() const;
        Vec<Uptr<IRFunction>> &get_IR_functions() { return this->IR_functions; }
		
		class Builder {
			Vec<Uptr<IRFunction>> IR_functions;
			Vec<Uptr<ExternalFunction>> external_functions;
			AggregateScope agg_scope;
			
			public:
			Builder();
			Vec<Uptr<IRFunction>> &get_ir_functions() { return this->IR_functions; }
			void add_ir_function(Uptr<IRFunction> &&function);
			Uptr<Program> get_result();
		};
    };
}