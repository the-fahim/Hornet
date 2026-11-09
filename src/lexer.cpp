#include <lexer.h>
#include <cctype>
#include <stdexcept>
#include <utility>

[[noreturn]] void syntaxError(const Token& token, const std::string& message) {
    throw std::runtime_error("SyntaxError at " + std::to_string(token.line) + ":" +
                             std::to_string(token.column) + ": " + message);
}

std::vector<Token> tokenize(const std::string& source) {
    std::vector<Token> tokens;
    std::size_t pos = 0;
    int line = 1, column = 1;
    auto peek = [&](std::size_t offset = 0) { return pos + offset < source.size() ? source[pos + offset] : '\0'; };
    auto advance = [&]() {
        char c = source[pos++];
        if (c == '\n') { ++line; column = 1; } else ++column;
        return c;
    };
    auto digit = [](char c) { return c >= '0' && c <= '9'; };
    auto nameStart = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
    };
    while (pos < source.size()) {
        if (std::isspace(static_cast<unsigned char>(peek()))) { advance(); continue; }
        Token token{TokenKind::Symbol, "", line, column};
        if (peek() == '/' && peek(1) == '/') {
            while (pos < source.size() && peek() != '\n') advance();
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            advance(); advance();
            while (pos < source.size() && !(peek() == '*' && peek(1) == '/')) advance();
            if (pos == source.size()) syntaxError(token, "unterminated comment");
            advance(); advance();
            continue;
        }
        if (nameStart(peek())) {
            token.kind = TokenKind::Name;
            do { token.text += advance(); } while (nameStart(peek()) || digit(peek()));
        } else if (digit(peek()) || (peek() == '.' && digit(peek(1)))) {
            token.kind = TokenKind::Number;
            while (digit(peek())) token.text += advance();
            if (peek() == '.') {
                token.text += advance();
                while (digit(peek())) token.text += advance();
            }
            if (peek() == 'e' || peek() == 'E') {
                token.text += advance();
                if (peek() == '+' || peek() == '-') token.text += advance();
                if (!digit(peek())) syntaxError(token, "invalid exponent");
                while (digit(peek())) token.text += advance();
            }
            if (nameStart(peek())) syntaxError(token, "invalid numeric literal");
        } else if (peek() == '\'' || peek() == '"') {
            token.kind = TokenKind::String;
            const char quote = advance();
            while (pos < source.size() && peek() != quote) {
                char c = advance();
                if (c == '\n' || c == '\r') syntaxError(token, "newline in string");
                if (c == '\\') {
                    if (pos == source.size()) syntaxError(token, "unterminated escape");
                    switch (advance()) {
                    case 'n': c = '\n'; break;
                    case 'r': c = '\r'; break;
                    case 't': c = '\t'; break;
                    case '\\': c = '\\'; break;
                    case '\'': c = '\''; break;
                    case '"': c = '"'; break;
                    default: syntaxError(token, "unsupported string escape");
                    }
                }
                token.text += c;
            }
            if (pos == source.size()) syntaxError(token, "unterminated string");
            advance();
        } else {
            for (const std::string op : {"===", "!==", "==", "!=", "<=", ">=", "&&", "||",
                                         "++", "--", "+=", "-=", "*=", "/=", "%=", "=>", "**"}) {
                if (source.compare(pos, op.size(), op) == 0) {
                    token.text = op;
                    for (std::size_t i = 0; i < op.size(); ++i) advance();
                    break;
                }
            }
            if (token.text.empty()) {
                if (std::string("+-*/%(){}[]=;<>.!,: ").find(peek()) == std::string::npos)
                    syntaxError(token, "unsupported character");
                token.text += advance();
            }
        }
        tokens.push_back(std::move(token));
    }
    tokens.push_back({TokenKind::End, "<end>", line, column});
    return tokens;
}
