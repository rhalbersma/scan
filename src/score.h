
// score.h

#ifndef SCORE_H
#define SCORE_H

// includes

#include "libmy.hpp"

namespace score {

// constants

const int Inf      = 2000;
const int BB_Inf   = 1900;
const int Eval_Inf = 1800;

const int None = -Inf - 1;

// functions

extern int loss (int ply);

extern int to_trans   (int sc, int ply);
extern int from_trans (int sc, int ply);

extern int clamp (int sc);
extern int add   (int sc, int inc);

inline bool is_eval (int sc) { return sc >= -Eval_Inf && sc <= +Eval_Inf; }

}

#endif // !defined SCORE_H

// end of score.h

