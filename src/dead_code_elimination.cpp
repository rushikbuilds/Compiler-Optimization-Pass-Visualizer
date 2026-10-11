#include "dead_code_elimination.h"

#include <unordered_set>
#include <sstream>

OptimizationResult DeadCodeEliminationPass::run(IRProgram& ir) {
    OptimizationResult res;
    res.changed = false;

    // Linear liveness is not sound across joins: a definition in one branch
    // may be required by a use reached through another branch.
    for (const auto& instr : ir) {
        if (instr.opcode == IROpcode::LABEL ||
            instr.opcode == IROpcode::JUMP ||
            instr.opcode == IROpcode::JUMP_IF_ZERO) {
            res.explanations.push_back(
                "Skipped DCE because control-flow-aware liveness is required");
            return res;
        }
    }

    // Backward liveness analysis
    // Variables/temporaries that are live at the current program point.
    std::unordered_set<std::string> live_vars;

    // First, if there are branches, all branch conditions and label dependencies
    // must be marked. In backward traversal:
    for (int i = static_cast<int>(ir.size()) - 1; i >= 0; --i) {
        auto& instr = ir[i];
        if (instr.eliminated) continue;

        // Side-effect instructions are inherently live
        if (instr.opcode == IROpcode::PRINT) {
            if (!instr.arg1.is_const && !instr.arg1.var_name.empty()) {
                live_vars.insert(instr.arg1.var_name);
            }
            continue;
        }

        // Control flow instructions are live
        if (instr.opcode == IROpcode::LABEL || instr.opcode == IROpcode::JUMP) {
            continue;
        }

        if (instr.opcode == IROpcode::JUMP_IF_ZERO) {
            if (!instr.arg1.is_const && !instr.arg1.var_name.empty()) {
                live_vars.insert(instr.arg1.var_name);
            }
            continue;
        }

        // Division and modulo can fail at runtime even when their result is
        // unused, so removing them would change observable behavior.
        if (instr.opcode == IROpcode::DIV || instr.opcode == IROpcode::MOD) {
            if (!instr.arg1.is_const && !instr.arg1.var_name.empty()) {
                live_vars.insert(instr.arg1.var_name);
            }
            if (!instr.arg2.is_const && !instr.arg2.var_name.empty()) {
                live_vars.insert(instr.arg2.var_name);
            }
            if (!instr.dest.empty()) live_vars.erase(instr.dest);
            continue;
        }

        // Instructions with a destination
        if (!instr.dest.empty()) {
            bool is_needed = (live_vars.find(instr.dest) != live_vars.end());

            if (is_needed) {
                // The destination was needed. Kill it from the live set (it's defined here)
                live_vars.erase(instr.dest);

                // Add the operands used by this instruction to the live set
                if (!instr.arg1.is_const && !instr.arg1.var_name.empty()) {
                    live_vars.insert(instr.arg1.var_name);
                }
                if (!instr.arg2.is_const && !instr.arg2.var_name.empty()) {
                    live_vars.insert(instr.arg2.var_name);
                }
            } else {
                // Instruction destination is never read later! Eliminate it.
                std::string old_str = instr.toString();
                instr.eliminated = true;
                res.changed = true;

                std::ostringstream ss;
                ss << "Eliminated dead instruction '" << old_str
                   << "' (result '" << instr.dest << "' is never used)";
                res.explanations.push_back(ss.str());
            }
        }
    }

    return res;
}
