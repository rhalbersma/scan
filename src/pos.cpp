
// includes

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "bb_base.hpp"
#include "bit.hpp"
#include "common.hpp"
#include "gen.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "score.hpp"
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

   Bit side[Side_Size] { white, black }; // for debug
   assert(side[side_opp(turn)] != 0);
   if (var::Variant == var::BT) assert((side[turn] & king) == 0);

   m_piece = { man, king };
   m_side = { white, black };
   m_all = all;
   m_turn = turn;

   for (int sd = 0; sd < Side_Size; sd++) {
      m_wolf[sd] = -1;
      m_count[sd] = 0;
   }
}

Pos Pos::succ(Move mv) const {

   Square from = move::from(mv, *this);
   Square to   = move::to(mv, *this);
   Bit    caps = move::captured(mv, *this);

   Side atk = m_turn;
   Side def = side_opp(atk);

   assert(is_side(from, atk));
   assert(from == to || is_empty(to));
   assert(bit::is_incl(caps, side(def)));

   auto piece = m_piece;
   auto side = m_side;
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

   Pos pos(piece[Man], piece[King], side[White], side[Black], all, def);

   if (var::Variant == var::Frisian) {

      if (m_count[def] != 0 && pos.is_piece(square_make(m_wolf[def]), King) && pos.man(def) != 0) {
         pos.m_wolf[def] = m_wolf[def];
         pos.m_count[def] = m_count[def];
      }

      if (pos.man(atk) != 0 && !move::is_conversion(mv, *this)) { // quiet king move

         if (from == m_wolf[atk]) pos.m_count[atk] = m_count[atk]; // same king

         pos.m_wolf[atk] = to;
         pos.m_count[atk] += 1;
         assert(pos.m_count[atk] <= 3);
      }
   }

   return pos;
}

bool operator==(const Pos & p0, const Pos & p1) { // for repetition detection

   if (p0.m_all != p1.m_all) return false;

   for (int pc = 0; pc < Piece_Size; pc++) {
      if (p0.m_piece[pc] != p1.m_piece[pc]) return false;
   }

   for (int sd = 0; sd < Side_Size; sd++) {
      if (p0.m_side[sd] != p1.m_side[sd]) return false;
   }

   if (p0.m_turn != p1.m_turn) return false;

   if (var::Variant == var::Frisian) {

      for (int sd = 0; sd < Side_Size; sd++) {
         if (p0.m_count[sd] != p1.m_count[sd]) return false;
         if (p0.m_count[sd] != 0 && p0.m_wolf[sd] != p1.m_wolf[sd]) return false;
      }
   }

   return true;
}

Node::Node(const Pos & pos) : Node{pos, 0, nullptr} {}

Node::Node(const Pos & pos, int ply, const Node * parent) {
   m_pos = pos;
   m_ply = ply;
   m_parent = parent;
}

Node Node::succ(Move mv) const {

   Pos new_pos = m_pos.succ(mv);

   if (move::is_conversion(mv, m_pos)) {
      return Node{new_pos};
   } else {
      return Node{new_pos, m_ply + 1, this};
   }
}

bool Node::is_end() const {
   return pos::is_end(m_pos) || is_draw(3);
}

bool Node::is_draw(int rep) const {

   if (m_ply < 4) return false;

   int n = 1;

   const Node * node = this;

   for (int i = 0; i < m_ply / 2; i++) {

      node = node->m_parent;
      assert(node != nullptr);

      node = node->m_parent;
      assert(node != nullptr);

      if (node->m_pos == m_pos) {
         n += 1;
         if (n == rep) return true;
      }
   }

   return false;
}

namespace pos { // ###

// types

using fun_t = int (Piece pc, Side sd, Square sq);

// constants

const int Table_Bit  {18};
const int Table_Size {1 << Table_Bit};
const int Table_Mask {Table_Size - 1};

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

static int table_val (fun_t f, Piece pc, Side sd, int index, int offset);

static int tempo (Piece pc, Side sd, Square sq);
static int skew  (Piece pc, Side sd, Square sq);

// functions

void init() {

   // starting position

   Start = Pos(White, Bit(0x7DF3EF8000000000), Bit(0x0000000000FBE7DF), Bit(0), Bit(0));

   // men tables

   for (int index = 0; index < Table_Size; index++) {

      Tempo_Ranks_123[index] = table_val(tempo, Man, White, index,  6);
      Tempo_Ranks_456[index] = table_val(tempo, Man, White, index, 26);
      Tempo_Ranks_789[index] = table_val(tempo, Man, White, index, 45);

      Tempo_Ranks_012[index] = table_val(tempo, Man, Black, index,  0);
      Tempo_Ranks_345[index] = table_val(tempo, Man, Black, index, 19);
      Tempo_Ranks_678[index] = table_val(tempo, Man, Black, index, 39);

      Skew_Ranks_123[index]  = table_val(skew,  Man, White, index,  6);
      Skew_Ranks_456[index]  = table_val(skew,  Man, White, index, 26);
      Skew_Ranks_789[index]  = table_val(skew,  Man, White, index, 45);

      Skew_Ranks_012[index]  = table_val(skew,  Man, Black, index,  0);
      Skew_Ranks_345[index]  = table_val(skew,  Man, Black, index, 19);
      Skew_Ranks_678[index]  = table_val(skew,  Man, Black, index, 39);
   }
}

bool is_end(const Pos & pos) {
   return !can_move(pos, pos.turn())
       || (var::Variant == var::BT && has_king(pos, side_opp(pos.turn())));
}

bool is_wipe(const Pos & pos) { // fast subset of "is_end", for BT variant
   return pos.side(pos.turn()) == 0
       || (var::Variant == var::BT && has_king(pos, side_opp(pos.turn())));
}

int result(const Pos & pos, Side sd) { // for losing variant

   assert(is_end(pos));

   int res = (var::Variant == var::Losing) ? +1 : -1; // for turn
   res = score::side(res, pos.turn()); // for white
   return score::side(res, sd); // for sd
}

Piece_Side piece_side(const Pos & pos, Square sq) {

   int ps = (bit::bit(pos.empty(), sq) << 2)
          | (bit::bit(pos.king(),  sq) << 1)
          | (bit::bit(pos.black(), sq) << 0);

   return Piece_Side(ps); // can be Empty
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

static int tempo(Piece pc, Side sd, Square sq) { // pc for debug
   assert(pc == Man);
   return (Rank_Size - 1) - square_rank(sq, sd);
}

static int skew(Piece pc, Side /* sd */, Square sq) { // pc for debug
   assert(pc == Man);
   return square_file(sq) * 2 - (File_Size - 1);
}

static int table_val(fun_t f, Piece pc, Side sd, int index, int offset) {

   assert(index >= 0 && index < Table_Size);
   assert(square_is_ok(offset));

   int val = 0;

   for (Square sq : bit::Squares & (uint64(index) << offset)) {
      val += f(pc, sd, sq);
   }

   return val;
}

void disp(const Pos & pos) {

   // pieces

   for (int rk = 0; rk < Rank_Size; rk++) {

      for (int fl = 0; fl < File_Size; fl++) {

         if (square_is_light(fl, rk)) {

            std::cout << "  ";

         } else {

            Square sq = square_make(fl, rk);

            switch (piece_side(pos, sq)) {
               case White_Man :  std::cout << "O "; break;
               case Black_Man :  std::cout << "* "; break;
               case White_King : std::cout << "@ "; break;
               case Black_King : std::cout << "# "; break;
               case Empty :      std::cout << "- "; break;
            }
         }
      }

      std::cout << "  ";

      for (int fl = 0; fl < File_Size; fl++) {

         if (square_is_light(fl, rk)) {
            std::cout << "  ";
         } else {
            Square sq = square_make(fl, rk);
            std::cout << std::right << std::setfill('0') << std::setw(2) << square_to_std(sq);
         }
      }

      std::cout << '\n';
   }

   std::cout << '\n';

   // turn/result

   Side atk = pos.turn();
   Side def = side_opp(atk);

   if (is_end(pos)) {
      std::cout << side_to_string(var::Variant == var::Losing ? atk : def) << " wins #";
   } else {
      std::cout << side_to_string(atk) << " to play";
      if (bb::pos_is_load(pos)) std::cout << ", bitbase " << bb::value_to_string(bb::probe(pos));
   }

   std::cout << '\n';
   std::cout << std::endl;
}

} // namespace pos

