
// includes

#include <cmath>

#include "bit.hpp"
#include "common.hpp"
#include "gen.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "pos.hpp"
#include "var.hpp"

// prototypes

static void gen_quiets (List & list, const Pos & pos);

static void add_man_moves         (List & list, const Pos & pos, Bit froms);
static void add_man_captures      (List & list, const Pos & pos, Bit bd, Bit be, Bit froms);
static void add_man_captures      (List & list, const Pos & pos, Bit bd, Bit be, Square from, Inc inc);
static void add_man_captures_rec  (List & list, const Pos & pos, Bit bd, Bit be, Square start, Square jump, Square from, Bit caps);

static void add_king_moves        (List & list, const Pos & pos, Square from);
static void add_king_captures     (List & list, const Pos & pos, Bit bd, Bit be, Square from);
static void add_king_captures_rec (List & list, const Pos & pos, Bit bd, Bit be, Square start, Square jump, Inc inc, Bit caps);

static Bit  contact_captures (const Pos & pos, Side sd);
static bool king_can_capture (const Pos & pos, Square from, Side def);

static void add_moves_from (List & list, Bit froms, Inc inc);
static void add_moves_to   (List & list, Bit tos, Inc inc);

// functions

void gen_moves(List & list, const Pos & pos) {
   gen_captures(list, pos);
   if (list.size() == 0) gen_quiets(list, pos);
}

void gen_captures(List & list, const Pos & pos) {

   list.clear();

   Side atk = pos.turn();
   Side def = side_opp(atk);

   Bit bd = pos.side(def);
   Bit be = pos.empty();

   // men

   add_man_captures(list, pos, bd, be, pos.man(atk));

   // kings

   for (Square from : pos.king(atk)) {
      add_king_captures(list, pos, bd, be, from);
   }
}

void gen_promotions(List & list, const Pos & pos) {

   list.clear();

   Side atk = pos.turn();
   add_man_moves(list, pos, pos.man(atk) & bit::rank(Rank_Size - 2, atk));
}

static void gen_quiets(List & list, const Pos & pos) {

   list.clear();

   Side atk = pos.turn();

   // men

   add_man_moves(list, pos, pos.man(atk));

   // kings

   for (Square from : pos.king(atk)) {

      if (var::Variant == var::Frisian && pos.count(atk) >= 3 && from == pos.wolf(atk)) continue;

      add_king_moves(list, pos, from);
   }
}

static void add_man_moves(List & list, const Pos & pos, Bit froms) {

   Side atk = pos.turn();

   Bit be = pos.empty();

   if (atk == White) {
      add_moves_from(list, froms & (be << I1), -I1);
      add_moves_from(list, froms & (be << J1), -J1);
   } else {
      add_moves_from(list, froms & (be >> I1), +I1);
      add_moves_from(list, froms & (be >> J1), +J1);
   }
}

static void add_man_captures(List & list, const Pos & pos, Bit bd, Bit be, Bit froms) {

   for (Square from : froms & (bd << J1) & (be << J2)) add_man_captures(list, pos, bd, be, from, -J1);
   for (Square from : froms & (bd << I1) & (be << I2)) add_man_captures(list, pos, bd, be, from, -I1);
   for (Square from : froms & (bd >> I1) & (be >> I2)) add_man_captures(list, pos, bd, be, from, +I1);
   for (Square from : froms & (bd >> J1) & (be >> J2)) add_man_captures(list, pos, bd, be, from, +J1);

   if (var::Variant == var::Frisian) {
      for (Square from : froms & (bd << L1) & (be << L2)) add_man_captures(list, pos, bd, be, from, -L1);
      for (Square from : froms & (bd << K1) & (be << K2)) add_man_captures(list, pos, bd, be, from, -K1);
      for (Square from : froms & (bd >> K1) & (be >> K2)) add_man_captures(list, pos, bd, be, from, +K1);
      for (Square from : froms & (bd >> L1) & (be >> L2)) add_man_captures(list, pos, bd, be, from, +L1);
   }
}

static void add_man_captures(List & list, const Pos & pos, Bit bd, Bit be, Square from, Inc inc) {
   Square sq = square_make(from + inc);
   add_man_captures_rec(list, pos, bd, bit::add(be, from), from, sq, square_make(sq + inc), Bit(0));
}

static void add_man_captures_rec(List & list, const Pos & pos, Bit bd, Bit be, Square start, Square jump, Square from, Bit caps) {

   assert(bit::has(be, from));

   bd = bit::remove(bd, jump);
   caps = bit::add(caps, jump);

   for (Square sq : bit::man_captures(from) & bd) {
      Square to = square_make(sq * 2 - from); // square behind sq
      if (bit::has(be, to)) add_man_captures_rec(list, pos, bd, be, start, sq, to, caps);
   }

   list.add_capture(start, from, caps, pos, 0);
}

static void add_king_moves(List & list, const Pos & pos, Square from) {

   Bit be = pos.empty();

   for (Square to : bit::king_moves(from, be) & be) {
      list.add_move(from, to);
   }
}

static void add_king_captures(List & list, const Pos & pos, Bit bd, Bit be, Square from) {

   be = bit::add(be, from);

   for (Square sq : bit::king_captures(from) & bd) {
      if (bit::is_incl(bit::capture_mask(from, sq), be)) {
         Inc inc = bit::line_inc(from, sq);
         add_king_captures_rec(list, pos, bd, be, from, sq, inc, Bit(0));
      }
   }
}

static void add_king_captures_rec(List & list, const Pos & pos, Bit bd, Bit be, Square start, Square jump, Inc inc, Bit caps) {

   Square next = square_make(jump + inc);
   assert(bit::has(be, next));

   bd = bit::remove(bd, jump);
   caps = bit::add(caps, jump);

   Bit ray = bit::beyond(square_make(jump - inc), jump); // ray(jump, inc)
   Bit froms = bit::attack(jump, ray, be);

   for (Square from : froms & be) {

      for (Square sq : bit::king_captures(from) & bd) {

         if (bit::is_incl(bit::capture_mask(from, sq), be)) {

            Inc new_inc = bit::line_inc(from, sq);

            assert(new_inc != -inc);
            if (new_inc == +inc && from != next) continue; // duplicate capture

            add_king_captures_rec(list, pos, bd, be, start, sq, new_inc, caps);
         }
      }

      bool cond = var::Variant == var::Killer && pos.is_piece(jump, King) && from != next;
      if (!cond) list.add_capture(start, from, caps, pos, 1);
   }
}

void add_sacs(List & list, const Pos & pos) {

   Side atk = pos.turn();
   Side def = side_opp(atk);

   Bit mp = pos.man(atk);
   Bit op = pos.side(def);
   Bit e  = pos.empty();

   if (pos.turn() == White) {

      // init

      uint64 strong = ((bit::file(1)             | bit::rank(Rank_Size - 2) | (mp >> I2)) & (mp >> I1) & (e << I1))
                    | ((bit::file(File_Size - 2) | bit::rank(Rank_Size - 2) | (mp >> J2)) & (mp >> J1) & (e << J1));

      uint64 weak = ((mp << J1) & (e << J2)) | ((mp << I1) & (e << I2))
                  | ((mp >> I1) & (e >> I2)) | ((mp >> J1) & (e >> J2));

      uint64 target = strong & ~weak;

      uint64 pin = ((mp << J1) & (op << J2)) | ((mp << I1) & (op << I2))
                 | ((mp >> I1) & (op >> I2)) | ((mp >> J1) & (op >> J2));

      uint64 danger_i = ((op << I1) & (e >> I1)) | ((op >> I1) & (e << I1));
      uint64 danger_j = ((op << J1) & (e >> J1)) | ((op >> J1) & (e << J1));

      uint64 wi = 0;
      uint64 wj = 0;

      // exchange

      wi |= (target >> I1) & (op << I1) & ~danger_j;
      wj |= (target >> J1) & (op << J1) & ~danger_i;

      wi |= ~(op << I1) & (op << J1) & ((e & target) >> J1);
      wj |= ~(op << J1) & (op << I1) & ((e & target) >> I1);

      wi |= ~(op << I1) & (op >> J1) & ((e & target) << J1);
      wj |= ~(op << J1) & (op >> I1) & ((e & target) << I1);

      // create opponent hole

      uint64 opp_weak = ((op << I1) & (e  << I2)) | ((op << J1) & (e  << J2));
      uint64 opp_pin  = ((op >> I1) & (mp >> I2)) | ((op >> J1) & (mp >> J2));

      Bit opp_target = op & opp_weak & opp_pin;

      wi |= ((mp & ~weak) >> I1) & (opp_target << I1) & ~danger_j;
      wj |= ((mp & ~weak) >> J1) & (opp_target << J1) & ~danger_i;

      // wrap up

      wi &= ((mp & ~pin) >> I1) & e;
      wj &= ((mp & ~pin) >> J1) & e;

      add_moves_to(list, Bit(wi), -I1);
      add_moves_to(list, Bit(wj), -J1);

   } else { // Black

      // init

      uint64 strong = ((bit::file(1)             | bit::rank(1) | (mp << J2)) & (mp << J1) & (e >> J1))
                    | ((bit::file(File_Size - 2) | bit::rank(1) | (mp << I2)) & (mp << I1) & (e >> I1));

      uint64 weak = ((mp << J1) & (e << J2)) | ((mp << I1) & (e << I2))
                  | ((mp >> I1) & (e >> I2)) | ((mp >> J1) & (e >> J2));

      uint64 pin = ((mp << J1) & (op << J2)) | ((mp << I1) & (op << I2))
                 | ((mp >> I1) & (op >> I2)) | ((mp >> J1) & (op >> J2));

      uint64 danger_i = ((op >> I1) & (e << I1)) | ((op << I1) & (e >> I1));
      uint64 danger_j = ((op >> J1) & (e << J1)) | ((op << J1) & (e >> J1));

      uint64 bi = 0;
      uint64 bj = 0;

      // exchange

      uint64 target = strong & ~weak;

      bi |= (target << I1) & (op >> I1) & ~danger_j;
      bj |= (target << J1) & (op >> J1) & ~danger_i;

      bi |= ~(op >> I1) & (op >> J1) & ((e & target) << J1);
      bj |= ~(op >> J1) & (op >> I1) & ((e & target) << I1);

      bi |= ~(op >> I1) & (op << J1) & ((e & target) >> J1);
      bj |= ~(op >> J1) & (op << I1) & ((e & target) >> I1);

      // create opponent hole

      uint64 opp_weak = ((op >> I1) & (e  >> I2)) | ((op >> J1) & (e  >> J2));
      uint64 opp_pin  = ((op << I1) & (mp << I2)) | ((op << J1) & (mp << J2));

      Bit opp_target = op & opp_weak & opp_pin;

      bi |= ((mp & ~weak) << I1) & (opp_target >> I1) & ~danger_j;
      bj |= ((mp & ~weak) << J1) & (opp_target >> J1) & ~danger_i;

      // wrap up

      bi &= ((mp & ~pin) << I1) & e;
      bj &= ((mp & ~pin) << J1) & e;

      add_moves_to(list, Bit(bi), +I1);
      add_moves_to(list, Bit(bj), +J1);
   }
}

bool can_move(const Pos & pos, Side sd) {

   Side atk = sd;
   Side def = side_opp(atk);

   Bit be = pos.empty();

   // man moves

   Bit bm = pos.man(atk);

   if (atk == White) {
      bm &= (be << I1) | (be << J1);
   } else {
      bm &= (be >> I1) | (be >> J1);
   }

   if (bm != 0) return true;

   // contact captures

   if (contact_captures(pos, atk) != 0) return true;

   // king moves

   for (Square from : pos.king(atk)) {

      if (var::Variant == var::Frisian && pos.count(atk) >= 3 && from == pos.wolf(atk)) continue;

      if ((bit::man_moves(from) & be) != 0) return true; // HACK: single step
   }

   // king captures

   if (var::Variant == var::Frisian) { // superfluous for other variants
      for (Square from : pos.king(atk)) {
         if (king_can_capture(pos, from, def)) return true;
      }
   }

   return false;
}

bool can_capture(const Pos & pos, Side sd) {

   Side atk = sd;
   Side def = side_opp(atk);

   // men

   if (contact_captures(pos, atk) != 0) return true;

   // kings

   for (Square from : pos.king(atk)) {
      if (king_can_capture(pos, from, def)) return true;
   }

   return false;
}

static Bit contact_captures(const Pos & pos, Side sd) {

   Bit ba = pos.side(sd);
   Bit bd = pos.side(side_opp(sd));
   Bit be = pos.empty();

   uint64 b = 0;

   b |= ((bd << J1) & (be << J2)) | ((bd << I1) & (be << I2));
   b |= ((bd >> I1) & (be >> I2)) | ((bd >> J1) & (be >> J2));

   if (var::Variant == var::Frisian) {
      b |= ((bd << L1) & (be << L2)) | ((bd << K1) & (be << K2));
      b |= ((bd >> K1) & (be >> K2)) | ((bd >> L1) & (be >> L2));
   }

   return ba & b;
}

static bool king_can_capture(const Pos & pos, Square from, Side def) {

   Bit bd = pos.side(def);
   Bit be = pos.empty();

   for (Square sq : bit::king_captures(from) & bd) {
      if (bit::is_incl(bit::capture_mask(from, sq), be)) return true;
   }

   return false;
}

static void add_moves_from(List & list, Bit froms, Inc inc) {
   for (Square from : froms) {
      list.add_move(from, square_make(from + inc));
   }
}

static void add_moves_to(List & list, Bit tos, Inc inc) {
   for (Square to : tos) {
      list.add_move(square_make(to - inc), to);
   }
}

