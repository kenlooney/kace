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
#include "kasm/kasmlexer.hpp"

std::vector<Token> KasmLexer::tokenize(const std::string& input) {
 
    std::size_t cursor = 0;
    std::size_t begin = 0;
    while (cursor < input.size()) {
        // Tokenization logic for each character goes here
        ++cursor;
        addToken(Token{"character", input.substr(cursor - 1, 1), Span{begin, cursor}});
    }
    return getTokens();
}