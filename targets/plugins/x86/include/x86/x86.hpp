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
#include <kace/cpu.hpp>

enum class X86Mode {
    Bits16,
    Bits32,
    Bits64
};
class X86 : public Cpu {
    public:
    virtual ~X86() = default;
    // Encoding
    virtual bool encode_no_operand(const std::string& name, std::uint8_t& byte) const override;
    virtual bool encode_immediate(
        const std::string& name, 
        const std::string& reg, 
        std::uint64_t value,
        std::vector<std::vector<std::uint8_t>>& bytes,
        std::string& error
    ) const override;
    bool encode_short_jump(
        std::int64_t displacement,
        std::vector<std::uint8_t>& bytes,
        std::string& error
    ) const override;

    // Decoding
    bool decode(const std::vector<std::uint8_t>& bytes, std::size_t& offset,
                std::string& text) const override;

    // Other Public Functions
    X86Mode getMode() const { return mode; }
    void setMode(X86Mode newMode) { mode = newMode; }

    // Fields
    private:
    X86Mode mode = X86Mode::Bits16; // Default to 16-bit mode
    
};