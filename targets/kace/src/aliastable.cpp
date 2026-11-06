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
#include <kace/aliastable.hpp>


// Implementation of AliasTable methods

void AliasTable::add_alias(
    const std::string& symbol,
    const std::vector<std::string>& values
) {
    aliases_.push_back(Alias{symbol, values});
}

const Alias* AliasTable::resolve_alias(
    const std::string& symbol
) const {
    for (const auto& alias : aliases_) {
        if (alias.symbol == symbol) {
            return &alias;
        }
    }
    return nullptr;
}

bool AliasTable::resolve_alias(
    const std::string& symbol,
    std::vector<std::string>& values
) const {
    const Alias* alias = resolve_alias(symbol);
    if (alias == nullptr) {
        return false;
    }

    values = alias->values;
    return true;
}

const Alias* AliasTable::get_alias(
    const std::string& symbol
) const {
    return resolve_alias(symbol);
}