
#ifndef SORT_HPP
#define SORT_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

class List;
class Pos;

// functions

void sort_clear ();

void good_move (Move_Index index);
void bad_move  (Move_Index index);

void sort_all (List & list, const Pos & pos, Move_Index tt_move);

#endif // !defined SORT_HPP

