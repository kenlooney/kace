#include <x86/x86.hpp>

#include <cstdint>
#include <iostream>
#include <utility>

bool test_register_file() {
    X86 cpu;
    X86RegisterFile& registers = cpu.registers();
    X86Register* rax = registers.getRegisterByName("rax");
    if (!rax || registers.getRegisterByName("eax") != rax ||
        registers.getRegisterByName("ax") != rax ||
        registers.getRegisterByName("ah") != rax ||
        registers.getRegisterByName("al") != rax || registers.getRegisters().size() != 16) {
        std::cerr << "Register aliases did not resolve to RAX\n";
        return false;
    }
    rax->setValue(0x12345678ABCDEF42ULL);
    std::uint64_t value = 0;
    if (!registers.getRegisterValue("eax", value) || value != 0xABCDEF42ULL ||
        !registers.getRegisterValue("ax", value) || value != 0xEF42 ||
        !registers.getRegisterValue("ah", value) || value != 0xEF ||
        !registers.getRegisterValue("al", value) || value != 0x42 ||
        !registers.getRegisterValue("rax", value) || value != 0x12345678ABCDEF42ULL ||
        registers.getRegisterValue("unknown", value)) {
        std::cerr << "Register views returned incorrect values\n";
        return false;
    }
    unsigned code = 0;
    const auto* cx = registers.getRegisterByName("cx");
    const auto* ah = registers.getRegisterByName("ah");
    const auto* r15b = registers.getRegisterByName("r15b");
    if (!cx || !cx->getEncodingId("cx", code) || code != 1 ||
        !ah || !ah->getEncodingId("ah", code) || code != 4 ||
        !r15b || !r15b->getEncodingId("r15b", code) || code != 15) {
        std::cerr << "Register encoding IDs are incorrect\n";
        return false;
    }
    const Cpu& generic = cpu;
    if (generic.registers().getRegisterByName("al") != rax) {
        std::cerr << "CPU interface did not expose the x86 register file\n";
        return false;
    }
    X86 other;
    if (!other.registers().getRegisterValue("rax", value) || value != 0) {
        std::cerr << "CPU register files share state\n";
        return false;
    }
    return true;
}

bool test_near_modes() {
    X86 cpu;
    for (const X86Mode mode : {X86Mode::Bits16, X86Mode::Bits32, X86Mode::Bits64}) {
        cpu.setMode(mode);
        const bool wide = mode != X86Mode::Bits16;
        const std::int64_t minimum = wide ? -2147483648LL : -32768;
        const std::int64_t maximum = wide ? 2147483647LL : 32767;
        const std::vector<std::uint8_t> expected = wide
            ? std::vector<std::uint8_t>{0xE9, 0xFE, 0xFF, 0xFF, 0xFF}
            : std::vector<std::uint8_t>{0xE9, 0xFE, 0xFF};
        for (const auto value : {minimum, std::int64_t{-2}, std::int64_t{0}, maximum}) {
            std::vector<std::uint8_t> encoded;
            std::string error, decoded;
            std::size_t offset = 0;
            if (!cpu.encoder()->encode_near_jump(value, encoded, error) ||
                (value == -2 && encoded != expected) ||
                !cpu.decoder()->decode(encoded, offset, decoded) ||
                offset != encoded.size() || decoded != "jmp " + std::to_string(value)) {
                std::cerr << "Near jump mode round trip failed: " << error << '\n';
                return false;
            }
            for (std::size_t size = 0; size < encoded.size(); ++size) {
                std::vector<std::uint8_t> truncated(encoded.begin(), encoded.begin() + size);
                offset = 0;
                if (cpu.decoder()->decode(truncated, offset, decoded) || offset != 0) {
                    std::cerr << "Truncated near jump accepted\n";
                    return false;
                }
            }
        }
        for (const auto value : {minimum - 1, maximum + 1}) {
            std::vector<std::uint8_t> encoded{0x90};
            std::string error;
            if (cpu.encoder()->encode_near_jump(value, encoded, error) ||
                encoded != std::vector<std::uint8_t>{0x90}) {
                std::cerr << "Out-of-range near jump accepted or changed output\n";
                return false;
            }
        }

        std::vector<Statement> statements(5);
        statements[0].kind = StatementKind::Label; statements[0].name = "start";
        statements[1].kind = StatementKind::JumpNear; statements[1].target = "end";
        statements[2].name = "nop";
        statements[3].kind = StatementKind::Label; statements[3].name = "end";
        statements[4].kind = StatementKind::JumpNear; statements[4].target = "start";
        std::vector<std::uint8_t> assembled;
        SymbolTable symbols;
        std::string error, decoded;
        const std::vector<std::uint8_t> want = wide
            ? std::vector<std::uint8_t>{0xE9, 1, 0, 0, 0, 0x90, 0xE9, 0xF5, 0xFF, 0xFF, 0xFF}
            : std::vector<std::uint8_t>{0xE9, 1, 0, 0x90, 0xE9, 0xF9, 0xFF};
        if (!cpu.assembler()->assemble(statements, assembled, symbols, error) || assembled != want ||
            !cpu.disassembler()->disassemble(assembled, symbols, decoded, error) ||
            decoded != "start:\njmp near end\nnop\nend:\njmp near start\n") {
            std::cerr << "Mode-aware near jump fixup failed: " << error << '\n';
            return false;
        }
    }
    cpu.setMode(static_cast<X86Mode>(-1));
    std::vector<std::uint8_t> encoded{0x90};
    std::string error, text;
    std::size_t offset = 0;
    const std::vector<std::uint8_t> nearJump{0xE9, 0, 0};
    if (cpu.encoder()->encode_near_jump(0, encoded, error) ||
        encoded != std::vector<std::uint8_t>{0x90} ||
        cpu.decoder()->decode(nearJump, offset, text) || offset != 0) {
        std::cerr << "Invalid mode accepted for near jump\n";
        return false;
    }
    return true;
}

int main() {
    if (!test_register_file()) return 1;
    if (!test_near_modes()) return 1;
    X86 x86;
    std::uint8_t byte = 0;

    if (!x86.encoder()->encode_no_operand("nop", byte) || byte != 0x90) {
        std::cerr << "x86 nop did not encode as 0x90\n";
        return 1;
    }
    if (x86.encoder()->encode_no_operand("unknown_instruction", byte) ||
        x86.encoder()->encode_no_operand("", byte)) {
        std::cerr << "x86 accepted an unsupported instruction\n";
        return 1;
    }
    std::vector<std::vector<std::uint8_t>> bytes;
    std::string error;
    x86.setMode(X86Mode::Bits16);
    for (const auto& reg : {std::pair<const char*, std::uint8_t>{"ax", 0xB8},
                            {"cx", 0xB9}, {"dx", 0xBA}, {"bx", 0xBB},
                            {"sp", 0xBC}, {"bp", 0xBD}, {"si", 0xBE}, {"di", 0xBF}}) {
        for (std::uint64_t value : {0u, 0x1234u, 0xFFFFu}) {
            bytes.clear();
            error.clear();
            if (!x86.encoder()->encode_immediate("mov", reg.first, value, bytes, error)) {
                std::cerr << "mov " << reg.first << " immediate failed: " << error << '\n';
                return 1;
            }
            std::vector<std::uint8_t> encoded;
            for (const auto& part : bytes)
                encoded.insert(encoded.end(), part.begin(), part.end());
            if (encoded != std::vector<std::uint8_t>{reg.second, static_cast<std::uint8_t>(value),
                                                     static_cast<std::uint8_t>(value >> 8)}) {
                std::cerr << "mov " << reg.first << " immediate produced incorrect bytes\n";
                return 1;
            }
        }
    }
    bytes.clear();
    if (x86.encoder()->encode_immediate("mov", "ax", 0x10000, bytes, error) || !bytes.empty()) {
        std::cerr << "x86 accepted an out-of-range 16-bit immediate\n";
        return 1;
    }
    for (const auto& reg : {std::pair<const char*, std::uint8_t>{"al", 0xB0},
                            {"cl", 0xB1}, {"dl", 0xB2}, {"bl", 0xB3},
                            {"ah", 0xB4}, {"ch", 0xB5}, {"dh", 0xB6}, {"bh", 0xB7}}) {
        bytes.clear();
        if (!x86.encoder()->encode_immediate("mov", reg.first, 0x42, bytes, error) ||
            bytes != std::vector<std::vector<std::uint8_t>>{{reg.second}, {0x42}}) {
            std::cerr << "mov " << reg.first << " immediate encoded incorrectly\n";
            return 1;
        }
    }
    for (const char* reg : {"eax", "spl", "r8w", "r8b"}) {
        bytes.clear();
        if (x86.encoder()->encode_immediate("mov", reg, 0, bytes, error) || !bytes.empty()) {
            std::cerr << "x86 accepted unsupported register " << reg << '\n';
            return 1;
        }
    }
    for (std::uint64_t value : {0u, 42u, 255u}) {
        bytes.clear();
        if (!x86.encoder()->encode_immediate("mov", "al", value, bytes, error)) {
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
    if (x86.encoder()->encode_immediate("mov", "al", 256, bytes, error) ||
        x86.encoder()->encode_immediate("copy", "ax", 42, bytes, error) ||
        x86.encoder()->encode_immediate("add", "al", 42, bytes, error) || !bytes.empty()) {
        std::cerr << "x86 accepted invalid immediate operands\n";
        return 1;
    }
    for (const auto& test : {std::pair<std::int64_t, std::vector<std::uint8_t>>{0x1234, {0xE9, 0x34, 0x12}},
                             {-5, {0xE9, 0xFB, 0xFF}}}) {
        std::vector<std::uint8_t> encoded;
        std::size_t position = 0;
        std::string decoded;
        if (!x86.encoder()->encode_near_jump(test.first, encoded, error) || encoded != test.second ||
            !x86.decoder()->decode(encoded, position, decoded) || position != 3 ||
            decoded != "jmp " + std::to_string(test.first)) {
            std::cerr << "Near jump encoding or decoding failed\n";
            return 1;
        }
    }
    {
        x86.setMode(X86Mode::Bits32);
        std::vector<std::uint8_t> encoded;
        if (!x86.encoder()->encode_near_jump(-2, encoded, error) ||
            encoded != std::vector<std::uint8_t>{0xE9, 0xFE, 0xFF, 0xFF, 0xFF}) {
            std::cerr << "32-bit near jump encoded incorrectly\n";
            return 1;
        }
        x86.setMode(X86Mode::Bits16);
    }
    {
        std::vector<std::uint8_t> encoded;
        std::size_t position = 0;
        std::string decoded;
        if (!x86.encoder()->encode_far_jump(0x1234, 0x5678, encoded, error) ||
            encoded != std::vector<std::uint8_t>{0xEA, 0x78, 0x56, 0x34, 0x12} ||
            !x86.decoder()->decode(encoded, position, decoded) || position != 5 ||
            decoded != "jmp far 4660:22136") {
            std::cerr << "Far jump encoding or decoding failed\n";
            return 1;
        }
        for (std::size_t length = 0; length < encoded.size(); ++length) {
            const std::vector<std::uint8_t> truncated(encoded.begin(), encoded.begin() + length);
            position = 0;
            if (x86.decoder()->decode(truncated, position, decoded) || position != 0) {
                std::cerr << "Truncated far jump was accepted\n";
                return 1;
            }
        }
        std::vector<std::uint8_t> invalid;
        if (x86.encoder()->encode_far_jump(0x10000, 0, invalid, error) ||
            x86.encoder()->encode_far_jump(0, 0x10000, invalid, error)) {
            std::cerr << "Out-of-range far jump was accepted\n";
            return 1;
        }
        x86.setMode(X86Mode::Bits32);
        position = 0;
        if (x86.encoder()->encode_far_jump(0, 0, invalid, error) ||
            x86.decoder()->decode(encoded, position, decoded) || position != 0) {
            std::cerr << "Far jump accepted in unsupported mode\n";
            return 1;
        }
        x86.setMode(X86Mode::Bits16);
    }
    const std::vector<std::uint8_t> instructions{0xEB, 0x01, 0x90, 0xEB, 0xFB, 0xB0, 42};
    std::size_t offset = 0;
    std::string text;
    if (!x86.decoder()->decode(instructions, offset, text) || text != "jmp 1" || offset != 2 ||
        !x86.decoder()->decode(instructions, offset, text) || text != "nop" || offset != 3 ||
        !x86.decoder()->decode(instructions, offset, text) || text != "jmp -5" || offset != 5 ||
        !x86.decoder()->decode(instructions, offset, text) || text != "mov al, 42" || offset != 7 ||
        x86.decoder()->decode(instructions, offset, text) || offset != 7) {
        std::cerr << "x86 failed to decode instruction sequence\n";
        return 1;
    }
    for (const std::vector<std::uint8_t> invalid :
         {std::vector<std::uint8_t>{0xB0}, std::vector<std::uint8_t>{0xEB}, std::vector<std::uint8_t>{0xFF}}) {
        offset = 0;
        if (x86.decoder()->decode(invalid, offset, text) || offset != 0) {
            std::cerr << "x86 accepted incomplete or unknown instruction\n";
            return 1;
        }
    }
    std::cout << "Decoded forward and backward short jumps; rejected incomplete jump\n";
    return 0;
}
