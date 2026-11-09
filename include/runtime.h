#pragma once
#include <bytecode.h>
#include <cstddef>
#include <memory>
#include <ostream>
#include <unordered_map>


struct Binding { Value value; bool constant = false; bool initialized = false; };
struct Environment {
    Environment* parent = nullptr;
    std::unordered_map<std::string, Binding> bindings;
};
struct Function {
    std::shared_ptr<FunctionCode> code;
    Environment* closure = nullptr;
    bool nativeLog = false;
    std::size_t calls = 0; // A future JIT can use this as a warm-up counter.
};
struct Limits {
    std::size_t instructions = 1000000;
    std::size_t allocations = 100000;
    std::size_t callDepth = 128;
};
class Runtime {
public:
    explicit Runtime(std::ostream& output, Limits limits = {});
    Value run(const std::shared_ptr<FunctionCode>& code);
    Value run(const std::string& source);
private:
    Environment* newEnvironment(Environment* parent);
    Object* newObject();
    Function* newFunction(std::shared_ptr<FunctionCode> code, Environment* closure);
    void allocation();
    void declare(Environment* env, const std::string& name, bool constant);
    Binding& binding(Environment* env, const std::string& name);
    Value load(Environment* env, const std::string& name);
    void initialize(Environment* env, const std::string& name, Value value);
    void store(Environment* env, const std::string& name, const Value& value);
    Value getProperty(const Value& object, const Value& key);
    void setProperty(const Value& object, const Value& key, const Value& value);
    Value call(const Value& callee, const Value& receiver, const std::vector<Value>& arguments);
    Value execute(const FunctionCode& code, Environment* environment);
    std::ostream& output;
    Limits limits;
    std::size_t remainingInstructions = 0;
    std::size_t depth = 0;
    std::vector<std::unique_ptr<Environment>> environments;
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Function>> functions;
    Environment* global = nullptr;
};
