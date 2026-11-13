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
#include <x86/x86Registers.hpp>
#include <kace/integers.hpp>

X86Mode X86Encoder::mode() const
{
    return cpu_.getMode();
}

void X86Encoder::setMode(X86Mode newMode)
{
    cpu_.setMode(newMode);
}

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

    X86Register *regPtr = cpu_.registers().getRegisterByName(reg);
    if (!regPtr)
    {
        error = "Register not found";
        return false;
    }

    if (name != "mov")
    {
        error = "Unsupported instruction for immediate encoding";
        return false;
    }

    unsigned id = 0;

    // 8-bit register immediate encoding
    if (regPtr->hasWidth(reg, 8) && regPtr->getGroupId() < 4 &&
        regPtr->getEncodingId(reg, id))
    {
        if (value > 0xFF)
        {
            error = "Immediate does not fit in 8 bits";
            return false;
        }
        bytes.push_back({static_cast<std::uint8_t>(0xB0 + id)});
        bytes.push_back({static_cast<std::uint8_t>(value)});
        return true;
    }
    // 16-bit register immediate encoding
    if (regPtr->hasWidth(reg, 16) && regPtr->getGroupId() < 8)
    {
        if (value > 0xFFFF)
        {
            error = "Immediate does not fit in 16 bits";
            return false;
        }
        if (mode() == X86Mode::Bits32 || mode() == X86Mode::Bits64)
        {
            bytes.push_back({static_cast<std::uint8_t>(0x66)}); // Operand-size override prefix for 16-bit immediate
        }
        bytes.push_back({static_cast<std::uint8_t>(0xB8 + regPtr->getGroupId())});
        bytes.push_back({static_cast<std::uint8_t>(value)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 8)});
        return true;
    }

    // 32-bit register immediate encoding (future)
    if (regPtr->hasWidth(reg, 32) && regPtr->getGroupId() < 8)
    {
        if (value > 0xFFFFFFFF)
        {
            error = "Immediate does not fit in 32 bits";
            return false;
        }
        if (mode() == X86Mode::Bits16)
        {
            bytes.push_back({static_cast<std::uint8_t>(0x66)}); // Operand-size override prefix for 32-bit immediate in 16-bit mode
        }
        bytes.push_back({static_cast<std::uint8_t>(0xB8 + regPtr->getGroupId())});
        bytes.push_back({static_cast<std::uint8_t>(value)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 8)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 16)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 24)});
        return true;
    }

    // 64-bit register immediate encoding (future)
    if (regPtr->hasWidth(reg, 64) && regPtr->getGroupId() < 8)
    {
        if (mode() != X86Mode::Bits64)
        {
            error = "64-bit immediate encoding is only supported in 64-bit mode";
            return false;
        }
        bytes.push_back({static_cast<std::uint8_t>(0x48)}); // REX.W prefix for 64-bit immediate
        bytes.push_back({static_cast<std::uint8_t>(0xB8 + regPtr->getGroupId())});
        bytes.push_back({static_cast<std::uint8_t>(value)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 8)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 16)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 24)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 32)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 40)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 48)});
        bytes.push_back({static_cast<std::uint8_t>(value >> 56)});
        return true;
    }

    error = "Unsupported register immediate encoding";
    return false;
}

bool X86Encoder::encode_register(
    const std::string &name,
    const std::string &reg_dest,
    const std::string &reg_source,
    std::vector<std::vector<std::uint8_t>> &bytes,
    std::string &error) const
{
    if (name != "mov")
    {
        error = "Unsupported register instruction";
        return false;
    }
    X86Register *sourceReg = cpu_.registers().getRegisterByName(reg_source);
    X86Register *destReg = cpu_.registers().getRegisterByName(reg_dest);
    if (!sourceReg)
    {
        error = "Invalid source register";
        return false;
    }
    if (!destReg)
    {
        error = "Invalid destination register";
        return false;
    }
    X86Mode mode = cpu_.getMode();
    int sourceId = sourceReg->getGroupId();
    int destId = destReg->getGroupId();

    switch (mode)
    {
    case X86Mode::Bits16:
    {
        // 16-bit mode encoding logic here
        if (!destReg->hasWidth(reg_dest, 16) || !sourceReg->hasWidth(reg_source, 16))
        {
            error = "Expected two 16-bit registers";
            return false;
        }
       
        if (sourceId < 8 && destId < 8)
        {

            bytes.push_back({0x89});
            bytes.push_back({static_cast<std::uint8_t>(0xC0 | (sourceId << 3) | destId)});
            return true;
        }
        error = "Unsupported 16-bit register encoding";
        return false;
    }
    break;
    case X86Mode::Bits32:
    {
        // 32-bit mode encoding logic here
        if (!destReg->hasWidth(reg_dest, 32) || !sourceReg->hasWidth(reg_source, 32))
        {
            error = "Expected two 32-bit registers";
            return false;
        }
        if(sourceId >= 8 || destId >= 8)
        {
            error = "Unsupported 32-bit register encoding";
            return false;
        }
        bytes.push_back({0x89});
        bytes.push_back({static_cast<std::uint8_t>(0xC0 | (sourceId << 3) | destId)});
        
        return true;
    }
    break;
    case X86Mode::Bits64:
    {
        // 64-bit mode encoding logic here
        if (!destReg->hasWidth(reg_dest, 64) || !sourceReg->hasWidth(reg_source, 64))
        {
            error = "Expected two 64-bit registers";
            return false;
        }
        if(sourceId >= 8 || destId >= 8)
        {
            error = "Unsupported 64-bit register encoding";
            return false;
        }
        bytes.push_back({0x48}); // REX.W prefix for 64-bit operand
        bytes.push_back({0x89});
        bytes.push_back({static_cast<std::uint8_t>(0xC0 | (sourceId << 3) | destId)});
        return true;

    }
    break;
    default:
        error = "Unsupported register instruction encoding";
        return false;
    }
    error = "Unsupported register instruction encoding";
    return false;
}

bool X86Encoder::encode_near_jump(
    std::int64_t displacement, std::vector<std::uint8_t> &bytes,
    std::string &error) const
{
    const std::size_t size = near_displacement_size(cpu_.getMode());
    if (size == 0)
    {
        error = "Invalid CPU mode for near jump";
        return false;
    }
    const std::int64_t min = size == 2 ? -32768 : -2147483648LL;
    const std::int64_t max = size == 2 ? 32767 : 2147483647LL;
    if (displacement < min || displacement > max)
    {
        error = "Near-jump displacement out of range";
        return false;
    }
    const auto raw = static_cast<std::uint64_t>(displacement);
    bytes.push_back(0xE9);
    for (std::size_t i = 0; i < size; ++i)
        bytes.push_back(static_cast<std::uint8_t>(raw >> (8 * i)));
    error.clear();
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

bool X86Encoder::encode_conditional_jump(
    const std::string &condition,
    std::int64_t displacement,
    std::vector<std::uint8_t> &bytes,
    std::string &error) const
{
    std::uint8_t opcode = 0;

    if (condition == "jz")
        opcode = 0x74; // jz short
    else if (condition == "jnz")
        opcode = 0x75; // jnz short
    else if (condition == "jc")
        opcode = 0x72; // jc short
    else if (condition == "jnc")
        opcode = 0x73; // jnc short
    else
    {
        error = "Unsupported conditional jump";
        return false;
    }

    if (displacement < -128 || displacement > 127)
    {
        error = "Conditional jump displacement out of range";
        return false;
    }

    bytes.push_back(opcode);
    bytes.push_back(static_cast<std::uint8_t>(displacement));
    error.clear();
    return true;
}

bool X86Encoder::encode_interrupt(
    const std::string &interruptNumber,
    std::vector<std::uint8_t> &bytes,
    std::string &error) const
{
    if (interruptNumber.empty())
    {
        error = "Interrupt number is required";
        return false;
    }
    std::uint64_t intNum = 0;
    if (!parse_integer(interruptNumber, intNum))
    {
        error = "Invalid interrupt number";
        return false;
    }
    if (intNum > 255)
    {
        error = "Interrupt number must fit in 8 bits";
        return false;
    }
    bytes.push_back(0xCD); // INT opcode
    bytes.push_back(static_cast<std::uint8_t>(intNum));
    error.clear();
    return true;
}
