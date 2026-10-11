#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// ─── Forward declarations ────────────────────────────────────────────────────

struct Expr;
struct Stmt;

using ExprPtr  = std::unique_ptr<Expr>;
using StmtPtr  = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

// ─── Expression nodes ────────────────────────────────────────────────────────

struct IntLiteral {
    int32_t value;
    int line, col;
};

struct VarRef {
    std::string name;
    int line, col;
};

struct BinaryExpr {
    std::string op;           // "+", "-", "*", "/", "%", "==", "!=", "<", ">"
    ExprPtr     left;
    ExprPtr     right;
    int         line, col;
};

struct UnaryExpr {
    std::string op;           // "-"
    ExprPtr     operand;
    int         line, col;
};

// Wrapper struct lets us use std::variant with recursive types.
struct Expr {
    using Node = std::variant<IntLiteral, VarRef, BinaryExpr, UnaryExpr>;
    Node node;
    template <typename T>
    explicit Expr(T&& n) : node(std::forward<T>(n)) {}
};

// ─── Statement nodes ────────────────────────────────────────────────────────

struct LetStmt {
    std::string name;
    ExprPtr     init;
    int         line;
};

struct AssignStmt {
    std::string name;
    ExprPtr     value;
    int         line;
};

struct PrintStmt {
    ExprPtr arg;
    int     line;
};

struct IfStmt {
    ExprPtr  condition;
    StmtList then_block;
    StmtList else_block;
    int      line;
};

struct Stmt {
    using Node = std::variant<LetStmt, AssignStmt, PrintStmt, IfStmt>;
    Node node;
    template <typename T>
    explicit Stmt(T&& n) : node(std::forward<T>(n)) {}
};

using Program = std::vector<StmtPtr>;

// ─── Factory helpers ─────────────────────────────────────────────────────────

inline ExprPtr makeExpr(IntLiteral&&  n) { return std::make_unique<Expr>(std::move(n)); }
inline ExprPtr makeExpr(VarRef&&      n) { return std::make_unique<Expr>(std::move(n)); }
inline ExprPtr makeExpr(BinaryExpr&&  n) { return std::make_unique<Expr>(std::move(n)); }
inline ExprPtr makeExpr(UnaryExpr&&   n) { return std::make_unique<Expr>(std::move(n)); }

inline StmtPtr makeStmt(LetStmt&&    n) { return std::make_unique<Stmt>(std::move(n)); }
inline StmtPtr makeStmt(AssignStmt&& n) { return std::make_unique<Stmt>(std::move(n)); }
inline StmtPtr makeStmt(PrintStmt&&  n) { return std::make_unique<Stmt>(std::move(n)); }
inline StmtPtr makeStmt(IfStmt&&     n) { return std::make_unique<Stmt>(std::move(n)); }
