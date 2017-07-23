
// includes

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bb_base.hpp"
#include "bit.hpp"
#include "common.hpp"
#include "fen.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "move_gen.hpp"
#include "pos.hpp"
#include "var.hpp"

// functions

Pos::Pos(Side turn, Bit wm, Bit bm, Bit wk, Bit bk)
: Pos(wm | bm, wk | bk, wm | wk, bm | bk, wm | bm | wk | bk, turn)
{
   assert(bit::count(wm | bm | wk | bk) == bit::count(wm) + bit::count(bm) + bit::count(wk) + bit::count(bk)); // all disjoint?

   assert(bit::is_incl(wm, bit::WM_Squares));
   assert(bit::is_incl(bm, bit::BM_Squares));
}

Pos::Pos(Bit man, Bit king, Bit white, Bit black, Bit all, Side turn) {

   assert((man ^ king) == all);
   assert((white ^ black) == all);

   assert(bit::is_incl(man & white, bit::WM_Squares));
   assert(bit::is_incl(man & black, bit::BM_Squares));

   Bit side[Side_Size] { white, black };
   assert(side[side_opp(turn)] != 0);
   if (var::Variant == var::BT) assert((side[turn] & king) == 0);

   assert(bit::count(white) <= 20);
   assert(bit::count(black) <= 20);

   p_piece[Man]  = man;
   p_piece[King] = king;
   p_side[White] = white;
   p_side[Black] = black;
   p_all = all;
   p_turn = turn;
}

Pos Pos::succ(Move mv) const {

   Square from = move::from(mv);
   Square to   = move::to(mv);
   Bit    caps = move::captured(mv);

   Side atk = p_turn;
   Side def = side_opp(atk);

   assert(is_side(from, atk));
   assert(from == to || is_empty(to));
   assert(bit::is_incl(caps, side(def)));

   Bit piece[Piece_Size] { p_piece[Man],  p_piece[King] };
   Bit side[Side_Size]   { p_side[White], p_side[Black] };
   Bit all = this->all();

   Bit delta = bit::bit(from) ^ bit::bit(to);

   side[atk] ^= delta;
   all ^= delta;

   if (is_piece(from, King)) { // king move
      piece[King] ^= delta;
   } else if (square_is_promotion(to, atk)) { // promotion
      bit::clear(piece[Man], from);
      bit::set(piece[King], to);
   } else { // man move
      piece[Man] ^= delta;
   }

   piece[Man]  &= ~caps;
   piece[King] &= ~caps;
   side[def]   &= ~caps;
   all         &= ~caps;

   return Pos(piece[Man], piece[King], side[White], side[Black], all, def);
}

Node::Node(const Pos & pos) : Node(pos, 0, nullptr) {
}

Node::Node(const Pos & pos, int ply, const Node * parent) {
   p_pos = pos;
   p_ply = ply;
   p_parent = parent;
}

Node Node::succ(Move mv) const {

   Pos new_pos = p_pos.succ(mv);

   if (move::is_conversion(mv, p_pos)) {
      return Node(new_pos);
   } else {
      return Node(new_pos, p_ply + 1, this);
   }
}

bool Node::is_end() const {
   return pos::is_loss(p_pos) || is_draw(3);
}

bool Node::is_draw(int rep) const {

   if (p_ply < 4) return false;

   const Node * node = this;

   int n = 1;

   for (int i = 0; i < p_ply / 2; i++) {

      node = node->p_parent;
      assert(node != nullptr);

      node = node->p_parent;
      assert(node != nullptr);

      assert(node->p_pos.turn() == p_pos.turn());

      if (node->p_pos.wk() == p_pos.wk()
       && node->p_pos.bk() == p_pos.bk()) {
         n++;
         if (n == rep) return true;
      }
   }

   return false;
}

namespace pos { // ###

// types

typedef int fun_t(Piece_Side ps, Square sq);

// constants

const int Table_Bit  { 18 };
const int Table_Size { 1 << Table_Bit };
const int Table_Mask { Table_Size - 1 };

// variables

Pos Start;

static int Tempo_Ranks_123[Table_Size];
static int Tempo_Ranks_456[Table_Size];
static int Tempo_Ranks_789[Table_Size];

static int Tempo_Ranks_012[Table_Size];
static int Tempo_Ranks_345[Table_Size];
static int Tempo_Ranks_678[Table_Size];

static int Skew_Ranks_123[Table_Size];
static int Skew_Ranks_456[Table_Size];
static int Skew_Ranks_789[Table_Size];

static int Skew_Ranks_012[Table_Size];
static int Skew_Ranks_345[Table_Size];
static int Skew_Ranks_678[Table_Size];

// prototypes

static int table_val (fun_t f, Piece_Side ps, int index, int offset);

static int tempo (Piece_Side ps, Square sq);
static int skew  (Piece_Side ps, Square sq);

// functions

void init() {

   // starting position

   Start = pos_from_fen(Start_FEN);

   // men tables

   for (int index = 0; index < Table_Size; index++) {

      Tempo_Ranks_123[index] = table_val(tempo, White_Man, index,  6);
      Tempo_Ranks_456[index] = table_val(tempo, White_Man, index, 26);
      Tempo_Ranks_789[index] = table_val(tempo, White_Man, index, 45);

      Tempo_Ranks_012[index] = table_val(tempo, Black_Man, index,  0);
      Tempo_Ranks_345[index] = table_val(tempo, Black_Man, index, 19);
      Tempo_Ranks_678[index] = table_val(tempo, Black_Man, index, 39);

      Skew_Ranks_123[index]  = table_val(skew,  White_Man, index,  6);
      Skew_Ranks_456[index]  = table_val(skew,  White_Man, index, 26);
      Skew_Ranks_789[index]  = table_val(skew,  White_Man, index, 45);

      Skew_Ranks_012[index]  = table_val(skew,  Black_Man, index,  0);
      Skew_Ranks_345[index]  = table_val(skew,  Black_Man, index, 19);
      Skew_Ranks_678[index]  = table_val(skew,  Black_Man, index, 39);
   }
}

bool is_loss(const Pos & pos) {
   return !can_move(pos, pos.turn())
       || (var::Variant == var::BT && has_king(pos, side_opp(pos.turn())));
}

bool is_wipe(const Pos & pos) { // fast subset of "is_loss"
   return pos.side(pos.turn()) == 0
       || (var::Variant == var::BT && has_king(pos, side_opp(pos.turn())));
}

Piece_Side piece_side(const Pos & pos, Square sq) {

   int ps = (bit::bit(pos.empty(), sq) << 2)
          | (bit::bit(pos.king(),  sq) << 1)
          | (bit::bit(pos.black(), sq) << 0);

   return Piece_Side(ps); // can be Empty
}

double phase(const Pos & pos) {

   double phase = double(stage(pos)) / double(Stage_Size);

   assert(phase >= 0.0 && phase <= 1.0);
   return phase;
}

int stage(const Pos & pos) {

   int stage = 300 - tempo(pos);

   assert(stage >= 0 && stage <= Stage_Size);
   return stage;
}

int tempo(const Pos & pos) {

   int tempo = 0;

   tempo += Tempo_Ranks_123[(pos.wm() >>  6) & Table_Mask];
   tempo += Tempo_Ranks_456[(pos.wm() >> 26) & Table_Mask];
   tempo += Tempo_Ranks_789[(pos.wm() >> 45) & Table_Mask];

   tempo += Tempo_Ranks_012[(pos.bm() >>  0) & Table_Mask];
   tempo += Tempo_Ranks_345[(pos.bm() >> 19) & Table_Mask];
   tempo += Tempo_Ranks_678[(pos.bm() >> 39) & Table_Mask];

   return tempo;
}

int skew(const Pos & pos, Side sd) {

   int skew = 0;

   if (sd == White) {
      skew += Skew_Ranks_123[(pos.wm() >>  6) & Table_Mask];
      skew += Skew_Ranks_456[(pos.wm() >> 26) & Table_Mask];
      skew += Skew_Ranks_789[(pos.wm() >> 45) & Table_Mask];
   } else {
      skew += Skew_Ranks_012[(pos.bm() >>  0) & Table_Mask];
      skew += Skew_Ranks_345[(pos.bm() >> 19) & Table_Mask];
      skew += Skew_Ranks_678[(pos.bm() >> 39) & Table_Mask];
   }

   return skew;
}

static int tempo(Piece_Side ps, Square sq) {

   switch (piece_side_piece(ps)) {
      case Man  : return (Line_Size - 1) - square_rank(sq, piece_side_side(ps));
      case King : return 0;
      default   : return 0;
   }
}

static int skew(Piece_Side ps, Square sq) {

   switch (piece_side_piece(ps)) {
      case Man  : return square_file(sq) * 2 - (Line_Size - 1);
      case King : return 0;
      default   : return 0;
   }
}

static int table_val(fun_t f, Piece_Side ps, int index, int offset) {

   assert(index >= 0 && index < Table_Size);
   assert(square_is_ok(offset));

   int val = 0;

   uint64 group = (uint64(index) << offset) & bit::Squares;

   for (Bit b = Bit(group); b != 0; b = bit::rest(b)) {
      Square sq = bit::first(b);
      val += f(ps, sq);
   }

   return val;
}

void disp(const Pos & pos) {

   // pieces

   for (int rk = 0; rk < Line_Size; rk++) {

      for (int fl = 0; fl < Line_Size; fl++) {

         if (square_is_light(fl, rk)) {

            std::printf("  ");

         } else {

            Square sq = square_make(fl, rk);

            switch (piece_side(pos, sq)) {
               case White_Man :  std::printf("O "); break;
               case Black_Man :  std::printf("* "); break;
               case White_King : std::printf("@ "); break;
               case Black_King : std::printf("# "); break;
               case Empty :      std::printf("- "); break;
               default :         std::printf("? "); break;
            }
         }
      }

      std::printf("  ");

      for (int fl = 0; fl < Line_Size; fl++) {

         if (square_is_light(fl, rk)) {
            std::printf("  ");
         } else {
            Square sq = square_make(fl, rk);
            std::printf("%02d", square_to_std(sq));
         }
      }

      std::printf("\n");
   }

   std::printf("\n");
   std::fflush(stdout);

   // turn

   Side atk = pos.turn();
   Side def = side_opp(atk);

   if (is_loss(pos)) {

      std::cout << side_to_string(def) << " wins #";

   } else {

      std::cout << side_to_string(atk) << " to play";

      if (bb::pos_is_load(pos)) {
         bb::Value val = bb::probe(pos);
         std::cout << ", endgame " << bb::value_to_string(val);
      }
   }

   std::cout << std::endl;
   std::cout << std::endl;
}

}

