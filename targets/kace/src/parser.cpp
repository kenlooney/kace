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
#include <kace/parser.hpp>
#include <kace/integers.hpp>
#include <kace/aliastable.hpp>
void Parser::add_operator_alias(
    const std::string& symbol,
    const std::string& value
) {
    if (symbol.empty() || value.empty()) {
        return;
    }
    if (symbol == value) {
        return;
    }
    if(symbol == "+")
        ops_table.addto_alias("+", value);
    else if(symbol == "-")
        ops_table.addto_alias("-", value);
    else if(symbol == "*")
        ops_table.addto_alias("*", value);
    else if(symbol == "/")
        ops_table.addto_alias("/", value);
    else if(symbol == "%")
        ops_table.addto_alias("%", value);
    else if(symbol == "(")
        ops_table.addto_alias("(", value);
    else if(symbol == ")")
        ops_table.addto_alias(")", value);
}

bool Parser::number(std::uint64_t& value, std::string& error) {
    const Alias* open_paren_alias = ops_table.get_operator_alias("(");
    const Alias* close_paren_alias = ops_table.get_operator_alias(")");

   if (pos < tokens.size()) {
       for (const std::string& alias : open_paren_alias->values) {
           if (tokens[pos].text == alias) {
               ++pos;
               if (!sum(value, error)) return false;
               if (pos < tokens.size()) {
                   for (const std::string& close_alias : close_paren_alias->values) {
                       if (tokens[pos].text == close_alias) {
                           ++pos;
                           return true;
                       }
                   }
               }
               error = "Expected a closing parenthesis";
               return false;
           }
       }
   }

   if(pos >= tokens.size() || tokens[pos].kind != "integer") {
       error = "Expected an integer";
       return false;
   }
   if(!parse_integer(tokens[pos].text, value)) {
       error = "Invalid integer: " + tokens[pos].text;
       return false;
   }
   ++pos;
   return true;
}

bool Parser::run(std::uint64_t& value, std::string& error) {
        if (!sum(value, error)) return false;
        if (pos != tokens.size()) {
            error = "Unexpected token: " + tokens[pos].text;
            return false;
        }
        return true;
    }

bool Parser::sum(std::uint64_t& value, std::string& error) {
    const Alias* add_alias = ops_table.get_operator_alias("+");
    const Alias* sub_alias = ops_table.get_operator_alias("-");
    if(!product(value, error)) {
        return false;
    }
    while(pos < tokens.size())
    {
        // Check for addition operator with operator alias
        bool matched = false;
        for(const std::string& alias : add_alias->values) {
            if(pos < tokens.size() && tokens[pos].text == alias) {
                ++pos;
                std::uint64_t rhs;
                if(!product(rhs, error)) {
                    return false;
                }
                value += rhs;
                matched = true;
                break;
            }
        }
        if(!matched) {
            for(const std::string& alias : sub_alias->values) {
                if(pos < tokens.size() && tokens[pos].text == alias) {
                    ++pos;
                    std::uint64_t rhs;
                    if(!product(rhs, error)) {
                        return false;
                    }
                    value -= rhs;
                    matched = true;
                    break;
                }
            }
        }
        if(!matched) {
            break;
        }
    }
    return true;
}

bool Parser::product(std::uint64_t& value, std::string& error) {
    const Alias* mul_alias = ops_table.get_operator_alias("*");
    const Alias* div_alias = ops_table.get_operator_alias("/");
    const Alias* mod_alias = ops_table.get_operator_alias("%");
    if(!number(value, error)) {
        return false;
    }
    while(pos < tokens.size())
    {
        bool matched = false;
        for(const std::string& alias : mul_alias->values) {
            if(pos < tokens.size() && tokens[pos].text == alias) {
                ++pos;
                std::uint64_t rhs;
                if(!number(rhs, error)) {
                    return false;
                }
                value *= rhs;
                matched = true;
                break;
            }
        }
        if(!matched) {
            for(const std::string& alias : div_alias->values) {
                if(pos < tokens.size() && tokens[pos].text == alias) {
                    ++pos;
                    std::uint64_t rhs;
                    if(!number(rhs, error)) {
                        return false;
                    }
                    if(rhs == 0) {
                        error = "Division by zero";
                        return false;
                    }
                    value /= rhs;
                    matched = true;
                    break;
                }
            }
        }
        if(!matched) {
            for(const std::string& alias : mod_alias->values) {
                if(pos < tokens.size() && tokens[pos].text == alias) {
                    ++pos;
                    std::uint64_t rhs;
                    if(!number(rhs, error)) {
                        return false;
                    }
                    if(rhs == 0) {
                        error = "Modulo by zero";
                        return false;
                    }
                    value %= rhs;
                    matched = true;
                    break;
                }
            }
        }
        if(!matched) {
            break;
        }
    }
    return true;
}

bool Parser::parse_expression(
    std::uint64_t& value,
    std::string& error
) {
    return run(value, error);
}

bool parse_expression(const std::vector<Token>& tokens,
                      std::uint64_t& value, std::string& error) {
    Parser parser(tokens);
    return parser.run(value, error);
}
