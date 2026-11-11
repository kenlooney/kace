#include "kace/source.hpp"
#include "kace/statement.hpp"
#include "kasm/kasmsemantic.hpp"
#include "kasm/diagnostic.hpp"
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
    std::vector<Token> no_tokens;
    KasmParser parser(no_tokens);
    std::vector<std::unique_ptr<Statement>> statements;
    std::string error;
    if (!parser.parse(source, statements, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    if (statements.empty())
    {
        std::cerr << source.name << ": no statements parsed\n";
        return 1;
    }

    for (const auto& statementPtr : statements) {
        const Statement& statement = *statementPtr;
        if (statement.has_kind("Label")) continue;
        std::uint8_t byte = 0;
        const auto* kasmStatement = dynamic_cast<const KasmStatement*>(statementPtr.get());
        if (!kasmStatement || !check_kasm_byte(*kasmStatement, byte, error)) {
            std::cerr << error << "\n";
            return 1;
        }
        std::cout << "line " << kasmStatement->line
                  << ": Checked byte: "
                  << static_cast<unsigned>(byte) << "\n";
    }
    std::cout << "parsed " << statements.size() << " statements\n";
    return 0;
}
