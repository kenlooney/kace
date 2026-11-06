#include <kace/integers.hpp>
#include <cstdint>
#include <string>

static bool parses_as(const std::string& text, std::uint64_t expected) {
    std::uint64_t value = 0;
    return parse_integer(text, value) && value == expected;
}

static bool rejects(const std::string& text) {
    std::uint64_t value = 123;
    return !parse_integer(text, value) && value == 123;
}

int main() {
    if (!parses_as("10", 10) ||
        !parses_as("0b1010", 10) ||
        !parses_as("0B1010", 10) ||
        !parses_as("1010b", 10) ||
        !parses_as("1010B", 10) ||
        !parses_as("0xA", 10) ||
        !parses_as("0XA", 10) ||
        !parses_as("0Ah", 10) ||
        !parses_as("0AH", 10) ||
        !parses_as("0o12", 10) ||
        !parses_as("0O12", 10) ||
        !parses_as("12o", 10) ||
        !parses_as("12O", 10)) {
        return 1;
    }

    if (!rejects("") ||
        !rejects("0b") ||
        !rejects("0x") ||
        !rejects("0o") ||
        !rejects("0b102") ||
        !rejects("0xG") ||
        !rejects("12q") ||
        !rejects("123abc")) {
        return 1;
    }

    return 0;
}
