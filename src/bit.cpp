
// bit.cpp

// includes

#include <cstdio>

#include "bit.h"
#include "libmy.hpp"
#include "pos.h"

// variables

static bit_t Bit_File[10];
static bit_t Bit_Rank[10];

static bit_t King_One[Square_Size];
static bit_t King_Almost[Square_Size];
static bit_t King_All[Square_Size];

static bit_t Beyond[64][64];
static bit_t Between[64][64];
static int Line_Inc[64][64];

static bit_t Bit_Rev_10[1 << 10];

// prototypes

static bit_t bit_one    (int sq, int inc);
static bit_t bit_almost (int sq, int inc);
static bit_t bit_all    (int sq, int inc);
static bit_t bit_5      (int sq, int inc);

static bit_t bit_rev_10 (bit_t b);

// functions

void bit_init() {

   // board lines

   for (int i = 0; i < 5; i++) {

      Bit_File[i * 2 + 0] = bit_5(i + 11, 11);
      Bit_File[i * 2 + 1] = bit_5(i +  6, 11);

      Bit_Rank[i * 2 + 0] = bit_5(i * 11 +  6, 1);
      Bit_Rank[i * 2 + 1] = bit_5(i * 11 + 11, 1);
   }

   // king attacks

   for (int from = 0; from < 64; from++) {
      for (int sq = 0; sq < 64; sq++) {
         Beyond[from][sq] = 0;
         Between[from][sq] = 0;
         Line_Inc[from][sq] = 0;
      }
   }

   for (int i = 0; i < 50; i++) {

      int from = square_from_50(i);

      King_One[from]    = 0;
      King_Almost[from] = 0;
      King_All[from]    = 0;

      for (int dir = 0; dir < Dir_Size; dir++) {

         int inc = Inc[dir];

         King_One[from]    |= bit_one(from, inc);
         King_Almost[from] |= bit_almost(from, inc);
         King_All[from]    |= bit_all(from, inc);

         for (bit_t b = bit_almost(from, inc); b != 0; b = bit_rest(b)) {
            int sq = bit_first(b);
            Beyond[from][sq] = bit_all(sq, inc);
         }

         for (bit_t b = bit_all(from, inc); b != 0; b = bit_rest(b)) {
            int sq = bit_first(b);
            Between[from][sq] = bit_all(from, inc) & ~bit(sq) & ~bit_all(sq, inc);
            Line_Inc[from][sq] = inc;
         }
      }
   }

   // board reverse

   for (int i = 0; i < (1 << 10); i++) {
      Bit_Rev_10[i] = bit_rev_10(i);
   }
}

bit_t bit_rev(bit_t b) {

   bit_t rev = 0;

   for (int i = 0; i < 5; i++) {

      int sq = +i * 11 +  6;
      int sr = -i * 11 + 50;

      bit_t line = (b >> sq) & bit_mask(10);
      rev |= Bit_Rev_10[line] << sr;
   }

   return rev;
}

static bit_t bit_rev_10(bit_t b) {

   bit_t rev = 0;

   for (; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      assert(sq < 10);
      bit_set(rev, 9 - sq);
   }

   return rev;
}

bit_t king_attack_one(int from) {

   assert(square_is_ok(from));

   return King_One[from];
}

bit_t king_attack_almost(int from) {

   assert(square_is_ok(from));

   return King_Almost[from];
}

bit_t king_attack(int from, bit_t blockers) {

   assert(square_is_ok(from));

   bit_t attacks = King_All[from];

   for (bit_t b = blockers & King_Almost[from]; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      attacks &= ~Beyond[from][sq];
   }

   return attacks;
}

static bit_t bit_one(int sq, int inc) {

   assert(square_is_ok(sq));

   bit_t b = 0;

   if (square_is_ok(sq + inc)) {
      bit_set(b, sq + inc);
   }

   return b;
}

static bit_t bit_almost(int sq, int inc) {

   assert(square_is_ok(sq));

   bit_t b = 0;

   if (square_is_ok(sq + inc)) {
      for (sq += inc; square_is_ok(sq + inc); sq += inc) {
         bit_set(b, sq);
      }
   }

   return b;
}

static bit_t bit_all(int sq, int inc) {

   assert(square_is_ok(sq));

   bit_t b = 0;

   for (sq += inc; square_is_ok(sq); sq += inc) {
      bit_set(b, sq);
   }

   return b;
}

static bit_t bit_5(int sq, int inc) {

   assert(square_is_ok(sq));

   bit_t b = 0;

   for (int i = 0; i < 5; i++) {
      square_is_ok(sq + inc * i);
      bit_set(b, sq + inc * i);
   }

   return b;
}

bit_t bit_file(int fl) {

   assert(fl >= 0 && fl < 10);
   return Bit_File[fl];
}

bit_t bit_rank(int rk) {

   assert(rk >= 0 && rk < 10);
   return Bit_Rank[rk];
}

bit_t bit_between(int from, int to) {

   assert(square_is_ok(from));
   assert(square_is_ok(to));

   return Between[from][to];
}

int line_inc(int from, int to) {

   assert(square_is_ok(from));
   assert(square_is_ok(to));

   return Line_Inc[from][to];
}

// end of bit.cpp

