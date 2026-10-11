#pragma once

#include "ast.h"
#include "lexer.h"
#include "token.h"

#include <string>
#include <vector>

// ─── Recursive Descent Parser ───────────────────────────────────────────────

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    Program parse();
    const std::vector<CompileError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& error_msg);

    void synchronize();
    void reportError(const std::string& message, const Token& token);

    // Grammar rules
    StmtPtr parseStatement();
    StmtPtr parseLetStatement();
    StmtPtr parseAssignStatement();
    StmtPtr parsePrintStatement();
    StmtPtr parseIfStatement();

    ExprPtr parseExpression();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parseMultiplication();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();

    std::vector<Token>        tokens_;
    int                       pos_ = 0;
    std::vector<CompileError> errors_;
};
