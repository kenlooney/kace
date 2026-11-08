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
enum class StatementKind {
    Label,
    Instruction,
    Directive,
    JumpShort,
    JumpNear,
    JumpFar,
    Times,
};
struct Statement {
    std::string name;
    std::size_t line;
    DataType type = DataType::None;
    Expression expression;
    std::uint64_t value = 0;
    std::string reg;
    std::size_t offset = 0;
    StatementKind kind = StatementKind::Instruction;
    std::string target; // For jump statements, the label or address to jump to
    std::uint64_t farSegment = 0;
    std::uint64_t farOffset = 0;
    std::uint64_t count = 0;
    Expression countExpression; // For times directive, the expression representing the repeat count
};

bool parse_statement(const std::vector<Token>& tokens, Statement& statement, std::string& error);
