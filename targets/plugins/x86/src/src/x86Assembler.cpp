#include <x86/x86Assembler.hpp>
#include <x86/x86.hpp>
#include <kace/fixup.hpp>
#include <kace/parser.hpp>
#include <kace/semantic.hpp>
#include <algorithm>
#include <limits>
#include <iostream>

bool X86Assembler::assemble(const std::vector<Statement> &statements,
                            std::vector<std::uint8_t> &bytes,
                            SymbolTable &symbols, std::string &error) const
{
    // KEEP: First pass: calculate offsets and collect fixups
    std::cout << "Starting first pass" << std::endl;
    std::vector<std::uint8_t> output;
    SymbolTable labels;
    std::vector<JumpFixup> fixups;
    auto fail = [&](const Statement &statement, const std::string &message)
    {
        error = statement.sourceName + ":" + std::to_string(statement.line) + ": " + message;
        return false;
    };
    for (const Statement &statement : statements)
    {
        const std::size_t offset = output.size();
        if (statement.kind == StatementKind::Directive &&
            statement.directiveStage == DirectiveStage::Assembly &&
            statement.directiveKind == DirectiveKind::AssertPosition)
        {
            if (offset != statement.assertPosition)
                return fail(statement, "Assert position failed: expected " + std::to_string(statement.assertPosition) + ", got " + std::to_string(offset));
            continue;
        }
        if (statement.kind == StatementKind::Label)
        {
            if (!labels.addLabel(statement.name, offset))
                return fail(statement, "Duplicate label: " + statement.name);
            continue;
        }

        if (statement.kind == StatementKind::JumpShort || statement.kind == StatementKind::JumpNear)
        {
            const bool near = statement.kind == StatementKind::JumpNear;
            const std::size_t displacementSize = near ? near_displacement_size(cpu_.getMode()) : 1;

            if (displacementSize == 0)
                return fail(statement, "Invalid CPU mode for near jump");

            fixups.push_back({statement.target, offset, statement.line, near ? JumpKind::Near : JumpKind::Short, statement.sourceName});
            output.insert(output.end(), 1 + displacementSize, 0);
            continue;
        }

        if (statement.kind == StatementKind::JumpFar)
        {
            std::vector<std::uint8_t> encoded;
            if (!cpu_.encoder()->encode_far_jump(statement.farSegment, statement.farOffset, encoded, error))
                return fail(statement, error);
            output.insert(output.end(), encoded.begin(), encoded.end());
            continue;
        }
        std::uint8_t byte = 0;
        if (statement.kind == StatementKind::Times)
        {
            if (!check_byte(statement, byte, error))
                return fail(statement, error);
            Parser evaluator(std::vector<Token>{});
            PositionContext positions{offset, 0};
            std::uint64_t count = 0;
            if (!evaluator.evaluate(statement.countExpression, statement.countExpression.root, count, error, &positions))
                return fail(statement, error);
            if (count > 1000000)
                return fail(statement, "Repeat count exceeds the limit");
            output.insert(output.end(), static_cast<std::size_t>(count), byte);
            continue;
        }
        if (statement.type != DataType::None)
        {
            if (!check_byte(statement, byte, error))
                return fail(statement, error);
        }
        else if (!statement.expression.nodes.empty() || !cpu_.encoder()->encode_no_operand(statement.name, byte))
        {
            return fail(statement, "Unsupported instruction or operands: " + statement.name);
        }
        output.push_back(byte);
    }
    std::cout << "First pass complete" << std::endl;
    if (output.size() > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
    {
        error = "Output too large";
        return false;
    }

    // KEEP: Second pass: resolve jump fixups
    std::cout << "Starting second pass" << std::endl;
    for (const JumpFixup &fixup : fixups)
    {
        const Symbol *label = labels.getSymbol(fixup.target);
        const std::string location = fixup.sourceName + ":" + std::to_string(fixup.line) + ": ";
        if (!label)
        {
            error = location + "Unknown label: " + fixup.target;
            return false;
        }
        const std::size_t size = fixup.kind == JumpKind::Near
                                     ? 1 + near_displacement_size(cpu_.getMode())
                                     : 2;
        const auto displacement = static_cast<std::int64_t>(label->offset) - static_cast<std::int64_t>(fixup.offset) - static_cast<std::int64_t>(size);
        std::vector<std::uint8_t> encoded;
        const bool ok = fixup.kind == JumpKind::Near
                            ? cpu_.encoder()->encode_near_jump(displacement, encoded, error)
                            : cpu_.encoder()->encode_short_jump(displacement, encoded, error);
        if (!ok || encoded.size() != size)
        {
            error = location + (error.empty() ? "Invalid jump encoding" : error);
            return false;
        }
        std::copy(encoded.begin(), encoded.end(), output.begin() + fixup.offset);
    }
    std::cout << "Second pass complete" << std::endl;
    bytes = std::move(output);
    symbols = std::move(labels);
    return true;
}
