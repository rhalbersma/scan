
#ifndef BOOK_HPP
#define BOOK_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace book {

// functions

void init  ();
bool probe (const Pos & pos, Score margin, Move & move, Score & score);

} // namespace book

#endif // !defined BOOK_HPP

