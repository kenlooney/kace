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
#include "kasm/kasmlexer.hpp"

static bool is_hex_digit(char ch) {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
}

std::vector<Token> KasmLexer::tokenize(const std::string& input) {
 
    std::size_t cursor = 0;
    std::size_t begin = 0;
    while (cursor < input.size()) {
        char ch = input[cursor];

        // Skip whitespace characters
        if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            ++cursor;
            continue;
        }

        // Handle comments
        
        // Traditional single-line comment starting with ';'
        if(ch == ';') {
            begin = cursor;
            while(cursor < input.size() && input[cursor] != '\n') {
                ++cursor;
            }
            continue;
        }

        // Modern single-line comment starting with '//'
        if(ch == '/' && cursor + 1 < input.size() && input[cursor + 1] == '/') {
            begin = cursor;
            cursor += 2; // Skip '//'
            while(cursor < input.size() && input[cursor] != '\n') {
                ++cursor;
            }
           
            continue;
        }

        // Modern multi-line comment starting with '/*' and ending with '*/'
        if(ch == '/' && cursor + 1 < input.size() && input[cursor + 1] == '*') {
            begin = cursor;
            cursor += 2; // Skip '/*'
            while(cursor + 1 < input.size() && !(input[cursor] == '*' && input[cursor + 1] == '/')) {
                ++cursor;
            }
            if(cursor + 1 < input.size()) {
                cursor += 2; // Skip '*/'
            }
            
            continue;
        }



       // Handle identifiers
       // Identifiers must start with a letter and can contain letters, digits, or underscores
       if(is_letter(ch)) {
           begin = cursor;
           while(cursor < input.size() && (is_letter(input[cursor]) || is_digit(input[cursor]) || input[cursor] == '_')) {
               ++cursor;
           }
           // Check if it is a label (ends with ':')
           if(cursor < input.size() && input[cursor] == ':') {
               ++cursor;
               addToken(Token{"label", input.substr(begin, cursor - begin), Span{begin, cursor}});
               continue;
           }

           addToken(Token{"identifier", input.substr(begin, cursor - begin), Span{begin, cursor}});
           continue;
       }

       // Handle string literals
       if(ch == '"' || ch == '\'') {
           begin = cursor;
           char quote = input[cursor];
           ++cursor; // Skip the opening quote
           while(cursor < input.size() && input[cursor] != quote) {
               ++cursor;
           }
           if(cursor == input.size()) {
               addToken(Token{"unterminated_string", input.substr(begin + 1), Span{begin, cursor}});
               continue;
           }
           ++cursor; // Skip the closing quote
           addToken(Token{"string", input.substr(begin + 1, cursor - begin - 2), Span{begin, cursor}});
           continue;
       }

       // Handle Integer literals

       // Binary literals (starting with '0b' or '0B')
       if(ch == '0' && cursor + 1 < input.size() && (input[cursor + 1] == 'b' || input[cursor + 1] == 'B')) {
           begin = cursor;
           cursor += 2; // Skip '0b' or '0B'
           while(cursor < input.size() && (input[cursor] == '0' || input[cursor] == '1')) {
               ++cursor;
           }
           addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
           continue;
       }
       // Hexadecimal literals (starting with '0x' or '0X')
       if(ch == '0' && cursor + 1 < input.size() && (input[cursor + 1] == 'x' || input[cursor + 1] == 'X')) {
           begin = cursor;
           cursor += 2; // Skip '0x' or '0X'
           while(cursor < input.size() && is_hex_digit(input[cursor])) {
               ++cursor;
           }
           addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
           continue;
       }
       // Octal literals (starting with '0o' or '0O')
       if(ch == '0' && cursor + 1 < input.size() && (input[cursor + 1] == 'o' || input[cursor + 1] == 'O')) {
           begin = cursor;
           cursor += 2; // Skip '0o' or '0O'
           while(cursor < input.size() && (input[cursor] >= '0' && input[cursor] <= '7')) {
               ++cursor;
           }
           addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
           continue;
       }

       // Binary literals (ending with 'b' or 'B')
       if(is_digit(ch)) {
           begin = cursor;
           while(cursor < input.size() && (input[cursor] == '0' || input[cursor] == '1')) {
               ++cursor;
           }
           if(cursor > begin && cursor < input.size() &&
              (input[cursor] == 'b' || input[cursor] == 'B')) {
               ++cursor;
               addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
               continue;
           }
           cursor = begin;
       }

       // Hexadecimal literals (ending with 'h' or 'H')
       if(is_digit(ch)) {
           begin = cursor;
           while(cursor < input.size() && is_hex_digit(input[cursor])) {
               ++cursor;
           }
           if(cursor > begin && cursor < input.size() &&
              (input[cursor] == 'h' || input[cursor] == 'H')) {
               ++cursor;
               addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
               continue;
           }
           cursor = begin;
       }

       // Octal literals (ending with 'o' or 'O')
       if(is_digit(ch)) {
           begin = cursor;
           while(cursor < input.size() && input[cursor] >= '0' && input[cursor] <= '7') {
               ++cursor;
           }
           if(cursor > begin && cursor < input.size() &&
              (input[cursor] == 'o' || input[cursor] == 'O')) {
               ++cursor;
               addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
               continue;
           }
           cursor = begin;
       }

       // Decimal literals
       if(is_digit(ch)) {
           begin = cursor;
           while(cursor < input.size() && is_digit(input[cursor])) {
               ++cursor;
           }
           addToken(Token{"integer", input.substr(begin, cursor - begin), Span{begin, cursor}});
           continue;
       }

        // Handle '$' and '$$' as special identifiers
        if(ch == '$') {
            begin = cursor;
            ++cursor;
            if(cursor < input.size() && input[cursor] == '$') {
                ++cursor;
                addToken(Token{"dollardollar", "$$", Span{begin, cursor}});
            } else {
                addToken(Token{"dollar", "$", Span{begin, cursor}});
            }
            continue;
        }
        // Handle directives starting with '#'
        if(ch == '#') {
            begin = cursor;
            ++cursor;
            addToken(Token{"directive", input.substr(begin, cursor - begin), Span{begin, cursor}});
            continue;
        }

       begin = cursor;
       ++cursor;
        addToken(Token{"punctuation", input.substr(cursor - 1, 1), Span{begin, cursor}});
    }
    return getTokens();
}
