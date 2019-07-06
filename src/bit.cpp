
// includes

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "var.hpp"

namespace bit {

// variables

static Bit File[File_Size];
static Bit Rank[Rank_Size];

static Bit Capture_Mask[Square_Size][64];
static Bit Beyond[Square_Size][64];
static int Line_Inc[Square_Size][64];

static Bit Man_Moves[Square_Size];
static Bit King_Moves[Square_Size];

static Bit Man_Captures[Square_Size];
static Bit King_Captures[Square_Size];

// prototypes

static Bit ray_first (Square from, Inc inc);
static Bit ray_last  (Square from, Inc inc);
static Bit ray_all   (Square from, Inc inc);

// functions

void init() {

   static_assert(sizeof(Bit) == 8, "");

   // files and ranks

   for (Square sq : Squares) {
      set(File[square_file(sq)], sq);
      set(Rank[square_rank(sq)], sq);
   }

   // king attacks

   for (Square from : Squares) {

      Man_Captures[from]  = Bit(0);
      King_Captures[from] = Bit(0);

      for (int dir = 0; dir < Dir_Size; dir++) {

         Inc inc = dir_inc(dir);

         if (dir < 4) {
            Man_Moves[from]  |= ray_first(from, inc); // HACK: all directions
            King_Moves[from] |= ray_all  (from, inc);
         }

         if (dir < 4 || var::Variant == var::Frisian) {
            Man_Captures[from]  |= ray_first(from, inc) & ~ray_last(from, inc);
            King_Captures[from] |= ray_all  (from, inc) & ~ray_last(from, inc);
         }

         for (Square to : ray_all(from, inc)) {
            Capture_Mask[from][to] = (ray_all(from, inc) & ~bit(to) & ~ray_all(to, inc)) | ray_first(to, inc);
            Beyond[from][to]       = ray_all(to, inc);
            Line_Inc[from][to]     = inc;
         }
      }
   }
}

static Bit ray_first(Square from, Inc inc) {
   Bit b {};
   if (square_is_ok(from + inc)) set(b, square_make(from + inc));
   return b;
}

static Bit ray_last(Square from, Inc inc) {

   Bit b {};

   if (square_is_ok(from + inc)) {

      int sq = from + inc;
      while (square_is_ok(sq + inc)) sq += inc;

      set(b, square_make(sq));
   }

   return b;
}

static Bit ray_all(Square from, Inc inc) {

   Bit b {};

   for (int sq = from + inc; square_is_ok(sq); sq += inc) {
      set(b, square_make(sq));
   }

   return b;
}

Bit file(int fl) {
   assert(fl >= 0 && fl < File_Size);
   return File[fl];
}

Bit rank(int rk) {
   assert(rk >= 0 && rk < Rank_Size);
   return Rank[rk];
}

Bit rank(int rk, Side sd) {
   assert(rk >= 0 && rk < Rank_Size);
   if (sd != Black) rk = (Rank_Size - 1) - rk;
   return Rank[rk];
}

Bit capture_mask(Square from, Square to) {
   assert(has(King_Captures[from], to));
   return Capture_Mask[from][to];
}

Bit beyond(Square from, Square to) {
   assert(has(King_Captures[from], to));
   return Beyond[from][to];
}

Inc line_inc(Square from, Square to) {
   assert(has(King_Captures[from], to));
   return inc_make(Line_Inc[from][to]);
}

Bit man_moves(Square from) {
   return Man_Moves[from];
}

Bit man_captures(Square from) {
   return Man_Captures[from];
}

Bit king_captures(Square from) {
   return King_Captures[from];
}

Bit king_moves(Square from, Bit empty) {
   return attack(from, King_Moves[from], empty);
}

Bit king_captures(Square from, Bit empty) {
   return attack(from, King_Captures[from], empty);
}

Bit attack(Square from, Bit tos, Bit empty) {

   for (Square sq : tos & King_Captures[from] & ~empty) {
      tos &= ~Beyond[from][sq];
   }

   return tos;
}

} // namespace bit

