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
#pragma once
#include <kace/statement.hpp>

enum class MoveKind
{
    ImmediateToRegister,
    RegisterToRegister,
    MemoryToRegister,
    RegisterToMemory,
    None,
};
class KasmStatement : public Statement
{
public:
    std::string sourceName;
    std::size_t line = 0;
    DataType type = DataType::None;
    std::uint64_t value = 0;
    MoveKind moveKind = MoveKind::None;
    Expression expression;
    std::string reg;
    DirectiveKind directiveKind = DirectiveKind::None;
    DirectiveStage directiveStage = DirectiveStage::SourceExpansion;
    std::string includePath;
    std::uint64_t assertPosition = 0;
    std::string target;
    std::uint64_t farSegment = 0;
    std::uint64_t farOffset = 0;
    std::string codeSegment;
    std::size_t offset = 0;
    std::uint64_t count = 0;
    Expression countExpression;
    std::size_t column = 0;
    std::string interruptNumber;
    std::string baseRegister;
    std::string indexRegister;
    Expression addressExpression;
    unsigned addressScale = 1;
    bool parse_statement(const std::vector<Token> &tokens, std::string &error) override
    {
        return false;
    }

};
