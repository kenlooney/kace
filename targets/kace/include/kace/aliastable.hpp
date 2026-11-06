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

#include <deque>
#include <string>
#include <vector>

struct Alias {
    std::string symbol;
    std::vector<std::string> values;
};

class AliasTable {
public:
    virtual ~AliasTable() = default;

    void add_alias(
        const std::string& symbol,
        const std::vector<std::string>& values
    );

    const Alias* resolve_alias(
        const std::string& symbol
    ) const;

    bool resolve_alias(
        const std::string& symbol,
        std::vector<std::string>& values
    ) const;

    const Alias* get_alias(
        const std::string& symbol
    ) const;

    const std::deque<Alias>& aliases() const {
        return aliases_;
    }

private:
    std::deque<Alias> aliases_;
};