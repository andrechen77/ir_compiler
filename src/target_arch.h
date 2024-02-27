#include "std_alias.h"
#include "program.h"
#include <string>

namespace IR::code_gen::target_arch {
	using namespace std_alias;
	using namespace IR::program;

    // Modifies a program so that its label names are all globally unique
	// and always start with an underscore (so that non-underscore names can
	// be used by the generator)
	void mangle_label_names(Program &program);
}