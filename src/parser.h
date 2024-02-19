#pragma once

#include "std_alias.h"
#include <memory>
#include <typeinfo>
#include <optional>

namespace IR::parser {
	using namespace std_alias;

	void parse_input(char *fileName, Opt<std::string> parse_tree_output);
}