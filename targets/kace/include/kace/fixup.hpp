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
#include <string>

enum class JumpKind {
    Short,
    Near,
    Far
};

struct JumpFixup {
    std::string target;
    std::size_t offset; // Where the jump starts in the output
    std::size_t line;   // For error messages
    JumpKind kind = JumpKind::Short;
    std::string sourceName;
  
};
