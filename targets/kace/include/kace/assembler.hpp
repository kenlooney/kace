#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <kace/statement.hpp>
#include <kace/symbols.hpp>

class Assembler {
public:
    virtual ~Assembler() = default;
    virtual bool assemble(const std::vector<std::unique_ptr<Statement>>& statements,
                          std::vector<std::uint8_t>& bytes,
                          SymbolTable& symbols, std::string& error) const = 0;
};
