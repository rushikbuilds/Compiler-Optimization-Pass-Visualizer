#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

// ─── IR value: an operand that is either a compile-time constant or a
//     reference to a variable / temporary ─────────────────────────────────────

struct IRValue {
    bool        is_const = false;
    int32_t     const_val = 0;
    std::string var_name;

    static IRValue makeConst(int32_t v)                { IRValue r; r.is_const = true; r.const_val = v; return r; }
    static IRValue makeVar(const std::string& name)    { IRValue r; r.var_name = name; return r; }
    static IRValue none()                              { return IRValue{}; }

    bool isNone() const { return !is_const && var_name.empty(); }

    std::string toString() const {
        if (is_const) return std::to_string(const_val);
        return var_name;
    }

    bool operator==(const IRValue& o) const {
        if (is_const != o.is_const) return false;
        if (is_const) return const_val == o.const_val;
        return var_name == o.var_name;
    }
    bool operator!=(const IRValue& o) const { return !(*this == o); }
};

// ─── IR opcodes ──────────────────────────────────────────────────────────────

enum class IROpcode {
    CONST,          // dest = <constant>
    COPY,           // dest = arg1
    ADD,            // dest = arg1 + arg2
    SUB,            // dest = arg1 - arg2
    MUL,            // dest = arg1 * arg2
    DIV,            // dest = arg1 / arg2
    MOD,            // dest = arg1 % arg2
    NEG,            // dest = -arg1
    PRINT,          // print arg1
    // P1 branch instructions
    LABEL,          // label:
    JUMP,           // goto label
    JUMP_IF_ZERO,   // if arg1 == 0 goto label
    CMP_EQ,         // dest = (arg1 == arg2) ? 1 : 0
    CMP_NEQ,        // dest = (arg1 != arg2) ? 1 : 0
    CMP_LT,         // dest = (arg1 <  arg2) ? 1 : 0
    CMP_GT,         // dest = (arg1 >  arg2) ? 1 : 0
    NOP             // placeholder
};

inline const char* opcodeToString(IROpcode op) {
    switch (op) {
        case IROpcode::CONST:        return "CONST";
        case IROpcode::COPY:         return "COPY";
        case IROpcode::ADD:          return "ADD";
        case IROpcode::SUB:          return "SUB";
        case IROpcode::MUL:          return "MUL";
        case IROpcode::DIV:          return "DIV";
        case IROpcode::MOD:          return "MOD";
        case IROpcode::NEG:          return "NEG";
        case IROpcode::PRINT:        return "PRINT";
        case IROpcode::LABEL:        return "LABEL";
        case IROpcode::JUMP:         return "JUMP";
        case IROpcode::JUMP_IF_ZERO: return "JUMP_IF_ZERO";
        case IROpcode::CMP_EQ:       return "CMP_EQ";
        case IROpcode::CMP_NEQ:      return "CMP_NEQ";
        case IROpcode::CMP_LT:       return "CMP_LT";
        case IROpcode::CMP_GT:       return "CMP_GT";
        case IROpcode::NOP:          return "NOP";
        default:                     return "UNKNOWN";
    }
}

inline const char* opcodeSymbol(IROpcode op) {
    switch (op) {
        case IROpcode::ADD:     return "+";
        case IROpcode::SUB:     return "-";
        case IROpcode::MUL:     return "*";
        case IROpcode::DIV:     return "/";
        case IROpcode::MOD:     return "%";
        case IROpcode::CMP_EQ:  return "==";
        case IROpcode::CMP_NEQ: return "!=";
        case IROpcode::CMP_LT:  return "<";
        case IROpcode::CMP_GT:  return ">";
        default:                return "?";
    }
}

inline bool isBinaryOp(IROpcode op) {
    return op == IROpcode::ADD || op == IROpcode::SUB ||
           op == IROpcode::MUL || op == IROpcode::DIV ||
           op == IROpcode::MOD || op == IROpcode::CMP_EQ ||
           op == IROpcode::CMP_NEQ || op == IROpcode::CMP_LT ||
           op == IROpcode::CMP_GT;
}

// ─── IR instruction ──────────────────────────────────────────────────────────

struct IRInstruction {
    int         id = 0;                // unique, stable identifier
    IROpcode    opcode = IROpcode::NOP;
    std::string dest;                  // destination variable / temp
    IRValue     arg1;                  // first operand
    IRValue     arg2;                  // second operand (binary ops)
    std::string label;                 // for LABEL / JUMP / JUMP_IF_ZERO
    bool        eliminated = false;    // soft-delete flag for DCE
    int         source_line = 0;       // line in original source

    // Human-readable three-address code representation.
    std::string toString() const {
        if (eliminated) return "// (eliminated)";

        std::ostringstream ss;
        switch (opcode) {
            case IROpcode::CONST:
                ss << dest << " = " << arg1.const_val;
                break;
            case IROpcode::COPY:
                ss << dest << " = " << arg1.toString();
                break;
            case IROpcode::ADD: case IROpcode::SUB:
            case IROpcode::MUL: case IROpcode::DIV:
            case IROpcode::MOD:
            case IROpcode::CMP_EQ: case IROpcode::CMP_NEQ:
            case IROpcode::CMP_LT: case IROpcode::CMP_GT:
                ss << dest << " = " << arg1.toString()
                   << " " << opcodeSymbol(opcode) << " "
                   << arg2.toString();
                break;
            case IROpcode::NEG:
                ss << dest << " = -" << arg1.toString();
                break;
            case IROpcode::PRINT:
                ss << "print " << arg1.toString();
                break;
            case IROpcode::LABEL:
                ss << label << ":";
                break;
            case IROpcode::JUMP:
                ss << "goto " << label;
                break;
            case IROpcode::JUMP_IF_ZERO:
                ss << "if " << arg1.toString() << " == 0 goto " << label;
                break;
            case IROpcode::NOP:
                ss << "nop";
                break;
        }
        return ss.str();
    }

    bool operator==(const IRInstruction& o) const {
        return opcode == o.opcode && dest == o.dest &&
               arg1 == o.arg1 && arg2 == o.arg2 &&
               label == o.label && eliminated == o.eliminated;
    }
    bool operator!=(const IRInstruction& o) const { return !(*this == o); }
};

using IRProgram = std::vector<IRInstruction>;
