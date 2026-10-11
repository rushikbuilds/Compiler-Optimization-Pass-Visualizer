#include "constant_folding.h"

#include <climits>
#include <sstream>

bool ConstantFoldingPass::tryFoldBinary(IROpcode op, int32_t a, int32_t b, int32_t& result, std::string& err_msg) {
    uint32_t ua = static_cast<uint32_t>(a);
    uint32_t ub = static_cast<uint32_t>(b);

    switch (op) {
        case IROpcode::ADD:
            result = static_cast<int32_t>(ua + ub);
            return true;
        case IROpcode::SUB:
            result = static_cast<int32_t>(ua - ub);
            return true;
        case IROpcode::MUL:
            result = static_cast<int32_t>(ua * ub);
            return true;
        case IROpcode::DIV:
            if (b == 0) {
                err_msg = "Division by zero during compile-time constant folding";
                return false;
            }
            // Avoid INT32_MIN / -1 overflow in host C++
            if (a == INT32_MIN && b == -1) {
                result = INT32_MIN;
            } else {
                result = a / b;
            }
            return true;
        case IROpcode::MOD:
            if (b == 0) {
                err_msg = "Modulo by zero during compile-time constant folding";
                return false;
            }
            if (a == INT32_MIN && b == -1) {
                result = 0;
            } else {
                result = a % b;
            }
            return true;
        case IROpcode::CMP_EQ:
            result = (a == b) ? 1 : 0;
            return true;
        case IROpcode::CMP_NEQ:
            result = (a != b) ? 1 : 0;
            return true;
        case IROpcode::CMP_LT:
            result = (a < b) ? 1 : 0;
            return true;
        case IROpcode::CMP_GT:
            result = (a > b) ? 1 : 0;
            return true;
        default:
            return false;
    }
}

bool ConstantFoldingPass::tryFoldUnary(IROpcode op, int32_t a, int32_t& result) {
    if (op == IROpcode::NEG) {
        uint32_t ua = static_cast<uint32_t>(a);
        result = static_cast<int32_t>(~ua + 1); // 2's complement negation
        return true;
    }
    return false;
}

OptimizationResult ConstantFoldingPass::run(IRProgram& ir) {
    OptimizationResult res;
    res.changed = false;

    for (auto& instr : ir) {
        if (instr.eliminated) continue;

        if (isBinaryOp(instr.opcode)) {
            if (instr.arg1.is_const && instr.arg2.is_const) {
                int32_t folded_val = 0;
                std::string err_msg;
                if (tryFoldBinary(instr.opcode, instr.arg1.const_val, instr.arg2.const_val, folded_val, err_msg)) {
                    std::string before_str = instr.toString();
                    instr.opcode = IROpcode::CONST;
                    instr.arg1 = IRValue::makeConst(folded_val);
                    instr.arg2 = IRValue::none();
                    res.changed = true;

                    std::ostringstream ss;
                    ss << "Folded " << before_str << " -> " << instr.toString()
                       << " (evaluated compile-time arithmetic)";
                    res.explanations.push_back(ss.str());
                }
            }
        } else if (instr.opcode == IROpcode::NEG) {
            if (instr.arg1.is_const) {
                int32_t folded_val = 0;
                if (tryFoldUnary(instr.opcode, instr.arg1.const_val, folded_val)) {
                    std::string before_str = instr.toString();
                    instr.opcode = IROpcode::CONST;
                    instr.arg1 = IRValue::makeConst(folded_val);
                    res.changed = true;

                    std::ostringstream ss;
                    ss << "Folded " << before_str << " -> " << instr.toString()
                       << " (evaluated compile-time negation)";
                    res.explanations.push_back(ss.str());
                }
            }
        }
    }

    return res;
}
