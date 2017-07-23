
// includes

#include "bit.hpp"
#include "common.hpp"
#include "hash.hpp"
#include "libmy.hpp"
#include "pos.hpp"

namespace hash {

// constants

const int Table_Bit  { 18 };
const int Table_Size { 1 << Table_Bit };
const int Table_Mask { Table_Size - 1 };

// variables

static Key Key_Turn;
static Key Key_Piece[Piece_Side_Size][64];

static Key Key_Ranks_123[Table_Size];
static Key Key_Ranks_456[Table_Size];
static Key Key_Ranks_789[Table_Size];

static Key Key_Ranks_012[Table_Size];
static Key Key_Ranks_345[Table_Size];
static Key Key_Ranks_678[Table_Size];

// prototypes

static Key table_key (Piece_Side ps, int index, int offset);

// functions

void init() {

   // hash keys

   Key_Turn = Key(ml::rand_int_64());

   for (int ps = 0; ps < Piece_Side_Size; ps++) {
      for (int dense = 0; dense < Dense_Size; dense++) {
         Square sq = square_sparse(dense);
         Key_Piece[ps][sq] = Key(ml::rand_int_64());
      }
   }

   // men tables

   for (int index = 0; index < Table_Size; index++) {

      Key_Ranks_123[index] = table_key(White_Man, index,  6);
      Key_Ranks_456[index] = table_key(White_Man, index, 26);
      Key_Ranks_789[index] = table_key(White_Man, index, 45);

      Key_Ranks_012[index] = table_key(Black_Man, index,  0);
      Key_Ranks_345[index] = table_key(Black_Man, index, 19);
      Key_Ranks_678[index] = table_key(Black_Man, index, 39);
   }
}

static Key table_key(Piece_Side ps, int index, int offset) {

   assert(index >= 0 && index < Table_Size);
   assert(square_is_ok(offset));

   Key key = Key(0);

   uint64 group = (uint64(index) << offset) & bit::Squares;

   for (Bit b = Bit(group); b != 0; b = bit::rest(b)) {
      Square sq = bit::first(b);
      key ^= Key_Piece[ps][sq];
   }

   return key;
}

Key key(const Pos & pos) {

   Key key = Key(0);

   // men

   Bit wm = pos.wm();
   Bit bm = pos.bm();

   key ^= Key_Ranks_123[(wm >>  6) & Table_Mask];
   key ^= Key_Ranks_456[(wm >> 26) & Table_Mask];
   key ^= Key_Ranks_789[(wm >> 45) & Table_Mask];

   key ^= Key_Ranks_012[(bm >>  0) & Table_Mask];
   key ^= Key_Ranks_345[(bm >> 19) & Table_Mask];
   key ^= Key_Ranks_678[(bm >> 39) & Table_Mask];

   // kings

   for (Bit b = pos.wk(); b != 0; b = bit::rest(b)) {
      Square sq = bit::first(b);
      key ^= Key_Piece[White_King][sq];
   }

   for (Bit b = pos.bk(); b != 0; b = bit::rest(b)) {
      Square sq = bit::first(b);
      key ^= Key_Piece[Black_King][sq];
   }

   // turn

   if (pos.turn() != White) key ^= Key_Turn;

   return key;
}

}

