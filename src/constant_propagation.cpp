#include "constant_propagation.h"

#include <unordered_map>
#include <sstream>

OptimizationResult ConstantPropagationPass::run(IRProgram& ir) {
    OptimizationResult res;
    res.changed = false;

    std::unordered_map<std::string, int32_t> known_constants;

    for (auto& instr : ir) {
        if (instr.eliminated) continue;

        // In the presence of control flow (labels / jumps), clear known constants
        // to avoid invalid cross-branch assumptions without full SSA/meet operator.
        if (instr.opcode == IROpcode::LABEL || instr.opcode == IROpcode::JUMP ||
            instr.opcode == IROpcode::JUMP_IF_ZERO) {
            known_constants.clear();
            continue;
        }

        // 1. Substitute operands if they are known constants
        if (!instr.arg1.is_const && !instr.arg1.var_name.empty()) {
            auto it = known_constants.find(instr.arg1.var_name);
            if (it != known_constants.end()) {
                std::string old_str = instr.toString();
                std::string var = instr.arg1.var_name;
                int32_t val = it->second;

                instr.arg1 = IRValue::makeConst(val);
                res.changed = true;

                std::ostringstream ss;
                ss << "Propagated " << var << " = " << val
                   << " into '" << old_str << "' -> '" << instr.toString() << "'";
                res.explanations.push_back(ss.str());
            }
        }

        if (!instr.arg2.is_const && !instr.arg2.var_name.empty()) {
            auto it = known_constants.find(instr.arg2.var_name);
            if (it != known_constants.end()) {
                std::string old_str = instr.toString();
                std::string var = instr.arg2.var_name;
                int32_t val = it->second;

                instr.arg2 = IRValue::makeConst(val);
                res.changed = true;

                std::ostringstream ss;
                ss << "Propagated " << var << " = " << val
                   << " into '" << old_str << "' -> '" << instr.toString() << "'";
                res.explanations.push_back(ss.str());
            }
        }

        // 2. Update known constants table based on this instruction definition
        if (!instr.dest.empty()) {
            if (instr.opcode == IROpcode::CONST && instr.arg1.is_const) {
                known_constants[instr.dest] = instr.arg1.const_val;
            } else if (instr.opcode == IROpcode::COPY && instr.arg1.is_const) {
                known_constants[instr.dest] = instr.arg1.const_val;
            } else {
                // Destination is redefined with a non-constant value: kill entry
                known_constants.erase(instr.dest);
            }
        }
    }

    return res;
}
