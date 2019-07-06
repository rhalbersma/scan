
#ifndef UTIL_HPP
#define UTIL_HPP

// includes

#include <iostream>
#include <string>
#include <vector>

#include <chrono>

#include "libmy.hpp"

// types

class Bad_Input  : public std::exception {};
class Bad_Output : public std::exception {};

class Scanner_Number {

private:

   const std::string m_string;
   int m_pos {0};

public:

   explicit Scanner_Number (const std::string & s);

   std::string get_token ();

   bool eos () const;
   char get_char ();
   void unget_char ();
};

class Timer {

private:

   using time_t   = std::chrono::time_point<std::chrono::system_clock>;
   using second_t = std::chrono::duration<double>;

   double m_elapsed {0.0};
   bool m_running {false};
   time_t m_start;

public:

   void reset() {
      m_elapsed = 0.0;
      m_running = false;
   }

   void start() {
      m_start = now();
      m_running = true;
   }

   void stop() {
      m_elapsed += time();
      m_running = false;
   }

   double elapsed() const {
      double time = m_elapsed;
      if (m_running) time += this->time();
      return time;
   }

private:

   static time_t now() {
      return std::chrono::system_clock::now();
   }

   double time() const {
      assert(m_running);
      return std::chrono::duration_cast<second_t>(now() - m_start).count();
   }
};

// functions

void load_file (std::vector<uint8> & table, std::istream & file);

bool string_is_nat (const std::string & s);

#endif // !defined UTIL_HPP

