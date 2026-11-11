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
#include <kace/ast.hpp>
#include <kace/lexer.hpp>
#include <kace/source.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <kace/statement.hpp>

struct PositionContext
{
    std::uint64_t current;
    std::uint64_t sectionStart;
};

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

class KeywordTable : public AliasTable {
public:
    KeywordTable() {
        add_alias("mov", {"mov"});
        add_alias("db", {"db"});
        add_alias("dw", {"dw"});

        add_alias("times", {"times"});
        add_alias("$", {"$"});
        add_alias("$$", {"$$"});
        add_alias("jmp", {"jmp"});
       
    }
    bool is_keyword(const std::string& symbol) const {
        return resolve_alias(symbol) != nullptr;
    }
    const Alias* get_keyword_alias(
        const std::string& symbol
    ) const {
        return resolve_alias(symbol);
    }
};


class Parser {
public:
    explicit Parser(const std::vector<Token>& input)
        : tokens(input), ops_table(), keywords_table(),
        tree(&owned_tree) {}
    Parser(const std::vector<Token>& input, Expression& result)
        : tokens(input), ops_table(), keywords_table(), tree(&result) {}
   
    virtual ~Parser() = default;

    void add_operator_alias(
        const std::string& symbol,
        const std::string& value
    );
    bool parse_statement(Statement& statement, std::string& error);
    bool run(std::uint64_t& value, std::string& error);
    bool number(std::size_t& node, std::string& error);
    bool sum(std::size_t& node, std::string& error);
    bool product(std::size_t& node, std::string& error);
    bool parse_expression_tree(Expression& expression, std::string& error);
    
    bool parse_expression(Expression& expression, std::string& error);
    bool parse_expression(
        Expression& expression,
        std::uint64_t& value,
        std::string& error
    );
    bool parse_expression(std::uint64_t& value, std::string& error);
    bool evaluate(const Expression& expression, std::size_t node, std::uint64_t& value, std::string& error, const PositionContext* positions = nullptr);
    OperatorTable ops_table;
    KeywordTable keywords_table;

private:
    std::vector<Token> tokens;
    std::size_t pos = 0;
    Expression owned_tree;
    Expression* tree;
};
