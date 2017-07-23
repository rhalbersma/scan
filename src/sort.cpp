
// includes

#include "common.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "move_gen.hpp"
#include "pos.hpp"
#include "search.hpp"
#include "sort.hpp"

// constants

const int Prob_Bit   { 12 };
const int Prob_One   { 1 << Prob_Bit };
const int Prob_Half  { 1 << (Prob_Bit - 1) };
const int Prob_Shift { 5 }; // smaller => more adaptive

// variables

static int G_Hist[Move_Index_Size];

// functions

void sort_clear() {

   for (int index = 0; index < Move_Index_Size; index++) {
      G_Hist[index] = Prob_Half;
   }
}

void good_move(Move_Index index) {
   assert(index != Move_Index_None);
   G_Hist[index] += (Prob_One - G_Hist[index]) >> Prob_Shift;
}

void bad_move(Move_Index index) {
   assert(index != Move_Index_None);
   G_Hist[index] -= G_Hist[index] >> Prob_Shift;
}

void sort_all(List & list, const Pos & pos, Move_Index tt_move) {

   if (list.size() <= 1) return;

   for (int i = 0; i < list.size(); i++) {

      Move mv = list.move(i);
      Move_Index index = move::index(mv, pos);

      int sc;

      if (index == tt_move) {
         sc = (1 << 15) - 1;
      } else {
         sc = G_Hist[index];
         assert(sc < (1 << 12));
      }

      assert(sc >= 0 && sc < (1 << 15));
      list.set_score(i, sc);
   }

   list.sort();
}

