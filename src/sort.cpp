
// includes

#include <array>

#include "common.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "sort.hpp"

// constants

const int Prob_Bit   {12};
const int Prob_One   {1 << Prob_Bit};
const int Prob_Half  {1 << (Prob_Bit - 1)};
const int Prob_Shift {5}; // smaller => more adaptive

// variables

static std::array<int, Move_Index_Size> G_Hist;

// functions

void sort_clear() {
   G_Hist.fill(Prob_Half);
}

void good_move(Move mv, const Pos & pos) {
   Move_Index index = move::index(mv, pos);
   G_Hist[index] += (Prob_One - G_Hist[index]) >> Prob_Shift;
}

void bad_move(Move mv, const Pos & pos) {
   Move_Index index = move::index(mv, pos);
   G_Hist[index] -= G_Hist[index] >> Prob_Shift;
}

void sort_moves(List & list, const Pos & pos, Move_Index tt_move) {

   if (list.size() <= 1) return;

   for (int i = 0; i < list.size(); i++) {

      Move mv = list[i];
      Move_Index index = move::index(mv, pos);

      int sc = (index == tt_move) ? Prob_One - 1 : G_Hist[index];
      assert(sc >= 0 && sc < Prob_One);

      list.set_score(i, sc);
   }

   list.sort();
}

