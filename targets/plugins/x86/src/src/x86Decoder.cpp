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
   if(offset >= bytes.size())
       return false;

    std::size_t opcodeOffset = offset;
    bool hasRexW = false;


    if(mode() == X86Mode::Bits64 && bytes[offset] == 0x48)
    {
        hasRexW = true;
        opcodeOffset = offset + 1;
    }

    // Check for MOV r64, r/m64 opcode (0x8B) after REX.W prefix if present
    if(bytes.size() - opcodeOffset < 2 || bytes[opcodeOffset] != 0x8B)
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


    
    if(mod != 0)
        return false; // Only support mod == 0 for now

    const X86Register *destReg = cpu_.registers().getRegisterById(destId);
    if(!destReg)
        return false; // Invalid destination register ID

    unsigned destWidth = 0;

    switch(mode())
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
    if(!destName)
        return false; // Failed to get destination register name

    std::string baseName;

    if(mode() == X86Mode::Bits16)
    {
        switch(memoryCode)
        {
            case 4: baseName = "si"; break;
            case 5: baseName = "di"; break;
            case 7: baseName = "bx"; break;
           
            default: return false; // Invalid memory code
        }
    }
    else if(mode() == X86Mode::Bits32)
    {
        switch(memoryCode)
        {
            case 0: baseName = "eax"; break;
            case 1: baseName = "ecx"; break;
            case 2: baseName = "edx"; break;
            case 3: baseName = "ebx"; break;
            case 6: baseName = "esi"; break;
            case 7: baseName = "edi"; break;
            default: return false; // Invalid memory code
        }
    }
    else if(mode() == X86Mode::Bits64)
    {
        switch(memoryCode)
        {
            case 0: baseName = "rax"; break;
            case 1: baseName = "rcx"; break;
            case 2: baseName = "rdx"; break;
            case 3: baseName = "rbx"; break;
            case 6: baseName = "rsi"; break;
            case 7: baseName = "rdi"; break;
            default: return false; // Invalid memory code
        }
    }

    text = "mov " + *destName + ", [" + baseName + "]";
    offset = opcodeOffset + 2; // Update offset to point after the decoded instruction
    return true; // Successfully decoded instruction
}
