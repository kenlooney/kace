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
#include <kace/decoder.hpp>
#include <x86/mode.hpp>
class X86;

class X86Decoder : public Decoder
{
public:
    explicit X86Decoder(const X86& cpu) : cpu_(cpu) {}
    virtual ~X86Decoder() = default;
    bool decode(
        const std::vector<std::uint8_t> &bytes,
        std::size_t &offset,
        std::string &text) const override;
    bool decode_no_operand(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_short_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool short_jump_displacement(const std::vector<std::uint8_t> &bytes, std::size_t offset,
                                 std::int64_t &displacement) const;
    bool decode_immediate(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_near_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_far_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_conditional_jump(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_register(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_memory_register(
    const std::vector<std::uint8_t> &bytes,
    std::size_t &offset,
    std::string &text) const;

    bool decode_segment_move(
        const std::vector<std::uint8_t> &bytes,
        std::size_t &offset,
        std::string &text) const;
    bool decode_scaled_memory_load(
        const std::vector<std::uint8_t> &bytes,
        std::size_t &offset,
        std::string &text) const;
    bool decode_displaced_memory_load(
        const std::vector<std::uint8_t> &bytes,
        std::size_t &offset,
        std::string &text) const;
    
    bool near_jump_displacement(const std::vector<std::uint8_t> &bytes, std::size_t offset,
                                std::int64_t &displacement) const;
    bool decode_interrupt(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    bool decode_halt(const std::vector<std::uint8_t> &bytes, std::size_t &offset, std::string &text) const;
    
    // Return CPU mode
    X86Mode mode() const;
private:
    const X86& cpu_;
};
