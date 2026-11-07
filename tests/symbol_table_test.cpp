#include <kace/symbols.hpp>

#include <cstdio>
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
    if (!table.addLabel("start", 0) || !table.addLabel("middle", 1) ||
        !table.addLabel("end", 2) || table.addLabel("middle", 99) ||
        !check_labels(table)) {
        std::cerr << "Label insertion, lookup, or duplicate detection failed\n";
        return 1;
    }

    const std::string filename = "symbol_table_test_symbols.txt";
    if (!table.save_to_file(filename)) {
        std::cerr << "Could not save symbol table\n";
        return 1;
    }
    SymbolTable loaded;
    const bool loaded_ok = loaded.load_from_file(filename);
    std::remove(filename.c_str());
    if (!loaded_ok || !check_labels(loaded)) {
        std::cerr << "Symbol table did not survive save and load\n";
        return 1;
    }
    for (const char* name : {"start", "middle", "end"}) {
        std::cout << name << ": offset " << loaded.getSymbol(name)->offset << '\n';
    }
    return 0;
}
