#pragma once

#include "token.h"

#include <string>
#include <vector>

// ─── Compile error reported by any phase ─────────────────────────────────────

struct CompileError {
    std::string message;
    int         line   = 0;
    int         column = 0;
    std::string phase;          // "lexer", "parser", "ir_generator"
};

// ─── Lexer ───────────────────────────────────────────────────────────────────

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token>              tokenize();
    const std::vector<CompileError>& getErrors() const { return errors_; }
    bool                            hasErrors()  const { return !errors_.empty(); }

private:
    char  current() const;
    char  peek()    const;
    void  advance();
    void  skipWhitespaceAndComments();
    Token scanIdentifierOrKeyword();
    Token scanNumber();

    std::string              source_;
    int                      pos_    = 0;
    int                      line_   = 1;
    int                      column_ = 1;
    std::vector<CompileError> errors_;
};
