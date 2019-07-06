
// includes

#include <cctype>
#include <iostream>
#include <string>

#include "hub.hpp"
#include "libmy.hpp"
#include "thread.hpp" // for get_line
#include "util.hpp"

namespace hub {

// prototypes

static std::string hub_pair  (const std::string & name, const std::string & value);
static std::string hub_value (const std::string & value);

// functions

void error(const std::string & msg) {
   write(std::string("error ") + hub_pair("message", msg));
}

std::string read() {

   std::string line;
   if (!get_line(line)) std::exit(EXIT_SUCCESS); // EOF

   return line;
}

void write(const std::string & line) {
   std::cout << line << std::endl;
}

void add_pair(std::string & line, const std::string & name, int value) {
   line += " " + hub_pair(name, std::to_string(value));
}

void add_pair(std::string & line, const std::string & name, double value, int /* precision */) { // TODO: use precision
   line += " " + hub_pair(name, std::to_string(value));
}

void add_pair(std::string & line, const std::string & name, const std::string & value) {
   line += " " + hub_pair(name, value);
}

static std::string hub_pair(const std::string & name, const std::string & value) {
   return name + "=" + hub_value(value);
}

static std::string hub_value(const std::string & value) {

   if (Scanner::is_name(value)) {
      return value;
   } else {
      return std::string("\"") + value + "\""; // TODO: protect '"'?
   }
}

Scanner::Scanner(const std::string & s) : m_string{s} {}

std::string Scanner::get_command() {
   return get_name();
}

Pair Scanner::get_pair() {

   std::string name = get_name(); // <name>

   std::string value;

   skip_blank();

   if (peek_char() == '=') { // = <value>
      skip_char();
      value = get_value();
   }

   return {name, value};
}

std::string Scanner::get_name() {

   std::string name;

   skip_blank();

   while (is_id(peek_char())) {
      name += get_char();
   }

   if (name.empty()) throw Bad_Input(); // not a name

   return name;
}

std::string Scanner::get_value() {

   std::string value;

   skip_blank();

   if (peek_char() == '"') { // "<value>"

      skip_char();

      while (peek_char() != '"') {
         if (is_end()) throw Bad_Input(); // missing closing '"'
         value += get_char();
      }

      skip_char(); // closing '"'

   } else { // <value>

      value = get_name();
   }

   return value;
}

bool Scanner::eos() {
   skip_blank();
   return is_end();
}

void Scanner::skip_blank() {
   while (is_blank(peek_char())) {
      skip_char();
   }
}

void Scanner::skip_char() {
   assert(!is_end());
   m_pos++;
}

char Scanner::get_char() {
   assert(!is_end());
   return m_string[m_pos++];
}

char Scanner::peek_char() const {
   return is_end() ? '\0' : m_string[m_pos]; // HACK but makes parsing easier
}

bool Scanner::is_end() const {
   return m_pos == int(m_string.size());
}

bool Scanner::is_name(const std::string & s) {

   for (char c : s) {
      if (!is_id(c)) return false;
   }

   return !s.empty();
}

bool Scanner::is_blank(char c) {
   return std::isspace(c);
}

bool Scanner::is_id(char c) {
   return !std::iscntrl(c) && !is_blank(c) && c != '=' && c != '"'; // excludes '\0'
}

} // namespace hub

