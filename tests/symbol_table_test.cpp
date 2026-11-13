#include <kace/symbols.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>

bool check_labels(const SymbolTable& table) {
    const char* names[] = {"start", "middle", "end"};
    if (table.symbolCount() != 3 || table.getSymbol("missing") != nullptr) {
        return false;
    }
    for (std::size_t i = 0; i < 3; ++i) {
        const Symbol* symbol = table.getSymbol(names[i]);
        if (symbol == nullptr || symbol->kind != SymbolKind::Label || symbol->offset != i) {
            return false;
        }
    }
    return true;
}

int main() {
    SymbolTable table;
    const std::string sourceName = "source folder/file, \"quoted\".asm";
    if (!table.addLabel("start", 0, sourceName, 3, 5) || !table.addLabel("middle", 1) ||
        !table.addLabel("end", 2) || table.addLabel("middle", 99) ||
        !check_labels(table)) {
        std::cerr << "Label insertion, lookup, or duplicate detection failed\n";
        return 1;
    }
    Symbol tagged = *table.getSymbol("start");
    tagged.metadata.tags.push_back({"test.tag", {0, 16, 255}});
    table.addSymbol(tagged);
    SymbolByteRange taggedRange{0, 2, sourceName, 4, 2};
    taggedRange.metadata.tags.push_back({"x86.mode", {16}});
    table.addByteRange(taggedRange);
    table.addByteRange({2, 1, "included.asm", 8, 1});

    const std::string filename = "symbol_table_test_symbols.txt";
    if (!table.save_to_file(filename)) {
        std::cerr << "Could not save symbol table\n";
        return 1;
    }
    SymbolTable loaded;
    const bool loaded_ok = loaded.load_from_file(filename);
    if (!loaded_ok || !check_labels(loaded)) {
        std::cerr << "Symbol table did not survive save and load\n";
        return 1;
    }
    const Symbol* start = loaded.getSymbol("start");
    const auto& ranges = loaded.getByteRanges();
    if (!start || start->sourceName != sourceName || start->line != 3 || start->column != 5 ||
        ranges.size() != 2 || ranges[0].offset != 0 || ranges[0].length != 2 ||
        ranges[0].sourceName != sourceName || ranges[0].line != 4 || ranges[0].column != 2 ||
        ranges[1].offset != 2 || ranges[1].length != 1 || ranges[1].sourceName != "included.asm" ||
        ranges[1].line != 8 || ranges[1].column != 1 ||
        start->metadata.tags.size() != 1 || start->metadata.tags[0].name != "test.tag" ||
        start->metadata.tags[0].value != std::vector<std::uint8_t>({0, 16, 255}) ||
        ranges[0].metadata.tags.size() != 1 || ranges[0].metadata.tags[0].name != "x86.mode" ||
        ranges[0].metadata.tags[0].value != std::vector<std::uint8_t>({16})) {
        std::cerr << "Symbol source locations or byte ranges did not survive save and load\n";
        std::remove(filename.c_str());
        return 1;
    }
    {
        std::ofstream v2(filename);
        v2 << "KACE-SYM 2\nsymbol \"old\" 0 0 \"old.asm\" 1 1\nbytes 0 1 \"old.asm\" 1 1\n";
    }
    if (!loaded.load_from_file(filename) || !loaded.getSymbol("old") ||
        loaded.getByteRanges().size() != 1 || !loaded.getByteRanges()[0].metadata.tags.empty()) {
        std::cerr << "Version 2 symbol file did not load correctly\n";
        return 1;
    }
    {
        std::ofstream legacy(filename);
        legacy << "legacy 0 12\n";
    }
    if (!loaded.load_from_file(filename) || !loaded.getSymbol("legacy") ||
        loaded.getSymbol("legacy")->offset != 12 || !loaded.getByteRanges().empty()) {
        std::cerr << "Legacy symbol file did not load correctly\n";
        std::remove(filename.c_str());
        return 1;
    }
    {
        std::ofstream invalid(filename);
        invalid << "KACE-SYM 2\nbytes 1 2 \"missing fields\"\n";
    }
    if (loaded.load_from_file(filename) || !loaded.getSymbol("legacy")) {
        std::cerr << "Invalid symbol file was accepted or changed existing data\n";
        std::remove(filename.c_str());
        return 1;
    }
    std::remove(filename.c_str());
    loaded = std::move(table);
    for (const char* name : {"start", "middle", "end"}) {
        std::cout << name << ": offset " << loaded.getSymbol(name)->offset << '\n';
    }
    return 0;
}
