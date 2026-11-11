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
#include <kace/statement.hpp>

// bool Statement::parse_statement(const std::vector<Token> &tokens, std::string &error)
// {
//     if (tokens.empty() || tokens[0].kind != "identifier") {
//         error = "Expected a statement name";
//         return false;
//     }
//     set_name(tokens[0].text);
//     std::vector<Token> operands(tokens.begin() + 1, tokens.end());
//     Parser parser(operands, expression);
//     return parser.parse_expression(
//         expression,
//         value,
//         error
//     );
// }
