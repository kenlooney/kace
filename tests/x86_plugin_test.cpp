#include <x86/x86.hpp>

#include <cstdint>
#include <iostream>
#include <utility>

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
    for (const auto& test : {std::pair<std::int64_t, std::vector<std::uint8_t>>{0x1234, {0xE9, 0x34, 0x12}},
                             {-5, {0xE9, 0xFB, 0xFF}}}) {
        std::vector<std::uint8_t> encoded;
        std::size_t position = 0;
        std::string decoded;
        if (!cpu.encode_near_jump(test.first, encoded, error) || encoded != test.second ||
            !cpu.decode(encoded, position, decoded) || position != 3 ||
            decoded != "jmp " + std::to_string(test.first)) {
            std::cerr << "Near jump encoding or decoding failed\n";
            return 1;
        }
    }
    {
        std::vector<std::uint8_t> encoded;
        std::size_t position = 0;
        std::string decoded;
        if (!cpu.encode_far_jump(0x1234, 0x5678, encoded, error) ||
            encoded != std::vector<std::uint8_t>{0xEA, 0x78, 0x56, 0x34, 0x12} ||
            !cpu.decode(encoded, position, decoded) || position != 5 ||
            decoded != "jmp far 4660:22136") {
            std::cerr << "Far jump encoding or decoding failed\n";
            return 1;
        }
        for (std::size_t length = 0; length < encoded.size(); ++length) {
            const std::vector<std::uint8_t> truncated(encoded.begin(), encoded.begin() + length);
            position = 0;
            if (cpu.decode(truncated, position, decoded) || position != 0) {
                std::cerr << "Truncated far jump was accepted\n";
                return 1;
            }
        }
        std::vector<std::uint8_t> invalid;
        if (cpu.encode_far_jump(0x10000, 0, invalid, error) ||
            cpu.encode_far_jump(0, 0x10000, invalid, error)) {
            std::cerr << "Out-of-range far jump was accepted\n";
            return 1;
        }
        x86.setMode(X86Mode::Bits32);
        position = 0;
        if (cpu.encode_far_jump(0, 0, invalid, error) ||
            cpu.decode(encoded, position, decoded) || position != 0) {
            std::cerr << "Far jump accepted in unsupported mode\n";
            return 1;
        }
        x86.setMode(X86Mode::Bits16);
    }
    const std::vector<std::uint8_t> instructions{0xEB, 0x01, 0x90, 0xEB, 0xFB, 0xB0, 42};
    std::size_t offset = 0;
    std::string text;
    if (!cpu.decode(instructions, offset, text) || text != "jmp 1" || offset != 2 ||
        !cpu.decode(instructions, offset, text) || text != "nop" || offset != 3 ||
        !cpu.decode(instructions, offset, text) || text != "jmp -5" || offset != 5 ||
        !cpu.decode(instructions, offset, text) || text != "mov al, 42" || offset != 7 ||
        cpu.decode(instructions, offset, text) || offset != 7) {
        std::cerr << "x86 failed to decode instruction sequence\n";
        return 1;
    }
    for (const std::vector<std::uint8_t> invalid :
         {std::vector<std::uint8_t>{0xB0}, std::vector<std::uint8_t>{0xEB}, std::vector<std::uint8_t>{0xFF}}) {
        offset = 0;
        if (cpu.decode(invalid, offset, text) || offset != 0) {
            std::cerr << "x86 accepted incomplete or unknown instruction\n";
            return 1;
        }
    }
    std::cout << "Decoded forward and backward short jumps; rejected incomplete jump\n";
    return 0;
}
