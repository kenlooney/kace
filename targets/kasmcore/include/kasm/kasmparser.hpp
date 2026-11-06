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
};