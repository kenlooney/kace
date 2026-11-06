#include <kace/source.hpp>
#include <iostream>
#include <kasm/diagnostic.hpp>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: kasm_diagnostic_test <source-file>\n";
        return 1;
    }

    Source source;
    if (!Source::load(argv[1], source)) {
        std::cerr << "Could not open: " << argv[1] << "\n";
        return 1;
    }
    KasmcoreDiagnostic diag(
        "An example diagnostic",
        Span{source.name.c_str(), 1, 1}
    );
    diag.report();

    return 0;
}