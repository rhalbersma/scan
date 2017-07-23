
// includes

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bit.hpp"
#include "common.hpp"
#include "eval.hpp"
#include "libmy.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "var.hpp"

// constants

const int P { 2125820 };

const int Unit { 10 }; // units per cp

const int Pat_Squares { 12 };

// variables

static std::vector<int> G_Weight;

// types

class Score_2 {

private :

   int p_mg;
   int p_eg;

public :

   Score_2() {
      p_mg = 0;
      p_eg = 0;
   }

   void add(int var, int val) {
      p_mg += G_Weight[var + 0] * val;
      p_eg += G_Weight[var + P] * val;
   }

   int mg () const { return p_mg; }
   int eg () const { return p_eg; }
};

// "constants"

const int Perm    [Pat_Squares] {  0,  1,  4,  5,  8,  9,  2,  3,  6,  7, 10, 11 };
const int Perm_Rev[Pat_Squares] { 11, 10,  7,  6,  3,  2,  9,  8,  5,  4,  1,  0 };

// compile-time functions

constexpr int pow(int a, int b) {
   return (b == 0) ? 1 : pow(a, b - 1) * a;
}

// variables

static int Index_3    [pow(2, Pat_Squares)];
static int Index_3_Rev[pow(2, Pat_Squares)];

// prototypes

static int  conv (int index, int size, int bf, int bt, const int perm[]);

static void pst      (Score_2 & s2, int var, Bit wb, Bit bb);
static void king_mob (Score_2 & s2, int var, const Pos & pos);
static void pattern  (Score_2 & s2, int var, const Pos & pos);

static void indices_column (uint64 white, uint64 black, int & index_top, int & index_bottom);
static void indices_column (uint64 b, int & i0, int & i2);

// functions

void eval_init(const std::string & file_name) {

   std::cout << "init eval" << std::endl;

   // load weights

   std::ifstream file(file_name.c_str(), std::ios::binary);

   if (!file) {
      std::cerr << "unable to open file \"" << file_name << "\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   G_Weight.resize(P * 2);

   for (int i = 0; i < P * 2; i++) {
      G_Weight[i] = int16(ml::get_bytes(file, 2)); // HACK: extend sign
   }

   // init base conversion (2 -> 3)

   int size = Pat_Squares;
   int bf = 2;
   int bt = 3;

   for (int i = 0; i < pow(bf, size); i++) {
      Index_3    [i] = conv(i, size, bf, bt, Perm_Rev);
      Index_3_Rev[i] = conv(i, size, bf, bt, Perm);
   }
}

static int conv(int index, int size, int bf, int bt, const int perm[]) {

   assert(index >= 0 && index < pow(bf, size));

   int from = index;
   int to = 0;

   for (int i = 0; i < size; i++) {

      int digit = from % bf;
      from /= bf;

      int j = perm[i];
      assert(j >= 0 && j < size);

      assert(digit >= 0 && digit < bt);
      to += digit * pow(bt, j);
   }

   assert(from == 0);

   assert(to >= 0 && to < pow(bt, size));
   return to;
}

Score eval(const Pos & pos, Side sd) {

   // features

   Score_2 s2;
   int var = 0;

   // material

   int nwm = bit::count(pos.wm());
   int nbm = bit::count(pos.bm());
   int nwk = bit::count(pos.wk());
   int nbk = bit::count(pos.bk());

   s2.add(var + 0, nwm - nbm);
   s2.add(var + 1, (nwk >= 1) - (nbk >= 1));
   s2.add(var + 2, std::max(nwk - 1, 0) - std::max(nbk - 1, 0));
   var += 3;

   // king position

   pst(s2, var, pos.wk(), pos.bk());
   var += Dense_Size;

   // king mobility

   king_mob(s2, var, pos);
   var += 2;

   // left/right balance

   s2.add(var, std::abs(pos::skew(pos, Black)) - std::abs(pos::skew(pos, White)));
   var += 1;

   // patterns

   pattern(s2, var, pos);
   var += pow(3, 12) * 4;

   // game phase

   int stage = pos::stage(pos);
   assert(stage >= 0 && stage <= Stage_Size);

   int sc = ml::div_round(s2.mg() * (Stage_Size - stage) + s2.eg() * stage, Unit * Stage_Size);

   // drawish material

   if (var::Variant == var::Normal) {

      if (sc > 0 && nbk != 0) { // white ahead

         if (nwm + nwk <= 3) {
            sc /= 8;
         } else if (nwk == nbk && std::abs(nwm - nbm) <= 1) {
            sc /= 2;
         }

      } else if (sc < 0 && nwk != 0) { // black ahead

         if (nbm + nbk <= 3) {
            sc /= 8;
         } else if (nwk == nbk && std::abs(nwm - nbm) <= 1) {
            sc /= 2;
         }
      }
   }

   return score::clamp(Score(score::side(sc, sd))); // for sd
}

static void pst(Score_2 & s2, int var, Bit wb, Bit bb) {

   for (Bit b = wb; b != 0; b = bit::rest(b)) {
      Square sq = bit::first(b);
      s2.add(var + square_dense(sq), +1);
   }

   for (Bit b = bb; b != 0; b = bit::rest(b)) {
      Square sq = square_opp(bit::first(b));
      s2.add(var + square_dense(sq), -1);
   }
}

static void king_mob(Score_2 & s2, int var, const Pos & pos) {

   int ns = 0;
   int nd = 0;

   Bit wm = pos.wm();
   Bit bm = pos.bm();

   Bit e = bit::Squares ^ wm ^ bm;

   // white

   {
      Bit ee = e & ~(((bm << J1) & (e >> J1)) | ((bm << I1) & (e >> I1))
                   | ((bm >> I1) & (e << I1)) | ((bm >> J1) & (e << J1)));

      for (Bit b = pos.wk(); b != 0; b = bit::rest(b)) {

         Square from = bit::first(b);

         Bit atk  = bit::king_attack(from, pos.empty()) & e;
         Bit safe = atk & ee;
         Bit deny = atk & ~safe;

         ns += bit::count(safe);
         nd += bit::count(deny);
      }
   }

   // black

   {
      Bit ee = e & ~(((wm << J1) & (e >> J1)) | ((wm << I1) & (e >> I1))
                   | ((wm >> I1) & (e << I1)) | ((wm >> J1) & (e << J1)));

      for (Bit b = pos.bk(); b != 0; b = bit::rest(b)) {

         Square from = bit::first(b);

         Bit atk  = bit::king_attack(from, pos.empty()) & e;
         Bit safe = atk & ee;
         Bit deny = atk & ~safe;

         ns -= bit::count(safe);
         nd -= bit::count(deny);
      }
   }

   s2.add(var + 0, ns);
   s2.add(var + 1, nd);
}

static void pattern(Score_2 & s2, int var, const Pos & pos) {

   int i0, i1, i2, i3; // top
   int i4, i5, i6, i7; // bottom

   Bit wm = pos.wm();
   Bit bm = pos.bm();

   indices_column(wm >> 0, bm >> 0, i0, i4);
   indices_column(wm >> 1, bm >> 1, i1, i5);
   indices_column(wm >> 2, bm >> 2, i2, i6);
   indices_column(wm >> 3, bm >> 3, i3, i7);

   s2.add(var +  265720 + i0, +1);
   s2.add(var +  797161 + i1, +1);
   s2.add(var + 1328602 + i2, +1);
   s2.add(var + 1860043 + i3, +1);

   s2.add(var + 1860043 - i4, -1);
   s2.add(var + 1328602 - i5, -1);
   s2.add(var +  797161 - i6, -1);
   s2.add(var +  265720 - i7, -1);
}

static void indices_column(uint64 white, uint64 black, int & index_top, int & index_bottom) {

   int wt, wb;
   int bt, bb;

   indices_column(white, wt, wb);
   indices_column(black, bt, bb);

   index_top    = Index_3    [bt] - Index_3    [wt];
   index_bottom = Index_3_Rev[bb] - Index_3_Rev[wb];
}

static void indices_column(uint64 b, int & i0, int & i2) {

   uint64 left = b & U64(0x0C3061830C1860C3); // 4 left files
   uint64 shuffle = (left >> 0) | (left >> 11) | (left >> 22);

   uint64 mask = (1 << 12) - 1;
   i0 = (shuffle >>  0) & mask;
   i2 = (shuffle >> 26) & mask;
}

