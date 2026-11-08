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
#include <kace/semantic.hpp>
#include <kace/statement.hpp>

bool check_byte(const Statement& statement, std::uint8_t& byte, std::string& error) {
    const bool is_define_byte =
        statement.name == "define" && statement.type == DataType::Byte;
    const bool is_legacy_db = statement.name == "db";
    const bool is_times_byte = statement.kind == StatementKind::Times && (
        statement.type == DataType::Byte
    );

    if (!is_define_byte && !is_legacy_db && !is_times_byte) {
        error = "Expected a byte definition";
        return false;
    }

    if (statement.value > 0xff) {
        error = "Byte value out of range: " +
            std::to_string(statement.value);
        return false;
    }

    byte = static_cast<std::uint8_t>(statement.value);
    return true;
}

bool encode_statement(const Statement& statement, const Cpu& cpu, std::uint8_t& byte, std::string& error) {
    if (statement.type != DataType::None) {
        return check_byte(statement, byte, error);
    }
    if (!statement.expression.nodes.empty() || !cpu.encode_no_operand(statement.name, byte)) {
        error = "Unsupported instruction or operands: " + statement.name;
        return false;
    }
    return true;
}
