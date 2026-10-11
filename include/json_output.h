#pragma once

#include "ast.h"
#include "interpreter.h"
#include "ir.h"
#include "lexer.h"
#include "optimizer.h"
#include "token.h"

#include <string>
#include <vector>

// ─── JSON Output Generator ──────────────────────────────────────────────────
// Serializes compiler stages, tokens, AST, initial IR, snapshots of each pass,
// final IR, and execution results into clean JSON.

struct CompilationReport {
    bool                     success = true;
    std::string              source;
    std::vector<Token>       tokens;
    std::vector<PassSnapshot> snapshots;
    IRProgram                initial_ir;
    IRProgram                final_ir;
    ExecutionResult          exec_before;
    ExecutionResult          exec_after;
    bool                     results_match = true;
    std::vector<CompileError> errors;
};

class JSONOutput {
public:
    static std::string serialize(const CompilationReport& report, const Program* ast = nullptr);

    // Escape helpers
    static std::string escapeString(const std::string& str);
};
