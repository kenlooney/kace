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
#include "assembler.hpp"
#include "disassembler.hpp"
#include "register.hpp"

class Cpu
{
public:
    virtual ~Cpu() = default;
    virtual const Assembler *assembler() const = 0;
    virtual const Disassembler *disassembler() const { return nullptr; }
    virtual const RegisterFile& registers() const = 0;

};
