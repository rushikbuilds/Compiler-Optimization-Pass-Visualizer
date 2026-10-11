#include "ir_generator.h"

#include <iostream>

IRGenerator::IRGenerator() {}

std::string IRGenerator::newTemp() {
    return "t" + std::to_string(++temp_counter_);
}

std::string IRGenerator::newLabel(const std::string& prefix) {
    return prefix + std::to_string(++label_counter_);
}

void IRGenerator::emit(IROpcode opcode, const std::string& dest, IRValue arg1,
                       IRValue arg2, const std::string& label, int line) {
    IRInstruction instr;
    instr.id = instr_id_++;
    instr.opcode = opcode;
    instr.dest = dest;
    instr.arg1 = arg1;
    instr.arg2 = arg2;
    instr.label = label;
    instr.source_line = line;
    instructions_.push_back(instr);
}

IRProgram IRGenerator::generate(const Program& program) {
    instructions_.clear();
    errors_.clear();
    declared_vars_.clear();
    temp_counter_ = 0;
    label_counter_ = 0;
    instr_id_ = 1;

    for (const auto& stmt : program) {
        if (stmt) {
            generateStmt(*stmt);
        }
    }

    return instructions_;
}

void IRGenerator::generateStmt(const Stmt& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;

        if constexpr (std::is_same_v<T, LetStmt>) {
            declared_vars_.insert(s.name);
            IRValue init_val = generateExpr(*s.init);
            if (init_val.is_const) {
                emit(IROpcode::CONST, s.name, init_val, IRValue::none(), "", s.line);
            } else {
                emit(IROpcode::COPY, s.name, init_val, IRValue::none(), "", s.line);
            }
        }
        else if constexpr (std::is_same_v<T, AssignStmt>) {
            if (declared_vars_.find(s.name) == declared_vars_.end()) {
                errors_.push_back(CompileError{
                    "Assignment to undeclared variable '" + s.name + "'",
                    s.line, 0, "ir_generator"
                });
            }
            IRValue val = generateExpr(*s.value);
            if (val.is_const) {
                emit(IROpcode::CONST, s.name, val, IRValue::none(), "", s.line);
            } else {
                emit(IROpcode::COPY, s.name, val, IRValue::none(), "", s.line);
            }
        }
        else if constexpr (std::is_same_v<T, PrintStmt>) {
            IRValue arg_val = generateExpr(*s.arg);
            emit(IROpcode::PRINT, "", arg_val, IRValue::none(), "", s.line);
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            IRValue cond_val = generateExpr(*s.condition);
            std::string else_label = newLabel("L_else_");
            std::string end_label = newLabel("L_end_");

            // Branch if condition == 0 to else
            emit(IROpcode::JUMP_IF_ZERO, "", cond_val, IRValue::none(), else_label, s.line);

            // Then branch
            for (const auto& then_stmt : s.then_block) {
                if (then_stmt) generateStmt(*then_stmt);
            }

            if (!s.else_block.empty()) {
                emit(IROpcode::JUMP, "", IRValue::none(), IRValue::none(), end_label, s.line);
            }

            // Else label
            emit(IROpcode::LABEL, "", IRValue::none(), IRValue::none(), else_label, s.line);

            // Else branch
            for (const auto& else_stmt : s.else_block) {
                if (else_stmt) generateStmt(*else_stmt);
            }

            if (!s.else_block.empty()) {
                emit(IROpcode::LABEL, "", IRValue::none(), IRValue::none(), end_label, s.line);
            }
        }
    }, stmt.node);
}

IRValue IRGenerator::generateExpr(const Expr& expr) {
    return std::visit([this](const auto& e) -> IRValue {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, IntLiteral>) {
            return IRValue::makeConst(e.value);
        }
        else if constexpr (std::is_same_v<T, VarRef>) {
            if (declared_vars_.find(e.name) == declared_vars_.end()) {
                errors_.push_back(CompileError{
                    "Use of undeclared variable '" + e.name + "'",
                    e.line, e.col, "ir_generator"
                });
            }
            return IRValue::makeVar(e.name);
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            IRValue operand = generateExpr(*e.operand);
            std::string temp = newTemp();
            emit(IROpcode::NEG, temp, operand, IRValue::none(), "", e.line);
            return IRValue::makeVar(temp);
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            IRValue left = generateExpr(*e.left);
            IRValue right = generateExpr(*e.right);

            IROpcode op = IROpcode::ADD;
            if (e.op == "+") op = IROpcode::ADD;
            else if (e.op == "-") op = IROpcode::SUB;
            else if (e.op == "*") op = IROpcode::MUL;
            else if (e.op == "/") op = IROpcode::DIV;
            else if (e.op == "%") op = IROpcode::MOD;
            else if (e.op == "==") op = IROpcode::CMP_EQ;
            else if (e.op == "!=") op = IROpcode::CMP_NEQ;
            else if (e.op == "<") op = IROpcode::CMP_LT;
            else if (e.op == ">") op = IROpcode::CMP_GT;

            std::string temp = newTemp();
            emit(op, temp, left, right, "", e.line);
            return IRValue::makeVar(temp);
        }
        return IRValue::none();
    }, expr.node);
}
