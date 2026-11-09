#pragma once
#include <string>
#include <unordered_map>
#include <variant>

using namespace std;

namespace hornet{
    struct Object;
    struct Function;
    struct Null {};
    using Value = variant<std::monostate, Null, bool, double, std::string, Object*, Function*>;
    struct Object { 
        std::unordered_map<std::string, Value> properties; 
    };

    string toString(const Value& value);
    double toNumber(const Value& value);
    bool truthy(const Value& value);
    bool strictEqual(const Value& left, const Value& right);
    Value binaryOperation(
        const string& op, 
        const Value& left, 
        const Value& right
    );
    Value unaryOperation(
        const string& op, 
        const Value& value
    );
}