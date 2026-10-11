#pragma once

#include "optimizer.h"

// ─── Constant Propagation Pass ──────────────────────────────────────────────
// Replaces variable uses with known constant values where valid.
// Within basic blocks / sequential sequences, maintains reaching constant
// definitions and substitutes operands.
// For example:
//   x = 10
//   y = x + 5   -->   y = 10 + 5

class ConstantPropagationPass : public OptimizationPass {
public:
    std::string name() const override { return "Constant Propagation"; }
    OptimizationResult run(IRProgram& ir) override;
};
