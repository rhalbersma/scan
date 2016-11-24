
// pos.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "bit.h"
#include "board.h"
#include "fen.h"
#include "libmy.hpp"
#include "move.h"
#include "move_gen.h"
#include "pos.h"
#include "util.h"
#include "var.h"

// "constants"

const int Square_From_50[50] = {
      6,  7,  8,  9, 10,
   11, 12, 13, 14, 15,
     17, 18, 19, 20, 21,
   22, 23, 24, 25, 26,
     28, 29, 30, 31, 32,
   33, 34, 35, 36, 37,
     39, 40, 41, 42, 43,
   44, 45, 46, 47, 48,
     50, 51, 52, 53, 54,
   55, 56, 57, 58, 59,
};

const int Square_To_50[Square_Size] = {
   -1, -1, -1, -1, -1, -1,
      0,  1,  2,  3,  4,
    5,  6,  7,  8,  9, -1,
     10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, -1,
     20, 21, 22, 23, 24,
   25, 26, 27, 28, 29, -1,
     30, 31, 32, 33, 34,
   35, 36, 37, 38, 39, -1,
     40, 41, 42, 43, 44,
   45, 46, 47, 48, 49, -1,
     -1, -1, -1, -1, -1,
};

const int Square_File[Square_Size] = {
   -1, -1, -1, -1, -1, -1,
      1,  3,  5,  7,  9,
    0,  2,  4,  6,  8, -1,
      1,  3,  5,  7,  9,
    0,  2,  4,  6,  8, -1,
      1,  3,  5,  7,  9,
    0,  2,  4,  6,  8, -1,
      1,  3,  5,  7,  9,
    0,  2,  4,  6,  8, -1,
      1,  3,  5,  7,  9,
    0,  2,  4,  6,  8, -1,
     -1, -1, -1, -1, -1,
};

const int Square_Rank[Square_Size] = {
   -1, -1, -1, -1, -1, -1,
      0,  0,  0,  0,  0,
    1,  1,  1,  1,  1, -1,
      2,  2,  2,  2,  2,
    3,  3,  3,  3,  3, -1,
      4,  4,  4,  4,  4,
    5,  5,  5,  5,  5, -1,
      6,  6,  6,  6,  6,
    7,  7,  7,  7,  7, -1,
      8,  8,  8,  8,  8,
    9,  9,  9,  9,  9, -1,
     -1, -1, -1, -1, -1,
};

const int Inc[Dir_Size] = {
   NW_Inc, NE_Inc, SW_Inc, SE_Inc,
};

// functions

std::string square_to_string(int sq) {

   return ml::itos(square_to_50(sq) + 1);
}

bool string_is_square(const std::string & s) {

   if (!string_is_nat(s)) return false;

   int sq = ml::stoi(s);
   return sq >= 1 && sq <= 50;
}

int square_from_string(const std::string & s) {

   if (!string_is_nat(s)) throw Bad_Input();
   return square_from_int(ml::stoi(s));
}

int square_from_int(int sq) {

   if (sq < 1 || sq > 50) throw Bad_Input();
   return square_from_50(sq - 1);
}

void Pos::copy(const Pos & pos) {

   init(pos.p_turn, pos.p_piece[White], pos.p_piece[Black], pos.p_king);
}

void Pos::init() {

   pos_from_fen(*this, Start_FEN);
}

void Pos::init(int turn, bit_t wp, bit_t bp, bit_t k) {

   assert((wp & bp) == 0);
   assert(bit_incl(k, wp | bp));

   p_turn = turn;
   p_piece[White] = wp;
   p_piece[Black] = bp;
   p_king = k;
}

void Pos::from_bit(int turn, bit_t wm, bit_t bm, bit_t wk, bit_t bk) {

   init(turn, wm | wk, bm | bk, wk | bk);
}

extern void pos_rev(Pos & dst, const Pos & src) {

   dst.init(side_opp(src.turn()), bit_rev(src.piece(Black)), bit_rev(src.piece(White)), bit_rev(src.king()));
}

void pos_do_move(Pos & dst, const Pos & src, move_t mv) {

   int from = move_from(mv);
   int to = move_to(mv);
   bit_t captured = move_captured(mv);

   int atk = src.turn();
   int def = side_opp(atk);

   assert(bit_test(src.piece(atk), from));
   assert(from == to || bit_test(src.empty(), to));
   assert(bit_incl(captured, src.piece(def)));

   bit_t piece[Side_Size] = { src.piece(White), src.piece(Black) };
   bit_t king = src.king();

   bit_clear(piece[atk], from);
   bit_set(piece[atk], to);

   if (bit_test(src.man(), from)) { // man move

      if (square_is_promotion(to, atk)) {
         bit_set(king, to);
      }

   } else { // king move

      assert(bit_test(king, from));

      bit_clear(king, from);
      bit_set(king, to);
   }

   piece[def] &= ~captured;
   king       &= ~captured;

   dst.init(def, piece[White], piece[Black], king);
}

bool pos_is_capture(const Pos & pos) {

   return can_capture(pos, pos.turn());
}

int pos_size(const Pos & pos) {

   int nw = bit_count(pos.piece(White));
   int nb = bit_count(pos.piece(Black));

   return nw + nb;
}

bool pos_has_king(const Pos & pos) {

   return pos.king() != 0;
}

int pos_square(const Pos & pos, int sq) {

   assert(square_is_ok(sq));

   if (false) {
   } else if (bit_test(pos.empty(), sq)) {
      return Empty;
   } else if (bit_test(pos.wm(), sq)) {
      return WM;
   } else if (bit_test(pos.bm(), sq)) {
      return BM;
   } else if (bit_test(pos.wk(), sq)) {
      return WK;
   } else if (bit_test(pos.bk(), sq)) {
      return BK;
   } else {
      assert(false);
      return Frame;
   }
}

// end of pos.cpp

