
// hash.h

#ifndef HASH_H
#define HASH_H

// includes

#include "board.h"
#include "libmy.hpp"

// functions

extern void hash_init ();

extern uint64 hash_key (const Board & bd);

extern uint64 turn_key  ();
extern uint64 piece_key (int pc, int sq);

#endif // !defined HASH_H

// end of hash.h

