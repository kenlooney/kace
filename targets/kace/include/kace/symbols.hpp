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
#include <unordered_map>
#include <vector>
#include <fstream>

enum class SymbolKind {
    Label,
    Instruction,
    Directive
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    std::size_t offset = 0;
};

class SymbolTable {
public:
    void addSymbol(const Symbol& symbol) {
        symbols[symbol.name] = symbol;
    }

    const Symbol* getSymbol(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        return nullptr;
    }
    bool hasSymbol(const std::string& name) const {
        return symbols.find(name) != symbols.end();
    }
    std::size_t symbolCount() const {
        return symbols.size();
    }
    void removeSymbol(const std::string& name) {
        symbols.erase(name);
    }
    void clear() {
        symbols.clear();
    }
    std::vector<std::string> getSymbolNames() const {
        std::vector<std::string> names;
        for (const auto& pair : symbols) {
            names.push_back(pair.first);
        }
        return names;
    }
    std::vector<Symbol> getSymbols() const {
        std::vector<Symbol> result;
        for (const auto& pair : symbols) {
            result.push_back(pair.second);
        }
        return result;
    }

    std::vector<Symbol> getSymbolsByKind(SymbolKind kind) const {
        std::vector<Symbol> result;
        for (const auto& pair : symbols) {
            if (pair.second.kind == kind) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    std::vector<Symbol> getSymbolsByOffset(std::size_t offset) const {
        std::vector<Symbol> result;
        for (const auto& pair : symbols) {
            if (pair.second.offset == offset) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    std::vector<Symbol> getSymbolsByName(const std::string& name) const {
        std::vector<Symbol> result;
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            result.push_back(it->second);
        }
        return result;
    }

    bool addLabel(const std::string& name, std::size_t offset = 0) {
        if (hasSymbol(name)) {
            return false;
        }
        addSymbol({name, SymbolKind::Label, offset});
        return true;
    }
    bool addInstruction(const std::string& name, std::size_t offset = 0) {
        if (hasSymbol(name)) {
            return false;
        }
        addSymbol({name, SymbolKind::Instruction, offset});
        return true;
    }

    bool addDirective(const std::string& name, std::size_t offset = 0) {
        if (hasSymbol(name)) {
            return false;
        }
        addSymbol({name, SymbolKind::Directive, offset});
        return true;
    }
    bool save_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        for (const auto& pair : symbols) {
            file << pair.first << " " << static_cast<int>(pair.second.kind) << " " << pair.second.offset << "\n";
        }
        return true;
    }
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        symbols.clear();
        std::string name;
        int kindInt;
        std::size_t offset;
        while (file >> name >> kindInt >> offset) {
            addSymbol({name, static_cast<SymbolKind>(kindInt), offset});
        }
        return true;
    }

private:
    std::unordered_map<std::string, Symbol> symbols;
};

