
// move_gen.cpp

// includes

#include <cmath>

#include "bit.h"
#include "libmy.hpp"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "pos.h"

// prototypes

static void add_man_moves         (List & list, const Pos & pos, bit_t froms);
static void add_man_captures      (List & list, const Pos & pos, bit_t froms);
static void add_man_captures      (List & list, const Pos & pos, bit_t bo, bit_t be, int from, int inc);
static void add_man_captures_rec  (List & list, const Pos & pos, int start, bit_t captures, bit_t bo, bit_t be, int from);

static void add_king_moves        (List & list, const Pos & pos, int from);
static void add_king_captures     (List & list, const Pos & pos, int from, int opp);
static void add_king_captures_rec (List & list, const Pos & pos, int start, bit_t captures, bit_t bo, bit_t be, int captured, int inc, int opp);

static bit_t contact_captures (const Pos & pos, int sd);

static bool king_can_capture (const Pos & pos, int from, int opp);

static void add_moves_from (List & list, bit_t froms, int inc);
static void add_moves_to   (List & list, bit_t tos, int inc);

static void add_move    (List & list, int from, int to);
static void add_capture (List & list, int from, int to, bit_t captures);

// functions

void gen_moves(List & list, const Pos & pos) {

   gen_captures(list, pos);

   if (list.size() == 0) {
      gen_quiets(list, pos);
   }
}

void gen_captures(List & list, const Pos & pos) {

   list.clear();

   int me  = pos.turn();
   int opp = side_opp(me);

   // men

   add_man_captures(list, pos, pos.man(me));

   // kings

   for (bit_t b = pos.king(me); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_king_captures(list, pos, from, opp);
   }
}

void gen_promotions(List & list, const Pos & pos) {

   assert(!pos_is_capture(pos));

   list.clear();

   int me = pos.turn();
   bit_t mp = pos.man(me);

   if (me == White) {
      add_man_moves(list, pos, mp & bit_rank(1));
   } else {
      add_man_moves(list, pos, mp & bit_rank(8));
   }
}

void gen_quiets(List & list, const Pos & pos) {

   assert(!pos_is_capture(pos));

   list.clear();

   int me = pos.turn();

   // men

   add_man_moves(list, pos, pos.man(me));

   // kings

   for (bit_t b = pos.king(me); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_king_moves(list, pos, from);
   }
}

static void add_man_moves(List & list, const Pos & pos, bit_t froms) {

   int me = pos.turn();
   bit_t e = pos.empty();

   if (me == White) {
      add_moves_from(list, froms & (e << 5), -5);
      add_moves_from(list, froms & (e << 6), -6);
   } else {
      add_moves_from(list, froms & (e >> 5), +5);
      add_moves_from(list, froms & (e >> 6), +6);
   }
}

static void add_man_captures(List & list, const Pos & pos, bit_t froms) {

   int me  = pos.turn();
   int opp = side_opp(me);

   // bit_t mp = pos.man(me);
   bit_t op = pos.piece(opp);
   bit_t e = pos.empty();

   for (bit_t b = froms & (op << 6) & (e << 12); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_man_captures(list, pos, op, e, from, -6);
   }

   for (bit_t b = froms & (op << 5) & (e << 10); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_man_captures(list, pos, op, e, from, -5);
   }

   for (bit_t b = froms & (op >> 5) & (e >> 10); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_man_captures(list, pos, op, e, from, +5);
   }

   for (bit_t b = froms & (op >> 6) & (e >> 12); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_man_captures(list, pos, op, e, from, +6);
   }
}

static void add_man_captures(List & list, const Pos & pos, bit_t bo, bit_t be, int from, int inc) {

   assert(square_is_ok(from));
   assert(inc != 0);

   int sq = from + inc;

   add_man_captures_rec(list, pos, from, bit(sq), bit_remove(bo, sq), bit_add(be, from), sq + inc);
}

static void add_man_captures_rec(List & list, const Pos & pos, int start, bit_t captures, bit_t bo, bit_t be, int from) {

   assert(square_is_ok(from));
   assert(bit_test(be, from));

   for (int dir = 0; dir < Dir_Size; dir++) {

      int inc = Inc[dir];
      int sq = from + inc;

      if (bit_test(bo, sq) && bit_test(be, sq + inc)) { // TODO: robust test ###
         add_man_captures_rec(list, pos, start, bit_add(captures, sq), bit_remove(bo, sq), be, sq + inc);
      }
   }

   add_capture(list, start, from, captures);
}

static void add_king_moves(List & list, const Pos & pos, int from) {

   assert(square_is_ok(from));

   bit_t blockers = pos.all();
   bit_t moves = king_attack(from, blockers) & ~blockers;

   for (bit_t b = moves; b != 0; b = bit_rest(b)) {
      int to = bit_first(b);
      add_move(list, from, to);
   }
}

static void add_king_captures(List & list, const Pos & pos, int from, int opp) {

   assert(square_is_ok(from));
   assert(side_is_ok(opp));

   bit_t blockers = pos.all();

   bit_t bo = pos.piece(opp);
   bit_t be = bit_add(pos.empty(), from);

   bit_t targets = king_attack_almost(from) & bo; // would be captures on an empty board

   for (bit_t b = targets; b != 0; b = bit_rest(b)) {

      int sq = bit_first(b);

      if ((bit_between(from, sq) & blockers) == 0) {

         int inc = line_inc(from, sq);
         assert(inc != 0);

         assert(square_is_ok(sq + inc));

         if (bit_test(be, sq + inc)) {
            add_king_captures_rec(list, pos, from, 0, bo, be, sq, inc, opp);
         }
      }
   }
}

static void add_king_captures_rec(List & list, const Pos & pos, int start, bit_t captures, bit_t bo, bit_t be, int captured, int inc, int opp) {

   assert(square_is_ok(captured));
   assert(inc != 0);
   assert(side_is_ok(opp));

   assert(bit_test(be, captured + inc));

   captures = bit_add(captures, captured);
   bo = bit_remove(bo, captured);

   for (int from = captured + inc; bit_test(be, from); from += inc) { // TODO: robust test ###

      bit_t targets = king_attack_almost(from) & bo; // would be captures on an empty board

      for (bit_t b = targets; b != 0; b = bit_rest(b)) {

         int sq = bit_first(b);

         if ((bit_between(from, sq) & ~be) == 0) {

            int new_inc = line_inc(from, sq);
            assert(new_inc != 0);

            assert(new_inc != -inc);
            if (new_inc == +inc && from != captured + inc) {
               continue;
            }

            if (bit_test(be, sq + new_inc)) {
               add_king_captures_rec(list, pos, start, captures, bo, be, sq, new_inc, opp);
            }
         }
      }

      add_capture(list, start, from, captures);
   }
}

void add_exchanges(List & list, const Pos & pos) {

   bit_t wm = pos.wm();
   bit_t bm = pos.bm();

   bit_t e = Bit_Squares ^ wm ^ bm;

   if (pos.turn() == White) {

      bit_t mp = wm;
      bit_t op = bm;

      bit_t strong = ((bit_file(1) | bit_rank(8) | (mp >> 10)) & (mp >> 5) & (e << 5))
                   | ((bit_file(8) | bit_rank(8) | (mp >> 12)) & (mp >> 6) & (e << 6));

      bit_t weak = ((mp << 6) & (e << 12)) | ((mp << 5) & (e << 10))
                 | ((mp >> 5) & (e >> 10)) | ((mp >> 6) & (e >> 12));

      bit_t target = strong & ~weak;

      bit_t pin = ((mp << 6) & (op << 12)) | ((mp << 5) & (op << 10))
                | ((mp >> 5) & (op >> 10)) | ((mp >> 6) & (op >> 12));

      bit_t w5 = 0;
      w5 |= ((op << 5) & (target >> 5)) & ~((op << 6) & (e >> 6)) & ~((op >> 6) & (e << 6));
      w5 |= ((op << 6) & ((e & target) >> 6)) & ~(op << 5) & ~((op >> 6) & (e << 6));
      w5 |= ((op >> 6) & ((e & target) << 6)) & ~(op << 5) & ~((op << 6) & (e >> 6));
      w5 &= ((mp & ~pin) >> 5) & e;

      bit_t w6 = 0;
      w6 |= ((op << 6) & (target >> 6)) & ~((op << 5) & (e >> 5)) & ~((op >> 5) & (e << 5));
      w6 |= ((op << 5) & ((e & target) >> 5)) & ~(op << 6) & ~((op >> 5) & (e << 5));
      w6 |= ((op >> 5) & ((e & target) << 5)) & ~(op << 6) & ~((op << 5) & (e >> 5));
      w6 &= ((mp & ~pin) >> 6) & e;

      add_moves_to(list, w5, -5);
      add_moves_to(list, w6, -6);

   } else { // Black

      bit_t mp = bm;
      bit_t op = wm;

      bit_t strong = ((bit_file(1) | bit_rank(1) | (mp << 12)) & (mp << 6) & (e >> 6))
                   | ((bit_file(8) | bit_rank(1) | (mp << 10)) & (mp << 5) & (e >> 5));

      bit_t weak = ((mp << 6) & (e << 12)) | ((mp << 5) & (e << 10))
                 | ((mp >> 5) & (e >> 10)) | ((mp >> 6) & (e >> 12));

      bit_t target = strong & ~weak;

      bit_t pin = ((mp << 6) & (op << 12)) | ((mp << 5) & (op << 10))
                | ((mp >> 5) & (op >> 10)) | ((mp >> 6) & (op >> 12));

      bit_t b5 = 0;
      b5 |= ((op >> 5) & (target << 5)) & ~((op >> 6) & (e << 6)) & ~((op << 6) & (e >> 6));
      b5 |= ((op >> 6) & ((e & target) << 6)) & ~(op >> 5) & ~((op << 6) & (e >> 6));
      b5 |= ((op << 6) & ((e & target) >> 6)) & ~(op >> 5) & ~((op >> 6) & (e << 6));
      b5 &= ((mp & ~pin) << 5) & e;

      bit_t b6 = 0;
      b6 |= ((op >> 6) & (target << 6)) & ~((op >> 5) & (e << 5)) & ~((op << 5) & (e >> 5));
      b6 |= ((op >> 5) & ((e & target) << 5)) & ~(op >> 6) & ~((op << 5) & (e >> 5));
      b6 |= ((op << 5) & ((e & target) >> 5)) & ~(op >> 6) & ~((op >> 5) & (e << 5));
      b6 &= ((mp & ~pin) << 6) & e;

      add_moves_to(list, b5, +5);
      add_moves_to(list, b6, +6);
   }
}

bool can_move(const Pos & pos, int sd) {

   assert(side_is_ok(sd));

   bit_t e = pos.empty();

   // man moves

   bit_t mp = pos.man(sd);

   if (sd == White) {
      mp &= (e << 5) | (e << 6);
   } else {
      mp &= (e >> 5) | (e >> 6);
   }

   if (mp != 0) return true;

   // contact captures

   if (contact_captures(pos, sd) != 0) return true;

   // king moves

   for (bit_t b = pos.king(sd); b != 0; b = bit_rest(b)) {

      int from = bit_first(b);

      bit_t moves = king_attack_one(from) & e;
      if (moves != 0) return true;
   }

   return false;
}

bool can_capture(const Pos & pos, int sd) {

   assert(side_is_ok(sd));

   int me  = sd;
   int opp = side_opp(me);

   // men

   if (contact_captures(pos, me) != 0) return true;

   // kings

   for (bit_t b = pos.king(me); b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      if (king_can_capture(pos, from, opp)) return true;
   }

   return false;
}

static bit_t contact_captures(const Pos & pos, int sd) {

   assert(side_is_ok(sd));

   bit_t mp = pos.piece(sd);
   bit_t op = pos.piece(side_opp(sd));

   bit_t e = Bit_Squares ^ mp ^ op;

   return mp & (((op << 6) & (e << 12)) | ((op << 5) & (e << 10))
              | ((op >> 5) & (e >> 10)) | ((op >> 6) & (e >> 12)));
}

static bool king_can_capture(const Pos & pos, int from, int opp) {

   assert(square_is_ok(from));
   assert(side_is_ok(opp));

   bit_t blockers = pos.all();

   bit_t op = pos.piece(opp);
   bit_t e = pos.empty();

   bit_t targets = king_attack_almost(from) & op; // would be captures on an empty board

   for (bit_t b = targets; b != 0; b = bit_rest(b)) {

      int sq = bit_first(b);

      if ((bit_between(from, sq) & blockers) == 0
       && bit_test(e, sq + line_inc(from, sq))) {
         return true;
      }
   }

   return false;
}

static void add_moves_from(List & list, bit_t froms, int inc) {

   for (bit_t b = froms; b != 0; b = bit_rest(b)) {
      int from = bit_first(b);
      add_move(list, from, from + inc);
   }
}

static void add_moves_to(List & list, bit_t tos, int inc) {

   for (bit_t b = tos; b != 0; b = bit_rest(b)) {
      int to = bit_first(b);
      add_move(list, to - inc, to);
   }
}

static void add_move(List & list, int from, int to) {

   move_t move = move_make(from, to);
   list.add_move(move);
}

static void add_capture(List & list, int from, int to, bit_t captures) {

   move_t move = move_make(from, to, captures);
   list.add_capture(move);
}

// end of move_gen.cpp

