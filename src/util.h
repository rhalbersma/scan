
// util.h

#ifndef UTIL_H
#define UTIL_H

// includes

#include <iostream>
#include <string>
#include <vector>

#include <chrono>

#include "libmy.hpp"

// classes

class Bad_Input : public std::exception {

};

class Bad_Output : public std::exception {

};

class Scanner_Number {

private :

   std::string p_string;
   int p_pos;

public :

   Scanner_Number (const std::string & string);

   std::string get_token ();

   std::string string () const { return p_string; }
   bool eos () const;
   char get_char ();
   void unget_char ();
};

class Timer {

private :

   typedef std::chrono::time_point<std::chrono::system_clock> time_t;
   typedef std::chrono::duration<double> second_t;

   double p_elapsed;
   bool p_running;
   time_t p_start;

   static time_t now() {
      return std::chrono::system_clock::now();
   }

   double time() const {
      assert(p_running);
      return std::chrono::duration_cast<second_t>(now() - p_start).count();
   }

public :

   Timer() {
      reset();
   }

   void reset() {
      p_elapsed = 0;
      p_running = false;
   }

   void start() {
      p_start = now();
      p_running = true;
   }

   void stop() {
      p_elapsed += time();
      p_running = false;
   }

   double elapsed() const {
      double time = p_elapsed;
      if (p_running) time += this->time();
      return time;
   }
};

// functions

extern void load_file (std::vector<uint8> & table, std::istream & file);

extern bool string_is_nat (const std::string & s);

#endif // !defined UTIL_H

// end of util.h

