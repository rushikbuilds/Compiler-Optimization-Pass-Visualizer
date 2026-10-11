#pragma once

#include "optimizer.h"

// ─── Constant Folding Pass ──────────────────────────────────────────────────
// Evaluates operations whose operands are known compile-time constants.
// For example:
//   t1 = 3 * 4   -->   t1 = 12
// Uses wrapping modular 2^32 arithmetic for defined overflow semantics.

class ConstantFoldingPass : public OptimizationPass {
public:
    std::string name() const override { return "Constant Folding"; }
    OptimizationResult run(IRProgram& ir) override;

private:
    static bool tryFoldBinary(IROpcode op, int32_t a, int32_t b, int32_t& result, std::string& err_msg);
    static bool tryFoldUnary(IROpcode op, int32_t a, int32_t& result);
};
