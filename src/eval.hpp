
#ifndef EVAL_HPP
#define EVAL_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

// functions

void  eval_init (const std::string & file_name);
Score eval      (const Pos & pos, Side sd);

#endif // !defined EVAL_HPP

