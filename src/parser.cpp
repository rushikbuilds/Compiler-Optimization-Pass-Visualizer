#include "parser.h"

#include <iostream>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::current() const {
    if (pos_ >= static_cast<int>(tokens_.size())) {
        return tokens_.back();
    }
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    int idx = pos_ + offset;
    if (idx >= static_cast<int>(tokens_.size())) {
        return tokens_.back();
    }
    return tokens_[idx];
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::EOF_TOKEN;
}

Token Parser::advance() {
    if (!isAtEnd()) {
        pos_++;
    }
    return tokens_[pos_ - 1];
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& error_msg) {
    if (check(type)) return advance();
    reportError(error_msg, current());
    return current();
}

void Parser::reportError(const std::string& message, const Token& token) {
    errors_.push_back(CompileError{
        message,
        token.line,
        token.column,
        "parser"
    });
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (tokens_[pos_ - 1].type == TokenType::SEMICOLON) return;
        if (tokens_[pos_ - 1].type == TokenType::RBRACE) return;

        switch (current().type) {
            case TokenType::LET:
            case TokenType::PRINT:
            case TokenType::IF:
                return;
            default:
                advance();
                break;
        }
    }
}

Program Parser::parse() {
    Program program;
    while (!isAtEnd()) {
        if (current().type == TokenType::SEMICOLON) {
            advance(); // Skip stray semicolons
            continue;
        }
        auto stmt = parseStatement();
        if (stmt) {
            program.push_back(std::move(stmt));
        } else {
            synchronize();
        }
    }
    return program;
}

StmtPtr Parser::parseStatement() {
    if (check(TokenType::LET)) {
        return parseLetStatement();
    }
    if (check(TokenType::PRINT)) {
        return parsePrintStatement();
    }
    if (check(TokenType::IF)) {
        return parseIfStatement();
    }
    if (check(TokenType::IDENTIFIER)) {
        return parseAssignStatement();
    }

    reportError("Expected statement (let, assignment, print, or if), found '" + current().value + "'", current());
    return nullptr;
}

StmtPtr Parser::parseLetStatement() {
    Token let_tok = advance(); // Consume 'let'
    Token name_tok = consume(TokenType::IDENTIFIER, "Expected identifier after 'let'");
    consume(TokenType::ASSIGN, "Expected '=' in variable declaration");
    ExprPtr init = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    if (!init) return nullptr;
    return makeStmt(LetStmt{name_tok.value, std::move(init), let_tok.line});
}

StmtPtr Parser::parseAssignStatement() {
    Token name_tok = advance(); // Consume identifier
    consume(TokenType::ASSIGN, "Expected '=' in assignment");
    ExprPtr val = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment");

    if (!val) return nullptr;
    return makeStmt(AssignStmt{name_tok.value, std::move(val), name_tok.line});
}

StmtPtr Parser::parsePrintStatement() {
    Token print_tok = advance(); // Consume 'print'
    consume(TokenType::LPAREN, "Expected '(' after 'print'");
    ExprPtr arg = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after print argument");
    consume(TokenType::SEMICOLON, "Expected ';' after print statement");

    if (!arg) return nullptr;
    return makeStmt(PrintStmt{std::move(arg), print_tok.line});
}

StmtPtr Parser::parseIfStatement() {
    Token if_tok = advance(); // Consume 'if'
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    ExprPtr cond = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after if condition");

    consume(TokenType::LBRACE, "Expected '{' before then branch");
    StmtList then_block;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto s = parseStatement();
        if (s) then_block.push_back(std::move(s));
        else synchronize();
    }
    consume(TokenType::RBRACE, "Expected '}' after then branch");

    StmtList else_block;
    if (match(TokenType::ELSE)) {
        consume(TokenType::LBRACE, "Expected '{' before else branch");
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            auto s = parseStatement();
            if (s) else_block.push_back(std::move(s));
            else synchronize();
        }
        consume(TokenType::RBRACE, "Expected '}' after else branch");
    }

    if (!cond) return nullptr;
    return makeStmt(IfStmt{std::move(cond), std::move(then_block), std::move(else_block), if_tok.line});
}

ExprPtr Parser::parseExpression() {
    return parseComparison();
}

ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddition();
    if (!left) return nullptr;

    while (check(TokenType::EQ) || check(TokenType::NEQ) ||
           check(TokenType::LT) || check(TokenType::GT)) {
        Token op_tok = advance();
        ExprPtr right = parseAddition();
        if (!right) return nullptr;
        left = makeExpr(BinaryExpr{op_tok.value, std::move(left), std::move(right), op_tok.line, op_tok.column});
    }

    return left;
}

ExprPtr Parser::parseAddition() {
    ExprPtr left = parseMultiplication();
    if (!left) return nullptr;

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op_tok = advance();
        ExprPtr right = parseMultiplication();
        if (!right) return nullptr;
        left = makeExpr(BinaryExpr{op_tok.value, std::move(left), std::move(right), op_tok.line, op_tok.column});
    }

    return left;
}

ExprPtr Parser::parseMultiplication() {
    ExprPtr left = parseUnary();
    if (!left) return nullptr;

    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        Token op_tok = advance();
        ExprPtr right = parseUnary();
        if (!right) return nullptr;
        left = makeExpr(BinaryExpr{op_tok.value, std::move(left), std::move(right), op_tok.line, op_tok.column});
    }

    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::MINUS)) {
        Token op_tok = advance();
        ExprPtr operand = parseUnary();
        if (!operand) return nullptr;
        return makeExpr(UnaryExpr{op_tok.value, std::move(operand), op_tok.line, op_tok.column});
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::INTEGER)) {
        Token tok = advance();
        int32_t val = 0;
        try {
            val = static_cast<int32_t>(std::stoll(tok.value));
        } catch (...) {
            reportError("Integer literal out of range: " + tok.value, tok);
        }
        return makeExpr(IntLiteral{val, tok.line, tok.column});
    }

    if (check(TokenType::IDENTIFIER)) {
        Token tok = advance();
        return makeExpr(VarRef{tok.value, tok.line, tok.column});
    }

    if (match(TokenType::LPAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    reportError("Expected expression, found '" + current().value + "'", current());
    return nullptr;
}
