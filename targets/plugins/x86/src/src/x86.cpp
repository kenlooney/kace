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
