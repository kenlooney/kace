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
#include <kace/encoder.hpp>
enum class X86Mode;
class X86;

class X86Encoder : public Encoder {
public:
    explicit X86Encoder(X86& cpu) : cpu_(cpu) {}
    virtual ~X86Encoder() = default;

    X86Mode mode() const;
    void setMode(X86Mode newMode);
    virtual bool encode_no_operand(const std::string& name, std::uint8_t& byte) const;
    virtual bool encode_immediate(
        const std::string& name, 
        const std::string& reg, 
        std::uint64_t value,
        std::vector<std::vector<std::uint8_t>>& bytes,
        std::string& error
    ) const;
    virtual bool encode_register(
        const std::string& name,
        const std::string& reg_dest,
        const std::string& reg_source,
        std::vector<std::vector<std::uint8_t>>& bytes,
        std::string& error
    ) const;
    bool encode_short_jump(
        std::int64_t displacement,
        std::vector<std::uint8_t>& bytes,
        std::string& error
    ) const;
    bool encode_near_jump(
        std::int64_t displacement,
        std::vector<std::uint8_t>& bytes,
        std::string& error
    ) const;

    bool encode_far_jump(
        std::uint64_t segment,
        std::uint64_t offset,
        std::vector<std::uint8_t>& bytes,
        std::string& error
    ) const;

    bool encode_conditional_jump(
        const std::string& condition,
        std::int64_t displacement,
        std::vector<std::uint8_t>& bytes,
        std::string& error
    ) const;

    bool encode_interrupt(
        const std::string& interruptNumber,
        std::vector<std::uint8_t>& bytes,
        std::string& error
    ) const;
private:
    X86& cpu_;
};
