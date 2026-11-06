#include "kace/integers.hpp"
#include <sstream>
#include <stdexcept>

bool parse_integer(const std::string& text, std::uint64_t& value) {
   if (text.empty()) {
       return false;
   }

   std::size_t begin = 0;
   std::size_t end = text.size();
   int base = 10;
   bool has_prefix = false;

   if (end >= 2 && text[0] == '0') {
       switch (text[1]) {
       case 'b':
       case 'B':
           base = 2;
           begin = 2;
           has_prefix = true;
           break;
       case 'o':
       case 'O':
           base = 8;
           begin = 2;
           has_prefix = true;
           break;
       case 'x':
       case 'X':
           base = 16;
           begin = 2;
           has_prefix = true;
           break;
       default:
           break;
       }
   }

   if (!has_prefix && end >= 2) {
       switch (text.back()) {
       case 'b':
       case 'B':
           base = 2;
           --end;
           break;
       case 'o':
       case 'O':
           base = 8;
           --end;
           break;
       case 'h':
       case 'H':
           base = 16;
           --end;
           break;
       default:
           break;
       }
   }

   if (begin == end) {
       return false;
   }

   const std::string digits = text.substr(begin, end - begin);
   if (digits.empty() || digits[0] == '+' || digits[0] == '-') {
       return false;
   }

   std::size_t parsed = 0;
   try {
       const std::uint64_t parsed_value =
           std::stoull(digits, &parsed, base);
       if (parsed != digits.size()) {
           return false;
       }

       value = parsed_value;
       return true;
   } catch (const std::invalid_argument&) {
       return false;
   } catch (const std::out_of_range&) {
       return false;
   }
}
