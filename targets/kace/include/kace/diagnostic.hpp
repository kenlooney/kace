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

struct Span {
    const char* file;
    int line;
    int column;
};
class Diagnostic {
public:
    virtual ~Diagnostic() = default;
    Diagnostic(const char* message, Span span) : message(message), span(span) {}
    const char* getMessage() const { return message; }
    Span getSpan() const { return span; }
    void setMessage(const char* newMessage) { message = newMessage; }
    void setSpan(Span newSpan) { span = newSpan; }

    virtual void report() const = 0;

private:
    const char* message;
    Span span;
};