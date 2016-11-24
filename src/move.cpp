
// move.cpp

// includes

#include <cctype>
#include <string>

#include "bit.h"
#include "libmy.hpp"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "pos.h"
#include "util.h"

// prototypes

static bit_t amb (const List & list, int from, int to);

// functions

move_t move_make(int from, int to, bit_t captured) {

   assert(square_is_ok(from));
   assert(square_is_ok(to));
   assert((captured & 07777) == 0); // TODO: stricter #

   return move_t(uint64(captured) | (uint64(from) << 6) | uint64(to));
}

bool move_is_promotion(move_t mv, const Pos & pos) {

   return move_is_man(mv, pos) && square_is_promotion(move_to(mv), pos.turn());
}

bool move_is_man(move_t mv, const Pos & pos) {

   return bit_test(pos.man(), move_from(mv));
}

bool move_is_conversion(move_t mv, const Pos & pos) {

   return move_is_man(mv, pos) || move_is_capture(mv, pos);
}

bool move_is_legal(move_t mv, const Pos & pos) {

   List list;
   gen_moves(list, pos);
   return list_has(list, mv);
}

std::string move_to_string(move_t mv, const Pos & pos) {

   assert(move_is_legal(mv, pos));

   int   from = move_from(mv);
   int   to   = move_to(mv);
   bit_t caps = move_captured(mv);

   std::string s = "";

   s += square_to_string(from);
   s += (caps != 0) ? "x" : "-";
   s += square_to_string(to);

   if (caps != 0) { // capture => test for ambiguity

      List list;
      gen_captures(list, pos);

      for (bit_t b = caps & ~amb(list, from, to); b != 0; b = bit_rest(b)) {
         int sq = bit_first(b);
         s += "x";
         s += square_to_string(sq);
      }
   }

   return s;
}

static bit_t amb(const List & list, int from, int to) {

   bit_t b = ~0;

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);

      if (move_from(mv) == from && move_to(mv) == to) {
         b &= move_captured(mv);
      }
   }

   assert(~b != 0);
   return b;
}

std::string move_to_hub(move_t mv) {

   int   from = move_from(mv);
   int   to   = move_to(mv);
   bit_t caps = move_captured(mv);

   std::string s = "";

   s += square_to_string(from);
   s += (caps != 0) ? "x" : "-";
   s += square_to_string(to);

   for (bit_t b = caps; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      s += "x";
      s += square_to_string(sq);
   }

   return s;
}

move_t move_from_string(const std::string & s, const Pos & pos) {

   Scanner_Number scan(s);
   std::string token;

   token = scan.get_token();
   int from = square_from_string(token);

   token = scan.get_token();
   if (token != "-" && token != "x") throw Bad_Input();
   bool is_cap = token == "x";

   token = scan.get_token();
   int to = square_from_string(token);

   bit_t caps = 0;

   while (!scan.eos()) {

      token = scan.get_token();
      if (token != "x") throw Bad_Input();

      token = scan.get_token();
      bit_set(caps, square_from_string(token));
   }

   if (!is_cap && caps == 0) { // quiet move
      return move_make(from, to);
   }

   // capture => check legality

   List list;
   gen_captures(list, pos);

   move_t move = Move_None;
   int size = 0;

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);

      if (move_from(mv) == from && move_to(mv) == to && bit_incl(caps, move_captured(mv))) {
         move = mv;
         size++;
      }
   }

   if (size > 1) return Move_None; // ambiguous move

   return move;
}

move_t move_from_hub(const std::string & s) {

   Scanner_Number scan(s);
   std::string token;

   token = scan.get_token();
   int from = square_from_string(token);

   token = scan.get_token();
   if (token != "-" && token != "x") throw Bad_Input();

   token = scan.get_token();
   int to = square_from_string(token);

   bit_t caps = 0;

   while (!scan.eos()) {

      token = scan.get_token();
      if (token != "x") throw Bad_Input();

      token = scan.get_token();
      bit_set(caps, square_from_string(token));
   }

   return move_make(from, to, caps);
}

// end of move.cpp

