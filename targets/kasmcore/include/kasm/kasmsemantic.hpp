#pragma once

#include <kasm/kasmstatement.hpp>
#include <cstdint>
#include <string>

bool check_kasm_byte(const KasmStatement &statement, std::uint8_t &byte,
                     std::string &error);
