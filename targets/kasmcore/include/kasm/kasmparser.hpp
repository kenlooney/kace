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
#include <sstream>
#include <vector>

class KasmParser : public Parser {
public:
    explicit KasmParser(const std::vector<Token>& input) : Parser(input) {
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
    }

    bool parse(
        const Source& source,
        std::vector<Statement>& statements,
        std::string& error
    ) {
        statements.clear();
        std::istringstream input(source.text);
        std::string line;
        std::size_t line_number = 0;

        while (std::getline(input, line)) {
            ++line_number;

            KasmLexer lexer;
            const std::vector<Token>& tokens = lexer.tokenize(line);
            if (tokens.empty()) {
                continue;
            }

            Statement statement;
            statement.line = line_number;
            if (tokens[0].kind != "identifier") {
                error = source.name + ":" +
                    std::to_string(line_number) +
                    ": Expected a statement name";
                return false;
            }
            statement.name = tokens[0].text;
            for (const Alias& keyword : keywords_table.aliases()) {
                if (statement.name == keyword.symbol) {
                    break;
                }
                for (const std::string& alias : keyword.values) {
                    if (statement.name == alias) {
                        statement.name = keyword.symbol;
                        break;
                    }
                }
                if (statement.name == keyword.symbol) {
                    break;
                }
            }

            std::size_t operand_start = 1;
            if (statement.name == "define") {
                if (tokens.size() < 3 || tokens[1].kind != "identifier") {
                    error = source.name + ":" +
                        std::to_string(line_number) +
                        ": Expected a data type and expression";
                    return false;
                }

                const std::string& type = tokens[1].text;
                if (type == "byte") {
                    statement.type = DataType::Byte;
                } else if (type == "word") {
                    statement.type = DataType::Word;
                } else if (type == "dword") {
                    statement.type = DataType::Dword;
                } else if (type == "qword") {
                    statement.type = DataType::Qword;
                } else {
                    error = source.name + ":" +
                        std::to_string(line_number) +
                        ": Unknown data type: " + type;
                    return false;
                }
                operand_start = 2;
            } else if (statement.name == "db") {
                statement.type = DataType::Byte;
            }

            std::vector<Token> operands(
                tokens.begin() + operand_start,
                tokens.end()
            );
            if (operands.empty() && statement.type == DataType::None) {
                statements.push_back(statement);
                continue;
            }
            KasmParser line_parser(operands);
            if (!line_parser.parse_expression(
                    statement.expression,
                    statement.value,
                    error)) {
                error = source.name + ":" +
                    std::to_string(line_number) + ": " + error;
                return false;
            }

            statements.push_back(statement);
        }

        return true;
    }
};
