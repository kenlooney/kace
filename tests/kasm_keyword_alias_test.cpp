#include <kasm/kasmparser.hpp>
#include <kace/assembler.hpp>

#include <iostream>

class StatementProbe : public Assembler {
public:
    bool assemble(const std::vector<std::unique_ptr<Statement>>& statements,
                  std::vector<std::uint8_t>&, SymbolTable&, std::string&) const override {
        if (statements.empty()) return false;
        const auto* statement = dynamic_cast<const KasmStatement*>(statements[0].get());
        return statement && statement->moveKind == MoveKind::ImmediateToRegister;
    }
};

int main() {
    const std::vector<Token> tokens;
    KasmParser parser(tokens);
    parser.keywords_table.addto_alias("mov", "transfer");
    Source source{"aliases", "mov ax, 10 sum 2\nmove ax, 10 sum 2\ncopy ax, 10 sum 2\ntransfer ax, 10 sum 2\n"};
    std::vector<std::unique_ptr<Statement>> statements;
    std::string error;
    if (!parser.parse(source, statements, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    if (statements.size() != 4) return 1;
    StatementProbe probe;
    const Assembler& assembler = probe;
    std::vector<std::uint8_t> output;
    SymbolTable symbols;
    if (!assembler.assemble(statements, output, symbols, error)) {
        std::cerr << "KASM move kind did not survive the assembler contract\n";
        return 1;
    }
    for (const auto& statementPtr : statements) {
        const Statement& statement = *statementPtr;
        const auto* kasmStatement = dynamic_cast<const KasmStatement*>(statementPtr.get());
        if (!kasmStatement || kasmStatement->moveKind != MoveKind::ImmediateToRegister ||
            statement.name() != "mov" || kasmStatement->reg != "ax" || kasmStatement->value != 12) {
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
    source.text = "jz done\nifequal done\njnz done\nifnotequal done\n"
                  "jc done\nifcarry done\njnc done\nifnotcarry done\n";
    const char* expectedConditions[] = {"jz", "jz", "jnz", "jnz", "jc", "jc", "jnc", "jnc"};
    if (!parser.parse(source, statements, error) || statements.size() != 8) {
        std::cerr << "Conditional jump parsing failed: " << error << '\n';
        return 1;
    }
    for (std::size_t i = 0; i < statements.size(); ++i) {
        const auto* statement = dynamic_cast<const KasmStatement*>(statements[i].get());
        if (!statement || !statement->has_kind("ConditionalJump") ||
            statement->name() != expectedConditions[i] || statement->target != "done") {
            std::cerr << "Conditional jump or alias parsed incorrectly at index " << i << '\n';
            return 1;
        }
    }
    for (const std::string& invalid : {"jz", "ifequal 42", "ifcarry done extra", "jnc ,"}) {
        source.text = invalid;
        if (parser.parse(source, statements, error) || error.find("aliases:1:") == std::string::npos) {
            std::cerr << "Invalid conditional jump accepted: " << invalid << '\n';
            return 1;
        }
    }
    source.text = "jmp short done\njump short done\ngoto short done\n";
    if (!parser.parse(source, statements, error) || statements.size() != 3 ||
        !statements[0]->has_kind("JumpShort") ||
        !statements[1]->has_kind("JumpShort") ||
        !statements[2]->has_kind("JumpShort")) {
        std::cerr << "Jump kinds or aliases did not parse correctly: " << error << '\n';
        return 1;
    }
    for (const auto& statementPtr : statements) {
        const Statement& statement = *statementPtr;
        const auto* kasmStatement = dynamic_cast<const KasmStatement*>(statementPtr.get());
        if (!kasmStatement || statement.name() != "jmp" || kasmStatement->target != "done") return 1;
    }
    source.text = "jmp near done";
    if (!parser.parse(source, statements, error) || statements.size() != 1 ||
        !statements[0]->has_kind("JumpNear") || (dynamic_cast<const KasmStatement*>(statements[0].get()) == nullptr ||
         dynamic_cast<const KasmStatement*>(statements[0].get())->target != "done")) {
        std::cerr << "Near jump did not parse correctly\n";
        return 1;
    }
    source.text = "jmp far 0x1234:0b1010";
    if (!parser.parse(source, statements, error) || statements.size() != 1 ||
        !statements[0]->has_kind("JumpFar") ||
        (dynamic_cast<const KasmStatement*>(statements[0].get()) == nullptr ||
         dynamic_cast<const KasmStatement*>(statements[0].get())->farSegment != 0x1234 ||
         dynamic_cast<const KasmStatement*>(statements[0].get())->farOffset != 10)) {
        std::cerr << "Far jump did not parse correctly: " << error << '\n';
        return 1;
    }
    for (const std::string& invalid : {"jmp short", "jmp short 42", "jmp short done extra", "jmp wide done", "jmp far done", "jmp far 1:", "jmp far 1:0x", "jmp far 1:2 extra"}) {
        source.text = invalid;
        if (parser.parse(source, statements, error)) {
            std::cerr << "Invalid jump accepted: " << invalid << '\n';
            return 1;
        }
    }
    source.text = "int 0x21\ninterrupt 0x21\n";
    if (!parser.parse(source, statements, error) || statements.size() != 2) {
        std::cerr << "Interrupt parsing failed: " << error << '\n';
        return 1;
    }
    for (const auto& statementPtr : statements) {
        const auto* statement = dynamic_cast<const KasmStatement*>(statementPtr.get());
        if (!statement || !statement->has_kind("Interrupt") ||
            statement->name() != "int" || statement->interruptNumber != "0x21") {
            std::cerr << "Interrupt or alias parsed incorrectly\n";
            return 1;
        }
    }
    source.text = "mov ax, [bx]";
    if (!parser.parse(source, statements, error) || statements.size() != 1) {
        std::cerr << "Single-base memory load parsing failed: " << error << '\n';
        return 1;
    }
    const auto* memory = dynamic_cast<const KasmStatement*>(statements[0].get());
    if (!memory || memory->moveKind != MoveKind::MemoryToRegister ||
        memory->reg != "ax" || memory->baseRegister != "bx" ||
        memory->addressExpression.nodes.empty() ||
        memory->addressExpression.nodes[memory->addressExpression.root].kind != "identifier" ||
        memory->addressExpression.nodes[memory->addressExpression.root].text != "bx") {
        std::cerr << "Single-base memory load or address tree was incorrect\n";
        return 1;
    }
    for (const std::string& address : {"ebx + ecx * 4", "ebx sum ecx mul 4"}) {
        source.text = "mov eax, [" + address + "]";
        if (!parser.parse(source, statements, error) || statements.size() != 1) {
            std::cerr << "Scaled address parsing failed: " << error << '\n';
            return 1;
        }
        const auto* scaled = dynamic_cast<const KasmStatement*>(statements[0].get());
        if (!scaled || scaled->moveKind != MoveKind::MemoryToRegister || scaled->reg != "eax" ||
            scaled->baseRegister != "ebx" || scaled->indexRegister != "ecx" || scaled->addressScale != 4) {
            std::cerr << "Scaled address fields were incorrect\n";
            return 1;
        }
    }
    source.text = "mov eax, [ebx + ecx * 3]";
    if (parser.parse(source, statements, error)) {
        std::cerr << "Invalid scale accepted\n";
        return 1;
    }
    return 0;
}
