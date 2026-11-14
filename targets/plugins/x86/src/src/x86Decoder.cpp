/*
 * Copyright (C) 2026 Kenneth Looney
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <x86/x86.hpp>

X86Mode X86Decoder::mode() const
{
    return cpu_.getMode();
}

// Decoding
bool X86Decoder::decode(
    const std::vector<std::uint8_t> &bytes,
    std::size_t &offset,
    std::string &text) const
{
    if (offset >= bytes.size())
        return false;

    switch (bytes[offset])
    {
    case 0x90:
        return decode_no_operand(bytes, offset, text);
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF:
    case 0x66:
        return decode_immediate(bytes, offset, text);
    case 0x48:
    {
        if (mode() != X86Mode::Bits64 || bytes.size() - offset < 2)
            return false;
        if (bytes[offset + 1] == 0x89)
            return decode_register(bytes, offset, text);
        if (bytes[offset + 1] == 0x8B)
            return decode_memory_register(bytes, offset, text);

        return decode_immediate(bytes, offset, text);
    }
    case 0xEB:
        return decode_short_jump(bytes, offset, text);
    case 0xE9:
        return decode_near_jump(bytes, offset, text);
    case 0xEA:
        return decode_far_jump(bytes, offset, text);
    case 0x74:
    case 0x75:
    case 0x72:
    case 0x73:
        return decode_conditional_jump(bytes, offset, text);
    case 0x89:
        return decode_register(bytes, offset, text);
    case 0xCD:
        return decode_interrupt(bytes, offset, text);
    case 0xF4:
        return decode_halt(bytes, offset, text);
    case 0x8B:
        return decode_memory_register(bytes, offset, text);
    case 0x8C:
    case 0x8E:
        return decode_segment_move(bytes, offset, text);

    default:
        return false;
    }
}

bool X86Decoder::decode_halt(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if (offset >= bytes.size())
        return false;

    if (bytes[offset] == 0xF4)
    {
        ++offset;
        text = "hlt";
        return true;
    }

    return false;
}
bool X86Decoder::decode_no_operand(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if (offset >= bytes.size())
        return false;

    if (bytes[offset] == 0x90)
    {
        ++offset;
        text = "nop";
        return true;
    }

    return false;
}

bool X86Decoder::decode_immediate(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if (offset >= bytes.size() || bytes.size() - offset < 2)
        return false;

    // 16-, 32-, and 64-bit register immediate decoding
    std::size_t opcode_offset = offset;
    unsigned width = 0;
    if (bytes[opcode_offset] == 0x66)
    {
        if (mode() == X86Mode::Bits16)
            width = 32;
        else if (mode() == X86Mode::Bits32 || mode() == X86Mode::Bits64)
            width = 16;
        else
            return false;
        ++opcode_offset;
    }
    else if (bytes[opcode_offset] == 0x48 && mode() == X86Mode::Bits64)
    {
        width = 64;
        ++opcode_offset;
    }

    const std::uint8_t opcode = bytes[opcode_offset];
    if (opcode >= 0xB8 && opcode <= 0xBF)
    {
        if (width == 0)
        {
            if (mode() == X86Mode::Bits16)
                width = 16;
            else if (mode() == X86Mode::Bits32 || mode() == X86Mode::Bits64)
                width = 32;
            else
                return false;
        }
        const std::size_t immediate_size = width / 8;
        if (bytes.size() - opcode_offset - 1 < immediate_size)
            return false;

        static const char *const names16[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
        static const char *const names32[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
        static const char *const names64[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi"};
        const char *const *names = width == 16 ? names16 : width == 32 ? names32
                                                                       : names64;
        std::uint64_t value = 0;
        for (std::size_t i = 0; i < immediate_size; ++i)
            value |= static_cast<std::uint64_t>(bytes[opcode_offset + 1 + i]) << (8 * i);
        text = std::string("mov ") + names[opcode - 0xB8] + ", " + std::to_string(value);
        offset = opcode_offset + 1 + immediate_size;
        return true;
    }
    if (opcode_offset != offset)
        return false;

    // 8-bit register immediate decoding
    switch (bytes[offset])
    {
    case 0xB0:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov al, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB1:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov cl, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB2:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov dl, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB3:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov bl, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB4:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov ah, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB5:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov ch, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB6:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov dh, " + std::to_string(value);
        return true;
    }
    break;
    case 0xB7:
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov bh, " + std::to_string(value);
        return true;
    }
    break;

    default:
        return false;
    }
}

bool X86Decoder::decode_register(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    bool hasRexW = false;
    std::size_t opcodeOffset = offset;

    if (offset >= bytes.size() || bytes.size() - offset < 2)
        return false;

    if (mode() == X86Mode::Bits64)
    {

        if (mode() == X86Mode::Bits64 && bytes[offset] == 0x48)
        {
            if (bytes.size() - offset < 3)
                return false;
            hasRexW = true;
            opcodeOffset = offset + 1;
        }
    }
    if (mode() != X86Mode::Bits16 &&
        mode() != X86Mode::Bits32 &&
        mode() != X86Mode::Bits64)
        return false;

    if (bytes[opcodeOffset] != 0x89)
        return false;

    // Read ModR/M byte and check its top two bits
    std::uint8_t modrm = bytes[opcodeOffset + 1];

    if ((modrm & 0xC0) != 0xC0)
        return false;

    const unsigned reg_source = (modrm >> 3) & 0x07;
    const unsigned reg_dest = modrm & 0x07;

    // Find both source and destination registers by their IDs
    const X86Register *src = cpu_.registers().getRegisterById(reg_source);
    const X86Register *dst = cpu_.registers().getRegisterById(reg_dest);

    // If either source or destination register is not found, decoding fails
    if (!src || !dst)
        return false;

    // Get names
    const std::string *sourceName = nullptr;
    const std::string *destName = nullptr;
    if (mode() == X86Mode::Bits16)
    {
        sourceName = src->getAliasByWidth(16);
        destName = dst->getAliasByWidth(16);
    }
    else if (mode() == X86Mode::Bits32)
    {
        sourceName = src->getAliasByWidth(32);
        destName = dst->getAliasByWidth(32);
    }
    else if (mode() == X86Mode::Bits64)
    {
        unsigned width = hasRexW ? 64 : 32;
        sourceName = src->getAliasByWidth(width);
        destName = dst->getAliasByWidth(width);
    }
    else
        return false;

    if (!sourceName || !destName)
        return false;

    // Advance offset past the opcode and ModR/M byte
    offset = opcodeOffset + 2;

    text = "mov " + *destName + ", " + *sourceName;

    return true;
}

bool X86Decoder::decode_short_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    std::int64_t displacement = 0;
    if (!short_jump_displacement(bytes, offset, displacement))
        return false;
    offset += 2;
    text = "jmp " + std::to_string(displacement);
    return true;
}

bool X86Decoder::decode_conditional_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if (offset >= bytes.size())
        return false;
    std::uint8_t opcode = bytes[offset];
    if (opcode >= 0x70 && opcode <= 0x7F)
    {
        if (offset + 1 >= bytes.size())
            return false;
        const auto raw = bytes[offset + 1];
        const int displacement = raw < 128 ? raw : static_cast<int>(raw) - 256;

        switch (opcode)
        {
        case 0x74:
            text = "jz " + std::to_string(displacement);
            break;
        case 0x75:
            text = "jnz " + std::to_string(displacement);
            break;
        case 0x72:
            text = "jc " + std::to_string(displacement);
            break;
        case 0x73:
            text = "jnc " + std::to_string(displacement);
            break;
        default:
            return false;
        }
        offset += 2;
        return true;
    }
    return false;
}

bool X86Decoder::short_jump_displacement(const std::vector<std::uint8_t> &bytes, std::size_t offset,
                                         std::int64_t &displacement) const
{
    if (offset >= bytes.size() || bytes.size() - offset < 2 || bytes[offset] != 0xEB)
        return false;
    const auto value = bytes[offset + 1];
    displacement = value < 128 ? value : static_cast<int>(value) - 256;
    return true;
}

bool X86Decoder::near_jump_displacement(
    const std::vector<std::uint8_t> &bytes, std::size_t offset,
    std::int64_t &displacement) const
{
    const std::size_t size = near_displacement_size(cpu_.getMode());
    if (size == 0 || offset >= bytes.size() ||
        bytes[offset] != 0xE9 || bytes.size() - offset - 1 < size)
        return false;

    std::uint64_t raw = 0;
    for (std::size_t i = 0; i < size; ++i)
        raw |= static_cast<std::uint64_t>(bytes[offset + 1 + i]) << (8 * i);
    const unsigned bits = static_cast<unsigned>(size * 8);
    const std::uint64_t sign = std::uint64_t{1} << (bits - 1);
    displacement = raw < sign ? static_cast<std::int64_t>(raw)
                              : static_cast<std::int64_t>(raw) - (std::int64_t{1} << bits);
    return true;
}

bool X86Decoder::decode_near_jump(
    const std::vector<std::uint8_t> &bytes, std::size_t &offset,
    std::string &text) const
{
    std::int64_t displacement = 0;
    if (!near_jump_displacement(bytes, offset, displacement))
        return false;
    offset += 1 + near_displacement_size(cpu_.getMode());
    text = "jmp " + std::to_string(displacement);
    return true;
}

bool X86Decoder::decode_far_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if (cpu_.getMode() != X86Mode::Bits16 || offset >= bytes.size() || bytes.size() - offset < 5)
        return false;
    if (bytes[offset] == 0xEA)
    {
        std::uint16_t offset_value = bytes[offset + 1] | (bytes[offset + 2] << 8);
        std::uint16_t segment_value = bytes[offset + 3] | (bytes[offset + 4] << 8);
        offset += 5;
        text = "jmp far " + std::to_string(segment_value) + ":" + std::to_string(offset_value);
        return true;
    }
    return false;
}

bool X86Decoder::decode_interrupt(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if (offset >= bytes.size() || bytes[offset] != 0xCD || bytes.size() - offset < 2)
        return false;
    std::uint8_t intNum = bytes[offset + 1];
    offset += 2;
    text = "int " + std::to_string(intNum);
    return true;
}

bool X86Decoder::decode_memory_register(
    const std::vector<std::uint8_t> &bytes,
    std::size_t &offset,
    std::string &text) const
{
    // Memory register decoding logic here

    // Check for REX.W prefix in 64-bit mode
    if (offset >= bytes.size())
        return false;

    std::size_t opcodeOffset = offset;
    bool hasRexW = false;

    if (mode() == X86Mode::Bits64 && bytes[offset] == 0x48)
    {
        hasRexW = true;
        opcodeOffset = offset + 1;
    }

    // Check for MOV r64, r/m64 opcode (0x8B) after REX.W prefix if present
    if (bytes.size() - opcodeOffset < 2 || bytes[opcodeOffset] != 0x8B)
        return false;

    // Extract ModR/M byte
    std::uint8_t modrm = bytes[opcodeOffset + 1];

    // Decode ModR/M byte into its components
    unsigned mod = (modrm >> 6) & 0x3;
    // mod: 2 bits specifying addressing mode
    unsigned destId = (modrm >> 3) & 0x7;
    // destId: 3 bits specifying the destination register
    unsigned memoryCode = modrm & 0x7;
    // memoryCode: 3 bits specifying the memory operand (r/m)

    if (mod == 1 || mod == 2)
        return decode_displaced_memory_load(bytes, offset, text);

    if (mod != 0)
        return false;

    bool hasSib = mode() != X86Mode::Bits16 && memoryCode == 4;
    if (hasSib)
    {
        if (bytes.size() - opcodeOffset < 3)
            return false;

        if (bytes[opcodeOffset + 2] != 0x24)
            return decode_scaled_memory_load(bytes, offset, text);
    }

    const X86Register *destReg = cpu_.registers().getRegisterById(destId);
    if (!destReg)
        return false; // Invalid destination register ID

    unsigned destWidth = 0;

    switch (mode())
    {
    case X86Mode::Bits16:
        destWidth = 16;
        break;
    case X86Mode::Bits32:
        destWidth = 32;
        break;
    case X86Mode::Bits64:
        destWidth = hasRexW ? 64 : 32;
        break;
    default:
        return false; // Unsupported mode
    }

    const std::string *destName = destReg->getAliasByWidth(destWidth);
    if (!destName)
        return false; // Failed to get destination register name

    std::string baseName;

    if (mode() == X86Mode::Bits16)
    {
        switch (memoryCode)
        {
        case 4:
            baseName = "si";
            break;
        case 5:
            baseName = "di";
            break;
        case 7:
            baseName = "bx";
            break;

        default:
            return false; // Invalid memory code
        }
    }
    else if (mode() == X86Mode::Bits32)
    {
        switch (memoryCode)
        {
        case 0:
            baseName = "eax";
            break;
        case 1:
            baseName = "ecx";
            break;
        case 2:
            baseName = "edx";
            break;
        case 3:
            baseName = "ebx";
            break;
        case 4:
            baseName = "esp";
            break;
        case 6:
            baseName = "esi";
            break;
        case 7:
            baseName = "edi";
            break;
        default:
            return false; // Invalid memory code
        }
    }
    else if (mode() == X86Mode::Bits64)
    {
        switch (memoryCode)
        {
        case 0:
            baseName = "rax";
            break;
        case 1:
            baseName = "rcx";
            break;
        case 2:
            baseName = "rdx";
            break;
        case 3:
            baseName = "rbx";
            break;
        case 4:
            baseName = "rsp";
            break;
        case 6:
            baseName = "rsi";
            break;
        case 7:
            baseName = "rdi";
            break;
        default:
            return false; // Invalid memory code
        }
    }

    text = "mov " + *destName + ", [" + baseName + "]";
    offset = opcodeOffset + 2 + (hasSib ? 1 : 0); // Update offset to account for optional SIB byte
    return true;                                  // Successfully decoded instruction
}

bool X86Decoder::decode_scaled_memory_load(
    const std::vector<std::uint8_t> &bytes,
    std::size_t &offset,
    std::string &text) const
{
    if (offset >= bytes.size())
        return false;

    std::size_t opcodeOffset = offset;
    bool hasRexW = false;

    if (mode() == X86Mode::Bits64 && bytes[offset] == 0x48)
    {
        hasRexW = true;
        opcodeOffset = offset + 1;
    }

    if (bytes.size() - opcodeOffset < 3 ||
        bytes[opcodeOffset] != 0x8B)
        return false;

    // Extract ModR/M and SIB bytes for the scaled memory load instruction
    std::uint8_t modrm = bytes[opcodeOffset + 1];
    std::uint8_t sib = bytes[opcodeOffset + 2];

    // Decode the ModR/M byte
    unsigned mod = (modrm >> 6) & 0x03;
    // Extract the destination register ID and memory code from the ModR/M byte
    unsigned destId = (modrm >> 3) & 0x07;
    // Extract the memory code from the ModR/M byte
    unsigned memoryCode = modrm & 0x07;

    // Validate the ModR/M and SIB bytes for a scaled memory load
    if ((
            mode() != X86Mode::Bits32 &&
            mode() != X86Mode::Bits64) ||
        mod != 0 ||
        memoryCode != 4)
        return false;

    // Decode the SIB byte
    unsigned scaleBits = (sib >> 6) & 0x03;
    // Extract the scale factor from the SIB byte
    unsigned indexId = (sib >> 3) & 0x07;
    // Extract the index register ID from the SIB byte
    unsigned baseId = sib & 0x07;

    // Validate the index and base registers for the scaled memory load
    if (indexId == 4 || baseId == 5)
        return false; // Invalid index or base register for scaled memory load

    // Retrieve the register objects for the destination, index, and base registers
    const X86Register *destReg = cpu_.registers().getRegisterById(destId);
    // Retrieve the register object for the index register
    const X86Register *indexReg = cpu_.registers().getRegisterById(indexId);
    // Retrieve the register object for the base register
    const X86Register *baseReg = cpu_.registers().getRegisterById(baseId);

    // Ensure that all required registers were successfully retrieved
    if (!destReg || !indexReg || !baseReg)
        return false;

    // Determine the width of the destination and address registers based on the mode and REX prefix
    unsigned destWidth = mode() == X86Mode::Bits64 && hasRexW ? 64 : 32;
    unsigned addressWidth = mode() == X86Mode::Bits64 ? 64 : 32;

    // Retrieve the register aliases for the destination, base, and index registers based on their widths
    const std::string *destName = destReg->getAliasByWidth(destWidth);
    const std::string *baseName = baseReg->getAliasByWidth(addressWidth);
    const std::string *indexName = indexReg->getAliasByWidth(addressWidth);

    // Ensure that the register aliases were successfully retrieved
    if (!destName || !baseName || !indexName)
        return false;

    // Calculate the scale factor from the scale bits in the SIB byte
    unsigned scale = 1u << scaleBits;

    // Construct the disassembled instruction text for the scaled memory load
    text = "mov " + *destName + ", [" + *baseName + " + " +
           *indexName + " * " + std::to_string(scale) + "]";
    offset = opcodeOffset + 3;
    return true;
}

bool X86Decoder::decode_displaced_memory_load(
    const std::vector<std::uint8_t> &bytes,
    std::size_t &offset,
    std::string &text) const
{

    if (offset >= bytes.size())
        return false;

    std::size_t opcodeOffset = offset;
    bool hasRexW = false;
    // Check for REX.W prefix in 64-bit mode
    if (mode() == X86Mode::Bits64 && bytes[offset] == 0x48)
    {
        hasRexW = true;
        opcodeOffset = offset + 1;
    }
    // Ensure that there are enough bytes for the opcode and ModR/M byte
    if (bytes.size() - opcodeOffset < 2 ||
        bytes[opcodeOffset] != 0x8B)
        return false;

    // Extract the ModR/M byte following the opcode
    std::uint8_t modrm = bytes[opcodeOffset + 1];

    unsigned mod = (modrm >> 6) & 0x03;
    unsigned destId = (modrm >> 3) & 0x07;
    unsigned baseId = modrm & 0x07;

    if ((mode() != X86Mode::Bits32 && mode() != X86Mode::Bits64) ||
        (mod != 1 && mod != 2))
        return false;

    std::size_t displacementOffset = opcodeOffset + 2;

    if (baseId == 4)
    {
        if (bytes.size() - displacementOffset < 1 ||
            bytes[displacementOffset] != 0x24)
            return false;

        ++displacementOffset;
    }
    unsigned displacementBytes = mod == 1 ? 1 : 4;

    if (bytes.size() - displacementOffset < displacementBytes)
        return false;

    std::uint32_t raw = 0;

    for (unsigned i = 0; i < displacementBytes; ++i)
        raw |= static_cast<std::uint32_t>(bytes[displacementOffset + i]) << (i * 8);

    std::int64_t displacement;

    if (displacementBytes == 1)
        displacement = raw < 0x80 ? raw : static_cast<std::int64_t>(raw) - 0x100;
    else
        displacement = raw < 0x80000000u ? raw : static_cast<std::int64_t>(raw) - 0x100000000LL;

    const X86Register *destReg = cpu_.registers().getRegisterById(destId);
    const X86Register *baseReg = cpu_.registers().getRegisterById(baseId);

    if (!destReg || !baseReg)
        return false;

    unsigned destWidth = mode() == X86Mode::Bits64 && hasRexW ? 64 : 32;
    unsigned addressWidth = mode() == X86Mode::Bits64 ? 64 : 32;

    const std::string *destName = destReg->getAliasByWidth(destWidth);
    const std::string *baseName = baseReg->getAliasByWidth(addressWidth);

    if (!destName || !baseName)
        return false;

    text = "mov " + *destName + ", [" + *baseName;

    if (displacement < 0)
        text += " - " + std::to_string(-displacement);
    else
        text += " + " + std::to_string(displacement);

    text += "]";
    offset = displacementOffset + displacementBytes;
    return true;
}

bool X86Decoder::decode_segment_move(
    const std::vector<std::uint8_t> &bytes,
    std::size_t &offset,
    std::string &text) const
{
    if (mode() != X86Mode::Bits16 ||
        offset >= bytes.size() ||
        bytes.size() - offset < 2)
        return false;

    std::uint8_t opcode = bytes[offset];

    // Segment move instructions have opcodes 0x8C (store segment) and 0x8E (load segment)
    if (opcode != 0x8C && opcode != 0x8E)
        return false;

    std::uint8_t modrm = bytes[offset + 1];
    unsigned mod = (modrm >> 6) & 0x03;
    unsigned segmentCode = (modrm >> 3) & 0x07;
    unsigned regId = modrm & 0x07;

    // Only register-direct addressing is allowed for segment moves in 16-bit mode
    if (mod != 3 || segmentCode > 5)
        return false;

    // Check for invalid segment load (CS cannot be loaded)
    if (opcode == 0x8E && segmentCode == 1)
        return false; // MOV cannot load CS

    const X86Register *reg = cpu_.registers().getRegisterById(regId);
    if (!reg)
        return false;

    const std::string *regName = reg->getAliasByWidth(16);
    if (!regName)
        return false;

    const char *segmentNames[] = {"es", "cs", "ss", "ds", "fs", "gs"};
    const char *segmentName = segmentNames[segmentCode];

    if (opcode == 0x8E)
        text = "mov " + std::string(segmentName) + ", " + *regName;
    else
        text = "mov " + *regName + ", " + segmentName;

    offset += 2;
    return true;
}