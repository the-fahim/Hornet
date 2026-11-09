#pragma once
#include <lexer.h>
#include <memory>
#include <utility>
#include <vector>


struct Statement;
struct Expression {
    enum class Kind { Number, String, Boolean, Null, Undefined, Name, Unary, Binary,
                      Assignment, Object, Member, Call, Function };
    Kind kind;
    Token token;
    // Operands, call arguments, or object property values.
    std::vector<std::unique_ptr<Expression>> children;
    std::vector<std::string> names; // object keys or function parameters
    std::vector<std::unique_ptr<Statement>> body;
};
struct Statement {
    enum class Kind { Empty, Expression, Variable, Block, If, While, Function, Return };
    Kind kind;
    Token token;
    std::string name;
    bool constant = false;
    std::unique_ptr<Expression> expression;
    std::vector<std::unique_ptr<Statement>> body;
    std::vector<std::unique_ptr<Statement>> alternate;
    std::vector<std::string> parameters;
};
using Program = std::vector<std::unique_ptr<Statement>>;

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}
    Program parse();
private:
    const Token& peek() const;
    bool is(const std::string& text) const;
    bool take(const std::string& text);
    Token expect(const std::string& text);
    Token identifier();
    void terminator();
    std::vector<std::string> parameters();
    Program block();
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Expression> expression(int minimum = 1);
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> primary();
    std::vector<Token> tokens;
    std::size_t position = 0;
    int functionDepth = 0;
    int nesting = 0;
};
