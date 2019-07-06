
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
   file.read((char *) table.data(), size);
}

Scanner_Number::Scanner_Number(const std::string & s) : m_string{s} {}

std::string Scanner_Number::get_token() {

   if (eos()) return "";

   std::string token {};

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

bool Scanner_Number::eos() const {
   return m_pos == int(m_string.size());
}

char Scanner_Number::get_char() {
   assert(!eos());
   return m_string[m_pos++];
}

void Scanner_Number::unget_char() {
   assert(m_pos != 0);
   m_pos--;
}

bool string_is_nat(const std::string & s) {

   for (char c : s) {
      if (!std::isdigit(c)) return false;
   }

   return !s.empty();
}

