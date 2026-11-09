#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <kace/symbols.hpp>

class Disassembler {
public:
    virtual ~Disassembler() = default;
    virtual bool disassemble(const std::vector<std::uint8_t>& bytes,
                             const SymbolTable& symbols, std::string& text,
                             std::string& error) const = 0;
};
