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

#include <x86/x86Disassembler.hpp>
#include <x86/x86.hpp>
#include <algorithm>
#include <map>
#include <sstream>

bool X86Disassembler::disassemble(const std::vector<std::uint8_t>& bytes,
                                  const SymbolTable& symbols, std::string& text,
                                  std::string& error) const
{
    std::map<std::size_t, std::vector<std::string>> labels;
    for (const Symbol& symbol : symbols.getSymbolsByKind(SymbolKind::Label))
        labels[symbol.offset].push_back(symbol.name);
    for (auto& entry : labels)
        std::sort(entry.second.begin(), entry.second.end());

    struct Instruction { std::size_t start, end; std::string text; };
    std::vector<Instruction> instructions;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto start = offset;
        std::string decoded;
        if (!cpu_.decoder()->decode(bytes, offset, decoded) || offset <= start || offset > bytes.size()) {
            error = "kasm: could not decode instruction at byte offset " + std::to_string(start);
            return false;
        }
        instructions.push_back({start, offset, std::move(decoded)});
    }
    for (const auto& label : labels) {
        if (label.first != bytes.size() && std::none_of(instructions.begin(), instructions.end(),
            [&](const Instruction& instruction) { return instruction.start == label.first; })) {
            error = "kasm: symbol offset is not an instruction boundary: " + std::to_string(label.first);
            return false;
        }
    }
    std::ostringstream output;
    for (const auto& instruction : instructions) {
        const auto definitions = labels.find(instruction.start);
        if (definitions != labels.end())
            for (const auto& name : definitions->second) output << name << ":\n";
        std::int64_t displacement = 0;
        std::string decoded = instruction.text;
        const char* jumpType = nullptr;
        if (cpu_.decoder()->short_jump_displacement(bytes, instruction.start, displacement))
            jumpType = "short";
        else if (cpu_.decoder()->near_jump_displacement(bytes, instruction.start, displacement))
            jumpType = "near";
        if (jumpType) {
            std::size_t target = instruction.end;
            bool valid = true;
            if (displacement < 0) {
                const auto magnitude = static_cast<std::uint64_t>(-(displacement + 1)) + 1;
                if (magnitude > target) valid = false;
                else target -= static_cast<std::size_t>(magnitude);
            } else if (static_cast<std::uint64_t>(displacement) > bytes.size() - target) {
                valid = false;
            } else {
                target += static_cast<std::size_t>(displacement);
            }
            const auto found = valid ? labels.find(target) : labels.end();
            if (found != labels.end()) decoded = std::string("jmp ") + jumpType + " " + found->second.front();
        }
        output << decoded << '\n';
    }
    const auto endLabels = labels.find(bytes.size());
    if (endLabels != labels.end())
        for (const auto& name : endLabels->second) output << name << ":\n";
    text = output.str();
    return true;
}
