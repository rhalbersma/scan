
// includes

#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#include "libmy.hpp"
#include "util.hpp"

// functions

void load_file(std::vector<uint8> & table, std::istream & file) {

   int64 size = ml::stream_size(file);
   table.resize(size);

   file.read((char *) &table[0], size);
}

Scanner_Number::Scanner_Number(const std::string & s) {
   p_string = s;
   p_pos = 0;
}

std::string Scanner_Number::get_token() {

   if (eos()) return "";

   std::string token;

   char c = get_char();
   token += c;

   if (std::isdigit(c)) {

      while (!eos()) {

         char c = get_char();

         if (!std::isdigit(c)) {
            unget_char();
            break;
         }

         token += c;
      }
   }

   return token;
}

std::string Scanner_Number::string() const {
   return p_string;
}

bool Scanner_Number::eos() const {
   return p_pos >= int(p_string.size());
}

char Scanner_Number::get_char() {
   assert(!eos());
   return p_string[p_pos++];
}

void Scanner_Number::unget_char() {
   assert(p_pos > 0);
   p_pos--;
}

bool string_is_nat(const std::string & s) {

   if (s.size() == 0) return false;

   for (int i = 0; i < int(s.size()); i++) {
      if (!std::isdigit(s[i])) return false;
   }

   return true;
}

