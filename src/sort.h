
// sort.h

#ifndef SORT_H
#define SORT_H

// includes

#include "libmy.hpp"

class List;
class Pos;

// functions

extern void sort_clear ();

extern void good_move (int index);
extern void bad_move  (int index);

extern void sort_all   (List & list, const Pos & pos, int killer);
extern void sort_trans (List & list, const Pos & pos, int killer);

#endif // !defined SORT_H

// end of sort.h

