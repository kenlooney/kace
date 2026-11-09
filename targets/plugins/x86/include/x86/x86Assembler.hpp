#pragma once
#include <kace/assembler.hpp>
class X86;

class X86Assembler : public Assembler {
public:
    explicit X86Assembler(const X86& cpu) : cpu_(cpu) {}
    bool assemble(const std::vector<Statement>& statements,
                  std::vector<std::uint8_t>& bytes,
                  SymbolTable& symbols, std::string& error) const override;
private:
    const X86& cpu_;
};
