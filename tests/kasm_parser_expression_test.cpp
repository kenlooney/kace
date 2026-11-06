#include "kace/source.hpp"
#include "kasm/diagnostic.hpp"
#include "kasm/kasmlexer.hpp"
#include "kasm/kasmparser.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    KasmDiagnostic* diag = nullptr;

    if (argc != 2)
    {
        diag = new KasmDiagnostic(
            "An example diagnostic",
            "<command line>",
            1,
            1);
        diag->report();
        return 1;
    }
    Source source;
    if (!Source::load(argv[1], source))
    {
        diag = new KasmDiagnostic(
            "An example diagnostic",
            source.name.c_str(),
            1,
            1);
        diag->report();
        return 1;
    }
    std::uint64_t value = 0;
    std::string error;
    KasmLexer lexer;
    const std::vector<Token>& tokens = lexer.tokenize(source.text);
    KasmParser parser(tokens);
    if (!parser.parse_expression(value, error))
    {
        std::cerr << source.name << ": " << error << "\n";
        return 1;
    }
    std::cout << value << "\n";
    return 0;
}
