#pragma once

#include <cstddef>
#include <cstdint>
#include <kace/ast.hpp>
#include <string>
#include <vector>
#include <kace/lexer.hpp>

enum class DataType {
    None,
    Byte,
    Word,
    Dword,
    Qword
};

struct Statement {
    std::string name;
    std::size_t line;
    DataType type = DataType::None;
    Expression expression;
    std::uint64_t value = 0;
    std::string reg;
};

bool parse_statement(const std::vector<Token>& tokens, Statement& statement, std::string& error);
