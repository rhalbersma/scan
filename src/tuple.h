
// tuple.h

#ifndef TUPLE_H
#define TUPLE_H

// includes

#include "bit.h"
#include "libmy.hpp"

// constants

const int Tuple_N_Max = 50;
const int Tuple_P_Max = 6;

// types

typedef uint32 tuple_t; // enough for 6 pieces

// variables

extern tuple_t Tuple_Size[Tuple_P_Max + 1][64];

// functions

extern void tuple_init ();

extern tuple_t tuple_index     (bit_t pieces, bit_t squares, int p, int n);
extern tuple_t tuple_index_rev (bit_t pieces, bit_t squares, int p, int n);

inline tuple_t tuple_size(int p, int n) { return Tuple_Size[p][n]; }

#endif // !defined TUPLE_H

// end of tuple.h

