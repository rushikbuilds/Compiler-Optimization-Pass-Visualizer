#pragma once

#include "ir.h"

#include <memory>
#include <string>
#include <vector>

// ─── Optimization pass interface ─────────────────────────────────────────────

struct OptimizationResult {
    bool                     changed = false;
    std::vector<std::string> explanations;
};

class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    virtual std::string        name() const = 0;
    virtual OptimizationResult run(IRProgram& ir) = 0;
};

using PassPtr = std::unique_ptr<OptimizationPass>;

// A snapshot of the IR after a single pass execution.
struct PassSnapshot {
    std::string              pass_name;
    int                      iteration = 0;
    bool                     changed   = false;
    IRProgram                ir_after;
    std::vector<std::string> explanations;
};
