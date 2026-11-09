#include <compiler.h>
#include <runtime.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc, char** argv) {
    bool dump = false;
    std::string path;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--version") { std::cout << "Hornet 0.1.0\n"; return 0; }
        if (argument == "--dump-bytecode") dump = true;
        else if (argument == "--help" || argument == "-h") {
            std::cout << "Usage: hornet [--dump-bytecode] <file.js>\n"
                         "       hornet --version\n"
                         "--dump-bytecode prints bytecode without executing the script.\n";
            return 0;
        } else if (path.empty() && !argument.empty() && argument[0] != '-') path = argument;
        else { std::cerr << "Unknown or extra argument: " << argument << '\n'; return 1; }
    }
    if (path.empty()) { std::cerr << "Usage: hornet [--dump-bytecode] <file.js>\n"; return 1; }
    try {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("cannot open file: " + path);
        std::ostringstream source;
        source << input.rdbuf();
        auto program = Parser(tokenize(source.str())).parse();
        auto code = Compiler().compile(program);
        if (dump) dumpBytecode(*code, std::cout);
        else {
            Runtime runtime(std::cout);
            runtime.run(code);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << path << ": " << error.what() << '\n';
        return 1;
    }
}
