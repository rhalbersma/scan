
// tuple.cpp

// includes

#include <algorithm>
#include <iostream>
#include <string>

#include "bit.h"
#include "libmy.hpp"
#include "pos.h"
#include "tuple.h"

// variables

tuple_t Tuple_Size[Tuple_P_Max + 1][64];

// prototypes

static int bit_pos     (int sq, bit_t squares);
static int bit_pos_rev (int sq, bit_t squares);

static bit_t before (int sq);
static bit_t after  (int sq);

// functions

void tuple_init() {

   // Pascal triangle

   for (int n = 0; n <= Tuple_N_Max; n++) {

      Tuple_Size[0][n] = 1;

      for (int p = 1; p <= Tuple_P_Max; p++) {
         Tuple_Size[p][n] = 0;
      }
   }

   for (int n = 1; n <= Tuple_N_Max; n++) {
      for (int p = 1; p <= std::min(n, Tuple_P_Max); p++) {
         Tuple_Size[p][n] = Tuple_Size[p][n - 1] + Tuple_Size[p - 1][n - 1];
      }
   }
}

tuple_t tuple_index(bit_t pieces, bit_t squares, int p, int n) {

   assert(p >= 0 && p <= Tuple_P_Max);
   assert(n >= p && n <= Tuple_N_Max);

   assert(bit_count(pieces) == p);
   assert(bit_count(squares) == n);
   assert(bit_incl(pieces, squares));

   tuple_t index = 0;

   bit_t b;
   int i;

   for (b = pieces, i = 0; b != 0; b = bit_rest(b), i++) {
      int sq = bit_first(b);
      int pos = bit_pos(sq, squares);
      index += tuple_size(i + 1, pos);
   }

   assert(i == p);

   assert(index < tuple_size(p, n));
   return index;
}

tuple_t tuple_index_rev(bit_t pieces, bit_t squares, int p, int n) {

   assert(p >= 0 && p <= Tuple_P_Max);
   assert(n >= p && n <= Tuple_N_Max);

   assert(bit_count(pieces) == p);
   assert(bit_count(squares) == n);
   assert(bit_incl(pieces, squares));

   tuple_t index = 0;

   int square[Tuple_P_Max + 1];

   bit_t b;
   int i;

   for (b = pieces, i = 0; b != 0; b = bit_rest(b), i++) {
      int sq = bit_first(b);
      assert(i < p);
      square[p - i - 1] = sq;
   }

   assert(i == p);

   for (int i = 0; i < p; i++) {
      int sq = square[i];
      int pos = bit_pos_rev(sq, squares);
      index += tuple_size(i + 1, pos);
   }

   assert(index < tuple_size(p, n));
   return index;
}

static int bit_pos(int sq, bit_t squares) {

   assert(square_is_ok(sq));
   assert(bit_test(squares, sq));

   return bit_count(before(sq) & squares);
}

static int bit_pos_rev(int sq, bit_t squares) {

   assert(square_is_ok(sq));
   assert(bit_test(squares, sq));

   return bit_count(after(sq) & squares);
}

static bit_t before(int sq) {

   assert(square_is_ok(sq));

   return bit_t(bit(sq) - 1);
}

static bit_t after(int sq) {

   assert(square_is_ok(sq));

   assert(sq < 63);
   return bit_t(-bit(sq + 1));
}

// end of tuple.cpp

