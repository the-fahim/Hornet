#include <compiler.h>
#include <cstdlib>
#include <stdexcept>
#include <unordered_set>


int Compiler::emit(Op op, int operand, const std::string& text, int line) {
    code->instructions.push_back({op, operand, text, line});
    return static_cast<int>(code->instructions.size() - 1);
}
void Compiler::constant(Value value, int line) {
    code->constants.push_back(std::move(value));
    emit(Op::Constant, static_cast<int>(code->constants.size() - 1), "", line);
}
std::shared_ptr<FunctionCode> Compiler::compile(const Program& program, const std::string& name,
                                             const std::vector<std::string>& parameters) {
    code = std::make_shared<FunctionCode>();
    code->name = name;
    code->parameters = parameters;
    // Parameters and top-level declarations in a function share a scope.
    for (const auto& stmt : program) {
        if ((stmt->kind == Statement::Kind::Variable || stmt->kind == Statement::Kind::Function))
            for (const auto& parameter : parameters)
                if (stmt->name == parameter) syntaxError(stmt->token, "declaration conflicts with parameter");
    }
    sequence(program);
    constant(Value{}, 1);
    emit(Op::Return, 0, "", 1);
    return code;
}
void Compiler::nestedFunction(const Program& body, const std::vector<std::string>& parameters,
                              const std::string& name, int line) {
    Compiler child;
    child.functionDepth = functionDepth + 1;
    if (child.functionDepth > 64) throw std::runtime_error("SyntaxError: function nesting limit exceeded");
    code->functions.push_back(child.compile(body, name, parameters));
    emit(Op::MakeFunction, static_cast<int>(code->functions.size() - 1), "", line);
}
void Compiler::sequence(const Program& statements) {
    // Declare lexical bindings before executing the block, then hoist functions.
    std::unordered_set<std::string> names;
    for (const auto& stmt : statements) {
        if (stmt->kind != Statement::Kind::Variable && stmt->kind != Statement::Kind::Function) continue;
        if (!names.insert(stmt->name).second) syntaxError(stmt->token, "duplicate declaration: " + stmt->name);
        emit(Op::Declare, stmt->constant ? 1 : 0, stmt->name, stmt->token.line);
    }
    for (const auto& stmt : statements) {
        if (stmt->kind != Statement::Kind::Function) continue;
        nestedFunction(stmt->body, stmt->parameters, stmt->name, stmt->token.line);
        emit(Op::Initialize, 0, stmt->name, stmt->token.line);
    }
    for (const auto& stmt : statements) statement(*stmt);
}
void Compiler::statement(const Statement& stmt) {
    const int line = stmt.token.line;
    switch (stmt.kind) {
    case Statement::Kind::Empty: case Statement::Kind::Function: break;
    case Statement::Kind::Expression:
        expression(*stmt.expression); emit(Op::Pop, 0, "", line); break;
    case Statement::Kind::Variable:
        if (stmt.expression) expression(*stmt.expression); else constant(Value{}, line);
        emit(Op::Initialize, 0, stmt.name, line); break;
    case Statement::Kind::Block:
        emit(Op::EnterScope, 0, "", line);
        sequence(stmt.body);
        emit(Op::LeaveScope, 0, "", line); break;
    case Statement::Kind::If: {
        expression(*stmt.expression);
        const int branch = emit(Op::JumpIfFalse, 0, "", line);
        statement(*stmt.body[0]);
        const int end = emit(Op::Jump, 0, "", line);
        code->instructions[branch].operand = static_cast<int>(code->instructions.size());
        if (!stmt.alternate.empty()) statement(*stmt.alternate[0]);
        code->instructions[end].operand = static_cast<int>(code->instructions.size());
        break;
    }
    case Statement::Kind::While: {
        const int start = static_cast<int>(code->instructions.size());
        expression(*stmt.expression);
        const int end = emit(Op::JumpIfFalse, 0, "", line);
        statement(*stmt.body[0]);
        emit(Op::Jump, start, "", line);
        code->instructions[end].operand = static_cast<int>(code->instructions.size());
        break;
    }
    case Statement::Kind::Return:
        if (stmt.expression) expression(*stmt.expression); else constant(Value{}, line);
        emit(Op::Return, 0, "", line); break;
    }
}
void Compiler::expression(const Expression& expr) {
    if (++expressionDepth > 256) syntaxError(expr.token, "expression tree limit exceeded");
    const int line = expr.token.line;
    const auto& children = expr.children;
    switch (expr.kind) {
    case Expression::Kind::Number: constant(std::strtod(expr.token.text.c_str(), nullptr), line); break;
    case Expression::Kind::String: constant(expr.token.text, line); break;
    case Expression::Kind::Boolean: constant(expr.token.text == "true", line); break;
    case Expression::Kind::Null: constant(Null{}, line); break;
    case Expression::Kind::Undefined: constant(Value{}, line); break;
    case Expression::Kind::Name: emit(Op::Load, 0, expr.token.text, line); break;
    case Expression::Kind::Unary:
        expression(*children[0]); emit(Op::Unary, 0, expr.token.text, line); break;
    case Expression::Kind::Binary:
        expression(*children[0]);
        if (expr.token.text == "&&" || expr.token.text == "||") {
            emit(Op::Dup, 0, "", line);
            const auto branch = emit(expr.token.text == "&&" ? Op::JumpIfFalse : Op::JumpIfTrue, 0, "", line);
            emit(Op::Pop, 0, "", line);
            expression(*children[1]);
            code->instructions[branch].operand = static_cast<int>(code->instructions.size());
        } else {
            expression(*children[1]); emit(Op::Binary, 0, expr.token.text, line);
        }
        break;
    case Expression::Kind::Assignment:
        if (children[0]->kind == Expression::Kind::Name) {
            expression(*children[1]); emit(Op::Store, 0, children[0]->token.text, line);
        } else {
            expression(*children[0]->children[0]);
            expression(*children[0]->children[1]);
            expression(*children[1]); emit(Op::SetProperty, 0, "", line);
        }
        break;
    case Expression::Kind::Object:
        emit(Op::NewObject, 0, "", line);
        for (std::size_t i = 0; i < children.size(); ++i) {
            emit(Op::Dup, 0, "", line);
            constant(expr.names[i], line);
            expression(*children[i]);
            emit(Op::SetProperty, 0, "", line);
            emit(Op::Pop, 0, "", line);
        }
        break;
    case Expression::Kind::Member:
        expression(*children[0]); expression(*children[1]); emit(Op::GetProperty, 0, "", line); break;
    case Expression::Kind::Call: {
        const bool method = children[0]->kind == Expression::Kind::Member;
        if (method) {
            expression(*children[0]->children[0]);
            emit(Op::Dup, 0, "", line); // receiver retained for this
            expression(*children[0]->children[1]);
            emit(Op::GetProperty, 0, "", line);
        } else expression(*children[0]);
        for (std::size_t i = 1; i < children.size(); ++i) expression(*children[i]);
        emit(method ? Op::CallMethod : Op::Call, static_cast<int>(children.size() - 1), "", line);
        break;
    }
    case Expression::Kind::Function: nestedFunction(expr.body, expr.names, "<anonymous>", line); break;
    }
    --expressionDepth;
}

static const char* opName(Op op) {
    switch (op) {
#define NAME(x) case Op::x: return #x
        NAME(Constant); NAME(Pop); NAME(Dup); NAME(Load); NAME(Declare); NAME(Initialize); NAME(Store);
        NAME(EnterScope); NAME(LeaveScope); NAME(Unary); NAME(Binary); NAME(NewObject);
        NAME(GetProperty); NAME(SetProperty); NAME(MakeFunction); NAME(Call); NAME(CallMethod);
        NAME(Jump); NAME(JumpIfFalse); NAME(JumpIfTrue); NAME(Return);
#undef NAME
    }
    return "?";
}
void dumpBytecode(const FunctionCode& code, std::ostream& out, int indentation) {
    const std::string pad(static_cast<std::size_t>(indentation), ' ');
    out << pad << "function " << code.name << " (" << code.parameters.size() << " parameters)\n";
    for (std::size_t i = 0; i < code.instructions.size(); ++i) {
        const auto& instruction = code.instructions[i];
        out << pad << "  " << i << "  " << opName(instruction.op);
        switch (instruction.op) {
        case Op::Constant:
            out << " " << instruction.operand << " (" << toString(code.constants.at(instruction.operand)) << ")"; break;
        case Op::Call: case Op::CallMethod: case Op::Jump: case Op::JumpIfFalse:
        case Op::JumpIfTrue: case Op::MakeFunction: case Op::Declare:
            out << " " << instruction.operand; break;
        default: break;
        }
        if (!instruction.text.empty()) out << " " << instruction.text;
        out << "  ; line " << instruction.line << '\n';
    }
    for (const auto& function : code.functions) dumpBytecode(*function, out, indentation + 2);
}
