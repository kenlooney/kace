#include <kasm/kasmsemantic.hpp>

bool check_kasm_byte(const KasmStatement &statement, std::uint8_t &byte,
                     std::string &error)
{
    const bool is_define_byte = statement.name() == "define" && statement.type == DataType::Byte;
    const bool is_db = statement.name() == "db";
    const bool is_times_byte = statement.has_kind("Times") && statement.type == DataType::Byte;

    if (!is_define_byte && !is_db && !is_times_byte)
    {
        error = "Expected a byte definition";
        return false;
    }
    if (statement.value > 0xff)
    {
        error = "Byte value out of range: " + std::to_string(statement.value);
        return false;
    }
    byte = static_cast<std::uint8_t>(statement.value);
    return true;
}
