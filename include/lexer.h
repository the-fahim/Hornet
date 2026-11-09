#include<string>
#include<vector>
using namespace std;

enum class TokenKind { Name, Number, String, Symbol, End };
struct Token {
    TokenKind kind;
    string text;
    int line = 1;
    int column = 1;
};
vector<Token> tokenize(const string& source);
[[noreturn]] void syntaxError(const Token& token, const string& message);
