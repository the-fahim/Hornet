#pragma once
#include<value.h>

#include <memory>
#include <ostream>
#include <vector>


enum class Op {
    Constant, Pop, Dup, Load, Declare, Initialize, Store,
    EnterScope, LeaveScope, Unary, Binary,
    NewObject, GetProperty, SetProperty, MakeFunction,
    Call, CallMethod, Jump, JumpIfFalse, JumpIfTrue, Return
};
struct Instruction {
    Op op;
    int operand = 0;
    std::string text;
    int line = 1;
};
struct FunctionCode {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<Instruction> instructions;
    std::vector<Value> constants;
    std::vector<std::shared_ptr<FunctionCode>> functions;
};
void dumpBytecode(const FunctionCode& code, std::ostream& out, int indentation = 0);
