#include "kace/source.hpp"
#include "kace/statement.hpp"
#include "kasm/kasmsemantic.hpp"
#include "kasm/diagnostic.hpp"
#include "kasm/kasmparser.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>

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

    std::vector<std::uint8_t> bytes;
    for (const auto& statementPtr : statements) {
        const Statement& statement = *statementPtr;
        if (statement.kind == StatementKind::Label) continue;
        std::uint8_t byte = 0;
        const auto* kasmStatement = dynamic_cast<const KasmStatement*>(statementPtr.get());
        if (!kasmStatement || !check_kasm_byte(*kasmStatement, byte, error)) {
            std::cerr << error << "\n";
            return 1;
        }
        bytes.push_back(byte);
      
    }
    std::cout << "collected " << bytes.size() << " bytes\n";
    std::cout << "hex: ";
    std::cout << std::uppercase << std::hex << std::setfill('0');
    for (std::uint8_t byte : bytes) {
        std::cout << std::setw(2)
                  << static_cast<unsigned>(byte) << " ";
    }
    std::cout << std::dec << "\n";
    const std::filesystem::path output_path =
        std::filesystem::absolute("collected_bytes.bin");
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Could not open output file: "
                  << output_path.string() << "\n";
        return 1;
    }

    out.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
    if (!out) {
        std::cerr << "Could not write output file: "
                  << output_path.string() << "\n";
        return 1;
    }
    std::cout << "wrote " << output_path.string() << "\n";
   
    return 0;
}
