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

#include <kace/aliastable.hpp>
#include <kace/lexer.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class OperatorTable : public AliasTable {
public:
    OperatorTable() {
        add_alias("+", {"+"});
        add_alias("-", {"-"});
        add_alias("*", {"*"});
        add_alias("/", {"/"});
        add_alias("%", {"%"});
        add_alias("(", {"("});
        add_alias(")", {")"});
    }
    bool is_operator(const std::string& symbol) const {
        return resolve_alias(symbol) != nullptr;
    }
    const Alias* get_operator_alias(
        const std::string& symbol
    ) const {
        return resolve_alias(symbol);
    }
};
class Parser {
public:
    explicit Parser(const std::vector<Token>& input) : tokens(input), ops_table() {
    };
   
    virtual ~Parser() = default;

    void add_operator_alias(
        const std::string& symbol,
        const std::string& value
    );
    bool run(std::uint64_t& value, std::string& error);
    bool number(std::uint64_t& value, std::string& error);
    bool sum(std::uint64_t& value, std::string& error);
    bool product(std::uint64_t& value, std::string& error);
    bool parse_expression(std::uint64_t& value, std::string& error);
    OperatorTable ops_table;

private:
    std::vector<Token> tokens;
    std::size_t pos = 0;
};