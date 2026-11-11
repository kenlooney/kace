#include <kasm/kasmstatement.hpp>
#include <memory>
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
#include <kasm/kasmlexer.hpp>
#include <kace/parser.hpp>
#include <kace/integers.hpp>
#include <sstream>
#include <vector>
#include <cstddef>
// For testing purposes
#include <iostream>

class KasmParser : public Parser
{
public:
    explicit KasmParser(const std::vector<Token> &input) : Parser(input)
    {
        ops_table.addto_alias("+", "add");
        ops_table.addto_alias("+", "sum");
        ops_table.addto_alias("-", "sub");
        ops_table.addto_alias("-", "take");
        ops_table.addto_alias("*", "mul");
        ops_table.addto_alias("/", "div");
        ops_table.addto_alias("%", "mod");

        // Add aliases for parentheses
        ops_table.addto_alias("(", "{");
        ops_table.addto_alias(")", "}");

        // "jmp" already has an alias in the KeywordTable, but we can add more if needed.
        keywords_table.addto_alias("jmp", "jump");
        keywords_table.addto_alias("jmp", "goto");

        keywords_table.addto_alias("mov", "move");
        keywords_table.addto_alias("mov", "copy");

        // Keyword directives
        keywords_table.addto_alias("times", "fill");
        keywords_table.addto_alias("times", "repeat");
        keywords_table.addto_alias("times", "dup");
        keywords_table.addto_alias("$", "currentposition");
        keywords_table.addto_alias("$$", "sectionstart");
        keywords_table.addto_alias("db", "definebyte");
        keywords_table.addto_alias("db", "byte");
        keywords_table.addto_alias("dw", "defineword");
        keywords_table.addto_alias("dw", "word");
    }

    bool parse(
        const Source &source,
        std::vector<std::unique_ptr<Statement>> &statements,
        std::string &error)
    {
        statements.clear();
        std::istringstream input(source.text);
        std::string line;
        std::size_t line_number = 0;

        while (std::getline(input, line))
        {
            ++line_number;

            KasmLexer lexer;
            const std::vector<Token> &tokens = lexer.tokenize(line);
            if (tokens.empty())
            {
                continue;
            }

            KasmStatement statement;
            statement.line = line_number;
            statement.sourceName = source.name;

            // Check for code segment keywords like "code16", "code32", "code64" before identifiers
            if (tokens[0].kind == "identifier" &&
            
                (tokens[0].text == "code16" || tokens[0].text == "code32" || tokens[0].text == "code64"))
            {
                if(tokens.size() != 1)
                {
                    error = source.name + ":" + std::to_string(line_number) +
                            ": Unexpected tokens after code segment declaration";
                    return false;
                }
                statement.add_kind("CodeSegment");
                statement.codeSegment = tokens[0].text;
                statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
                continue;
            }
            
            // Check for directives before identifiers
            if (tokens[0].kind == "directive")
            {
                if (tokens.size() < 2)
                {
                    error = source.name + ":" + std::to_string(line_number) +
                            ": Expected a directive name after #";
                    return false;
                }
                statement.add_kind("Directive");
                statement.set_name(tokens[1].text);

                if (tokens[1].kind != "identifier" ||
                    (statement.name() != "include" &&
                     statement.name() != "assertposition"))
                {
                    error = source.name + ":" +
                            std::to_string(line_number) +
                            ": Unknown directive " + statement.name();
                    return false;
                }

                if (statement.name() == "include")
                {
                    statement.set_name("include");
                    statement.directiveKind = DirectiveKind::Include;
                    statement.directiveStage = DirectiveStage::SourceExpansion;

                    if (tokens.size() != 3) // If token count is not exactly 3, it's an error because the path is not included
                    {
                        error = source.name + ":" + std::to_string(line_number) +
                                ": Expected an include path after #include";
                        return false;
                    }
                    if (tokens[2].kind != "string" || tokens[2].text.empty())
                    {
                        error = source.name + ":" + std::to_string(line_number) +
                                ": Include path must be quoted and not empty";
                        return false;
                    }

                    std::string include_path = tokens[2].text;
                    statement.includePath = include_path;
                }
                else if (statement.name() == "assertposition")
                {
                    if (tokens.size() != 3) // If token count is not exactly 3, it's an error because the position is not included
                    {
                        error = source.name + ":" + std::to_string(line_number) +
                                ": Expected a position after #assertposition";
                        return false;
                    }
                    if (tokens[2].kind != "integer" || tokens[2].text.empty())
                    {
                        error = source.name + ":" + std::to_string(line_number) +
                                ": Position must be a number and not empty";
                        return false;
                    }

                   
                    statement.set_name("assertposition");
                    statement.directiveKind = DirectiveKind::AssertPosition;
                    statement.directiveStage = DirectiveStage::Assembly;
                     
                    std::uint64_t position;
                    if (!parse_integer(tokens[2].text, position))
                    {
                        error = source.name + ":" + std::to_string(line_number) +
                                ": Invalid position";
                        return false;
                    }
                    statement.assertPosition = position;

                }

                statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
                continue;
            }

            // Check for labels before identifiers
            if (tokens[0].kind == "label")
            {

                statement.add_kind("Label");
                statement.set_name(tokens[0].text.substr(0, tokens[0].text.size() - 1));
                statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
                continue;
            }

            if (tokens[0].kind != "identifier")
            {
                error = source.name + ":" +
                        std::to_string(line_number) +
                        ": Expected a statement name";
                return false;
            }

            statement.set_name(tokens[0].text);

            for (const Alias &keyword : keywords_table.aliases())
            {
                if (statement.name() == keyword.symbol)
                {
                    break;
                }
                for (const std::string &alias : keyword.values)
                {
                    if (statement.name() == alias)
                    {
                        statement.set_name(keyword.symbol);
                        break;
                    }
                }
                if (statement.name() == keyword.symbol)
                {
                    break;
                }
            }

            std::size_t operand_start = 1;
            std::size_t operand_end = tokens.size();

            if (statement.name() == "times")
            {

                statement.add_kind("Times");
                std::size_t byte_index = 1;
                std::string data_directive;
                while (byte_index < tokens.size())
                {
                    const std::string &spelling = tokens[byte_index].text;
                    for (const Alias &keyword : keywords_table.aliases())
                    {
                        if (keyword.symbol != "db" && keyword.symbol != "dw" &&
                            keyword.symbol != "dd" && keyword.symbol != "dq")
                            continue;
                        if (spelling == keyword.symbol)
                            data_directive = keyword.symbol;
                        for (const std::string &alias : keyword.values)
                            if (spelling == alias)
                                data_directive = keyword.symbol;
                    }
                    if (!data_directive.empty() || spelling == "dd" || spelling == "dq")
                    {
                        break;
                    }
                    ++byte_index;
                }
                if (byte_index == 1 || byte_index == tokens.size() || byte_index + 1 == tokens.size())
                {
                    error = source.name + ":" + std::to_string(line_number) +
                            ": Expected repetition count, data directive, and a value";
                    return false;
                }
                std::vector<Token> count_tokens(
                    tokens.begin() + 1,
                    tokens.begin() + byte_index);
                operand_start = byte_index + 1;

                if (data_directive != "db")
                {
                    error = source.name + ":" + std::to_string(line_number) +
                            ": Only byte repetition is supported";
                    return false;
                }

                KasmParser count_parser(count_tokens);
                if (!count_parser.parse_expression_tree(statement.countExpression, error))
                {
                    error = source.name + ":" + std::to_string(line_number) + ": " + error;
                    return false;
                }

                statement.type = DataType::Byte;
            }

            if (statement.name() == "mov")
            {
                statement.moveKind = MoveKind::ImmediateToRegister;
                // move/copy/mov <expression> into <register>
                if (tokens.size() < 4)
                {
                    error = source.name + ":" +
                            std::to_string(line_number) +
                            ": Expected an expression and a target register";
                    return false;
                }
                if (tokens[1].kind == "identifier" && tokens[2].text == ",")
                {
                    statement.reg = tokens[1].text;
                    operand_start = 3;
                }
                else if (tokens[tokens.size() - 2].text == "into" &&
                         tokens.back().kind == "identifier")
                {
                    statement.reg = tokens.back().text;
                    operand_end -= 2;
                }
                else
                {
                    error = source.name + ":" + std::to_string(line_number) +
                            ": Expected an expression and a target register";
                    return false;
                }
            }
            if (statement.name() == "jmp")
            {
                if (tokens.size() < 2)
                {
                    error = source.name + ":" +
                            std::to_string(line_number) +
                            ": Expected a jump type and target label";
                    return false;
                }
                const std::string &type = tokens[1].text;
                if (type == "far")
                {
                    if (tokens.size() != 5 || tokens[2].kind != "integer" ||
                        tokens[3].text != ":" || tokens[4].kind != "integer" ||
                        !parse_integer(tokens[2].text, statement.farSegment) ||
                        !parse_integer(tokens[4].text, statement.farOffset))
                    {
                        error = source.name + ":" + std::to_string(line_number) +
                                ": Expected a far jump segment:offset";
                        return false;
                    }
                    statement.add_kind("JumpFar");
                    statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
                    continue;
                }
                if (tokens.size() != 3 || tokens[2].kind != "identifier")
                {
                    error = source.name + ":" + std::to_string(line_number) +
                            ": Expected a jump type and target label";
                    return false;
                }
                if (type == "short")
                    statement.add_kind("JumpShort");
                else if (type == "near")
                    statement.add_kind("JumpNear");
                else
                {
                    error = source.name + ":" +
                            std::to_string(line_number) +
                            ": Expected a jump type of short, near, or far";
                    return false;
                }
                statement.target = tokens[2].text;
                statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
                continue;
            }

            if (statement.name() == "define")
            {
                statement.add_kind("Directive");
                if (tokens.size() < 3 || tokens[1].kind != "identifier")
                {
                    error = source.name + ":" +
                            std::to_string(line_number) +
                            ": Expected a data type and expression";
                    return false;
                }

                const std::string &type = tokens[1].text;
                if (type == "byte")
                {
                    statement.type = DataType::Byte;
                }
                else if (type == "word")
                {
                    statement.type = DataType::Word;
                }
                else if (type == "dword")
                {
                    statement.type = DataType::Dword;
                }
                else if (type == "qword")
                {
                    statement.type = DataType::Qword;
                }
                else
                {
                    error = source.name + ":" +
                            std::to_string(line_number) +
                            ": Unknown data type: " + type;
                    return false;
                }
                operand_start = 2;
            }
            else if (statement.name() == "db")
            {
                statement.type = DataType::Byte;
            }

            std::vector<Token> operands(
                tokens.begin() + operand_start,
                tokens.begin() + operand_end);
            if (operands.empty() && statement.type == DataType::None)
            {
                statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
                continue;
            }
            KasmParser line_parser(operands);
            if (!line_parser.parse_expression(
                    statement.expression,
                    statement.value,
                    error))
            {
                error = source.name + ":" +
                        std::to_string(line_number) + ": " + error;
                return false;
            }

            statements.push_back(std::make_unique<KasmStatement>(std::move(statement)));
        }

        return true;
    }
};
