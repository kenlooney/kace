#pragma once
#include <kace/disassembler.hpp>
class X86;

class X86Disassembler : public Disassembler {
public:
    explicit X86Disassembler(const X86& cpu) : cpu_(cpu) {}
    bool disassemble(const std::vector<std::uint8_t>& bytes,
                     const SymbolTable& symbols, std::string& text,
                     std::string& error) const override;
private:
    const X86& cpu_;
};
