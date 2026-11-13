#include <x86/x86.hpp>
#include <kasm/kasmstatement.hpp>
#include <kasm/kasmparser.hpp>

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
    const char* groupNames[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
                                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
    for (unsigned id = 0; id < 16; ++id) {
        const auto* reg = registers.getRegisterById(id);
        if (!reg || reg != registers.getRegisterByName(groupNames[id])) {
            std::cerr << "Register group lookup failed for ID " << id << '\n';
            return false;
        }
    }
    registers.addRegister(std::make_unique<Register>("other"));
    if (registers.getRegisterById(16) != nullptr) {
        std::cerr << "Unknown register group ID was accepted\n";
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

bool test_displaced_memory_encoding() {
    X86 cpu;
    struct Case {
        X86Mode mode;
        const char* dest;
        const char* base;
        std::int64_t displacement;
        std::vector<std::vector<std::uint8_t>> expected;
    };
    for (const auto& test : {
             Case{X86Mode::Bits32, "eax", "ebx", 127, {{0x8B}, {0x43}, {0x7F}}},
             {X86Mode::Bits32, "eax", "ebx", -128, {{0x8B}, {0x43}, {0x80}}},
             {X86Mode::Bits32, "eax", "ebx", 128, {{0x8B}, {0x83}, {0x80}, {0}, {0}, {0}}},
             {X86Mode::Bits32, "eax", "ebx", -129, {{0x8B}, {0x83}, {0x7F}, {0xFF}, {0xFF}, {0xFF}}},
             {X86Mode::Bits32, "eax", "esp", 4, {{0x8B}, {0x44}, {0x24}, {0x04}}},
             {X86Mode::Bits32, "eax", "ebp", 0, {{0x8B}, {0x45}, {0}}},
             {X86Mode::Bits64, "rax", "rbx", 4, {{0x48}, {0x8B}, {0x43}, {0x04}}},
             {X86Mode::Bits64, "eax", "rbx", -4, {{0x8B}, {0x43}, {0xFC}}},
             {X86Mode::Bits64, "rax", "rsp", 4, {{0x48}, {0x8B}, {0x44}, {0x24}, {0x04}}},
             {X86Mode::Bits64, "rax", "rbp", 0, {{0x48}, {0x8B}, {0x45}, {0}}}}) {
        cpu.setMode(test.mode);
        std::vector<std::vector<std::uint8_t>> bytes;
        std::string error;
        if (!cpu.encoder()->encode_displaced_memory_load(test.dest, test.base, test.displacement, bytes, error) ||
            bytes != test.expected) {
            std::cerr << "Displaced memory encoding failed: " << error << '\n';
            return false;
        }
        std::vector<std::uint8_t> input{0x90};
        for (const auto& part : bytes) input.insert(input.end(), part.begin(), part.end());
        std::size_t offset = 1;
        std::string text;
        const std::string expected = std::string("mov ") + test.dest + ", [" + test.base +
            (test.displacement < 0 ? " - " : " + ") +
            std::to_string(test.displacement < 0 ? -test.displacement : test.displacement) + "]";
        if (!cpu.decoder()->decode(input, offset, text) || offset != input.size() || text != expected) {
            std::cerr << "Displaced memory decoding failed: " << text << '\n';
            return false;
        }
        for (std::size_t length = 1; length < input.size(); ++length) {
            std::vector<std::uint8_t> truncated(input.begin(), input.begin() + length);
            offset = 1;
            if (cpu.decoder()->decode(truncated, offset, text) || offset != 1) {
                std::cerr << "Truncated displaced memory load accepted\n";
                return false;
            }
        }
    }
    for (const auto& test : {
             Case{X86Mode::Bits32, "eax", "ebx", 2147483648LL, {}},
             {X86Mode::Bits64, "rax", "rbx", -2147483649LL, {}},
             {X86Mode::Bits64, "r8", "rbx", 1, {}},
             {X86Mode::Bits32, "eax", "rbx", 1, {}},
             {X86Mode::Bits16, "eax", "ebx", 1, {}}}) {
        cpu.setMode(test.mode);
        std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
        std::string error;
        if (cpu.encoder()->encode_displaced_memory_load(test.dest, test.base, test.displacement, bytes, error) ||
            bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) {
            std::cerr << "Invalid displaced memory encoding accepted\n";
            return false;
        }
    }
    return true;
}

bool test_scaled_memory_encoding() {
    X86 cpu;
    for (const auto mode : {X86Mode::Bits32, X86Mode::Bits64}) {
        cpu.setMode(mode);
        const bool is64 = mode == X86Mode::Bits64;
        const char* dest = is64 ? "rax" : "eax";
        const char* base = is64 ? "rbx" : "ebx";
        const char* index = is64 ? "rcx" : "ecx";
        for (const auto test : {std::pair<unsigned, std::uint8_t>{1, 0x0B},
                                 {2, 0x4B}, {4, 0x8B}, {8, 0xCB}}) {
            std::vector<std::vector<std::uint8_t>> bytes;
            std::string error;
            std::vector<std::vector<std::uint8_t>> expected;
            if (is64) expected.push_back({0x48});
            expected.push_back({0x8B});
            expected.push_back({0x04});
            expected.push_back({test.second});
            if (!cpu.encoder()->encode_scaled_memory_load(dest, base, index, test.first, bytes, error) ||
                bytes != expected) {
                std::cerr << "Scaled memory encoding failed: " << error << '\n';
                return false;
            }
            std::vector<std::uint8_t> input{0x90};
            for (const auto& part : bytes) input.insert(input.end(), part.begin(), part.end());
            std::size_t offset = 1;
            std::string text;
            if (!cpu.decoder()->decode(input, offset, text) || offset != input.size() ||
                text != std::string("mov ") + dest + ", [" + base + " + " + index +
                        " * " + std::to_string(test.first) + "]") {
                std::cerr << "Scaled memory decoding failed: " << text << '\n';
                return false;
            }
            for (std::size_t length = 1; length < input.size(); ++length) {
                const std::vector<std::uint8_t> truncated(input.begin(), input.begin() + length);
                offset = 1;
                if (cpu.decoder()->decode(truncated, offset, text) || offset != 1) {
                    std::cerr << "Truncated scaled memory load accepted\n";
                    return false;
                }
            }
        }
        for (const auto* bad : {is64 ? "rsp" : "esp", is64 ? "r8" : "r8d", "unknown"}) {
            std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
            std::string error;
            if (cpu.encoder()->encode_scaled_memory_load(dest, base, bad, 4, bytes, error) ||
                bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) return false;
        }
        for (unsigned scale : {0u, 3u, 16u}) {
            std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
            std::string error;
            if (cpu.encoder()->encode_scaled_memory_load(dest, base, index, scale, bytes, error) ||
                bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) return false;
        }
        std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
        std::string error;
        if (cpu.encoder()->encode_scaled_memory_load(dest, is64 ? "rbp" : "ebp", index, 4, bytes, error) ||
            bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) return false;
    }
    cpu.setMode(X86Mode::Bits16);
    std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
    std::string error;
    if (cpu.encoder()->encode_scaled_memory_load("eax", "ebx", "ecx", 4, bytes, error) ||
        bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) return false;
    return true;
}

bool test_memory_load_encoding() {
    X86 cpu;
    struct Case {
        X86Mode mode;
        const char* dest;
        const char* base;
        std::vector<std::uint8_t> expected;
    };
    for (const auto& test : {
             Case{X86Mode::Bits16, "ax", "bx", {0x8B, 0x07}},
             {X86Mode::Bits16, "dx", "si", {0x8B, 0x14}},
             {X86Mode::Bits16, "di", "di", {0x8B, 0x3D}},
             {X86Mode::Bits32, "eax", "ebx", {0x8B, 0x03}},
             {X86Mode::Bits32, "eax", "esp", {0x8B, 0x04, 0x24}},
             {X86Mode::Bits32, "edi", "esi", {0x8B, 0x3E}},
             {X86Mode::Bits64, "rax", "rbx", {0x48, 0x8B, 0x03}},
             {X86Mode::Bits64, "rax", "rsp", {0x48, 0x8B, 0x04, 0x24}},
             {X86Mode::Bits64, "rdi", "rax", {0x48, 0x8B, 0x38}}}) {
        cpu.setMode(test.mode);
        std::vector<std::vector<std::uint8_t>> parts;
        std::string error, text;
        if (!cpu.encoder()->encode_memory_register("mov", test.dest, test.base, parts, error)) {
            std::cerr << "Memory load encoding failed: " << error << '\n';
            return false;
        }
        std::vector<std::uint8_t> actual;
        for (const auto& part : parts) actual.insert(actual.end(), part.begin(), part.end());
        if (actual != test.expected) {
            std::cerr << "Incorrect memory load bytes for " << test.dest << '\n';
            return false;
        }
        std::vector<std::uint8_t> input{0x90};
        input.insert(input.end(), actual.begin(), actual.end());
        std::size_t offset = 1;
        if (!cpu.decoder()->decode(input, offset, text) || offset != input.size() ||
            text != std::string("mov ") + test.dest + ", [" + test.base + "]") {
            std::cerr << "Memory load decoding failed: " << text << '\n';
            return false;
        }
        for (std::size_t length = 1; length < input.size(); ++length) {
            std::vector<std::uint8_t> truncated(input.begin(), input.begin() + length);
            offset = 1;
            if (cpu.decoder()->decode(truncated, offset, text) || offset != 1) {
                std::cerr << "Truncated memory load was accepted\n";
                return false;
            }
        }
    }
    for (const auto& test : {
             Case{X86Mode::Bits16, "ax", "bp", {}},
             {X86Mode::Bits32, "eax", "ebp", {}},
             {X86Mode::Bits64, "rax", "rbp", {}},
             {X86Mode::Bits64, "r8", "rax", {}},
             {X86Mode::Bits32, "ax", "ebx", {}}}) {
        cpu.setMode(test.mode);
        std::vector<std::vector<std::uint8_t>> parts{{0x90}};
        std::string error;
        if (cpu.encoder()->encode_memory_register("mov", test.dest, test.base, parts, error) ||
            parts != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) {
            std::cerr << "Invalid memory load accepted\n";
            return false;
        }
    }
    cpu.setMode(X86Mode::Bits64);
    {
        const std::vector<std::uint8_t> invalidSib{0x48, 0x8B, 0x04, 0x20};
        std::size_t offset = 0;
        std::string text;
        if (cpu.decoder()->decode(invalidSib, offset, text) || offset != 0) {
            std::cerr << "Unsupported SIB was accepted\n";
            return false;
        }
    }
    const std::vector<std::uint8_t> unprefixed{0x8B, 0x03};
    std::size_t offset = 0;
    std::string text;
    if (!cpu.decoder()->decode(unprefixed, offset, text) || offset != 2 ||
        text != "mov eax, [rbx]") {
        std::cerr << "Unprefixed 64-bit-mode memory load decoded incorrectly\n";
        return false;
    }
    return true;
}

bool test_register_move_encoding() {
    X86 cpu;
    cpu.setMode(X86Mode::Bits16);
    struct MoveCase { const char* dest; const char* source; std::uint8_t modrm; };
    for (const auto& test : {MoveCase{"ax", "bx", 0xD8}, {"bx", "ax", 0xC3},
                             {"cx", "dx", 0xD1}, {"di", "si", 0xF7},
                             {"sp", "sp", 0xE4}}) {
        std::vector<std::vector<std::uint8_t>> bytes;
        std::string error;
        if (!cpu.encoder()->encode_register("mov", test.dest, test.source, bytes, error) ||
            bytes != std::vector<std::vector<std::uint8_t>>{{0x89}, {test.modrm}}) {
            std::cerr << "Register move encoding failed for " << test.dest << ", "
                      << test.source << ": " << error << '\n';
            return false;
        }
        const std::vector<std::uint8_t> encoded{0x90, 0x89, test.modrm};
        std::size_t offset = 1;
        std::string decoded;
        if (!cpu.decoder()->decode(encoded, offset, decoded) || offset != encoded.size() ||
            decoded != std::string("mov ") + test.dest + ", " + test.source) {
            std::cerr << "Register move decoding failed for " << test.dest << ", "
                      << test.source << " (offset " << offset << ", text '" << decoded << "')\n";
            return false;
        }
    }
    for (const std::vector<std::uint8_t> invalid :
         {std::vector<std::uint8_t>{0x89}, std::vector<std::uint8_t>{0x89, 0x00}}) {
        std::size_t offset = 0;
        std::string decoded;
        if (cpu.decoder()->decode(invalid, offset, decoded) || offset != 0) {
            std::cerr << "Incomplete or memory register move accepted\n";
            return false;
        }
    }
    for (const auto& test : {MoveCase{"ax", "al", 0}, {"r8w", "ax", 0},
                             {"ax", "r9w", 0}, {"unknown", "ax", 0}}) {
        std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
        std::string error;
        if (cpu.encoder()->encode_register("mov", test.dest, test.source, bytes, error) ||
            bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) {
            std::cerr << "Invalid register move accepted\n";
            return false;
        }
    }
    struct WideMoveCase {
        X86Mode mode;
        const char* dest;
        const char* source;
        std::vector<std::vector<std::uint8_t>> expected;
    };
    for (const auto& test : {
             WideMoveCase{X86Mode::Bits32, "eax", "ebx", {{0x89}, {0xD8}}},
             {X86Mode::Bits32, "ebx", "eax", {{0x89}, {0xC3}}},
             {X86Mode::Bits32, "edi", "esi", {{0x89}, {0xF7}}},
             {X86Mode::Bits64, "rax", "rbx", {{0x48}, {0x89}, {0xD8}}},
             {X86Mode::Bits64, "rbx", "rax", {{0x48}, {0x89}, {0xC3}}},
             {X86Mode::Bits64, "rdi", "rsi", {{0x48}, {0x89}, {0xF7}}}}) {
        cpu.setMode(test.mode);
        std::vector<std::vector<std::uint8_t>> bytes;
        std::string error;
        if (!cpu.encoder()->encode_register("mov", test.dest, test.source, bytes, error) ||
            bytes != test.expected) {
            std::cerr << "Register move encoding failed for " << test.dest << ", "
                      << test.source << ": " << error << '\n';
            return false;
        }
        std::vector<std::uint8_t> encoded{0x90};
        for (const auto& part : bytes)
            encoded.insert(encoded.end(), part.begin(), part.end());
        std::size_t offset = 1;
        std::string decoded;
        if (!cpu.decoder()->decode(encoded, offset, decoded) || offset != encoded.size() ||
            decoded != std::string("mov ") + test.dest + ", " + test.source) {
            std::cerr << "Wide register move decoding failed for " << test.dest << ", "
                      << test.source << '\n';
            return false;
        }
        for (std::size_t length = 1; length < encoded.size(); ++length) {
            const std::vector<std::uint8_t> truncated(encoded.begin(), encoded.begin() + length);
            offset = 1;
            if (cpu.decoder()->decode(truncated, offset, decoded) || offset != 1) {
                std::cerr << "Truncated wide register move accepted\n";
                return false;
            }
        }
    }
    cpu.setMode(X86Mode::Bits64);
    {
        const std::vector<std::uint8_t> bytes{0x90, 0x89, 0xD8};
        std::size_t offset = 1;
        std::string decoded;
        if (!cpu.decoder()->decode(bytes, offset, decoded) || offset != bytes.size() ||
            decoded != "mov eax, ebx") {
            std::cerr << "Unprefixed register move in 64-bit mode decoded incorrectly\n";
            return false;
        }
    }
    {
        const std::vector<std::uint8_t> bytes{0x90, 0x48, 0x89, 0x00};
        std::size_t offset = 1;
        std::string decoded;
        if (cpu.decoder()->decode(bytes, offset, decoded) || offset != 1) {
            std::cerr << "Unsupported memory move accepted\n";
            return false;
        }
    }
    for (const auto& test : {
             WideMoveCase{X86Mode::Bits32, "eax", "bx", {}},
             {X86Mode::Bits32, "r8d", "eax", {}},
             {X86Mode::Bits64, "rax", "ebx", {}},
             {X86Mode::Bits64, "rax", "r8", {}}}) {
        cpu.setMode(test.mode);
        std::vector<std::vector<std::uint8_t>> bytes{{0x90}};
        std::string error;
        if (cpu.encoder()->encode_register("mov", test.dest, test.source, bytes, error) ||
            bytes != std::vector<std::vector<std::uint8_t>>{{0x90}} || error.empty()) {
            std::cerr << "Invalid wide register move accepted\n";
            return false;
        }
    }
    return true;
}

bool test_conditional_jumps() {
    X86 cpu;
    for (X86Mode mode : {X86Mode::Bits16, X86Mode::Bits32, X86Mode::Bits64}) {
        cpu.setMode(mode);
        for (const auto& condition : {std::pair<const char*, std::uint8_t>{"jz", 0x74},
                                      {"jnz", 0x75}, {"jc", 0x72}, {"jnc", 0x73}}) {
            for (std::int64_t displacement : {-128, -2, 0, 5, 127}) {
                std::vector<std::uint8_t> bytes{0x90};
                std::string error, text;
                if (!cpu.encoder()->encode_conditional_jump(condition.first, displacement, bytes, error) ||
                    bytes != std::vector<std::uint8_t>{0x90, condition.second,
                                                       static_cast<std::uint8_t>(displacement)}) {
                    std::cerr << "Conditional jump encoding failed for " << condition.first << '\n';
                    return false;
                }
                std::size_t offset = 1;
                if (!cpu.decoder()->decode(bytes, offset, text) || offset != bytes.size() ||
                    text != std::string(condition.first) + " " + std::to_string(displacement)) {
                    std::cerr << "Conditional jump decoding failed for " << condition.first << '\n';
                    return false;
                }
            }
            for (std::int64_t displacement : {-129, 128}) {
                std::vector<std::uint8_t> bytes{0x90};
                std::string error;
                if (cpu.encoder()->encode_conditional_jump(condition.first, displacement, bytes, error) ||
                    error.empty() || bytes != std::vector<std::uint8_t>{0x90}) {
                    std::cerr << "Invalid conditional displacement accepted\n";
                    return false;
                }
            }
            const std::vector<std::uint8_t> truncated{0x90, condition.second};
            std::size_t offset = 1;
            std::string text;
            if (cpu.decoder()->decode(truncated, offset, text) || offset != 1) {
                std::cerr << "Truncated conditional jump accepted\n";
                return false;
            }
        }
        std::vector<std::uint8_t> bytes{0x90};
        std::string error, text;
        if (cpu.encoder()->encode_conditional_jump("unknown", 0, bytes, error) ||
            error.empty() || bytes != std::vector<std::uint8_t>{0x90}) {
            std::cerr << "Unknown conditional jump accepted\n";
            return false;
        }
        const std::vector<std::uint8_t> unsupported{0x70, 0};
        std::size_t offset = 0;
        if (cpu.decoder()->decode_conditional_jump(unsupported, offset, text) || offset != 0) {
            std::cerr << "Unsupported conditional opcode accepted\n";
            return false;
        }
    }
    return true;
}

bool test_source_byte_ranges() {
    const std::vector<Token> tokens;
    KasmParser parser(tokens);
    Source source{"ranges.asm", "start:\n  jz done\n nop\nbyte 0x55\ndone:\n times 2 byte 0\n"};
    std::vector<std::unique_ptr<Statement>> statements;
    std::string error;
    if (!parser.parse(source, statements, error)) {
        std::cerr << "Source range test parsing failed: " << error << '\n';
        return false;
    }
    X86 cpu;
    std::vector<std::uint8_t> bytes;
    SymbolTable symbols;
    if (!cpu.assembler()->assemble(statements, bytes, symbols, error) ||
        bytes != std::vector<std::uint8_t>{0x74, 0x02, 0x90, 0x55, 0x00, 0x00}) {
        std::cerr << "Source range test assembly failed: " << error << '\n';
        return false;
    }
    const auto& ranges = symbols.getByteRanges();
    const std::vector<SymbolByteRange> expected{
        {0, 2, "ranges.asm", 2, 3}, {2, 1, "ranges.asm", 3, 2},
        {3, 1, "ranges.asm", 4, 1}, {4, 2, "ranges.asm", 6, 2}};
    if (ranges.size() != expected.size()) {
        std::cerr << "Unexpected number of source byte ranges\n";
        return false;
    }
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        if (ranges[i].offset != expected[i].offset || ranges[i].length != expected[i].length ||
            ranges[i].sourceName != expected[i].sourceName || ranges[i].line != expected[i].line ||
            ranges[i].column != expected[i].column) {
            std::cerr << "Incorrect source byte range at index " << i << '\n';
            return false;
        }
    }
    symbols.clear();
    if (!symbols.getByteRanges().empty() || symbols.symbolCount() != 0) {
        std::cerr << "Clearing symbols did not clear source byte ranges\n";
        return false;
    }
    return true;
}

bool test_conditional_fixups() {
    X86 cpu;
    std::vector<std::unique_ptr<Statement>> statements;
    auto start = std::make_unique<KasmStatement>();
    start->add_kind("Label");
    start->set_name("start");
    statements.push_back(std::move(start));
    auto forward = std::make_unique<KasmStatement>();
    forward->add_kind("ConditionalJump");
    forward->set_name("jz");
    forward->target = "end";
    statements.push_back(std::move(forward));
    auto nop = std::make_unique<KasmStatement>();
    nop->set_name("nop");
    statements.push_back(std::move(nop));
    auto end = std::make_unique<KasmStatement>();
    end->add_kind("Label");
    end->set_name("end");
    statements.push_back(std::move(end));
    auto backward = std::make_unique<KasmStatement>();
    backward->add_kind("ConditionalJump");
    backward->set_name("jnz");
    backward->target = "start";
    statements.push_back(std::move(backward));
    std::vector<std::uint8_t> bytes;
    SymbolTable symbols;
    std::string error;
    if (!cpu.assembler()->assemble(statements, bytes, symbols, error) ||
        bytes != std::vector<std::uint8_t>{0x74, 0x01, 0x90, 0x75, 0xFB}) {
        std::cerr << "Conditional jump fixups failed: " << error << '\n';
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

        std::vector<std::unique_ptr<Statement>> statements;
        auto start = std::make_unique<KasmStatement>();
        start->add_kind("Label"); start->set_name("start");
        statements.push_back(std::move(start));
        auto forward = std::make_unique<KasmStatement>();
        forward->add_kind("JumpNear"); forward->target = "end";
        statements.push_back(std::move(forward));
        auto nop = std::make_unique<KasmStatement>();
        nop->set_name("nop");
        statements.push_back(std::move(nop));
        auto end = std::make_unique<KasmStatement>();
        end->add_kind("Label"); end->set_name("end");
        statements.push_back(std::move(end));
        auto backward = std::make_unique<KasmStatement>();
        backward->add_kind("JumpNear"); backward->target = "start";
        statements.push_back(std::move(backward));
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
    if (!test_displaced_memory_encoding()) return 1;
    if (!test_scaled_memory_encoding()) return 1;
    if (!test_memory_load_encoding()) return 1;
    if (!test_register_file()) return 1;
    if (!test_register_move_encoding()) return 1;
    if (!test_conditional_jumps()) return 1;
    if (!test_source_byte_ranges()) return 1;
    if (!test_conditional_fixups()) return 1;
    if (!test_near_modes()) return 1;
    X86 x86;
    {
        std::vector<std::uint8_t> encoded{0x90};
        std::string error;
        if (!x86.encoder()->encode_interrupt("0x21", encoded, error) ||
            encoded != std::vector<std::uint8_t>{0x90, 0xCD, 0x21}) {
            std::cerr << "Software interrupt encoded incorrectly\n";
            return 1;
        }
        for (const auto* invalid : {"256", "0x100", "nope"}) {
            if (x86.encoder()->encode_interrupt(invalid, encoded, error) ||
                encoded != std::vector<std::uint8_t>{0x90, 0xCD, 0x21}) {
                std::cerr << "Invalid software interrupt was accepted or changed output\n";
                return 1;
            }
        }
    }
    {
        const std::vector<std::uint8_t> instruction{0x90, 0xCD, 0x21};
        std::size_t position = 1;
        std::string decoded;
        if (!x86.decoder()->decode(instruction, position, decoded) ||
            position != 3 || decoded != "int 33") {
            std::cerr << "Software interrupt decoded incorrectly\n";
            return 1;
        }
        const std::vector<std::uint8_t> truncated{0x90, 0xCD};
        position = 1;
        if (x86.decoder()->decode(truncated, position, decoded) || position != 1) {
            std::cerr << "Truncated software interrupt was accepted\n";
            return 1;
        }
    }
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
        for (std::uint8_t value : {std::uint8_t{0}, std::uint8_t{0x42}, std::uint8_t{0xFF}}) {
            const std::vector<std::uint8_t> encoded{0x90, reg.second, value};
            std::size_t position = 1;
            std::string decoded;
            if (!x86.decoder()->decode(encoded, position, decoded) || position != encoded.size() ||
                decoded != std::string("mov ") + reg.first + ", " + std::to_string(value)) {
                std::cerr << "mov " << reg.first << " immediate decoded incorrectly\n";
                return 1;
            }
        }
        const std::vector<std::uint8_t> truncated{0x90, reg.second};
        std::size_t position = 1;
        std::string decoded;
        if (x86.decoder()->decode(truncated, position, decoded) || position != 1) {
            std::cerr << "x86 accepted truncated mov " << reg.first << '\n';
            return 1;
        }
    }
    for (const char* reg : {"spl", "r8w", "r8b"}) {
        bytes.clear();
        if (x86.encoder()->encode_immediate("mov", reg, 0, bytes, error) || !bytes.empty()) {
            std::cerr << "x86 accepted unsupported register " << reg << '\n';
            return 1;
        }
    }
    for (const auto& test : {
             std::pair<X86Mode, std::vector<std::uint8_t>>{X86Mode::Bits16, {0xB9, 0x34, 0x12}},
             {X86Mode::Bits32, {0x66, 0xB9, 0x34, 0x12}},
             {X86Mode::Bits64, {0x66, 0xB9, 0x34, 0x12}}}) {
        x86.setMode(test.first);
        std::size_t position = 0;
        std::string decoded;
        if (!x86.decoder()->decode(test.second, position, decoded) ||
            position != test.second.size() || decoded != "mov cx, 4660") {
            std::cerr << "16-bit immediate decoding failed\n";
            return 1;
        }
        for (std::size_t length = 0; length < test.second.size(); ++length) {
            std::vector<std::uint8_t> truncated(test.second.begin(), test.second.begin() + length);
            position = 0;
            if (x86.decoder()->decode(truncated, position, decoded) || position != 0) {
                std::cerr << "Truncated immediate move accepted\n";
                return 1;
            }
        }
    }
    x86.setMode(X86Mode::Bits64);
    for (const auto& test : {
             std::pair<std::vector<std::uint8_t>, const char*>{{0xB8, 0x78, 0x56, 0x34, 0x12}, "mov eax, 305419896"},
             {{0x48, 0xB8, 0x78, 0x56, 0x34, 0x12, 0, 0, 0, 0}, "mov rax, 305419896"}}) {
        std::size_t position = 0;
        std::string decoded;
        if (!x86.decoder()->decode(test.first, position, decoded) ||
            position != test.first.size() || decoded != test.second) {
            std::cerr << "32/64-bit immediate decoding failed\n";
            return 1;
        }
    }
    const char* names16[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    const char* names32[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
    const char* names64[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi"};
    for (X86Mode mode : {X86Mode::Bits16, X86Mode::Bits32, X86Mode::Bits64}) {
        x86.setMode(mode);
        for (unsigned width : {16u, 32u, 64u}) {
            if (width == 64 && mode != X86Mode::Bits64) continue;
            for (unsigned id = 0; id < 8; ++id) {
                std::vector<std::uint8_t> instruction{0x90};
                if ((width == 16 && mode != X86Mode::Bits16) ||
                    (width == 32 && mode == X86Mode::Bits16))
                    instruction.push_back(0x66);
                if (width == 64) instruction.push_back(0x48);
                instruction.push_back(static_cast<std::uint8_t>(0xB8 + id));
                const std::uint64_t value = width == 16 ? 0xABCDULL :
                                            width == 32 ? 0x89ABCDEFULL : 0xFEDCBA9876543210ULL;
                for (unsigned i = 0; i < width / 8; ++i)
                    instruction.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
                const char* name = width == 16 ? names16[id] : width == 32 ? names32[id] : names64[id];
                std::string decoded;
                std::size_t position = 1;
                if (!x86.decoder()->decode(instruction, position, decoded) ||
                    position != instruction.size() ||
                    decoded != std::string("mov ") + name + ", " + std::to_string(value)) {
                    std::cerr << "Immediate decoding failed for " << name << '\n';
                    return 1;
                }
                for (std::size_t length = 1; length < instruction.size(); ++length) {
                    const std::vector<std::uint8_t> truncated(instruction.begin(), instruction.begin() + length);
                    position = 1;
                    if (x86.decoder()->decode(truncated, position, decoded) || position != 1) {
                        std::cerr << "Truncated immediate accepted for " << name << '\n';
                        return 1;
                    }
                }
            }
        }
    }
    x86.setMode(X86Mode::Bits16);
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
