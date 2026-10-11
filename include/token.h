#pragma once

#include <string>

// ─── Token types for the mini-language ───────────────────────────────────────

enum class TokenType {
    // Keywords
    LET, PRINT, IF, ELSE,

    // Literals and identifiers
    IDENTIFIER, INTEGER,

    // Arithmetic operators
    PLUS, MINUS, STAR, SLASH, PERCENT,

    // Assignment and comparison
    ASSIGN,
    EQ, NEQ, LT, GT,

    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    SEMICOLON,

    // Special
    EOF_TOKEN,
    ERROR
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

inline const char* tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::LET:        return "LET";
        case TokenType::PRINT:      return "PRINT";
        case TokenType::IF:         return "IF";
        case TokenType::ELSE:       return "ELSE";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER:    return "INTEGER";
        case TokenType::PLUS:       return "PLUS";
        case TokenType::MINUS:      return "MINUS";
        case TokenType::STAR:       return "STAR";
        case TokenType::SLASH:      return "SLASH";
        case TokenType::PERCENT:    return "PERCENT";
        case TokenType::ASSIGN:     return "ASSIGN";
        case TokenType::EQ:         return "EQ";
        case TokenType::NEQ:        return "NEQ";
        case TokenType::LT:         return "LT";
        case TokenType::GT:         return "GT";
        case TokenType::LPAREN:     return "LPAREN";
        case TokenType::RPAREN:     return "RPAREN";
        case TokenType::LBRACE:     return "LBRACE";
        case TokenType::RBRACE:     return "RBRACE";
        case TokenType::SEMICOLON:  return "SEMICOLON";
        case TokenType::EOF_TOKEN:  return "EOF";
        case TokenType::ERROR:      return "ERROR";
        default:                    return "UNKNOWN";
    }
}
