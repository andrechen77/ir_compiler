#pragma once
#include "std_alias.h"
#include "program.h"

#include <iostream>
#include <iomanip>
#include <queue>
#include <algorithm>
#include <assert.h>

namespace IR::tracer {
	using namespace std_alias;
    using namespace IR::program;

    struct Trace {
	    Vec<BasicBlock *> block_sequence; // must be a valid control flow
    };
    Vec<Trace> trace_cfg(const Vec<Uptr<BasicBlock>> &blocks);
}