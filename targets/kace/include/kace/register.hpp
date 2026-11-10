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
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Register {
public:
    explicit Register(std::string name) :  name_(std::move(name)), value_(0) {}
    virtual ~Register() = default;
    bool operator==(const Register& other) const { return name_ == other.name_; }
    const std::string& getName() const { return name_; }
    virtual bool matchesName(const std::string& name) const { return name_ == name; }
    std::uint64_t getValue() const { return value_; }
    void setValue(std::uint64_t value) { value_ = value; }
    
 
private:
    std::string name_;
    std::uint64_t value_;
 
};

 
class RegisterFile {
public:
    virtual ~RegisterFile() = default;
    void addRegister(std::unique_ptr<Register> reg) {
        if (reg) registers_.push_back(std::move(reg));
    }
    const std::vector<std::unique_ptr<Register>>& getRegisters() const { return registers_; }
    Register* getRegisterByName(const std::string& name) {
        for (auto& reg : registers_) {
            if (reg->matchesName(name)) {
                return reg.get();
            }
        }
        return nullptr;
    }
    const Register* getRegisterByName(const std::string& name) const {
        for (const auto& reg : registers_) {
            if (reg->matchesName(name)) return reg.get();
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Register>> registers_;
};
