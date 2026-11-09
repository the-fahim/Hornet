#include <runtime.h>
#include <compiler.h>
#include <limits>
#include <stdexcept>


Runtime::Runtime(std::ostream& output, Limits limits) : output(output), limits(limits) {
    global = newEnvironment(nullptr);
    auto* console = newObject();
    auto* log = newFunction(nullptr, global);
    log->nativeLog = true;
    console->properties["log"] = log;
    for (const auto& entry : std::vector<std::pair<std::string, Value>>{
             {"console", console}, {"undefined", Value{}}, {"NaN", std::numeric_limits<double>::quiet_NaN()},
             {"Infinity", std::numeric_limits<double>::infinity()}}) {
        declare(global, entry.first, true);
        initialize(global, entry.first, entry.second);
    }
}
void Runtime::allocation() {
    if (environments.size() + objects.size() + functions.size() >= limits.allocations)
        throw std::runtime_error("RangeError: runtime allocation limit exceeded");
}
Environment* Runtime::newEnvironment(Environment* parent) {
    allocation();
    auto env = std::make_unique<Environment>();
    env->parent = parent;
    auto* result = env.get();
    environments.push_back(std::move(env));
    return result;
}
Object* Runtime::newObject() {
    allocation();
    auto object = std::make_unique<Object>();
    auto* result = object.get();
    objects.push_back(std::move(object));
    return result;
}
Function* Runtime::newFunction(std::shared_ptr<FunctionCode> code, Environment* closure) {
    allocation();
    auto function = std::make_unique<Function>();
    function->code = std::move(code);
    function->closure = closure;
    auto* result = function.get();
    functions.push_back(std::move(function));
    return result;
}
void Runtime::declare(Environment* env, const std::string& name, bool constant) {
    if (!env->bindings.emplace(name, Binding{Value{}, constant, false}).second)
        throw std::runtime_error("SyntaxError: duplicate declaration: " + name);
}
Binding& Runtime::binding(Environment* env, const std::string& name) {
    for (; env; env = env->parent) {
        auto it = env->bindings.find(name);
        if (it != env->bindings.end()) return it->second;
    }
    throw std::runtime_error("ReferenceError: " + name + " is not defined");
}
Value Runtime::load(Environment* env, const std::string& name) {
    auto& item = binding(env, name);
    if (!item.initialized) throw std::runtime_error("ReferenceError: cannot access '" + name + "' before initialization");
    return item.value;
}
void Runtime::initialize(Environment* env, const std::string& name, Value value) {
    auto it = env->bindings.find(name);
    if (it == env->bindings.end() || it->second.initialized)
        throw std::runtime_error("internal error: invalid binding initialization: " + name);
    it->second.value = std::move(value);
    it->second.initialized = true;
}
void Runtime::store(Environment* env, const std::string& name, const Value& value) {
    auto& item = binding(env, name);
    if (!item.initialized) throw std::runtime_error("ReferenceError: cannot assign before initialization: " + name);
    if (item.constant) throw std::runtime_error("TypeError: assignment to constant: " + name);
    item.value = value;
}
static std::string propertyKey(const Value& key) {
    if (std::holds_alternative<Object*>(key) || std::holds_alternative<Function*>(key))
        throw std::runtime_error("TypeError: object property keys are not supported");
    return toString(key);
}
Value Runtime::getProperty(const Value& object, const Value& key) {
    const auto name = propertyKey(key);
    if (const auto* ptr = std::get_if<Object*>(&object)) {
        auto it = (*ptr)->properties.find(name);
        return it == (*ptr)->properties.end() ? Value{} : it->second;
    }
    if (const auto* text = std::get_if<std::string>(&object)) {
        if (name == "length") return static_cast<double>(text->size());
        return Value{};
    }
    if (std::holds_alternative<double>(object) || std::holds_alternative<bool>(object)) return Value{};
    throw std::runtime_error("TypeError: property access requires an object or supported primitive");
}
void Runtime::setProperty(const Value& object, const Value& key, const Value& value) {
    const auto* ptr = std::get_if<Object*>(&object);
    if (!ptr) throw std::runtime_error("TypeError: property assignment requires an object");
    (*ptr)->properties[propertyKey(key)] = value;
}
Value Runtime::call(const Value& callee, const Value& receiver, const std::vector<Value>& arguments) {
    const auto* ptr = std::get_if<Function*>(&callee);
    if (!ptr) throw std::runtime_error("TypeError: value is not callable");
    auto& function = **ptr;
    ++function.calls;
    if (function.nativeLog) {
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            if (i) output << ' ';
            output << toString(arguments[i]);
        }
        output << '\n';
        return Value{};
    }
    if (depth >= limits.callDepth) throw std::runtime_error("RangeError: call depth limit exceeded");
    auto* env = newEnvironment(function.closure);
    declare(env, "this", true);
    initialize(env, "this", receiver);
    for (std::size_t i = 0; i < function.code->parameters.size(); ++i) {
        const auto& name = function.code->parameters[i];
        declare(env, name, false);
        initialize(env, name, i < arguments.size() ? arguments[i] : Value{});
    }
    ++depth;
    try {
        auto value = execute(*function.code, env);
        --depth;
        return value;
    } catch (...) { --depth; throw; }
}
Value Runtime::execute(const FunctionCode& code, Environment* environment) {
    std::vector<Value> stack;
    auto pop = [&]() {
        if (stack.empty()) throw std::runtime_error("internal error: operand stack underflow");
        auto value = std::move(stack.back());
        stack.pop_back();
        return value;
    };
    std::size_t pc = 0;
    int line = 1;
    try {
        while (pc < code.instructions.size()) {
            const auto& instruction = code.instructions[pc++];
            line = instruction.line;
            if (remainingInstructions == 0) throw std::runtime_error("RangeError: instruction limit exceeded");
            --remainingInstructions;
            switch (instruction.op) {
            case Op::Constant: stack.push_back(code.constants.at(instruction.operand)); break;
            case Op::Pop: pop(); break;
            case Op::Dup:
                if (stack.empty()) throw std::runtime_error("internal error: duplicate on empty stack");
                stack.push_back(stack.back()); break;
            case Op::Load: stack.push_back(load(environment, instruction.text)); break;
            case Op::Declare: declare(environment, instruction.text, instruction.operand != 0); break;
            case Op::Initialize: initialize(environment, instruction.text, pop()); break;
            case Op::Store:
                if (stack.empty()) throw std::runtime_error("internal error: store on empty stack");
                store(environment, instruction.text, stack.back()); break;
            case Op::EnterScope: environment = newEnvironment(environment); break;
            case Op::LeaveScope:
                if (!environment->parent) throw std::runtime_error("internal error: no parent scope");
                environment = environment->parent; break;
            case Op::Unary: { auto value = pop(); stack.push_back(unaryOperation(instruction.text, value)); break; }
            case Op::Binary: {
                auto right = pop(); auto left = pop();
                stack.push_back(binaryOperation(instruction.text, left, right)); break;
            }
            case Op::NewObject: stack.push_back(newObject()); break;
            case Op::GetProperty: {
                auto key = pop(); auto object = pop(); stack.push_back(getProperty(object, key)); break;
            }
            case Op::SetProperty: {
                auto value = pop(); auto key = pop(); auto object = pop();
                setProperty(object, key, value); stack.push_back(std::move(value)); break;
            }
            case Op::MakeFunction:
                stack.push_back(newFunction(code.functions.at(instruction.operand), environment)); break;
            case Op::Call: case Op::CallMethod: {
                std::vector<Value> arguments(static_cast<std::size_t>(instruction.operand));
                for (std::size_t i = arguments.size(); i > 0; --i) arguments[i - 1] = pop();
                auto callee = pop();
                auto receiver = instruction.op == Op::CallMethod ? pop() : Value{};
                stack.push_back(call(callee, receiver, arguments)); break;
            }
            case Op::Jump: pc = static_cast<std::size_t>(instruction.operand); break;
            case Op::JumpIfFalse:
                if (!truthy(pop())) pc = static_cast<std::size_t>(instruction.operand);
                break;
            case Op::JumpIfTrue:
                if (truthy(pop())) pc = static_cast<std::size_t>(instruction.operand);
                break;
            case Op::Return: return pop();
            }
        }
        throw std::runtime_error("internal error: bytecode ended without return");
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string(error.what()) + "\n  at " + code.name + ":" + std::to_string(line));
    }
}
Value Runtime::run(const std::shared_ptr<FunctionCode>& code) {
    if (!code) throw std::runtime_error("internal error: missing script bytecode");
    remainingInstructions = limits.instructions;
    return execute(*code, global);
}
Value Runtime::run(const std::string& source) {
    auto program = Parser(tokenize(source)).parse();
    return run(Compiler().compile(program));
}
