
// fen.cpp

// includes

#include <string>

#include "bit.h"
#include "fen.h"
#include "libmy.hpp"
#include "pos.h"
#include "util.h"

// prototypes

static void add_pieces (bit_t & bm, bit_t & bk, Scanner_Number & scan);
static void add_piece  (bit_t & bm, bit_t & bk, int sq, bool king);

static std::string pos_pieces (const Pos & pos, int sd);

static std::string run_string (int pc, int sq, int len);

// functions

void pos_from_fen(Pos & pos, const std::string & s) {

   // init

   Scanner_Number scan(s);
   std::string token;

   // turn

   int turn;

   token = scan.get_token();

   if (token == "W") {
      turn = White;
   } else if (token == "B") {
      turn = Black;
   } else {
      throw Bad_Input();
   }

   // white pieces

   token = scan.get_token();
   if (token != ":") throw Bad_Input();

   token = scan.get_token();
   if (token != "W") throw Bad_Input();

   bit_t wm, wk;
   add_pieces(wm, wk, scan);

   // black pieces

   token = scan.get_token();
   if (token != ":") throw Bad_Input();

   token = scan.get_token();
   if (token != "B") throw Bad_Input();

   bit_t bm, bk;
   add_pieces(bm, bk, scan);

   // update board

   if (!scan.eos()) throw Bad_Input();

   pos.from_bit(turn, wm, bm, wk, bk);
}

static void add_pieces(bit_t & bm, bit_t & bk, Scanner_Number & scan) {

   bm = 0;
   bk = 0;

   std::string token;

   while (true) {

      bool king = false;

      token = scan.get_token();

      if (token == ",") {
         token = scan.get_token();
      }

      if (token == "") {
         return;
      } else if (token == ":") {
         scan.unget_char(); // HACK: no unget_token
         return;
      } else if (token == "K") {
         king = true;
         token = scan.get_token();
      }

      if (!string_is_square(token)) throw Bad_Input();
      int from = ml::stoi(token);

      token = scan.get_token();

      if (token != "-") {

         if (token.size() == 1) scan.unget_char(); // HACK: no unget_token

         add_piece(bm, bk, from, king);

      } else {

         token = scan.get_token();

         if (!string_is_square(token)) throw Bad_Input();
         int to = ml::stoi(token);
         if (to < from) throw Bad_Input();

         for (int sq = from; sq <= to; sq++) {
            add_piece(bm, bk, sq, king);
         }
      }
   }
}

static void add_piece(bit_t & bm, bit_t & bk, int sq, bool king) {

   assert(sq >= 1 && sq <= 50);

   sq = square_from_50(sq - 1);

   if (king) {
      bit_set(bk, sq);
   } else {
      bit_set(bm, sq);
   }
}

void pos_from_hub(Pos & pos, const std::string & s) {

   if (s.size() != 51) throw Bad_Input();

   // turn

   int turn;

   switch (s[0]) {
   case 'w' : turn = White;      break;
   case 'b' : turn = Black;      break;
   default  : throw Bad_Input(); break;
   }

   // squares

   bit_t wm = 0, bm = 0, wk = 0, bk = 0;

   for (int i = 0; i < 50; i++) {

      int sq = square_from_50(i);

      switch (s[i + 1]) {
      case 'w' : bit_set(wm, sq);   break;
      case 'b' : bit_set(bm, sq);   break;
      case 'W' : bit_set(wk, sq);   break;
      case 'B' : bit_set(bk, sq);   break;
      case 'e' : /* no-op */        break;
      default  : throw Bad_Input(); break;
      }
   }

   // update board

   pos.from_bit(turn, wm, bm, wk, bk);
}

void pos_from_dxp(Pos & pos, const std::string & s) {

   if (s.size() != 51) throw Bad_Input();

   // turn

   int turn;

   switch (s[0]) {
   case 'W' : turn = White;      break;
   case 'Z' : turn = Black;      break;
   default  : throw Bad_Input(); break;
   }

   // squares

   bit_t wm = 0, bm = 0, wk = 0, bk = 0;

   for (int i = 0; i < 50; i++) {

      int sq = square_from_50(i);

      switch (s[i + 1]) {
      case 'w' : bit_set(wm, sq);   break;
      case 'z' : bit_set(bm, sq);   break;
      case 'W' : bit_set(wk, sq);   break;
      case 'Z' : bit_set(bk, sq);   break;
      case 'e' : /* no-op */        break;
      default  : throw Bad_Input(); break;
      }
   }

   // update board

   pos.from_bit(turn, wm, bm, wk, bk);
}

std::string pos_fen(const Pos & pos) {

   std::string s;

   s += (pos.turn() == White) ? "W" : "B";
   s += ":W" + pos_pieces(pos, White);
   s += ":B" + pos_pieces(pos, Black);

   return s;
}

static std::string pos_pieces(const Pos & pos, int sd) {

   assert(side_is_ok(sd));

   std::string s;

   bool first = true;

   int run_pc = Frame;
   int run_sq = 0;
   int run_len = 0;

   for (int i = 0; i < 50; i++) {

      int sq = square_from_50(i);
      int pc = pos_square(pos, sq);

      if (pc == run_pc) {

         run_len++;

      } else {

         if (run_len != 0) {
            if (!first) s += ",";
            s += run_string(run_pc, run_sq, run_len);
            first = false;
         }

         if (pc != Empty && piece_is_side(pc, sd)) {
            run_pc = pc;
            run_sq = i;
            run_len = 1;
         } else {
            run_pc = Frame;
            run_sq = 0;
            run_len = 0;
         }
      }
   }

   if (run_len != 0) {
      if (!first) s += ",";
      s += run_string(run_pc, run_sq, run_len);
      first = false;
   }

   return s;
}

static std::string run_string(int pc, int sq, int len) {

   assert(piece_is_ok(pc));
   assert(len != 0);
   assert(sq + len <= 50);

   std::string s;

   if (piece_is_king(pc)) {
      s += "K";
   }

   s += ml::itos(sq + 1);

   if (len == 2) {
      s += ",";
      s += ml::itos(sq + 2);
   } else if (len >= 3) {
      s += "-";
      s += ml::itos(sq + len);
   }

   return s;
}

std::string pos_hub(const Pos & pos) {

   std::string s;

   s += (pos.turn() == White) ? "w" : "b";

   for (int i = 0; i < 50; i++) {

      int sq = square_from_50(i);

      switch (pos_square(pos, sq)) {
      case WM :    s += "w";      break;
      case BM :    s += "b";      break;
      case WK :    s += "W";      break;
      case BK :    s += "B";      break;
      case Empty : s += "e";      break;
      default :    assert(false); break;
      }
   }

   return s;
}

std::string pos_dxp(const Pos & pos) {

   std::string s;

   s += (pos.turn() == White) ? "W" : "Z";

   for (int i = 0; i < 50; i++) {

      int sq = square_from_50(i);

      switch (pos_square(pos, sq)) {
      case WM :    s += "w";      break;
      case BM :    s += "z";      break;
      case WK :    s += "W";      break;
      case BK :    s += "Z";      break;
      case Empty : s += "e";      break;
      default :    assert(false); break;
      }
   }

   return s;
}

// end of fen.cpp

