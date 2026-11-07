#include <kasm/kasmparser.hpp>

#include <iostream>

int main() {
    const std::vector<Token> tokens;
    KasmParser parser(tokens);
    parser.keywords_table.addto_alias("mov", "transfer");
    Source source{"aliases", "mov ax, 10 sum 2\nmove ax, 10 sum 2\ncopy ax, 10 sum 2\ntransfer ax, 10 sum 2\n"};
    std::vector<Statement> statements;
    std::string error;
    if (!parser.parse(source, statements, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    if (statements.size() != 4) return 1;
    for (const auto& statement : statements) {
        if (statement.name != "mov" || statement.reg != "ax" || statement.value != 12) {
            std::cerr << "mov alias did not parse as mov\n";
            return 1;
        }
    }
    for (const std::string& invalid : {"mov", "move ax", "copy ax 12", "transfer 12, 3", "mov ax, "}) {
        source.text = invalid;
        if (parser.parse(source, statements, error) || error.find("aliases:1:") == std::string::npos) {
            std::cerr << "Invalid mov accepted: " << invalid << '\n';
            return 1;
        }
    }
    return 0;
}
