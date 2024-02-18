#pragma once

#include "std_alias.h"
#include <string>
#include <string_view>
#include <iostream>
#include <typeinfo>
#include <map>
#include <optional>
#include <algorithm>

namespace L3::program {

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
    };

    class NumberLiteral : public Expr {
		int64_t value;

        public: 
        
        NumberLiteral(int64_t value) : value { value } {}
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

    class Variable {
        std::string name;

        public:

        Variable(std::string name) : name { mv(name) } {}
        const std::string &get_name() const { return this->name; }
        std::string to_string() const;
    };

    class Instruction {
        public:
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
			Uptr<Terminator *> te
		) :
			name { mv(name) },
			inst { mv(inst) },
			parameter_vars { mv(te) }
		{}
        std::string to_string() const;
        std::string get_name() const { return this->name; }
        Vec<Uptr<Instruction>> &get_inst() { return this->inst; }
        Uptr<Terminator> &get_terminator() { return this->te; }
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

        public:

        L3Function(
			std::string name,
			Vec<Uptr<BasicBlock>> &&blocks,
			Vec<Uptr<Variable>> &&vars,
			Vec<Variable *> parameter_vars
		) :
			name { mv(name) },
			blocks { mv(blocks) },
			vars { mv(vars) },
			parameter_vars { mv(parameter_vars) }
		{}

        public:

        virtual const std::string &get_name() const override { return this->name; }
        Vec<Uptr<BasicBlock>> &get_blocks() { return this->blocks; }
        const Vec<Variable *> &get_parameter_vars() const { return this->parameter_vars; }
        virtual std::string to_string() const override;
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
        Vec<Uptr<IRFunctions>> IR_functions;
        Vec<Uptr<ExternalFunction>> external_functions;

        Program(
			Vec<Uptr<L3Function>> &&l3_functions,
			Vec<Uptr<ExternalFunction>> &&external_functions,
			Uptr<ItemRef<L3Function>> &&main_function_ref
		) :
			l3_functions { mv(l3_functions) },
			external_functions { mv(external_functions) }
		{}

        public:

        std::string to_string() const;
        Vec<Uptr<L3Function>> &get_l3_functions() { return this->l3_functions; }

    };
}