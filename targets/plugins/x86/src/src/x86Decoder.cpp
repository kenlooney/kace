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

bool X86Decoder::near_jump_displacement(
    const std::vector<std::uint8_t>& bytes, std::size_t offset,
    std::int64_t& displacement) const {
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
    const std::vector<std::uint8_t>& bytes, std::size_t& offset,
    std::string& text) const {
    std::int64_t displacement = 0;
    if (!near_jump_displacement(bytes, offset, displacement))
        return false;
    offset += 1 + near_displacement_size(cpu_.getMode());
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
