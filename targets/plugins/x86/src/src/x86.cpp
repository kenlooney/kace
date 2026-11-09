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

// Encoding
bool X86Encoder::encode_short_jump(
    std::int64_t displacement,
    std::vector<std::uint8_t> &bytes,
    std::string &error) const
{
    if (displacement < -128 || displacement > 127)
    {
        error = "Jump displacement out of range";
        return false;
    }
    bytes.push_back(0xEB); // Short jump opcode
    bytes.push_back(static_cast<std::uint8_t>(displacement));
    return true;
}

bool X86Encoder::encode_no_operand(const std::string &name, std::uint8_t &byte) const
{
    if (name == "nop")
    {
        byte = 0x90;
        return true;
    }
    return false;
}
bool X86Encoder::encode_immediate(
    const std::string &name,
    const std::string &reg,
    std::uint64_t value,
    std::vector<std::vector<std::uint8_t>> &bytes,
    std::string &error) const
{
    if (name != "mov" || reg != "al")
    {
        error = "Expected mov al, immediate";
        return false;
    }
    if (value > 255)
    {
        error = "Immediate does not fit in AL";
        return false;
    }
    bytes.push_back({0xB0});
    bytes.push_back({static_cast<std::uint8_t>(value)});
    return true;
}
bool X86Encoder::encode_near_jump(
    std::int64_t displacement,
    std::vector<std::uint8_t> &bytes,
    std::string &error) const
{
    if (displacement < -32768 || displacement > 32767)
    {
        error = "Jump displacement out of range";
        return false;
    }
    const auto value = static_cast<std::uint16_t>(displacement);
    bytes.push_back(0xE9);                                  // Near jump opcode
    bytes.push_back(static_cast<std::uint8_t>(value));      // Low byte
    bytes.push_back(static_cast<std::uint8_t>(value >> 8)); // High byte
    return true;
}
bool X86Decoder::near_jump_displacement(const std::vector<std::uint8_t> &bytes, std::size_t offset,
                                 std::int64_t &displacement) const
{
    if (offset >= bytes.size() || bytes.size() - offset < 3 || bytes[offset] != 0xE9)
        return false;
    const auto value = bytes[offset + 1] | (bytes[offset + 2] << 8);
    displacement = value < 32768 ? value : static_cast<int>(value) - 65536;
    return true;
}

bool X86Encoder::encode_far_jump(
    std::uint64_t segment,
    std::uint64_t offset,
    std::vector<std::uint8_t> &bytes,
    std::string &error) const
{
    // Far jump encoding is only supported in 16-bit mode
    if (cpu_.getMode() != X86Mode::Bits16)
    {
        error = "far jump encoding is only supported in 16-bit mode";
        return false;
    }
    if (segment > 0xFFFF || offset > 0xFFFF)
    {
        error = "far jump segment and offset must fit in 16 bits";
        return false;
    }

    bytes = {
        0xEA,
        static_cast<std::uint8_t>(offset),
        static_cast<std::uint8_t>(offset >> 8),
        static_cast<std::uint8_t>(segment),
        static_cast<std::uint8_t>(segment >> 8)};
    return true;
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
        return decode_immediate(bytes, offset, text);
    case 0xEB:
        return decode_short_jump(bytes, offset, text);
    case 0xE9:
        return decode_near_jump(bytes, offset, text);
    case 0xEA:
        return decode_far_jump(bytes, offset, text);

    default:
        return false;
    }
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
    if (offset + 1 >= bytes.size())
        return false;

    if (bytes[offset] == 0xB0)
    {
        std::uint8_t value = bytes[offset + 1];
        offset += 2;
        text = "mov al, " + std::to_string(value);
        return true;
    }

    return false;
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

bool X86Decoder::short_jump_displacement(const std::vector<std::uint8_t> &bytes, std::size_t offset,
                                  std::int64_t &displacement) const
{
    if (offset >= bytes.size() || bytes.size() - offset < 2 || bytes[offset] != 0xEB)
        return false;
    const auto value = bytes[offset + 1];
    displacement = value < 128 ? value : static_cast<int>(value) - 256;
    return true;
}
bool X86Decoder::decode_near_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    std::int64_t displacement = 0;
    if (!near_jump_displacement(bytes, offset, displacement))
        return false;
    offset += 3;
    text = "jmp " + std::to_string(displacement);
    return true;
}
bool X86Decoder::decode_far_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
{
    if(cpu_.getMode() != X86Mode::Bits16 || offset >= bytes.size() || bytes.size() - offset < 5)
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
