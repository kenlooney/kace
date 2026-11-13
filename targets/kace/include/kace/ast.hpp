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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "span.hpp"

struct Node {
    std::string kind;
    std::uint64_t value;
    std::size_t left;
    std::size_t right;
    Span span;
    std::string text; // Name for an identifier node
    
};

struct Expression {
    std::vector<Node> nodes;
    std::size_t root = 0;
};
