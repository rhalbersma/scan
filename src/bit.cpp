
// includes

#include <cstdio>
#include <iostream>

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"

namespace bit {

// variables

static Bit File[Line_Size];
static Bit Rank[Line_Size];

static Bit Capture_Mask[Square_Size][64];
static Bit Beyond[Square_Size][64];
static int Line_Inc[Square_Size][64];

static Bit Man[Square_Size];
static Bit King[Square_Size];

// prototypes

static Bit ray_1 (Square from, Inc inc);
static Bit ray   (Square from, Inc inc);

// functions

void init() {

   // files and ranks

   for (int dense = 0; dense < Dense_Size; dense++) {

      Square sq = square_sparse(dense);

      set(File[square_file(sq)], sq);
      set(Rank[square_rank(sq)], sq);
   }

   // king attacks

   for (int dense = 0; dense < Dense_Size; dense++) {

      Square from = square_sparse(dense);

      for (int dir = 0; dir < Dir_Size; dir++) {

         Inc inc = dir_inc(dir);

         Man[from]  |= ray_1(from, inc);
         King[from] |= ray(from, inc);

         for (Bit b = ray(from, inc); b != 0; b = rest(b)) {

            Square to = first(b);

            Capture_Mask[from][to] = (ray(from, inc) & ~bit(to) & ~ray(to, inc)) | ray_1(to, inc);
            Beyond[from][to]       = ray(to, inc);
            Line_Inc[from][to]     = inc;
         }
      }
   }
}

static Bit ray_1(Square from, Inc inc) {

   Bit b = Bit(0);

   if (square_is_ok(from + inc)) {
      set(b, square_make(from + inc));
   }

   return b;
}

static Bit ray(Square from, Inc inc) {

   Bit b = Bit(0);

   for (int sq = from + inc; square_is_ok(sq); sq += inc) {
      set(b, square_make(sq));
   }

   return b;
}

Bit file(int fl) {
   assert(fl >= 0 && fl < Line_Size);
   return File[fl];
}

Bit rank(int rk) {
   assert(rk >= 0 && rk < Line_Size);
   return Rank[rk];
}

Bit rank(int rk, Side sd) {
   assert(rk >= 0 && rk < Line_Size);
   if (sd == White) rk = (Line_Size - 1) - rk;
   return Rank[rk];
}

Bit capture_mask(Square from, Square to) {
   assert(has(King[from] & Inner, to));
   return Capture_Mask[from][to];
}

Bit beyond(Square from, Square to) {
   assert(has(King[from], to));
   return Beyond[from][to];
}

Inc line_inc(Square from, Square to) {
   assert(has(King[from], to));
   return inc_make(Line_Inc[from][to]);
}

Bit man_attack(Square from) {
   return Man[from];
}

Bit king_attack(Square from) {
   return King[from];
}

Bit king_attack(Square from, Bit empty) {
   return attack(from, King[from], empty);
}

Bit attack(Square from, Bit tos, Bit empty) {

   assert(is_incl(tos, King[from]));

   for (Bit b = tos & Inner & ~empty; b != 0; b = rest(b)) {
      Square sq = first(b);
      tos &= ~Beyond[from][sq];
   }

   return tos;
}

}

