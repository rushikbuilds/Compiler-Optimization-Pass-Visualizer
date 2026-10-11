#include "lexer.h"

#include <cctype>

Lexer::Lexer(const std::string& source) : source_(source) {}

char Lexer::current() const {
    if (pos_ >= static_cast<int>(source_.size())) return '\0';
    return source_[pos_];
}

char Lexer::peek() const {
    if (pos_ + 1 >= static_cast<int>(source_.size())) return '\0';
    return source_[pos_ + 1];
}

void Lexer::advance() {
    if (pos_ < static_cast<int>(source_.size())) {
        if (source_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < static_cast<int>(source_.size())) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek() == '/') {
            // Line comment: skip until newline or EOF
            while (pos_ < static_cast<int>(source_.size()) && current() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::scanIdentifierOrKeyword() {
    int start_col = column_;
    int start_line = line_;
    std::string val;

    while (std::isalnum(static_cast<unsigned char>(current())) || current() == '_') {
        val.push_back(current());
        advance();
    }

    TokenType type = TokenType::IDENTIFIER;
    if (val == "let") type = TokenType::LET;
    else if (val == "print") type = TokenType::PRINT;
    else if (val == "if") type = TokenType::IF;
    else if (val == "else") type = TokenType::ELSE;

    return Token{type, val, start_line, start_col};
}

Token Lexer::scanNumber() {
    int start_col = column_;
    int start_line = line_;
    std::string val;

    while (std::isdigit(static_cast<unsigned char>(current()))) {
        val.push_back(current());
        advance();
    }

    return Token{TokenType::INTEGER, val, start_line, start_col};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos_ < static_cast<int>(source_.size())) {
        skipWhitespaceAndComments();
        if (pos_ >= static_cast<int>(source_.size())) break;

        char c = current();
        int token_line = line_;
        int token_col = column_;

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(scanNumber());
        } else {
            switch (c) {
                case '+':
                    advance();
                    tokens.push_back(Token{TokenType::PLUS, "+", token_line, token_col});
                    break;
                case '-':
                    advance();
                    tokens.push_back(Token{TokenType::MINUS, "-", token_line, token_col});
                    break;
                case '*':
                    advance();
                    tokens.push_back(Token{TokenType::STAR, "*", token_line, token_col});
                    break;
                case '/':
                    advance();
                    tokens.push_back(Token{TokenType::SLASH, "/", token_line, token_col});
                    break;
                case '%':
                    advance();
                    tokens.push_back(Token{TokenType::PERCENT, "%", token_line, token_col});
                    break;
                case '(':
                    advance();
                    tokens.push_back(Token{TokenType::LPAREN, "(", token_line, token_col});
                    break;
                case ')':
                    advance();
                    tokens.push_back(Token{TokenType::RPAREN, ")", token_line, token_col});
                    break;
                case '{':
                    advance();
                    tokens.push_back(Token{TokenType::LBRACE, "{", token_line, token_col});
                    break;
                case '}':
                    advance();
                    tokens.push_back(Token{TokenType::RBRACE, "}", token_line, token_col});
                    break;
                case ';':
                    advance();
                    tokens.push_back(Token{TokenType::SEMICOLON, ";", token_line, token_col});
                    break;
                case '=':
                    advance();
                    if (current() == '=') {
                        advance();
                        tokens.push_back(Token{TokenType::EQ, "==", token_line, token_col});
                    } else {
                        tokens.push_back(Token{TokenType::ASSIGN, "=", token_line, token_col});
                    }
                    break;
                case '!':
                    advance();
                    if (current() == '=') {
                        advance();
                        tokens.push_back(Token{TokenType::NEQ, "!=", token_line, token_col});
                    } else {
                        errors_.push_back(CompileError{
                            "Unexpected character '!' (did you mean '!= '?)",
                            token_line, token_col, "lexer"
                        });
                        tokens.push_back(Token{TokenType::ERROR, "!", token_line, token_col});
                    }
                    break;
                case '<':
                    advance();
                    tokens.push_back(Token{TokenType::LT, "<", token_line, token_col});
                    break;
                case '>':
                    advance();
                    tokens.push_back(Token{TokenType::GT, ">", token_line, token_col});
                    break;
                default: {
                    std::string err_char(1, c);
                    advance();
                    errors_.push_back(CompileError{
                        "Unexpected character '" + err_char + "'",
                        token_line, token_col, "lexer"
                    });
                    tokens.push_back(Token{TokenType::ERROR, err_char, token_line, token_col});
                    break;
                }
            }
        }
    }

    tokens.push_back(Token{TokenType::EOF_TOKEN, "", line_, column_});
    return tokens;
}
