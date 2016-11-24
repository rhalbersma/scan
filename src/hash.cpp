
// hash.cpp

// includes

#include "board.h"
#include "hash.h"
#include "libmy.hpp"

// constants

static const int Random_Size = 50 * 4 + 1;

// variables

static uint64 Turn_Key;
static uint64 Piece_Key[Piece_Size][64];

static uint64 Random_64[Random_Size];

// prototypes

static uint64 rand_64 ();

// functions

void hash_init() {

   for (int i = 0; i < Random_Size; i++) {
      Random_64[i] = rand_64();
   }

   Turn_Key = Random_64[0];

   for (int pc = 0; pc < Piece_Size; pc++) {
      for (int i = 0; i < 50; i++) {
         int sq = square_from_50(i);
         Piece_Key[pc][sq] = Random_64[i * 4 + pc + 1];
      }
   }
}

uint64 hash_key(const Board & bd) {

   uint64 key = 0;

   // turn

   if (bd.turn() != White) key ^= turn_key();

   // pieces

   for (int pc = 0; pc < Piece_Size; pc++) {
      for (bit_t b = bd.bit(pc); b != 0; b = bit_rest(b)) {
         int sq = bit_first(b);
         key ^= piece_key(pc, sq);
      }
   }

   return key;
}

uint64 turn_key() {

   return Random_64[0];
}

uint64 piece_key(int pc, int sq) {

   assert(piece_is_ok(pc));
   assert(square_is_ok(sq));

   int sq_50 = square_to_50(sq);

   return Random_64[sq_50 * 4 + pc + 1];
}

static uint64 rand_64() {

   uint64 r = 0;

   for (int i = 0; i < 8; i++) {
      r = (r << 8) | ml::rand_int(256);
   }

   return r;
}

// end of hash.cpp

