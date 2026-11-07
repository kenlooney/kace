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
bool X86::encode_short_jump(
    std::int64_t displacement,
    std::vector<std::uint8_t>& bytes,
    std::string& error
) const
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

bool X86::encode_no_operand(const std::string &name, std::uint8_t &byte) const
{
    if (name == "nop")
    {
        byte = 0x90;
        return true;
    }
    return false;
}
bool X86::encode_immediate(
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

// Decoding
bool X86::decode(const std::vector<std::uint8_t> &bytes, std::size_t &offset,
                 std::string &text) const
{
    // Check if there are enough bytes to decode
    if (offset >= bytes.size())
        return false;

    // Decode NOP instruction
    if (bytes[offset] == 0x90)
    {
        ++offset;
        text = "nop";
        return true;
    }

    // Decode MOV AL, immediate instruction
    if (bytes[offset] == 0xB0 && offset + 1 < bytes.size())
    {
        text = "mov al, " + std::to_string(bytes[offset + 1]);
        offset += 2;
        return true;
    }

    // Unknown instruction
    return false;
}



