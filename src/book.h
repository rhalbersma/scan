
// book.h

#ifndef BOOK_H
#define BOOK_H

// includes

#include "libmy.hpp"
#include "move.h"

class Board;

namespace book {

// functions

extern void init ();

extern move_t move (const Board & bd, int margin);

}

#endif // !defined BOOK_H

// end of book.h

