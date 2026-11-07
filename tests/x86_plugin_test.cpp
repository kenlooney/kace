#include <x86/x86.hpp>

#include <cstdint>
#include <iostream>

int main() {
    X86 x86;
    const Cpu& cpu = x86;
    std::uint8_t byte = 0;

    if (!cpu.encode_no_operand("nop", byte) || byte != 0x90) {
        std::cerr << "x86 nop did not encode as 0x90\n";
        return 1;
    }
    if (cpu.encode_no_operand("unknown_instruction", byte) ||
        cpu.encode_no_operand("", byte)) {
        std::cerr << "x86 accepted an unsupported instruction\n";
        return 1;
    }
    std::vector<std::vector<std::uint8_t>> bytes;
    std::string error;
    for (std::uint64_t value : {0u, 42u, 255u}) {
        bytes.clear();
        if (!cpu.encode_immediate("mov", "al", value, bytes, error)) {
            std::cerr << "mov al immediate failed: " << error << '\n';
            return 1;
        }
        std::vector<std::uint8_t> encoded;
        for (const auto& part : bytes) {
            encoded.insert(encoded.end(), part.begin(), part.end());
        }
        if (encoded != std::vector<std::uint8_t>{0xB0, static_cast<std::uint8_t>(value)}) {
            std::cerr << "mov al immediate produced incorrect bytes\n";
            return 1;
        }
    }
    bytes.clear();
    if (cpu.encode_immediate("mov", "al", 256, bytes, error) ||
        cpu.encode_immediate("copy", "ax", 42, bytes, error) ||
        cpu.encode_immediate("add", "al", 42, bytes, error) || !bytes.empty()) {
        std::cerr << "x86 accepted invalid immediate operands\n";
        return 1;
    }
    return 0;
}
