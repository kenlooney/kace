/*
 * Copyright (C) 2026 Kenneth Looney
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <x86/x86Assembler.hpp>
#include <x86/x86.hpp>
#include <kace/fixup.hpp>
#include <kace/parser.hpp>
#include "kasm/kasmstatement.hpp"
#include <kasm/kasmsemantic.hpp>
#include <algorithm>
#include <limits>
#include <iostream>
#include <x86/mode.hpp>

bool X86Assembler::assemble(const std::vector<std::unique_ptr<Statement>> &statements,
                            std::vector<std::uint8_t> &bytes,
                            SymbolTable &symbols, std::string &error) const
{
    // KEEP: First pass: calculate offsets and collect fixups
    std::cout << "Starting first pass" << std::endl;
    // Start with the selected CPU mode
    X86Mode current_mode = this->cpu_.getMode();

    std::vector<std::uint8_t> output;
    SymbolTable labels;
    std::vector<X86JumpFixup> fixups;
    auto fail = [&](const KasmStatement &statement, const std::string &message)
    {
        error = statement.sourceName + ":" + std::to_string(statement.line) + ": " + message;
        return false;
    };
    for (const auto &statementPtr : statements)
    {
        const auto *kasmStatement = dynamic_cast<const KasmStatement *>(statementPtr.get());
        if (!kasmStatement)
        {
            error = "x86 assembler expected a KASM statement";
            return false;
        }
        const KasmStatement &statement = *kasmStatement;
        const std::size_t offset = output.size();
        auto recordBytes = [&]()
        {
            const std::size_t length = output.size() - offset;
            if (length != 0)
            {
                std::uint8_t mode = current_mode == X86Mode::Bits16 ? 16 : current_mode == X86Mode::Bits32 ? 32
                                                                                                           : 64;
                SymbolByteRange range{offset, length, statement.sourceName, statement.line, statement.column};
                range.metadata.tags.push_back({"x86.mode", {mode}});
                labels.addByteRange(range);
            }
        };

        if (statement.has_kind("CodeSegment"))
        {
            std::string segment = statement.codeSegment;
            if (segment.empty())
            {
                return fail(statement, "Code segment name is empty");
            }
            if (segment == "code16")
                current_mode = X86Mode::Bits16;
            else if (segment == "code32")
                current_mode = X86Mode::Bits32;
            else if (segment == "code64")
                current_mode = X86Mode::Bits64;
            else
                return fail(statement, "Unknown code segment mode: " + segment);
            continue;
        }

        if (statement.has_kind("Directive") &&
            statement.directiveStage == DirectiveStage::Assembly &&
            statement.directiveKind == DirectiveKind::AssertPosition)
        {
            if (offset != statement.assertPosition)
                return fail(statement, "Assert position failed: expected " + std::to_string(statement.assertPosition) + ", got " + std::to_string(offset));
            continue;
        }
        if (statement.has_kind("Label"))
        {

            if (!labels.addLabel(statement.name(), offset,
                                 statement.sourceName, statement.line, statement.column))
            {
                const Symbol *first = labels.getSymbol(statement.name());
                error = statement.sourceName + ":" +
                        std::to_string(statement.line) + ":" +
                        std::to_string(statement.column) +
                        ": Duplicate label: " + statement.name() +
                        " (first defined at " + first->sourceName + ":" +
                        std::to_string(first->line) + ":" +
                        std::to_string(first->column) + ")";

                return false;
            }

            continue;
        }

        if (statement.has_kind("ConditionalJump"))
        {
            fixups.push_back({{statement.target, offset, statement.line,
                               JumpKind::Short, statement.sourceName,
                               statement.name(), statement.targetSpan, statement.sourceLine},
                              current_mode});
            output.insert(output.end(), 2, 0);
            // NEW
            recordBytes();
            continue;
        }

        if (statement.has_kind("JumpShort") || statement.has_kind("JumpNear"))
        {
            const bool near = statement.has_kind("JumpNear");
            const std::size_t displacementSize = near ? near_displacement_size(current_mode) : 1;

            if (displacementSize == 0)
                return fail(statement, "Invalid CPU mode for near jump");

            fixups.push_back({{statement.target, offset, statement.line,
                               near ? JumpKind::Near : JumpKind::Short, statement.sourceName},
                              current_mode});
            output.insert(output.end(), 1 + displacementSize, 0);
            recordBytes();
            continue;
        }

        if (statement.has_kind("Interrupt"))
        {
            std::vector<std::uint8_t> encoded;
            if (!cpu_.encoder()->encode_interrupt(statement.interruptNumber, encoded, error))
                return fail(statement, error);
            output.insert(output.end(), encoded.begin(), encoded.end());
            recordBytes();
            continue;
        }

        if (statement.has_kind("Halt"))
        {
            std::vector<std::uint8_t> encoded;
            if (!cpu_.encoder()->encode_halt(encoded, error))
                return fail(statement, error);
            output.insert(output.end(), encoded.begin(), encoded.end());
            recordBytes();
            continue;
        }

        if (statement.has_kind("JumpFar"))
        {
            std::vector<std::uint8_t> encoded;
            if (!cpu_.encoder()->encode_far_jump(statement.farSegment, statement.farOffset, encoded, error))
                return fail(statement, error);
            output.insert(output.end(), encoded.begin(), encoded.end());
            recordBytes();
            continue;
        }

        if (statement.moveKind == MoveKind::MemoryToRegister)
        {
            cpu_.setMode(current_mode);
            std::vector<std::vector<std::uint8_t>> encoded;

            bool encodedOk;
            if (statement.hasDisplacement)
            {
                encodedOk = cpu_.encoder()->encode_displaced_memory_load(
                    statement.reg, statement.baseRegister, statement.displacement,
                    encoded, error);
            }
            else if (statement.indexRegister.empty())
            {
                encodedOk = cpu_.encoder()->encode_memory_register(
                    statement.name(), statement.reg, statement.baseRegister, encoded, error);
            }
            else
            {
                encodedOk = cpu_.encoder()->encode_scaled_memory_load(
                    statement.reg, statement.baseRegister, statement.indexRegister,
                    statement.addressScale, encoded, error);
            }

            if (!encodedOk)
                return fail(statement, error);

            for (const auto &part : encoded)
                output.insert(output.end(), part.begin(), part.end());

            recordBytes();
            continue;
        }
        if (statement.name() == "mov")
        {
            Parser evaluator(std::vector<Token>{});
            PositionContext positions{offset, 0};
            std::uint64_t value = 0;

            if (!evaluator.evaluate(statement.expression, statement.expression.root,
                                    value, error, &positions))
            {
                return fail(statement, error);
            }
            cpu_.setMode(current_mode);
            std::vector<std::vector<std::uint8_t>> encoded;

            if (!cpu_.encoder()->encode_immediate(
                    statement.name(), statement.reg, value, encoded, error))
            {
                return fail(statement, error);
            }
            for (const auto &part : encoded)
            {
                output.insert(output.end(), part.begin(), part.end());
            }
            // NEW
            recordBytes();
            continue;
        }
        std::uint8_t byte = 0;
        if (statement.has_kind("Times"))
        {
            if (!check_kasm_byte(*kasmStatement, byte, error))
                return fail(statement, error);
            Parser evaluator(std::vector<Token>{});
            PositionContext positions{offset, 0};
            std::uint64_t count = 0;
            if (!evaluator.evaluate(statement.countExpression, statement.countExpression.root, count, error, &positions))
                return fail(statement, error);
            if (count > 1000000)
                return fail(statement, "Repeat count exceeds the limit");
            output.insert(output.end(), static_cast<std::size_t>(count), byte);
            // NEW
            recordBytes();
            continue;
        }
        if (statement.type != DataType::None)
        {
            if (!check_kasm_byte(*kasmStatement, byte, error))
                return fail(statement, error);
        }
        else if (!statement.expression.nodes.empty() || !cpu_.encoder()->encode_no_operand(statement.name(), byte))
        {
            return fail(statement, "Unsupported instruction or operands: " + statement.name());
        }
        // NEW
        output.push_back(byte);
        recordBytes();
    }
    std::cout << "First pass complete" << std::endl;
    if (output.size() > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
    {
        error = "Output too large";
        return false;
    }

    // KEEP: Second pass: resolve jump fixups
    std::cout << "Starting second pass" << std::endl;
    for (const X86JumpFixup &fixup : fixups)
    {
        cpu_.setMode(fixup.mode);
        const Symbol *label = labels.getSymbol(fixup.target);
        const std::string location = fixup.sourceName + ":" + std::to_string(fixup.line) + ": ";
        if (!label)
        {
            error = location + "Unknown label: " + fixup.target;
            if (!fixup.sourceLine.empty() &&
                fixup.span.end > fixup.span.begin &&
                fixup.span.begin < fixup.sourceLine.size())
            {
                error += "\n" + fixup.sourceLine + "\n";
                error += std::string(fixup.span.begin, ' ') + "^";
            }
            return false;
        }
        const std::size_t size = fixup.kind == JumpKind::Near
                                     ? 1 + near_displacement_size(cpu_.getMode())
                                     : 2;
        const auto displacement = static_cast<std::int64_t>(label->offset) - static_cast<std::int64_t>(fixup.offset) - static_cast<std::int64_t>(size);

        std::vector<std::uint8_t> encoded;
        bool ok;
        if (!fixup.condition.empty())
            ok = cpu_.encoder()->encode_conditional_jump(fixup.condition, displacement, encoded, error);
        else if (fixup.kind == JumpKind::Near)
            ok = cpu_.encoder()->encode_near_jump(displacement, encoded, error);
        else
            ok = cpu_.encoder()->encode_short_jump(displacement, encoded, error);
        if (!ok || encoded.size() != size)
        {
            error = location + (error.empty() ? "Invalid jump encoding" : error);
            if (!fixup.sourceLine.empty() &&
                fixup.span.end > fixup.span.begin &&
                fixup.span.begin < fixup.sourceLine.size())
            {
                error += "\n" + fixup.sourceLine + "\n";
                error += std::string(fixup.span.begin, ' ') + "^";
            }
            return false;
        }
        std::copy(encoded.begin(), encoded.end(), output.begin() + fixup.offset);
    }
    std::cout << "Second pass complete" << std::endl;
    bytes = std::move(output);
    symbols = std::move(labels);
    return true;
}
