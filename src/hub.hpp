
#ifndef HUB_HPP
#define HUB_HPP

// includes

#include <string>
#include <utility>

#include "libmy.hpp"

namespace hub {

// types

typedef std::pair<std::string, std::string> Pair;

class Scanner {

private :

   std::string p_string;
   int p_pos;

public :

   explicit Scanner (const std::string & s);

   std::string get_command ();
   Pair        get_pair    ();
   std::string get_name    ();
   std::string get_value   ();

   bool eos (); // skips blanks before testing

   static bool is_name (const std::string & s);

private :

   void skip_blank ();
   void skip_char  ();
   char get_char   ();

   bool is_end    () const;
   char peek_char () const;

   static bool is_blank (char c);
   static bool is_id    (char c);
};

// functions

void error (const std::string & msg);

std::string read  ();
void        write (const std::string & line);

void add_pair (std::string & line, const std::string & name, int value);
void add_pair (std::string & line, const std::string & name, double value, int precision);
void add_pair (std::string & line, const std::string & name, const std::string & value);

}

#endif // !defined HUB_HPP

