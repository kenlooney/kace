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
#include <string>
#include "span.hpp"
class Diagnostic {
public:
    virtual ~Diagnostic() = default;
    Diagnostic(const char* message, const char* file, int line, int column) 
        : message(message), file(file), line(line), column(column) {}
    const char* getMessage() const { return message; }
    int getLine() const { return line; }
    int getColumn() const { return column; }
    const char* getFile() const { return file.c_str(); }
    void setMessage(const char* newMessage) { message = newMessage; }
    void setSpan(const Span& newSpan) { span = newSpan; }
    Span getSpan() const { return span; }
    virtual void report() const = 0;
    void setSourceLine(const std::string& newSourceLine) { sourceLine = newSourceLine; }
    std::string getSourceLine() const { return sourceLine; }

private:
    const char* message;
    int line;
    int column;
    std::string file;
    Span span{};
    std::string sourceLine;
    
};