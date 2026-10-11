#pragma once

#include "optimizer.h"

// ─── Dead Code Elimination Pass ─────────────────────────────────────────────
// Eliminates instructions that produce values that are never used
// (or overwritten before being read), while preserving side effects (like PRINT).
// Soft-deletes instructions with eliminated = true to maintain line stability
// for visualization diffs.

class DeadCodeEliminationPass : public OptimizationPass {
public:
    std::string name() const override { return "Dead Code Elimination"; }
    OptimizationResult run(IRProgram& ir) override;
};
