#include <kace/source.hpp>
#include <kasm/diagnostic.hpp>
#include <kasm/kasmlexer.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        KasmDiagnostic diagnostic(
            "Expected a source file",
            "<command line>",
            1,
            1
        );
        diagnostic.report();
        return 1;
    }

    Source source;
    if (!Source::load(argv[1], source)) {
        KasmDiagnostic diagnostic("Could not open file", argv[1], 1, 1);
        diagnostic.report();
        return 1;
    }

    KasmLexer lexer;
    for (const Token& token : lexer.tokenize(source.text)) {
        std::cout << token.span.begin << ".." << token.span.end << " "
                  << token.kind << " " << token.text << "\n";
    }
    return 0;
}
