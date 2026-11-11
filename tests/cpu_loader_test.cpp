#include <kace/loader.hpp>
#include <kasm/kasmstatement.hpp>

#include <cstdint>
#include <iostream>
#include <string>

class ExampleAssembler : public Assembler {
public:
    bool assemble(const std::vector<std::unique_ptr<Statement>>&, std::vector<std::uint8_t>& bytes,
                  SymbolTable&, std::string&) const override {
        bytes = {0x42};
        return true;
    }
};

class TestKasmStatement : public KasmStatement {
public:
    bool parse_statement(const std::vector<Token>&, std::string& error) override {
        error = "Parsing is not used by this loader test";
        return false;
    }
};

class ExampleDisassembler : public Disassembler {
public:
    bool disassemble(const std::vector<std::uint8_t>&, const SymbolTable&,
                     std::string& text, std::string&) const override {
        text = "example\n";
        return true;
    }
};

class ExampleCpu : public Cpu {
public:
    const Assembler* assembler() const override { return &assembler_; }
    const Disassembler* disassembler() const override { return &disassembler_; }
    const RegisterFile& registers() const override { return registers_; }
private:
    ExampleAssembler assembler_;
    ExampleDisassembler disassembler_;
    RegisterFile registers_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) return 1;
    ExampleCpu example;
    std::vector<std::uint8_t> exampleBytes;
    SymbolTable exampleSymbols;
    std::string exampleError;
    if (!example.assembler()->assemble({}, exampleBytes, exampleSymbols, exampleError) ||
        exampleBytes != std::vector<std::uint8_t>{0x42}) {
        std::cerr << "Independent assembler contract failed\n";
        return 1;
    }
    std::string exampleText;
    if (!example.disassembler()->disassemble(exampleBytes, exampleSymbols, exampleText, exampleError) ||
        exampleText != "example\n") {
        std::cerr << "Independent disassembler contract failed\n";
        return 1;
    }
    Loader loader;
    std::string error;
    if (loader.open(std::string(argv[1]) + ".missing", error) || error.empty()) {
        std::cerr << "Missing plugin was not rejected\n";
        return 1;
    }
    if (loader.open(argv[2], error) || error.find("factory") == std::string::npos) {
        std::cerr << "Library without a plugin factory was not rejected: " << error << '\n';
        return 1;
    }
    if (!loader.open(argv[1], error) || !loader.cpu()) {
        std::cerr << "Could not load x86 after a failed load: " << error << '\n';
        return 1;
    }
    auto statement = std::make_unique<TestKasmStatement>();
    statement->set_name("nop");
    statement->sourceName = "plugin-test";
    statement->line = 1;
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::move(statement));
    std::vector<std::uint8_t> output;
    SymbolTable symbols;
    if (!loader.cpu()->assembler() ||
        !loader.cpu()->assembler()->assemble(statements, output, symbols, error) ||
        output != std::vector<std::uint8_t>{0x90}) {
        std::cerr << "Loaded x86 plugin did not assemble nop: " << error << '\n';
        return 1;
    }
    if (loader.open(argv[1], error)) {
        std::cerr << "Loader accepted a second plugin while one was open\n";
        return 1;
    }
    return 0;
}
