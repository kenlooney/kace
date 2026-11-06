#include <kace/source.hpp>
#include <algorithm>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: kasm_source_load_test <source-file>\n";
        return 1;
    }

    Source source;
    if (!Source::load(argv[1], source)) {
        std::cerr << "Could not open: " << argv[1] << "\n";
        return 1;
    }

    std::string normalized_text = source.text;
    normalized_text.erase(
        std::remove(normalized_text.begin(), normalized_text.end(), '\r'),
        normalized_text.end()
    );
    if (source.name != argv[1] || normalized_text != "start:\n    nop") {
        std::cerr << "Loaded source did not match the expected example\n";
        return 1;
    }

    return 0;
}
