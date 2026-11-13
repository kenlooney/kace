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

bool X86Disassembler::disassemble(const std::vector<std::uint8_t> &bytes,
                                  const SymbolTable &symbols, std::string &text,
                                  std::string &error) const
{
    const auto &byteRanges = symbols.getByteRanges();
    const X86Mode initialMode = cpu_.getMode();
    X86 decodeCpu;
    decodeCpu.setMode(initialMode);

    std::map<std::size_t, std::vector<std::string>> labels;
    for (const Symbol &symbol : symbols.getSymbolsByKind(SymbolKind::Label))
        labels[symbol.offset].push_back(symbol.name);
    for (auto &entry : labels)
        std::sort(entry.second.begin(), entry.second.end());

    struct Instruction
    {
        std::size_t start, end;
        std::string text;
        X86Mode mode;
        bool tagged;
    };
    std::vector<Instruction> instructions;
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        const auto start = offset;
        bool tagged = false;
        auto range = std::find_if(byteRanges.begin(), byteRanges.end(),
                                  [&](const SymbolByteRange &entry) { return entry.offset == start; });
        if (range != byteRanges.end()) {
            for (const auto &tag : range->metadata.tags) {
                if (tag.name != "x86.mode") continue;
                if (tag.value.size() != 1 || (tag.value[0] != 16 && tag.value[0] != 32 && tag.value[0] != 64)) {
                    error = "kasm: invalid x86 mode metadata at byte offset " + std::to_string(start);
                    return false;
                }
                decodeCpu.setMode(tag.value[0] == 16 ? X86Mode::Bits16 : tag.value[0] == 32 ? X86Mode::Bits32 : X86Mode::Bits64);
                tagged = true;
                break;
            }
        }
        std::string decoded;
        if (!decodeCpu.decoder()->decode(bytes, offset, decoded) || offset <= start || offset > bytes.size())
        {
            error = "kasm: could not decode instruction at byte offset " + std::to_string(start);
            return false;
        }
        instructions.push_back({start, offset, std::move(decoded), decodeCpu.getMode(), tagged});
    }
    for (const auto &label : labels)
    {
        if (label.first != bytes.size() && std::none_of(instructions.begin(), instructions.end(),
                                                        [&](const Instruction &instruction)
                                                        { return instruction.start == label.first; }))
        {
            error = "kasm: symbol offset is not an instruction boundary: " + std::to_string(label.first);
            return false;
        }
    }
    std::ostringstream output;
    X86Mode outputMode = initialMode;
    for (const auto &instruction : instructions)
    {
        decodeCpu.setMode(instruction.mode);
        if (instruction.tagged && instruction.mode != outputMode) {
            output << (instruction.mode == X86Mode::Bits16 ? "code16\n" :
                       instruction.mode == X86Mode::Bits32 ? "code32\n" : "code64\n");
            outputMode = instruction.mode;
        }
        const auto range = std::find_if(byteRanges.begin(), byteRanges.end(),
                                        [&](const SymbolByteRange &entry)
                                        {
                                            return entry.offset == instruction.start &&
                                                   entry.length == instruction.end - instruction.start;
                                        });
        const auto definitions = labels.find(instruction.start);
        if (definitions != labels.end())
            for (const auto &name : definitions->second)
                output << name << ":\n";
        if (range != byteRanges.end() && !range->sourceName.empty() && range->line != 0)
        {
            output << "; source: ";
            for (char ch : range->sourceName)
            {
                if (ch == '\n') output << "\\n";
                else if (ch == '\r') output << "\\r";
                else output << ch;
            }
            output << ':' << range->line;
            if (range->column != 0) output << ':' << range->column;
            output << '\n';
        }
        std::int64_t displacement = 0;
        std::string decoded = instruction.text;
        const char *jumpType = nullptr;
        if (decodeCpu.decoder()->short_jump_displacement(bytes, instruction.start, displacement))
            jumpType = "short";
        else if (decodeCpu.decoder()->near_jump_displacement(bytes, instruction.start, displacement))
            jumpType = "near";
        if (jumpType)
        {
            std::size_t target = instruction.end;
            bool valid = true;
            if (displacement < 0)
            {
                const auto magnitude = static_cast<std::uint64_t>(-(displacement + 1)) + 1;
                if (magnitude > target)
                    valid = false;
                else
                    target -= static_cast<std::size_t>(magnitude);
            }
            else if (static_cast<std::uint64_t>(displacement) > bytes.size() - target)
            {
                valid = false;
            }
            else
            {
                target += static_cast<std::size_t>(displacement);
            }
            const auto found = valid ? labels.find(target) : labels.end();
            if (found != labels.end())
                decoded = std::string("jmp ") + jumpType + " " + found->second.front();
        }
        output << decoded << '\n';
    }
    const auto endLabels = labels.find(bytes.size());
    if (endLabels != labels.end())
        for (const auto &name : endLabels->second)
            output << name << ":\n";
    text = output.str();
    return true;
}
