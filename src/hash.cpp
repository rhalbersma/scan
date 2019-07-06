
// includes

#include "bit.hpp"
#include "common.hpp"
#include "hash.hpp"
#include "libmy.hpp"
#include "pos.hpp"
#include "var.hpp"

namespace hash {

// constants

const int Table_Bit  {18};
const int Table_Size {1 << Table_Bit};
const int Table_Mask {Table_Size - 1};

// variables

static Key Key_Turn;
static Key Key_Piece[Side_Size][Piece_Size][64];
static Key Key_Wolf[Side_Size][4][64];

static Key Key_Ranks_123[Table_Size];
static Key Key_Ranks_456[Table_Size];
static Key Key_Ranks_789[Table_Size];

static Key Key_Ranks_012[Table_Size];
static Key Key_Ranks_345[Table_Size];
static Key Key_Ranks_678[Table_Size];

// prototypes

static Key table_key (Piece pc, Side sd, int index, int offset);

// functions

void init() {

   // hash keys

   Key_Turn = Key(ml::rand_int_64());

   for (int sd = 0; sd < Side_Size; sd++) {
      for (int pc = 0; pc < Piece_Size; pc++) {
         for (Square sq : bit::Squares) {
            Key_Piece[sd][pc][sq] = Key(ml::rand_int_64());
         }
      }
   }

   for (int sd = 0; sd < Side_Size; sd++) {
      for (int count = 1; count <= 3; count++) {
         for (Square sq : bit::Squares) {
            Key_Wolf[sd][count][sq] = Key(ml::rand_int_64());
         }
      }
   }

   // men tables

   for (int index = 0; index < Table_Size; index++) {

      Key_Ranks_123[index] = table_key(Man, White, index,  6);
      Key_Ranks_456[index] = table_key(Man, White, index, 26);
      Key_Ranks_789[index] = table_key(Man, White, index, 45);

      Key_Ranks_012[index] = table_key(Man, Black, index,  0);
      Key_Ranks_345[index] = table_key(Man, Black, index, 19);
      Key_Ranks_678[index] = table_key(Man, Black, index, 39);
   }
}

static Key table_key(Piece pc, Side sd, int index, int offset) {

   assert(index >= 0 && index < Table_Size);
   assert(square_is_ok(offset));

   Key key {};

   for (Square sq : bit::Squares & (uint64(index) << offset)) {
      key ^= Key_Piece[sd][pc][sq];
   }

   return key;
}

Key key(const Pos & pos) {

   Key key {};

   // men

   key ^= Key_Ranks_123[(pos.wm() >>  6) & Table_Mask];
   key ^= Key_Ranks_456[(pos.wm() >> 26) & Table_Mask];
   key ^= Key_Ranks_789[(pos.wm() >> 45) & Table_Mask];

   key ^= Key_Ranks_012[(pos.bm() >>  0) & Table_Mask];
   key ^= Key_Ranks_345[(pos.bm() >> 19) & Table_Mask];
   key ^= Key_Ranks_678[(pos.bm() >> 39) & Table_Mask];

   // kings

   for (int sd = 0; sd < Side_Size; sd++) {
      for (Square sq : pos.king(Side(sd))) {
         key ^= Key_Piece[sd][King][sq];
      }
   }

   // wolves

   if (var::Variant == var::Frisian) {

      for (int side = 0; side < Side_Size; side++) {
         Side sd = side_make(side);
         if (pos.count(sd) != 0) key ^= Key_Wolf[sd][pos.count(sd)][pos.wolf(sd)];
      }
   }

   // turn

   if (pos.turn() != White) key ^= Key_Turn;

   return key;
}

} // namespace hash

