#pragma once
#include <bytecode.h>
#include <parser.h>

class Compiler {
public:
    std::shared_ptr<FunctionCode> compile(const Program& program, const std::string& name = "<script>", const std::vector<std::string>& parameters = {});

private:
    int emit(Op op, int operand, const std::string& text, int line);
    void constant(Value value, int line);
    void sequence(const Program& statements);
    void statement(const Statement& stmt);
    void expression(const Expression& expr);
    void nestedFunction(const Program& body, const std::vector<std::string>& parameters,
                        const std::string& name, int line);
    std::shared_ptr<FunctionCode> code;
    int expressionDepth = 0;
    int functionDepth = 0;
};

