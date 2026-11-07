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
#pragma once
#include <cstdint>
#include <string>
#include <vector>
class Cpu
{
public:
    virtual ~Cpu() = default;
    // Encoding
    virtual bool encode_no_operand(const std::string &name, std::uint8_t &byte) const
    {
        return false;
    };
    virtual bool encode_immediate(
        const std::string &name,
        const std::string &reg,
        std::uint64_t value,
        std::vector<std::vector<std::uint8_t>> &bytes,
        std::string &error) const
    {
        error = "immediate encoding is not supported by this CPU";
        return false;
    }
    virtual bool encode_short_jump(
        std::int64_t displacement,
        std::vector<std::uint8_t> &bytes,
        std::string &error) const
    {
        error = "short jump encoding is not supported by this CPU";
        return false;
    }
    // Decoding
    virtual bool decode(
        const std::vector<std::uint8_t> &,
        std::size_t &,
        std::string &) const
    {
        return false;
    }
    virtual bool decode_no_operand(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
    {
        text = "no operand decoding is not supported by this CPU";
        return false;
    }
    virtual bool decode_immediate(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
    {
        text = "immediate decoding is not supported by this CPU";
        return false;
    }
    virtual bool decode_short_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const
    {
        text = "short jump decoding is not supported by this CPU";
        return false;
    }
    // Reports the displacement only when the instruction at offset is a short jump.
    virtual bool short_jump_displacement(const std::vector<std::uint8_t>&, std::size_t, std::int64_t&) const
    {
        return false;
    }
};
