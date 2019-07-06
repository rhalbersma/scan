
// includes

#include <cmath>

#include "common.hpp"
#include "libmy.hpp"
#include "score.hpp"
#include "search.hpp" // for Ply_Max

namespace score {

// functions

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

   if (sc == score::None) return score::None; // HACK for SMP

   assert(is_ok(sc));
   assert(ply >= 0 && ply <= Ply_Max);

   if (is_win(sc)) {
      sc -= Score(ply);
      // assert(is_win(sc)); // triggers in SMP
   } else if (is_loss(sc)) {
      sc += Score(ply);
      // assert(is_loss(sc)); // triggers in SMP
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

} // namespace score

