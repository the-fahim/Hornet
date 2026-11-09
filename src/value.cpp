#include <value.h>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>


std::string toString(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return "undefined";
    if (std::holds_alternative<Null>(value)) return "null";
    if (const auto* b = std::get_if<bool>(&value)) return *b ? "true" : "false";
    if (const auto* n = std::get_if<double>(&value)) {
        if (std::isnan(*n)) return "NaN";
        if (std::isinf(*n)) return *n < 0 ? "-Infinity" : "Infinity";
        if (*n == 0) return "0";
        std::ostringstream out;
        out << std::setprecision(15) << *n;
        return out.str();
    }
    if (const auto* s = std::get_if<std::string>(&value)) return *s;
    if (std::holds_alternative<Object*>(value)) return "[object Object]";
    return "[function]";
}

double toNumber(const Value& value) {
    if (const auto* n = std::get_if<double>(&value)) return *n;
    if (const auto* b = std::get_if<bool>(&value)) return *b ? 1 : 0;
    if (std::holds_alternative<Null>(value)) return 0;
    if (std::holds_alternative<std::monostate>(value)) return std::numeric_limits<double>::quiet_NaN();
    if (const auto* s = std::get_if<std::string>(&value)) {
        const auto first = s->find_first_not_of(" \t\r\n\f\v");
        if (first == std::string::npos) return 0;
        const auto last = s->find_last_not_of(" \t\r\n\f\v");
        const auto text = s->substr(first, last - first + 1);
        if (text == "Infinity" || text == "+Infinity") return std::numeric_limits<double>::infinity();
        if (text == "-Infinity") return -std::numeric_limits<double>::infinity();
        // Only decimal conversion is supported in this first runtime.
        bool digit = false;
        std::size_t i = (text[0] == '+' || text[0] == '-') ? 1 : 0;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') { digit = true; ++i; }
        if (i < text.size() && text[i] == '.') {
            ++i;
            while (i < text.size() && text[i] >= '0' && text[i] <= '9') { digit = true; ++i; }
        }
        if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
            ++i;
            if (i < text.size() && (text[i] == '+' || text[i] == '-')) ++i;
            const auto start = i;
            while (i < text.size() && text[i] >= '0' && text[i] <= '9') ++i;
            if (start == i) digit = false;
        }
        if (!digit || i != text.size()) return std::numeric_limits<double>::quiet_NaN();
        try { return std::stod(text); }
        catch (const std::out_of_range&) {
            // strtod handles underflow to zero as well as overflow to infinity.
            return std::strtod(text.c_str(), nullptr);
        }
    }
    throw std::runtime_error("TypeError: object-to-primitive conversion is not implemented");
}

bool truthy(const Value& value) {
    if (std::holds_alternative<std::monostate>(value) || std::holds_alternative<Null>(value)) return false;
    if (const auto* b = std::get_if<bool>(&value)) return *b;
    if (const auto* n = std::get_if<double>(&value)) return *n != 0 && !std::isnan(*n);
    if (const auto* s = std::get_if<std::string>(&value)) return !s->empty();
    return true;
}
bool strictEqual(const Value& left, const Value& right) {
    if (left.index() != right.index()) return false;
    if (std::holds_alternative<std::monostate>(left) || std::holds_alternative<Null>(left)) return true;
    if (const auto* b = std::get_if<bool>(&left)) return *b == std::get<bool>(right);
    if (const auto* n = std::get_if<double>(&left)) return *n == std::get<double>(right);
    if (const auto* s = std::get_if<std::string>(&left)) return *s == std::get<std::string>(right);
    if (const auto* o = std::get_if<Object*>(&left)) return *o == std::get<Object*>(right);
    return std::get<Function*>(left) == std::get<Function*>(right);
}
static bool nullish(const Value& value) {
    return std::holds_alternative<Null>(value) || std::holds_alternative<std::monostate>(value);
}
static bool looseEqual(const Value& left, const Value& right) {
    if (left.index() == right.index()) return strictEqual(left, right);
    // Objects and functions are separate C++ alternatives but both are JS
    // reference values. Distinct kinds cannot be the same reference.
    const bool leftReference = std::holds_alternative<Object*>(left) || std::holds_alternative<Function*>(left);
    const bool rightReference = std::holds_alternative<Object*>(right) || std::holds_alternative<Function*>(right);
    if (leftReference && rightReference) return false;
    if (nullish(left) && nullish(right)) return true;
    if (std::holds_alternative<bool>(left)) return looseEqual(Value(toNumber(left)), right);
    if (std::holds_alternative<bool>(right)) return looseEqual(left, Value(toNumber(right)));
    if ((std::holds_alternative<double>(left) && std::holds_alternative<std::string>(right)) ||
        (std::holds_alternative<std::string>(left) && std::holds_alternative<double>(right)))
        return toNumber(left) == toNumber(right);
    if ((std::holds_alternative<Object*>(left) || std::holds_alternative<Function*>(left)) && !nullish(right))
        throw std::runtime_error("TypeError: object coercion is not implemented");
    if ((std::holds_alternative<Object*>(right) || std::holds_alternative<Function*>(right)) && !nullish(left))
        throw std::runtime_error("TypeError: object coercion is not implemented");
    return false;
}
Value binaryOperation(const std::string& op, const Value& left, const Value& right) {
    if (op == "===") return strictEqual(left, right);
    if (op == "!==") return !strictEqual(left, right);
    if (op == "==") return looseEqual(left, right);
    if (op == "!=") return !looseEqual(left, right);
    if (std::holds_alternative<Object*>(left) || std::holds_alternative<Object*>(right) ||
        std::holds_alternative<Function*>(left) || std::holds_alternative<Function*>(right))
        throw std::runtime_error("TypeError: object coercion is not implemented");
    if (op == "+" && (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)))
        return toString(left) + toString(right);
    const auto* a = std::get_if<std::string>(&left);
    const auto* b = std::get_if<std::string>(&right);
    if (a && b) {
        if (op == "<") return *a < *b;
        if (op == "<=") return *a <= *b;
        if (op == ">") return *a > *b;
        if (op == ">=") return *a >= *b;
    }
    const double x = toNumber(left), y = toNumber(right);
    if (op == "+") return x + y;
    if (op == "-") return x - y;
    if (op == "*") return x * y;
    if (op == "/") return x / y;
    if (op == "%") return std::fmod(x, y);
    if (op == "<") return x < y;
    if (op == "<=") return x <= y;
    if (op == ">") return x > y;
    if (op == ">=") return x >= y;
    throw std::runtime_error("internal error: unknown binary operator");
}
Value unaryOperation(const std::string& op, const Value& value) {
    if (op == "!") return !truthy(value);
    if (op == "-") return -toNumber(value);
    return toNumber(value);
}
