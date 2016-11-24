
// eval.cpp

// includes

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bit.h"
#include "board.h"
#include "eval.h"
#include "hash.h"
#include "libmy.hpp"
#include "pos.h"
#include "score.h"

// constants

static const int P = 91910; // number of features
static const int Unit = 60; // units per cp

// types

class Score_2 {

private :

   int p_mg;
   int p_eg;

public :

   Score_2 ();

   void add (int var, int val);

   int mg () const { return p_mg; }
   int eg () const { return p_eg; }
};

// variables

static int Weight[P * 2];

// prototypes

static void get_feature (Score_2 & s2, const Board & bd);

static void pst      (Score_2 & s2, int var, bit_t wb, bit_t bb);
static void king_mob (Score_2 & s2, int var, const Pos & pos);
static void pattern  (Score_2 & s2, int var, const Board & bd);

static int balance (const Board & bd);

static bit_t king_attack (int from, const Pos & pos);

static int i8 (const Board & bd, int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7);
static int i9 (const Board & bd, int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7, int s8);

static int excess (int n, int max);

// functions

void eval_init() {

   std::cout << "init eval" << std::endl;

   std::ifstream file("data/eval", std::ios::binary);

   if (!file) {
      std::cerr << "unable to open file \"data/eval\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   for (int i = 0; i < P * 2; i++) {
      Weight[i] = int16(ml::get_bytes(file, 2)); // HACK: extend sign
   }
}

int eval(const Board & bd) {

   // features

   Score_2 s2;
   get_feature(s2, bd);

   // phase interpolation

   int pip = bd.pip();
   assert(pip >= 0 && pip <= 300);

   int sc = ml::div_round(s2.mg() * pip + s2.eg() * (300 - pip), Unit * 300);

   // drawish material

   int wm = bd.size(WM);
   int bm = bd.size(BM);
   int wk = bd.size(WK);
   int bk = bd.size(BK);

   int wp = wm + wk;
   int bp = bm + bk;

   if (sc > 0 && bk != 0) { // white ahead

      if (wp <= 3) {
         sc /= 8;
      } else if (wk == bk && std::abs(wm - bm) <= 1) {
         sc /= 2;
      }

   } else if (sc < 0 && wk != 0) { // black ahead

      if (bp <= 3) {
         sc /= 8;
      } else if (wk == bk && std::abs(wm - bm) <= 1) {
         sc /= 2;
      }
   }

   if (bd.turn() != White) sc = -sc;

   return score::clamp(sc);
}

static void get_feature(Score_2 & s2, const Board & bd) {

   // init

   Pos pos(bd);

   int var = 0;

   // material

   int wm = bd.size(WM);
   int bm = bd.size(BM);
   int wk = bd.size(WK);
   int bk = bd.size(BK);

   s2.add(var + 0, wm - bm);
   s2.add(var + 1, (wk >= 1) - (bk >= 1));
   s2.add(var + 2, excess(wk, 1) - excess(bk, 1));
   var += 3;

   // king position

   pst(s2, var, pos.wk(), pos.bk());
   var += 50;

   // king mobility

   king_mob(s2, var, pos);
   var += 2;

   // left/right balance

   s2.add(var, balance(bd));
   var++;

   // patterns

   pattern(s2, var, bd);
}

static void pst(Score_2 & s2, int var, bit_t wb, bit_t bb) {

   for (bit_t b = wb; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      s2.add(var + square_to_50(sq), +1);
   }

   for (bit_t b = bb; b != 0; b = bit_rest(b)) {
      int sq = square_opp(bit_first(b));
      s2.add(var + square_to_50(sq), -1);
   }
}

static void king_mob(Score_2 & s2, int var, const Pos & pos) {

   int ns = 0;
   int nd = 0;

   bit_t wm = pos.wm();
   bit_t bm = pos.bm();

   bit_t e = Bit_Squares ^ wm ^ bm;

   // white

   {
      bit_t ee = e & ~(((bm << 6) & (e >> 6)) | ((bm << 5) & (e >> 5))
                     | ((bm >> 5) & (e << 5)) | ((bm >> 6) & (e << 6)));

      for (bit_t b = pos.wk(); b != 0; b = bit_rest(b)) {

         int from = bit_first(b);

         bit_t atk  = king_attack(from, pos) & e;
         bit_t safe = atk & ee;
         bit_t deny = atk & ~safe;

         ns += bit_count(safe);
         nd += bit_count(deny);
      }
   }

   // black

   {
      bit_t ee = e & ~(((wm << 6) & (e >> 6)) | ((wm << 5) & (e >> 5))
                     | ((wm >> 5) & (e << 5)) | ((wm >> 6) & (e << 6)));

      for (bit_t b = pos.bk(); b != 0; b = bit_rest(b)) {

         int from = bit_first(b);

         bit_t atk  = king_attack(from, pos) & e;
         bit_t safe = atk & ee;
         bit_t deny = atk & ~safe;

         ns -= bit_count(safe);
         nd -= bit_count(deny);
      }
   }

   s2.add(var + 0, ns);
   s2.add(var + 1, nd);
}

static void pattern(Score_2 & s2, int var, const Board & bd) {

   s2.add(var +  3280 + i8(bd,  6,  7, 11, 12, 17, 18, 22, 23), +1);
   s2.add(var +  3280 - i8(bd, 59, 58, 54, 53, 48, 47, 43, 42), -1);
   s2.add(var +  9841 + i8(bd,  7,  8, 12, 13, 18, 19, 23, 24), +1);
   s2.add(var +  9841 - i8(bd, 58, 57, 53, 52, 47, 46, 42, 41), -1);
   s2.add(var + 16402 + i8(bd,  8,  9, 13, 14, 19, 20, 24, 25), +1);
   s2.add(var + 16402 - i8(bd, 57, 56, 52, 51, 46, 45, 41, 40), -1);
   s2.add(var + 22963 + i8(bd,  9, 10, 14, 15, 20, 21, 25, 26), +1);
   s2.add(var + 22963 - i8(bd, 56, 55, 51, 50, 45, 44, 40, 39), -1);
   s2.add(var + 29524 + i8(bd, 17, 18, 22, 23, 28, 29, 33, 34), +1);
   s2.add(var + 29524 - i8(bd, 48, 47, 43, 42, 37, 36, 32, 31), -1);
   s2.add(var + 36085 + i8(bd, 18, 19, 23, 24, 29, 30, 34, 35), +1);
   s2.add(var + 36085 - i8(bd, 47, 46, 42, 41, 36, 35, 31, 30), -1);
   s2.add(var + 42646 + i8(bd, 19, 20, 24, 25, 30, 31, 35, 36), +1);
   s2.add(var + 42646 - i8(bd, 46, 45, 41, 40, 35, 34, 30, 29), -1);
   s2.add(var + 49207 + i8(bd, 20, 21, 25, 26, 31, 32, 36, 37), +1);
   s2.add(var + 49207 - i8(bd, 45, 44, 40, 39, 34, 33, 29, 28), -1);

   s2.add(var + 62329 + i9(bd,  6, 11, 12, 17, 22, 23, 28, 33, 34), +1);
   s2.add(var + 62329 - i9(bd, 59, 54, 53, 48, 43, 42, 37, 32, 31), -1);
   s2.add(var + 82012 + i9(bd,  9, 10, 15, 20, 21, 26, 31, 32, 37), +1);
   s2.add(var + 82012 - i9(bd, 56, 55, 50, 45, 44, 39, 34, 33, 28), -1);
}

static int balance(const Board & bd) {

   return std::abs(bd.skew(Black)) - std::abs(bd.skew(White)); // opposit sides: imbalance is a penalty
}

static bit_t king_attack(int from, const Pos & pos) {

   assert(square_is_ok(from));
   return king_attack(from, pos.all());
}

static int i8(const Board & bd, int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7) {

   return ((((((((bd.trit(s0)) * 3 +
                  bd.trit(s1)) * 3 +
                  bd.trit(s2)) * 3 +
                  bd.trit(s3)) * 3 +
                  bd.trit(s4)) * 3 +
                  bd.trit(s5)) * 3 +
                  bd.trit(s6)) * 3 +
                  bd.trit(s7));
}

static int i9(const Board & bd, int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7, int s8) {

   return (((((((((bd.trit(s0)) * 3 +
                   bd.trit(s1)) * 3 +
                   bd.trit(s2)) * 3 +
                   bd.trit(s3)) * 3 +
                   bd.trit(s4)) * 3 +
                   bd.trit(s5)) * 3 +
                   bd.trit(s6)) * 3 +
                   bd.trit(s7)) * 3 +
                   bd.trit(s8));
}

static int excess(int n, int max) {

   assert(n >= 0);
   assert(max > 0);

   return std::max(n - max, 0);
}

Score_2::Score_2() {

   p_mg = 0;
   p_eg = 0;
}

void Score_2::add(int var, int val) {

   p_mg += Weight[var + 0] * val;
   p_eg += Weight[var + P] * val;
}

// end of eval.cpp

