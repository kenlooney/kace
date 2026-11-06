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
#include <string>
#include "span.hpp"
#include <vector>

struct Token {
    std::string kind;
    std::string text;
    Span span;

};
class Lexer {
    public:
    virtual ~Lexer() = default;
    public:
    virtual std::vector<Token> tokenize(const std::string& input) = 0;
    void clearTokens() {
        tokens.clear();
    }
    const std::vector<Token>& getTokens() const {
        return tokens;
    }
    std::vector<Token>& getTokens() {
        return tokens;
    }
    void addToken(const Token& token) {
        tokens.push_back(token);
    }
    private:
    std::vector<Token> tokens;

};