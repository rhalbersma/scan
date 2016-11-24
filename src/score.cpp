
// score.cpp

// includes

#include "libmy.hpp"
#include "score.h"
#include "search.h" // for Ply_Max

namespace score {

// functions

int loss(int ply) {

   assert(ply >= 0 && ply <= Ply_Max + 2);

   return -Inf + ply;
}

int to_trans(int sc, int ply) {

   assert(sc >= -Inf && sc <= +Inf);
   assert(ply >= 0 && ply <= Ply_Max);

   if (sc > +Eval_Inf) {
      sc += ply;
      assert(sc > +Eval_Inf && sc <= +Inf);
   } else if (sc < -Eval_Inf) {
      sc -= ply;
      assert(sc >= -Inf && sc < -Eval_Inf);
   }

   return sc;
}

int from_trans(int sc, int ply) {

   assert(sc >= -Inf && sc <= +Inf);
   assert(ply >= 0 && ply <= Ply_Max);

   if (sc > +Eval_Inf) {
      sc -= ply;
      // assert(sc > +Eval_Inf && sc <= +Inf);
   } else if (sc < -Eval_Inf) {
      sc += ply;
      // assert(sc >= -Inf && sc < -Eval_Inf); // triggers in SMP
   }

   return sc;
}

int clamp(int sc) {

   if (sc > +Eval_Inf) {
      return +Eval_Inf;
   } else if (sc < -Eval_Inf) {
      return -Eval_Inf;
   }

   return sc;
}

int add(int sc, int inc) {

   if (is_eval(sc)) {
      return clamp(sc + inc);
   } else {
      return sc;
   }
}

}

// end of score.cpp

