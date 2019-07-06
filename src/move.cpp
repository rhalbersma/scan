
// includes

#include <string>

#include "bit.hpp"
#include "common.hpp"
#include "gen.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "util.hpp"

namespace move {

// prototypes

static bool is_man (Move mv, const Pos & pos);

static Bit amb (const List & list, Square from, Square to, const Pos & pos);

// functions

Move make(Square from, Square to, Bit captured) {
   assert(!bit::has(captured, from));
   assert(!bit::has(captured, to));
   return Move(uint64(bit::bit(from) | bit::bit(to) | captured));
}

Square from(Move mv, const Pos & pos) {
   Bit froms = pos.side(pos.turn()) & uint64(mv);
   return bit::first(froms);
}

Square to(Move mv, const Pos & pos) {
   Bit tos = pos.empty() & uint64(mv);
   if (tos == 0) tos = pos.side(pos.turn()) & uint64(mv); // to = from
   return bit::first(tos);
}

Bit captured(Move mv, const Pos & pos) {
   return pos.side(side_opp(pos.turn())) & uint64(mv);
}

Move_Index index(Move mv, const Pos & pos) { // mv can be None

   if (mv == None) {
      return Move_Index_None;
   } else {
      return Move_Index((from(mv, pos) << 6) | (to(mv, pos) << 0));
   }
}

bool is_capture(Move mv, const Pos & pos) {
   return captured(mv, pos) != 0;
}

bool is_promotion(Move mv, const Pos & pos) {
   return is_man(mv, pos) && square_is_promotion(move::to(mv, pos), pos.turn());
}

bool is_conversion(Move mv, const Pos & pos) {
   return is_man(mv, pos) || is_capture(mv, pos);
}

bool is_forcing(Move mv, const Pos & pos) {
   return !is_capture(mv, pos) && pos::is_capture(pos.succ(mv));
}

static bool is_man(Move mv, const Pos & pos) {
   return pos.is_piece(move::from(mv, pos), Man);
}

bool is_legal(Move mv, const Pos & pos) {
   List list;
   gen_moves(list, pos);
   return list::has(list, mv);
}

std::string to_string(Move mv, const Pos & pos) {

   Square from = move::from(mv, pos);
   Square to   = move::to(mv, pos);
   Bit    caps = captured(mv, pos);

   std::string s;

   s += square_to_string(from);
   s += (caps != 0) ? "x" : "-";
   s += square_to_string(to);

   if (caps != 0) { // capture => test for ambiguity

      List list;
      gen_captures(list, pos);

      for (Square sq : caps & ~amb(list, from, to, pos)) {
         s += "x";
         s += square_to_string(sq);
      }
   }

   return s;
}

static Bit amb(const List & list, Square from, Square to, const Pos & pos) {

   Bit b = bit::Squares;

   for (Move mv : list) {
      if (move::from(mv, pos) == from && move::to(mv, pos) == to) {
         b &= captured(mv, pos);
      }
   }

   assert(b != bit::Squares);
   return b;
}

std::string to_hub(Move mv, const Pos & pos) {

   Square from = move::from(mv, pos);
   Square to   = move::to(mv, pos);
   Bit    caps = captured(mv, pos);

   std::string s;

   s += square_to_string(from);
   s += (caps != 0) ? "x" : "-";
   s += square_to_string(to);

   for (Square sq : caps) {
      s += "x";
      s += square_to_string(sq);
   }

   return s;
}

Move from_string(const std::string & s, const Pos & pos) {

   Scanner_Number scan(s);
   std::string token;

   token = scan.get_token();
   Square from = square_from_string(token);

   token = scan.get_token();
   if (token != "-" && token != "x") throw Bad_Input();
   bool is_cap = token == "x";

   token = scan.get_token();
   Square to = square_from_string(token);

   Bit caps {};

   while (!scan.eos()) {

      token = scan.get_token();
      if (token != "x") throw Bad_Input();

      token = scan.get_token();
      bit::set(caps, square_from_string(token));
   }

   if (!is_cap && caps == 0) return make(from, to); // quiet move

   // capture => check legality

   List list;
   gen_captures(list, pos);

   Move move = None;
   int size = 0;

   for (Move mv : list) {
      if (move::from(mv, pos) == from && move::to(mv, pos) == to && bit::is_incl(caps, captured(mv, pos))) {
         move = mv;
         size += 1;
      }
   }

   if (size > 1) return None; // ambiguous move

   return move;
}

Move from_hub(const std::string & s, const Pos & /* pos */) {

   Scanner_Number scan(s);
   std::string token;

   token = scan.get_token();
   Square from = square_from_string(token);

   token = scan.get_token();
   if (token != "-" && token != "x") throw Bad_Input();

   token = scan.get_token();
   Square to = square_from_string(token);

   Bit caps {};

   while (!scan.eos()) {

      token = scan.get_token();
      if (token != "x") throw Bad_Input();

      token = scan.get_token();
      bit::set(caps, square_from_string(token));
   }

   return make(from, to, caps);
}

} // namespace move

