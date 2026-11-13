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
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iomanip>

enum class SymbolKind {
    Label,
    Instruction,
    Directive
};

struct SymbolTag {
    std::string name;
    std::vector<std::uint8_t> value;
};
struct SymbolMetadata {
    std::vector<SymbolTag> tags;
};
struct SymbolByteRange {
    std::size_t offset = 0;
    std::size_t length = 0;
    std::string sourceName;
    std::size_t line = 0;
    std::size_t column = 0;
    SymbolMetadata metadata = {};
};
struct Symbol {
    std::string name;
    SymbolKind kind;
    std::size_t offset = 0;
    std::string sourceName;
    std::size_t line = 0;
    std::size_t column = 0;
    SymbolMetadata metadata = {};
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
        byteRanges.clear();
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

    bool addLabel(const std::string& name, std::size_t offset = 0, std::string sourceName = "", std::size_t line = 0, std::size_t column = 0) {
        if (hasSymbol(name)) {
            return false;
        }
        addSymbol({name, SymbolKind::Label, offset, sourceName, line, column});
        return true;
    }
    bool addInstruction(const std::string& name, std::size_t offset = 0) {
        if (hasSymbol(name)) {
            return false;
        }
        addSymbol({name, SymbolKind::Instruction, offset, "", 0, 0});
        return true;
    }

    bool addDirective(const std::string& name, std::size_t offset = 0) {
        if (hasSymbol(name)) {
            return false;
        }
        addSymbol({name, SymbolKind::Directive, offset, "", 0, 0});
        return true;
    }
    bool save_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        file << "KACE-SYM 3\n";
        for (const auto& pair : symbols) {
            const Symbol& symbol = pair.second;
            file << "symbol " << std::quoted(symbol.name) << " " << static_cast<int>(symbol.kind)
                 << " " << symbol.offset << " " << std::quoted(symbol.sourceName)
                 << " " << symbol.line << " " << symbol.column;
            writeMetadata(file, symbol.metadata);
            file << "\n";
        }
        for (const auto& range : byteRanges) {
            file << "bytes " << range.offset << " " << range.length << " "
                 << std::quoted(range.sourceName) << " " << range.line << " " << range.column;
            writeMetadata(file, range.metadata);
            file << "\n";
        }
        file.close();
        return static_cast<bool>(file);
    }
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        SymbolTable loaded;
        std::string header;
        std::getline(file, header);
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header == "KACE-SYM 2" || header == "KACE-SYM 3") {
            const bool hasMetadata = header == "KACE-SYM 3";
            while (true) {
                file >> std::ws;
                if (file.eof()) break;
                if (!file) return false;
                std::string record;
                if (!(file >> record)) return false;
                if (record == "symbol") {
                    Symbol symbol;
                    int kindInt;
                    if (!(file >> std::quoted(symbol.name) >> kindInt >> symbol.offset >>
                          std::quoted(symbol.sourceName) >> symbol.line >> symbol.column) ||
                        kindInt < static_cast<int>(SymbolKind::Label) ||
                        kindInt > static_cast<int>(SymbolKind::Directive) ||
                        symbol.name.empty() || loaded.hasSymbol(symbol.name)) return false;
                    symbol.kind = static_cast<SymbolKind>(kindInt);
                    if (hasMetadata && !readMetadata(file, symbol.metadata)) return false;
                    loaded.addSymbol(symbol);
                } else if (record == "bytes") {
                    SymbolByteRange range;
                    if (!(file >> range.offset >> range.length >> std::quoted(range.sourceName) >>
                          range.line >> range.column)) return false;
                    if (hasMetadata && !readMetadata(file, range.metadata)) return false;
                    loaded.addByteRange(range);
                } else {
                    return false;
                }
            }
            symbols.swap(loaded.symbols);
            byteRanges.swap(loaded.byteRanges);
            return true;
        }
        file.clear();
        file.seekg(0);
        std::string name;
        int kindInt;
        std::size_t offset;
        while (file >> name) {
            if (!(file >> kindInt >> offset) || kindInt < static_cast<int>(SymbolKind::Label) ||
                kindInt > static_cast<int>(SymbolKind::Directive) || loaded.hasSymbol(name)) {
                return false;
            }
            loaded.addSymbol({name, static_cast<SymbolKind>(kindInt), offset});
        }
        if (!file.eof()) return false;
        symbols.swap(loaded.symbols);
        byteRanges.swap(loaded.byteRanges);
        return true;
    }
    // Accessor for byte ranges
    const std::vector<SymbolByteRange>& getByteRanges() const {
        return byteRanges;
    }
    // Mutator for byte ranges
    void addByteRange(const SymbolByteRange& range) {
        byteRanges.push_back(range);
    }
    void clearByteRanges() {
        byteRanges.clear();
    }
    void removeByteRange(std::size_t index) {
        if (index < byteRanges.size()) {
            byteRanges.erase(byteRanges.begin() + index);
        }
    }
    void updateByteRange(std::size_t index, const SymbolByteRange& range) {
        if (index < byteRanges.size()) {
            byteRanges[index] = range;
        }
    }


private:
    static void writeMetadata(std::ostream& out, const SymbolMetadata& metadata) {
        static const char hex[] = "0123456789abcdef";
        out << " " << metadata.tags.size();
        for (const auto& tag : metadata.tags) {
            std::string value;
            for (auto byte : tag.value) {
                value += hex[byte >> 4];
                value += hex[byte & 15];
            }
            out << " " << std::quoted(tag.name) << " " << std::quoted(value);
        }
    }
    static bool readMetadata(std::istream& in, SymbolMetadata& metadata) {
        std::size_t count;
        if (!(in >> count)) return false;
        for (std::size_t i = 0; i < count; ++i) {
            SymbolTag tag;
            std::string value;
            if (!(in >> std::quoted(tag.name) >> std::quoted(value)) || value.size() % 2) return false;
            auto digit = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            for (std::size_t j = 0; j < value.size(); j += 2) {
                int high = digit(value[j]), low = digit(value[j + 1]);
                if (high < 0 || low < 0) return false;
                tag.value.push_back(static_cast<std::uint8_t>((high << 4) | low));
            }
            metadata.tags.push_back(std::move(tag));
        }
        return true;
    }
    std::unordered_map<std::string, Symbol> symbols;
    std::vector<SymbolByteRange> byteRanges;
};

