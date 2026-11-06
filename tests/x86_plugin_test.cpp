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
    return 0;
}
