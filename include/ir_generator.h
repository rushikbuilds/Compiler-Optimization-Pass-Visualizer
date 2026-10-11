#pragma once

#include "ast.h"
#include "ir.h"
#include "lexer.h"

#include <string>
#include <unordered_set>
#include <vector>

// ─── Lower AST to Three-Address Code IR ──────────────────────────────────────

class IRGenerator {
public:
    IRGenerator();

    IRProgram generate(const Program& program);
    const std::vector<CompileError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    void generateStmt(const Stmt& stmt);
    IRValue generateExpr(const Expr& expr);

    std::string newTemp();
    std::string newLabel(const std::string& prefix = "L");
    void emit(IROpcode opcode, const std::string& dest, IRValue arg1 = IRValue::none(),
              IRValue arg2 = IRValue::none(), const std::string& label = "", int line = 0);

    int                       temp_counter_ = 0;
    int                       label_counter_ = 0;
    int                       instr_id_ = 1;
    IRProgram                 instructions_;
    std::vector<CompileError> errors_;
    std::unordered_set<std::string> declared_vars_;
};
