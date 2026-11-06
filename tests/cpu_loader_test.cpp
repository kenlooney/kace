#include <kace/loader.hpp>

#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 3) return 1;
    Loader loader;
    std::string error;
    if (loader.open(std::string(argv[1]) + ".missing", error) || error.empty()) {
        std::cerr << "Missing plugin was not rejected\n";
        return 1;
    }
    if (loader.open(argv[2], error) || error.find("factory") == std::string::npos) {
        std::cerr << "Library without a plugin factory was not rejected: " << error << '\n';
        return 1;
    }
    if (!loader.open(argv[1], error) || !loader.cpu()) {
        std::cerr << "Could not load x86 after a failed load: " << error << '\n';
        return 1;
    }
    std::uint8_t byte = 0;
    if (!loader.cpu()->encode_no_operand("nop", byte) || byte != 0x90) {
        std::cerr << "Loaded x86 plugin did not encode nop\n";
        return 1;
    }
    if (loader.open(argv[1], error)) {
        std::cerr << "Loader accepted a second plugin while one was open\n";
        return 1;
    }
    return 0;
}
