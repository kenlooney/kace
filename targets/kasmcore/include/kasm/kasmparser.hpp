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
// For testing purposes
#include <iostream>

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

        // KeywordTable already registers the mov aliases.
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
            statement.name = tokens[0].text;

            // Check for labels before identifiers
            if(tokens[0].kind == "label") {
               
                std::cout << "Found label: " << statement.name << std::endl;
                continue;
            }
            
            if (tokens[0].kind != "identifier") {
                error = source.name + ":" +
                    std::to_string(line_number) +
                    ": Expected a statement name";
                return false;
            }
          
           
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
            std::size_t operand_end = tokens.size();
            
            
            if (statement.name == "mov") {
                // move/copy/mov <expression> into <register>
                if (tokens.size() < 4) {
                    error = source.name + ":" +
                        std::to_string(line_number) +
                        ": Expected an expression and a target register";
                    return false;
                }
                if (tokens[1].kind == "identifier" && tokens[2].text == ",") {
                    statement.reg = tokens[1].text;
                    operand_start = 3;
                } else if (tokens[tokens.size() - 2].text == "into" &&
                           tokens.back().kind == "identifier") {
                    statement.reg = tokens.back().text;
                    operand_end -= 2;
                } else {
                    error = source.name + ":" + std::to_string(line_number) +
                        ": Expected an expression and a target register";
                    return false;
                }
            }
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
                tokens.begin() + operand_end
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
