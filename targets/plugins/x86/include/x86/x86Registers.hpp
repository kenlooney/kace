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
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <kace/register.hpp>

class X86Register : public Register
{
public:
    struct Alias
    {
        std::string name;
        unsigned shift;
        unsigned width;
    };

    X86Register(std::string name, unsigned id, std::initializer_list<Alias> aliases = {})
        : Register(std::move(name)), id_(id), aliases_(aliases) {}

    unsigned getGroupId() const { return id_; }

    // AH/CH/DH/BH have distinct byte-register codes from their low-byte views.
    bool getEncodingId(const std::string &name, unsigned &result) const
    {
        if (Register::matchesName(name))
        {
            result = id_;
            return true;
        }
        for (const auto &alias : aliases_)
        {
            if (alias.name == name)
            {
                result = alias.shift == 8 ? id_ + 4 : id_;
                return true;
            }
        }
        return false;
    }

    bool matchesName(const std::string &name) const override
    {
        if (Register::matchesName(name))
            return true;
        for (const auto &alias : aliases_)
            if (alias.name == name)
                return true;
        return false;
    }

    // Read an overlapping view of the same underlying register value.
    bool read(const std::string &name, std::uint64_t &result) const
    {
        if (Register::matchesName(name))
        {
            result = getValue();
            return true;
        }
        for (const auto &alias : aliases_)
        {
            if (alias.name == name)
            {
                result = (getValue() >> alias.shift) & ((std::uint64_t{1} << alias.width) - 1);
                return true;
            }
        }
        return false;
    }

    bool hasWidth(const std::string &name, unsigned width) const
    {
        if (name == getName())
            return width == 64;
        for (const auto &alias : aliases_)
        {
            if (alias.name == name)
                return alias.width == width;
        }
        return false;
    }
    const std::string *getAliasByWidth(unsigned width) const
    {
        if (width == 64)
            return &getName();
        for (const auto &alias : aliases_)
        {
            if (alias.width == width && alias.shift == 0)
                return &alias.name;
        }
        return nullptr;
    }
    unsigned getId() const { return id_; }

private:
    unsigned id_;
    std::vector<Alias> aliases_;
};

class X86RegisterFile : public RegisterFile
{
public:
    X86RegisterFile()
    {
        addRegister(std::make_unique<X86Register>("rax", 0, std::initializer_list<X86Register::Alias>{{"eax", 0, 32}, {"ax", 0, 16}, {"al", 0, 8}, {"ah", 8, 8}}));
        addRegister(std::make_unique<X86Register>("rcx", 1, std::initializer_list<X86Register::Alias>{{"ecx", 0, 32}, {"cx", 0, 16}, {"cl", 0, 8}, {"ch", 8, 8}}));
        addRegister(std::make_unique<X86Register>("rdx", 2, std::initializer_list<X86Register::Alias>{{"edx", 0, 32}, {"dx", 0, 16}, {"dl", 0, 8}, {"dh", 8, 8}}));
        addRegister(std::make_unique<X86Register>("rbx", 3, std::initializer_list<X86Register::Alias>{{"ebx", 0, 32}, {"bx", 0, 16}, {"bl", 0, 8}, {"bh", 8, 8}}));
        addRegister(std::make_unique<X86Register>("rsp", 4, std::initializer_list<X86Register::Alias>{{"esp", 0, 32}, {"sp", 0, 16}, {"spl", 0, 8}}));
        addRegister(std::make_unique<X86Register>("rbp", 5, std::initializer_list<X86Register::Alias>{{"ebp", 0, 32}, {"bp", 0, 16}, {"bpl", 0, 8}}));
        addRegister(std::make_unique<X86Register>("rsi", 6, std::initializer_list<X86Register::Alias>{{"esi", 0, 32}, {"si", 0, 16}, {"sil", 0, 8}}));
        addRegister(std::make_unique<X86Register>("rdi", 7, std::initializer_list<X86Register::Alias>{{"edi", 0, 32}, {"di", 0, 16}, {"dil", 0, 8}}));
        for (unsigned id = 8; id <= 15; ++id)
        {
            const std::string name = "r" + std::to_string(id);
            addRegister(std::make_unique<X86Register>(name, id, std::initializer_list<X86Register::Alias>{{name + "d", 0, 32}, {name + "w", 0, 16}, {name + "b", 0, 8}}));
        }
    }

    X86Register *getRegisterByName(const std::string &name)
    {
        return dynamic_cast<X86Register *>(RegisterFile::getRegisterByName(name));
    }

    const X86Register *getRegisterByName(const std::string &name) const
    {
        return dynamic_cast<const X86Register *>(RegisterFile::getRegisterByName(name));
    }
    const X86Register *getRegisterById(unsigned id)
    {
        X86RegisterFile *file = this;
        for (const auto &org : file->getRegisters())
        {
            X86Register *reg = dynamic_cast<X86Register *>(org.get());
            if (reg && reg->getId() == id)
                return reg;
        }
        return nullptr;
    }

    const X86Register *getRegisterById(unsigned id) const
    {
        const X86RegisterFile *file = this;
        for (const auto &org : file->getRegisters())
        {
            const X86Register *reg = dynamic_cast<const X86Register *>(org.get());
            if (reg && reg->getId() == id)
                return reg;
        }
        return nullptr;
    }

    bool getRegisterValue(const std::string &name, std::uint64_t &result) const
    {
        const X86Register *reg = getRegisterByName(name);
        return reg && reg->read(name, result);
    }
    bool is16BitRegister(const std::string &name) const
    {
        const X86Register *reg = getRegisterByName(name);
        return reg && reg->hasWidth(name, 16);
    }
    bool is8BitRegister(const std::string &name) const
    {
        const X86Register *reg = getRegisterByName(name);
        return reg && reg->hasWidth(name, 8);
    }
};
