#pragma once

#include <cstddef>
#include <cstdint>
#include <kace/ast.hpp>
#include <string>
#include <vector>
#include <kace/lexer.hpp>

enum class DataType
{
    None,
    Byte,
    Word,
    Dword,
    Qword
};
enum class DirectiveKind
{
    None,
    Include,
    AssertPosition
};
enum class DirectiveStage
{
    SourceExpansion,
    Assembly
};

class StatementKind
{
public:
    StatementKind(const std::string &name) : name_(name) {}
    virtual ~StatementKind() = default;
    const std::string &name() const { return name_; }

private:
    std::string name_;
};
class Statement
{
public:
    Statement() = default;
    virtual ~Statement() = default;

    void set_name(const std::string &name)
    {
        name_ = name;
    }
    const std::string &name() const
    {
        return name_;
    }

    const std::vector<StatementKind> &kinds() const
    {
        return kinds_;
    }

    void add_kind(const StatementKind &kind)
    {
        kinds_.push_back(kind);
    }
    bool has_kind(const StatementKind &kind) const
    {
        return has_kind(kind.name());
    }
    bool has_kind(const std::string &name) const
    {
        for (const auto &k : kinds_)
        {
            if (k.name() == name)
            {
                return true;
            }
        }
        return false;
    }
    virtual bool parse_statement(const std::vector<Token> &tokens, std::string &error) = 0;

private:
    std::string name_;
    std::vector<StatementKind> kinds_;
};

// struct Statement {
//     virtual ~Statement() = default;

//     std::string name;
//     std::string sourceName;
//     std::size_t line;
//     DataType type = DataType::None;
//     Expression expression;
//     std::uint64_t value = 0;
//     std::string reg;
//     std::size_t offset = 0;
//     StatementKind kind{};
//     std::string target; // For jump statements, the label or address to jump to
//     std::uint64_t farSegment = 0;
//     std::uint64_t farOffset = 0;
//     std::uint64_t count = 0;
//     Expression countExpression; // For times directive, the expression representing the repeat count
//     DirectiveKind directiveKind = DirectiveKind::None; // The kind of directive, if this statement is a directive
//     std::string includePath; // For include directive, the path of the included file
//     DirectiveStage directiveStage = DirectiveStage::SourceExpansion; // The stage at which the directive is processed
//     std::uint64_t assertPosition = 0; // For assert position directive, the expected position
//     std::string codeSegment; // For code segment statements, the segment type (e.g., "code16", "code32", "code64")
// };


