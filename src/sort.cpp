
// sort.cpp

// includes

#include "bit.h"
#include "libmy.hpp"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "pos.h"
#include "search.h"
#include "sort.h"

// constants

static const int Prob_Bit   = 12;
static const int Prob_One   = 1 << Prob_Bit;
static const int Prob_Half  = 1 << (Prob_Bit - 1);
static const int Prob_Shift = 6;

// variables

static int Hist[Index_Size];

// functions

void sort_clear() {

   for (int index = 0; index < Index_Size; index++) {
      Hist[index] = Prob_Half;
   }
}

void good_move(int index) {

   assert(index > 0 && index < Index_Size); // NOTE: skip 0
   Hist[index] += (Prob_One - Hist[index]) >> Prob_Shift;
}

void bad_move(int index) {

   assert(index > 0 && index < Index_Size); // NOTE: skip 0
   Hist[index] -= Hist[index] >> Prob_Shift;
}

void sort_all(List & list, const Pos & pos, int killer) {

   if (list.size() <= 1) return;

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);
      int index = move_index(mv, pos);

      int sc = (index == killer) ? ((1 << 15) - 1) : Hist[index];
      assert(sc >= 0 && sc < (1 << 15));

      list.set_score(i, sc);
   }

   list.sort();
}

void sort_trans(List & list, const Pos & pos, int killer) {

   if (list.size() <= 1) return;

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);

      if (move_index(mv, pos) == killer) {
         list.mtf(i);
         return;
      }
   }
}

// end of sort.cpp

