#include "code_gen.h"

namespace IR::code_gen {
    using namespace std_alias;
    using namespace IR::program;
    using namespace IR::tracer;

    void generate_ir_function_code(const IRFunction &ir_function, std::ostream &o) {
        // function header
        o << "define @" << ir_function.get_name() << "(";
        bool first = true;
        for (Variable *var: ir_function.get_parameter_vars()) {
            if (first){
                o << "%" << var->get_name();
                first = false;
            } else {
                o << ", %" << var->get_name();
            }
        }
        o << ") {\n";
        // print each block
        Vec<Trace> traces = trace_cfg(ir_function.get_blocks());
        for (Trace trace: traces) {
            bool first;
            Terminator *last;
            for (BasicBlock *bb: trace.block_sequence) {

            }
        }
        // for (const Uptr<BasicBlock> &block : l3_function.get_blocks()) {
        //     if (block->get_name().size() > 0) {
        //         o << "\t\t:" << block->get_name() << "\n";
        //     }
        //     Vec<Uptr<tiles::Tile>> tiles = tiles::tile_trees(block->get_tree_boxes());
        //     for (const Uptr<tiles::Tile> &tile : tiles) {
        //         for (const std::string &inst : tile->to_l2_instructions()) {
        //             o << "\t\t" << inst << "\n";
        //         }
        //     }
        // }
    }
    //     // assign parameter registers to variables
    //     const Vec<Variable *> &parameter_vars = l3_function.get_parameter_vars();
    //     for (int i = 0; i < parameter_vars.size(); ++i) {
    //         o << "\t\t" << target_arch::get_argument_loading_instruction(
    //             target_arch::to_l2_expr(parameter_vars[i]),
    //             i,
    //             parameter_vars.size()
    //         ) << "\n";
    //     }

        // // print each block
        // for (const Uptr<BasicBlock> &block : l3_function.get_blocks()) {
        //     if (block->get_name().size() > 0) {
        //         o << "\t\t:" << block->get_name() << "\n";
        //     }
        //     Vec<Uptr<tiles::Tile>> tiles = tiles::tile_trees(block->get_tree_boxes());
        //     for (const Uptr<tiles::Tile> &tile : tiles) {
        //         for (const std::string &inst : tile->to_l2_instructions()) {
        //             o << "\t\t" << inst << "\n";
        //         }
        //     }
        // }

    //     // close
    //     o << "\t)\n";
    // }

    void generate_program_code(Program &program, std::ostream &o) {
		target_arch::mangle_label_names(program);

		for (const Uptr<IRFunction> &function : program.get_ir_functions()) {
			generate_ir_function_code(*function, o);
		}
		o << "\n";
	}
}