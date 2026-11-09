#include <parser.h>
#include <value.h>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>

namespace {
// Keeping keyword handling explicit makes unsupported syntax fail early.
const std::unordered_set<std::string> reserved = {
    "let", "const", "var", "function", "return", "if", "else", "while", "for",
    "true", "false", "null", "this", "new", "class", "throw", "delete", "typeof",
    "void", "in", "instanceof", "break", "continue", "switch", "case", "default",
    "do", "try", "catch", "finally", "with", "debugger", "import", "export",
    "extends", "super", "yield", "enum", "await", "implements", "interface",
    "package", "private", "protected", "public", "static"};

int precedence(const std::string& op) {
    if (op == "=") return 1;
    if (op == "||") return 2;
    if (op == "&&") return 3;
    if (op == "==" || op == "!=" || op == "===" || op == "!==") return 4;
    if (op == "<" || op == "<=" || op == ">" || op == ">=") return 5;
    if (op == "+" || op == "-") return 6;
    if (op == "*" || op == "/" || op == "%") return 7;
    return 0;
}
struct Nesting {
    int& depth;
    Nesting(int& depth, const Token& token) : depth(depth) {
        if (++depth > 128) syntaxError(token, "parser nesting limit exceeded");
    }
    ~Nesting() { --depth; }
};
}

const Token& Parser::peek() const { return tokens[position]; }
bool Parser::is(const std::string& text) const {
    return peek().kind != TokenKind::String && peek().text == text;
}
bool Parser::take(const std::string& text) {
    if (!is(text)) return false;
    ++position;
    return true;
}
Token Parser::expect(const std::string& text) {
    const auto token = peek();
    if (!take(text)) syntaxError(token, "expected '" + text + "', got '" + token.text + "'");
    return token;
}
Token Parser::identifier() {
    const auto token = peek();
    if (token.kind != TokenKind::Name || reserved.count(token.text)) syntaxError(token, "expected identifier");
    ++position;
    return token;
}
void Parser::terminator() {
    if (!take(";") && !is("}") && peek().kind != TokenKind::End)
        syntaxError(peek(), "expected ';' (automatic semicolon insertion is not supported)");
}
std::vector<std::string> Parser::parameters() {
    expect("(");
    std::vector<std::string> names;
    if (!is(")")) {
        do {
            auto name = identifier();
            if (std::find(names.begin(), names.end(), name.text) != names.end())
                syntaxError(name, "duplicate parameter");
            names.push_back(name.text);
        } while (take(","));
    }
    expect(")");
    return names;
}
Program Parser::parse() {
    Program program;
    while (peek().kind != TokenKind::End) program.push_back(statement());
    return program;
}
Program Parser::block() {
    expect("{");
    Program body;
    while (!is("}")) {
        if (peek().kind == TokenKind::End) syntaxError(peek(), "unterminated block");
        body.push_back(statement());
    }
    expect("}");
    return body;
}
std::unique_ptr<Statement> Parser::statement() {
    Nesting guard(nesting, peek());
    auto stmt = std::make_unique<Statement>();
    stmt->token = peek();
    if (take(";")) stmt->kind = Statement::Kind::Empty;
    else if (is("{")) {
        stmt->kind = Statement::Kind::Block;
        stmt->body = block();
    } else if (is("let") || is("const")) {
        stmt->kind = Statement::Kind::Variable;
        stmt->constant = take("const");
        if (!stmt->constant) expect("let");
        stmt->name = identifier().text;
        if (take("=")) stmt->expression = expression();
        else if (stmt->constant) syntaxError(peek(), "const requires an initializer");
        terminator();
    } else if (take("function")) {
        stmt->kind = Statement::Kind::Function;
        stmt->name = identifier().text;
        stmt->parameters = parameters();
        ++functionDepth;
        stmt->body = block();
        --functionDepth;
    } else if (take("if")) {
        stmt->kind = Statement::Kind::If;
        expect("("); stmt->expression = expression(); expect(")");
        stmt->body.push_back(statement());
        if (stmt->body[0]->kind == Statement::Kind::Variable || stmt->body[0]->kind == Statement::Kind::Function)
            syntaxError(stmt->body[0]->token, "declarations in a branch require braces");
        if (take("else")) stmt->alternate.push_back(statement());
        if (!stmt->alternate.empty() && (stmt->alternate[0]->kind == Statement::Kind::Variable ||
                                        stmt->alternate[0]->kind == Statement::Kind::Function))
            syntaxError(stmt->alternate[0]->token, "declarations in a branch require braces");
    } else if (take("while")) {
        stmt->kind = Statement::Kind::While;
        expect("("); stmt->expression = expression(); expect(")");
        stmt->body.push_back(statement());
        if (stmt->body[0]->kind == Statement::Kind::Variable || stmt->body[0]->kind == Statement::Kind::Function)
            syntaxError(stmt->body[0]->token, "declarations in a loop require braces");
    } else if (take("return")) {
        if (!functionDepth) syntaxError(stmt->token, "return outside a function");
        stmt->kind = Statement::Kind::Return;
        if (!is(";") && !is("}") && peek().kind != TokenKind::End) {
            if (peek().line != stmt->token.line)
                syntaxError(peek(), "put a return expression on the same line, or use 'return;'");
            stmt->expression = expression();
        }
        terminator();
    } else {
        stmt->kind = Statement::Kind::Expression;
        stmt->expression = expression();
        terminator();
    }
    return stmt;
}

std::unique_ptr<Expression> Parser::expression(int minimum) {
    Nesting guard(nesting, peek());
    auto left = unary();
    while (precedence(peek().text) >= minimum && peek().kind == TokenKind::Symbol) {
        auto token = tokens[position++];
        const bool assign = token.text == "=";
        if (assign && left->kind != Expression::Kind::Name && left->kind != Expression::Kind::Member)
            syntaxError(token, "invalid assignment target");
        if (assign && left->kind == Expression::Kind::Name && left->token.text == "this")
            syntaxError(token, "'this' is not an assignment target");
        auto node = std::make_unique<Expression>();
        node->kind = assign ? Expression::Kind::Assignment : Expression::Kind::Binary;
        node->token = token;
        node->children.push_back(std::move(left));
        node->children.push_back(expression(precedence(token.text) + (assign ? 0 : 1)));
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<Expression> Parser::unary() {
    Nesting guard(nesting, peek());
    if (is("+") || is("-") || is("!")) {
        auto node = std::make_unique<Expression>();
        node->kind = Expression::Kind::Unary;
        node->token = tokens[position++];
        node->children.push_back(unary());
        return node;
    }
    auto node = primary();
    while (is(".") || is("[") || is("(")) {
        auto next = std::make_unique<Expression>();
        next->token = peek();
        next->children.push_back(std::move(node));
        if (take("(")) {
            next->kind = Expression::Kind::Call;
            if (!is(")")) do { next->children.push_back(expression()); } while (take(","));
            expect(")");
        } else {
            next->kind = Expression::Kind::Member;
            if (take(".")) {
                if (peek().kind != TokenKind::Name) syntaxError(peek(), "expected property name");
                auto key = std::make_unique<Expression>();
                key->kind = Expression::Kind::String;
                key->token = tokens[position++];
                next->children.push_back(std::move(key));
            } else {
                expect("["); next->children.push_back(expression()); expect("]");
            }
        }
        node = std::move(next);
    }
    return node;
}

std::unique_ptr<Expression> Parser::primary() {
    auto node = std::make_unique<Expression>();
    node->token = peek();
    if (peek().kind == TokenKind::Number) { node->kind = Expression::Kind::Number; ++position; }
    else if (peek().kind == TokenKind::String) { node->kind = Expression::Kind::String; ++position; }
    else if (take("true") || take("false")) node->kind = Expression::Kind::Boolean;
    else if (take("null")) node->kind = Expression::Kind::Null;
    else if (take("(")) { node = expression(); expect(")"); }
    else if (take("{")) {
        node->kind = Expression::Kind::Object;
        if (!is("}")) {
            do {
                if (is("}")) break; // trailing comma
                if (peek().kind != TokenKind::Name && peek().kind != TokenKind::String && peek().kind != TokenKind::Number)
                    syntaxError(peek(), "expected object property name");
                const auto key = tokens[position++];
                // Numeric property names use the same conversion as computed access.
                node->names.push_back(key.kind == TokenKind::Number ?
                    toString(Value(std::strtod(key.text.c_str(), nullptr))) : key.text);
                expect(":");
                node->children.push_back(expression());
            } while (take(","));
        }
        expect("}");
    } else if (take("function")) {
        node->kind = Expression::Kind::Function;
        node->names = parameters(); // anonymous function expressions
        ++functionDepth;
        node->body = block();
        --functionDepth;
    } else if (take("this")) node->kind = Expression::Kind::Name;
    else {
        node->kind = Expression::Kind::Name;
        node->token = identifier();
    }
    return node;
}
