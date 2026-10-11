#pragma once

#include "ir.h"

#include <string>
#include <vector>

// ─── IR Interpreter ─────────────────────────────────────────────────────────
// Executes IR instructions and captures console outputs and errors.
// Enables verifying semantics equivalence before and after optimization.

struct ExecutionResult {
    bool                     success = true;
    std::vector<std::string> output;
    std::string              error_message;
    int                      instructions_executed = 0;
};

class IRInterpreter {
public:
    explicit IRInterpreter(int max_instructions = 100000);

    ExecutionResult execute(const IRProgram& program);

private:
    int max_instructions_;
};
