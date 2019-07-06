
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

// compile-time functions

constexpr int pow(int a, int b) { return (b == 0) ? 1 : pow(a, b - 1) * a; }

// constants

const int Pattern_Size {12}; // squares per pattern
const int P {2125820}; // eval parameters
const int Unit {10}; // units per cp

// "constants"

const int Perm_0[Pattern_Size] { 11, 10,  7,  6,  3,  2,  9,  8,  5,  4,  1,  0 };
const int Perm_1[Pattern_Size] {  0,  1,  4,  5,  8,  9,  2,  3,  6,  7, 10, 11 };

// variables

static std::vector<int> G_Weight;

static int Trits_0[pow(2, Pattern_Size)];
static int Trits_1[pow(2, Pattern_Size)];

// types

class Score_2 {

private:

   int m_mg {0};
   int m_eg {0};

public:

   void add(int var, int val) {
      m_mg += G_Weight[var * 2 + 0] * val;
      m_eg += G_Weight[var * 2 + 1] * val;
   }

   int mg () const { return m_mg; }
   int eg () const { return m_eg; }
};

// prototypes

static int conv (int index, int size, int bf, int bt, const int perm[]);

static void pst      (Score_2 & s2, int var, Bit bw, Bit bb);
static void king_mob (Score_2 & s2, int var, const Pos & pos);
static void pattern  (Score_2 & s2, int var, const Pos & pos);

static void indices_column (uint64 white, uint64 black, int & index_top, int & index_bottom);
static void indices_column (uint64 b, int & i0, int & i2);

static Bit attacks (const Pos & pos, Side sd);

// functions

void eval_init() {

   std::cout << "init eval" << std::endl;

   // load weights

   std::string file_name = std::string("data/eval") + var::variant_name();
   std::ifstream file(file_name, std::ios::binary);

   if (!file) {
      std::cerr << "unable to open file \"" << file_name << "\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   G_Weight.resize(P * 2);

   for (int i = 0; i < P * 2; i++) {
      G_Weight[i] = int16(ml::get_bytes(file, 2)); // HACK: extend sign
   }

   // init base conversion (2 -> 3)

   int size = Pattern_Size;
   int bf = 2;
   int bt = 3;

   for (int i = 0; i < pow(bf, size); i++) {
      Trits_0[i] = conv(i, size, bf, bt, Perm_0);
      Trits_1[i] = conv(i, size, bf, bt, Perm_1);
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

Score eval(const Pos & pos) {

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

   if (var::Variant != var::Losing) {
      s2.add(var, std::abs(pos::skew(pos, White)) - std::abs(pos::skew(pos, Black)));
   }
   var += 1;

   // patterns

   pattern(s2, var, pos);
   var += pow(3, Pattern_Size) * 4;

   // game phase

   int stage = pos::stage(pos);
   assert(stage >= 0 && stage <= Stage_Size);

   int sc = ml::div_round(s2.mg() * (Stage_Size - stage) + s2.eg() * stage, Unit * Stage_Size);

   // Wolf rule

   if (var::Variant == var::Frisian) {
      static const int wolf[4] { 0, 1, 3, 6 };
      sc += (wolf[pos.count(White)] - wolf[pos.count(Black)]) * -5;
   }

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

   return score::clamp(score::side(Score(sc), pos.turn())); // for side to move
}

static void pst(Score_2 & s2, int var, Bit bw, Bit bb) {

   for (Square sq : bw) {
      s2.add(var + square_dense(sq), +1);
   }

   for (Square sq : bb) {
      s2.add(var + square_dense(square_opp(sq)), -1);
   }
}

static void king_mob(Score_2 & s2, int var, const Pos & pos) {

   int ns = 0;
   int nd = 0;

   Bit be = pos.empty();

   // white

   if (pos.wk() != 0) {

      Bit attacked = attacks(pos, Black);

      for (Square from : pos.wk()) {

         Bit atk  = bit::king_moves(from, be) & be;
         Bit safe = atk & ~attacked;
         Bit deny = atk &  attacked;

         ns += bit::count(safe);
         nd += bit::count(deny);
      }
   }

   // black

   if (pos.bk() != 0) {

      Bit attacked = attacks(pos, White);

      for (Square from : pos.bk()) {

         Bit atk  = bit::king_moves(from, be) & be;
         Bit safe = atk & ~attacked;
         Bit deny = atk &  attacked;

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

   indices_column(pos.wm() >> 0, pos.bm() >> 0, i0, i4);
   indices_column(pos.wm() >> 1, pos.bm() >> 1, i1, i5);
   indices_column(pos.wm() >> 2, pos.bm() >> 2, i2, i6);
   indices_column(pos.wm() >> 3, pos.bm() >> 3, i3, i7);

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

   int w0, w2;
   int b0, b2;

   indices_column(white, w0, w2);
   indices_column(black, b0, b2);

   index_top    = Trits_0[b0] - Trits_0[w0];
   index_bottom = Trits_1[b2] - Trits_1[w2];
}

static void indices_column(uint64 b, int & i0, int & i2) {

   uint64 left = b & 0x0C3061830C1860C3; // left 4 files
   uint64 shuffle = (left >> 0) | (left >> 11) | (left >> 22);

   uint64 mask = (1 << Pattern_Size) - 1;
   i0 = (shuffle >>  0) & mask;
   i2 = (shuffle >> 26) & mask;
}

static Bit attacks(const Pos & pos, Side sd) {

   Bit ba = pos.man(sd);
   Bit be = pos.empty();

   uint64 t = 0;

   t |= (ba >> J1) & (be << J1);
   t |= (ba >> I1) & (be << I1);
   t |= (ba << I1) & (be >> I1);
   t |= (ba << J1) & (be >> J1);

   if (var::Variant == var::Frisian) {
      t |= (ba >> L1) & (be << L1);
      t |= (ba >> K1) & (be << K1);
      t |= (ba << K1) & (be >> K1);
      t |= (ba << L1) & (be >> L1);
   }

   return bit::Squares & t;
}

