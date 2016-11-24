
// board.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bb_base.h"
#include "bit.h"
#include "board.h"
#include "fen.h"
#include "hash.h"
#include "libmy.hpp"
#include "move.h"
#include "move_gen.h"
#include "pos.h"

// constants

const int Trit_WM    = -1;
const int Trit_Empty = 0;
const int Trit_BM    = +1;

// "constants"

const int Piece_Trit[5] = {
   Trit_WM, Trit_Empty, Trit_BM, Trit_Empty, Trit_Empty
};

// variables

static int Piece_Pip[Piece_Size][64];
static int Piece_Skew[Piece_Size][64];

// prototypes

static int square_pip  (int sq, int sd);
static int square_skew (int sq);

static int piece_trit (int pc);

// functions

void board_init() {

   for (int pc = 0; pc < Piece_Size; pc++) {

      for (int i = 0; i < 50; i++) {

         int sq = square_from_50(i);

         if (piece_is_man(pc)) {
            Piece_Pip[pc][sq] = square_pip(sq, piece_side(pc));
            Piece_Skew[pc][sq] = square_skew(sq);
         } else {
            Piece_Pip[pc][sq] = 0;
            Piece_Skew[pc][sq] = 0;
         }
      }
   }
}

Board::Board() {

   for (int sq = 0; sq < Square_Size; sq++) {
      p_square[sq] = square_is_ok(sq) ? Empty : Frame;
   }
}

void Board::init() {

   board_from_fen(*this, Start_FEN);
}

void Board::copy(const Board & bd) {

   from_bit(bd.turn(), bd.bit_wm(), bd.bit_bm(), bd.bit_wk(), bd.bit_bk());

   p_ply = bd.p_ply;
   p_rep = bd.p_rep;
}

void Board::from_bit(int turn, bit_t wm, bit_t bm, bit_t wk, bit_t bk) {

   // clear

   // p_pos.p_turn = 0;

   for (int i = 0; i < 50; i++) {

      int sq = square_from_50(i);

      p_square[sq] = Empty;
      p_trit[sq] = Trit_Empty;
   }

   p_key = 0;
   p_pip = 0;
   p_skew[White] = 0;
   p_skew[Black] = 0;

   p_ply = 0;
   p_rep.clear();

   // init

   p_pos.p_turn = turn;

   p_bit[WM] = wm;
   p_bit[BM] = bm;
   p_bit[WK] = wk;
   p_bit[BK] = bk;

   p_pos.p_piece[White] = wm | wk;
   p_pos.p_piece[Black] = bm | bk;
   p_pos.p_king = wk | bk;

   for (int pc = 0; pc < Piece_Size; pc++) {

      for (bit_t b = p_bit[pc]; b != 0; b = bit_rest(b)) {

         int sq = bit_first(b);

         p_square[sq] = pc;
         p_trit[sq] = piece_trit(pc);
      }
   }

   p_key = hash_key(*this);
   p_pip = board_pip(*this);
   p_skew[White] = board_skew(*this, White);
   p_skew[Black] = board_skew(*this, Black);
}

void Board::from_pos(const Pos & pos) {

   from_bit(pos.turn(), pos.wm(), pos.bm(), pos.wk(), pos.bk());
}

void Board::do_move(move_t mv, Undo & undo) {

   undo.ply = p_ply;

   if (move_is_conversion(mv, *this)) {
      p_ply = 0;
   } else {
      p_ply++;
   }

   p_rep.add(key());

   int from = move_from(mv);
   int to = move_to(mv);
   bit_t captured = move_captured(mv);

   int atk = p_pos.p_turn;
   int def = side_opp(atk);

   int pc = p_square[from];
   assert(pc != Empty && piece_is_side(pc, atk));

   undo.promotion = move_is_promotion(mv, *this);
   undo.wp = p_pos.p_piece[White];
   undo.bp = p_pos.p_piece[Black];
   undo.king = p_pos.p_king;

   bit_clear(p_pos.p_piece[atk], from);
   bit_set(p_pos.p_piece[atk], to);

   if (piece_is_king(pc)) {
      bit_clear(p_pos.p_king, from);
      bit_set(p_pos.p_king, to);
   } else if (square_is_promotion(to, atk)) {
      bit_set(p_pos.p_king, to);
   }

   p_pos.p_piece[def] &= ~captured;
   p_pos.p_king       &= ~captured;

   for (bit_t b = captured; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      remove_piece(p_square[sq], sq);
   }

   if (!undo.promotion) {
      move_piece(pc, from, to);
   } else {
      remove_piece(pc, from);
      if (undo.promotion) pc = piece_promotion(pc);
      add_piece(pc, to);
   }

   switch_turn();
}

void Board::undo_move(move_t mv, const Undo & undo) {

   int from = move_from(mv);
   int to = move_to(mv);
   bit_t captured = move_captured(mv);

   int def = p_pos.p_turn;
   int atk = side_opp(def);

   p_pos.p_piece[White] = undo.wp;
   p_pos.p_piece[Black] = undo.bp;
   p_pos.p_king = undo.king;

   switch_turn();

   int pc = p_square[to];
   assert(pc != Empty && piece_is_side(pc, atk));

   if (!undo.promotion) {
      move_piece(pc, to, from);
   } else {
      remove_piece(pc, to);
      if (undo.promotion) pc = piece_promotion(pc);
      add_piece(pc, from);
   }

   for (bit_t b = captured & ~p_pos.p_king; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      add_piece(piece_man(def), sq);
   }

   for (bit_t b = captured & p_pos.p_king; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      add_piece(piece_king(def), sq);
   }

   p_ply = undo.ply;
   p_rep.remove();
}

void Board::set_root() {

   int ply = p_ply;
   int size = p_rep.size();

   assert(ply <= size);

   for (int i = 0; i < ply; i++) {
      p_rep[i] = p_rep[size - ply + i];
   }

   p_rep.set_size(ply);
}

bool Board::is_draw(int rep) const {

   int ply = p_ply;

   if (ply >= 50) {

      return !board_is_loss(*this);

   } else if (ply >= 4) {

      int n = 1;

      int size = p_rep.size();
      assert(ply <= size);

      uint64 key = this->key();

      for (int i = 4; i <= ply; i += 2) {
         if (p_rep[size - i] == key) {
            n++;
            if (n == rep) return true;
         }
      }
   }

   return false;
}

void Board::switch_turn() {

   p_pos.p_turn = side_opp(p_pos.p_turn);
   p_key ^= turn_key();
}

void Board::add_piece(int pc, int sq) {

   assert(piece_is_ok(pc));
   assert(square_is_ok(sq));

   assert(p_square[sq] == Empty);
   p_square[sq] = pc;
   p_trit[sq] = piece_trit(pc);

   bit_switch(p_bit[pc], sq);

   p_key ^= piece_key(pc, sq);
   p_pip += Piece_Pip[pc][sq];
   p_skew[piece_side(pc)] += Piece_Skew[pc][sq];
}

void Board::remove_piece(int pc, int sq) {

   assert(piece_is_ok(pc));
   assert(square_is_ok(sq));

   assert(p_square[sq] == pc);
   p_square[sq] = Empty;
   p_trit[sq] = Trit_Empty;

   bit_switch(p_bit[pc], sq);

   p_key ^= piece_key(pc, sq);
   p_pip -= Piece_Pip[pc][sq];
   p_skew[piece_side(pc)] -= Piece_Skew[pc][sq];
}

void Board::move_piece(int pc, int from, int to) {

   assert(piece_is_ok(pc));
   assert(square_is_ok(from));
   assert(square_is_ok(to));

   assert(p_square[from] == pc);
   p_square[from] = Empty;
   p_trit[from] = Trit_Empty;

   assert(p_square[to] == Empty);
   p_square[to] = pc;
   p_trit[to] = piece_trit(pc);

   bit_switch(p_bit[pc], from);
   bit_switch(p_bit[pc], to);

   p_key ^= piece_key(pc, from);
   p_key ^= piece_key(pc, to);

   p_pip -= Piece_Pip[pc][from];
   p_pip += Piece_Pip[pc][to];

   int sd = piece_side(pc);
   p_skew[sd] -= Piece_Skew[pc][from];
   p_skew[sd] += Piece_Skew[pc][to];
}

void board_from_fen(Board & bd, const std::string & s) {

   Pos pos;
   pos_from_fen(pos, s);

   bd.from_pos(pos);
}

void board_from_hub(Board & bd, const std::string & s) {

   Pos pos;
   pos_from_hub(pos, s);

   bd.from_pos(pos);
}

void board_do_move(Board & bd, move_t mv) {

   Undo undo;
   bd.do_move(mv, undo);
   bd.set_root();
}

bool board_is_end(const Board & bd, int rep) {

   return board_is_loss(bd) || bd.is_draw(rep);
}

bool board_is_loss(const Board & bd) {

   return !can_move(bd, bd.turn());
}

bool board_is_wipe(const Board & bd) {

   return bd.bit_piece(bd.turn()) == 0;
}

int board_pip(const Board & bd) {

   bit_t bit_wm = bd.bit_wm();
   bit_t bit_bm = bd.bit_bm();

   int pip = 0;

   for (int rk = 0; rk < 10; rk++) {

      bit_t rank = bit_rank(rk);

      pip += bit_count(bit_wm & rank) * rk;
      pip += bit_count(bit_bm & rank) * (9 - rk);
   }

   assert(pip >= 0 && pip <= 300);
   return pip;
}

int board_skew(const Board & bd, int sd) {

   assert(side_is_ok(sd));

   bit_t bm = bd.bit_man(sd);

   int skew = 0;

   for (int fl = 0; fl < 10; fl++) {
      skew += bit_count(bm & bit_file(fl)) * (fl * 2 - 9);
   }

   assert(skew >= -120 && skew <= +120);
   return skew;
}

double board_phase(const Board & bd) {

   double phase = double(300 - bd.pip()) / 300.0;

   assert(phase >= 0.0 && phase <= 1.0);
   return phase;
}

void board_disp(const Board & bd) {

   for (int y = 0; y < 10; y++) {

      if (y % 2 == 0) std::printf("  ");

      for (int x = 0; x < 5; x++) {

         int sq = square_from_50(y * 5 + x);

         switch (bd.square(sq)) {
         case WM :    std::printf("O   "); break;
         case BM :    std::printf("*   "); break;
         case WK :    std::printf("@   "); break;
         case BK :    std::printf("#   "); break;
         case Empty : std::printf("-   "); break;
         default :    std::printf("?   "); break;
         }
      }

      for (int x = 0; x < 5; x++) {
         std::printf("  %02d", y * 5 + x + 1);
      }

      std::printf("\n");
   }

   std::printf("\n");

   if (board_is_loss(bd)) {

      if (bd.turn() == White) {
         std::cout << "black wins #";
      } else {
         assert(bd.turn() == Black);
         std::cout << "white wins #";
      }

   } else if (bd.is_draw(3)) {

      std::cout << "draw #";

   } else {

      if (bd.turn() == White) {
         std::cout << "white to play";
      } else {
         assert(bd.turn() == Black);
         std::cout << "black to play";
      }

      if (bb::pos_is_game(bd)) {
         int val = bb::probe(bd);
         std::cout << ", bitbase " << bb::value_to_string(val);
      }
   }

   std::cout << std::endl;
   std::cout << std::endl;
}

static int square_pip(int sq, int sd) {

   return 9 - square_rank(sq, sd);
}

static int square_skew(int sq) {

   return square_file(sq) * 2 - 9;
}

static int piece_trit(int pc) {

   return Piece_Trit[pc]; 
}

// end of board.cpp

