#include <config.h>
#include <lexer.h>
#include <parse.h>
#include <runtime.h>

#include <iostream>

using namespace std;

void PrintInfo() {
    cout << PROJECT_NAME << " version: "sv << PROJECT_VER << endl;
}

void RunMythonProgram(istream &input, ostream &output) {
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context{output};
    runtime::Closure closure;
    program->Execute(closure, context);
}

int main() {
    PrintInfo();
    try {
        RunMythonProgram(cin, cout);
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}