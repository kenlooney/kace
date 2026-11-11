#pragma once
#include <kace/assembler.hpp>
#include <kace/fixup.hpp>
#include <x86/mode.hpp>
class X86;

struct X86JumpFixup : public JumpFixup {
    X86Mode mode;
};
class X86Assembler : public Assembler {
public:
    explicit X86Assembler(X86& cpu) : cpu_(cpu) {}
    bool assemble(const std::vector<std::unique_ptr<Statement>>& statements,
                  std::vector<std::uint8_t>& bytes,
                  SymbolTable& symbols, std::string& error) const override;
private:
    X86& cpu_;
};
