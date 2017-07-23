
// includes

#include "common.hpp"
#include "libmy.hpp"
#include "score.hpp"
#include "search.hpp" // for Ply_Max

namespace score {

// functions

inline bool is_win  (Score sc) { return sc > +Eval_Inf; }
inline bool is_loss (Score sc) { return sc < -Eval_Inf; }
inline bool is_eval (Score sc) { return sc >= -Eval_Inf && sc <= +Eval_Inf; }

Score to_tt(Score sc, Ply ply) {

   assert(is_ok(sc));
   assert(ply >= 0 && ply <= Ply_Max);

   if (is_win(sc)) {
      sc += Score(ply);
      assert(sc <= +Inf);
   } else if (is_loss(sc)) {
      sc -= Score(ply);
      assert(sc >= -Inf);
   }

   return sc;
}

Score from_tt(Score sc, Ply ply) {

   assert(is_ok(sc));
   assert(ply >= 0 && ply <= Ply_Max);

   if (is_win(sc)) {
      sc -= Score(ply);
      assert(is_win(sc));
   } else if (is_loss(sc)) {
      sc += Score(ply);
      assert(is_loss(sc));
   }

   return sc;
}

Score clamp(Score sc) {

   if (is_win(sc)) {
      sc = +Eval_Inf;
   } else if (is_loss(sc)) {
      sc = -Eval_Inf;
   }

   assert(is_eval(sc));
   return sc;
}

Score add_safe(Score sc, Score inc) {

   if (is_eval(sc)) {
      return clamp(sc + inc);
   } else {
      return sc;
   }
}

}

