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
#include <x86/x86Encoder.hpp>
#include <x86/x86Decoder.hpp>
#include <x86/x86Assembler.hpp>
#include <x86/x86Disassembler.hpp>

enum class X86Mode {
    Bits16,
    Bits32,
    Bits64
};
class X86 : public Cpu {
    public:
    X86() : enc(*this), dec(*this), assembler_(*this), disassembler_(*this) {}
    X86(const X86&) = delete;
    X86& operator=(const X86&) = delete;
    virtual ~X86() = default;
    const X86Encoder* encoder() const { return &enc; }
    const X86Decoder* decoder() const { return &dec; }
    const Assembler* assembler() const override { return &assembler_; }
    const Disassembler* disassembler() const override { return &disassembler_; }

    // Other Public Functions
    X86Mode getMode() const { return mode; }
    void setMode(X86Mode newMode) { mode = newMode; }

    // Fields
    private:
    X86Mode mode = X86Mode::Bits16; // Default to 16-bit mode
    X86Encoder enc;
    X86Decoder dec;
    X86Assembler assembler_;
    X86Disassembler disassembler_;
};
