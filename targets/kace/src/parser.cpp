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
#include <kace/integers.hpp>
#include <kace/parser.hpp>
#include <sstream>
#include <kace/source.hpp>
#include <sstream>
#include <limits>

void Parser::add_operator_alias(
    const std::string &symbol,
    const std::string &value)
{
    if (symbol.empty() || value.empty() || symbol == value)
    {
        return;
    }

    ops_table.addto_alias(symbol, value);
}

bool Parser::number(std::size_t &node, std::string &error)
{
    const Alias *open_alias = ops_table.get_operator_alias("(");
    const Alias *close_alias = ops_table.get_operator_alias(")");

    if (open_alias == nullptr || close_alias == nullptr)
    {
        error = "Parser parenthesis aliases are incomplete";
        return false;
    }

    if (pos < tokens.size())
    {
        for (const std::string &alias : open_alias->values)
        {
            if (tokens[pos].text != alias)
            {
                continue;
            }

            ++pos;
            if (!sum(node, error))
            {
                return false;
            }

            if (pos < tokens.size())
            {
                for (const std::string &close : close_alias->values)
                {
                    if (tokens[pos].text == close)
                    {
                        ++pos;
                        return true;
                    }
                }
            }

            error = "Expected a closing parenthesis";
            return false;
        }
    }

    auto matches_keyword = [&](const std::string &symbol)
    {
        const Alias *entry = keywords_table.get_keyword_alias(symbol);
        if (entry == nullptr || pos >= tokens.size())
            return false;

        for (const std::string &spelling : entry->values)
        {
            if (tokens[pos].text == spelling)
                return true;
        }
        return false;
    };

    if (pos < tokens.size() && matches_keyword("$"))
    {
        node = tree->nodes.size();
        tree->nodes.push_back(Node{"current_position", 0, node, node});
        ++pos;
        return true;
    }
    if (pos < tokens.size() && matches_keyword("$$"))
    {
        node = tree->nodes.size();
        tree->nodes.push_back(Node{"section_start", 0, node, node});
        ++pos;
        return true;
    }
    if (pos >= tokens.size() || tokens[pos].kind != "integer")
    {
        error = "Expected an integer";
        return false;
    }

    std::uint64_t value = 0;
    if (!parse_integer(tokens[pos].text, value))
    {
        error = "Invalid integer: " + tokens[pos].text;
        return false;
    }

    node = tree->nodes.size();
    tree->nodes.push_back(Node{"number", value, node, node});
    ++pos;
    return true;
}

bool Parser::sum(std::size_t &node, std::string &error)
{
    const Alias *add_alias = ops_table.get_operator_alias("+");
    const Alias *sub_alias = ops_table.get_operator_alias("-");
    if (add_alias == nullptr || sub_alias == nullptr)
    {
        error = "Parser sum aliases are incomplete";
        return false;
    }

    if (!product(node, error))
    {
        return false;
    }

    while (pos < tokens.size())
    {
        const Alias *matched_alias = nullptr;
        std::string operation;

        for (const std::string &alias : add_alias->values)
        {
            if (tokens[pos].text == alias)
            {
                matched_alias = add_alias;
                operation = "+";
                break;
            }
        }
        if (matched_alias == nullptr)
        {
            for (const std::string &alias : sub_alias->values)
            {
                if (tokens[pos].text == alias)
                {
                    matched_alias = sub_alias;
                    operation = "-";
                    break;
                }
            }
        }
        if (matched_alias == nullptr)
        {
            break;
        }

        ++pos;
        std::size_t right = 0;
        if (!product(right, error))
        {
            return false;
        }

        const std::size_t left = node;
        node = tree->nodes.size();
        tree->nodes.push_back(Node{operation, 0, left, right});
    }

    return true;
}

bool Parser::product(std::size_t &node, std::string &error)
{
    const Alias *mul_alias = ops_table.get_operator_alias("*");
    const Alias *div_alias = ops_table.get_operator_alias("/");
    const Alias *mod_alias = ops_table.get_operator_alias("%");
    if (mul_alias == nullptr || div_alias == nullptr || mod_alias == nullptr)
    {
        error = "Parser product aliases are incomplete";
        return false;
    }

    if (!number(node, error))
    {
        return false;
    }

    while (pos < tokens.size())
    {
        std::string operation;
        const Alias *aliases = nullptr;

        for (const std::string &alias : mul_alias->values)
        {
            if (tokens[pos].text == alias)
            {
                operation = "*";
                aliases = mul_alias;
                break;
            }
        }
        if (aliases == nullptr)
        {
            for (const std::string &alias : div_alias->values)
            {
                if (tokens[pos].text == alias)
                {
                    operation = "/";
                    aliases = div_alias;
                    break;
                }
            }
        }
        if (aliases == nullptr)
        {
            for (const std::string &alias : mod_alias->values)
            {
                if (tokens[pos].text == alias)
                {
                    operation = "%";
                    aliases = mod_alias;
                    break;
                }
            }
        }
        if (aliases == nullptr)
        {
            break;
        }

        ++pos;
        std::size_t right = 0;
        if (!number(right, error))
        {
            return false;
        }

        const std::size_t left = node;
        node = tree->nodes.size();
        tree->nodes.push_back(Node{operation, 0, left, right});
    }

    return true;
}

bool Parser::run(std::uint64_t &value, std::string &error)
{
    tree->nodes.clear();
    tree->root = 0;
    pos = 0;

    if (!sum(tree->root, error))
    {
        return false;
    }
    if (pos != tokens.size())
    {
        error = "Unexpected token: " + tokens[pos].text;
        return false;
    }

    return evaluate(*tree, tree->root, value, error);
}

bool Parser::evaluate(
    const Expression &expression,
    std::size_t node,
    std::uint64_t &value,
    std::string &error, const PositionContext *positions)
{
    if (node >= expression.nodes.size())
    {
        error = "Invalid expression node";
        return false;
    }

    const Node &current = expression.nodes[node];
    if (current.kind == "number")
    {
        value = current.value;
        return true;
    }
    if (current.kind == "current_position" || current.kind == "section_start")
    {
        if (positions == nullptr)
        {
            error = "Position is not available in this expression";
            return false;
        }

        value = current.kind == "current_position"
                    ? positions->current
                    : positions->sectionStart;
        return true;
    }

    std::uint64_t left = 0;
    std::uint64_t right = 0;
    if (!evaluate(expression, current.left, left, error, positions) ||
        !evaluate(expression, current.right, right, error, positions))
    {
        return false;
    }

    if (current.kind == "+")
    {
        if (left > std::numeric_limits<std::uint64_t>::max() - right)
        {
            error = "Addition overflow";
            return false;
        }
        value = left + right;
    }
    else if (current.kind == "-")
    {
        if (right > left)
        {
            error = "Subtraction would produce a negative value";
            return false;
        }

        value = left - right;
    }
    else if (current.kind == "*")
    {
        if (right != 0 && left > std::numeric_limits<std::uint64_t>::max() / right)
        {
            error = "Multiplication overflow";
            return false;
        }
        value = left * right;
    }
    else if (current.kind == "/")
    {
        if (right == 0)
        {
            error = "Division by zero";
            return false;
        }
        value = left / right;
    }
    else if (current.kind == "%")
    {
        if (right == 0)
        {
            error = "Modulo by zero";
            return false;
        }
        value = left % right;
    }
    else
    {
        error = "Unknown expression operator: " + current.kind;
        return false;
    }

    return true;
}

bool Parser::parse_expression(
    Expression &expression,
    std::string &error)
{
    if (tree != &expression)
    {
        expression = *tree;
        tree = &expression;
    }

    std::uint64_t value = 0;
    return run(value, error);
}

bool Parser::parse_expression_tree(Expression &expression, std::string &error)
{
    tree = &expression;
    tree->nodes.clear();
    tree->root = 0;
    pos = 0;

    if (!sum(tree->root, error))
        return false;

    if (pos != tokens.size())
    {
        error = "Unexpected token: " + tokens[pos].text;
        return false;
    }

    return true;
}

bool Parser::parse_expression(
    Expression &expression,
    std::uint64_t &value,
    std::string &error)
{
    if (tree != &expression)
    {
        expression = *tree;
        tree = &expression;
    }
    return run(value, error);
}

bool Parser::parse_expression(
    std::uint64_t &value,
    std::string &error)
{
    return run(value, error);
}

bool Parser::parse(Source &source, std::string &error)
{
    std::vector<Statement> statements;
    std::istringstream input(source.text);
    std::uint64_t offset = 0;
    std::string line_text;
    std::size_t line_number = 1;
    while (std::getline(input, line_text))
    {
        if (line_text.empty())
        {
            ++line_number;
            continue;
        }

        Statement statement;
        statement.line = line_number;
        std::istringstream line_stream(line_text);
        std::vector<Token> line_tokens;
        Token token;
        while (line_stream >> token.text)
        {
            line_tokens.push_back(token);
        }

        Parser line_parser(line_tokens, statement.expression);
        std::uint64_t value = 0;
        if (!line_parser.run(value, error))
        {
            return false;
        }

        statement.offset = offset;
        statements.push_back(statement);
        offset += value;
        ++line_number;
    }

    return true;
}

bool parse_expression(
    const std::vector<Token> &tokens,
    Expression &expression,
    std::uint64_t &value,
    std::string &error)
{
    Parser parser(tokens, expression);
    return parser.run(value, error);
}
