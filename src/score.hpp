
#ifndef SCORE_HPP
#define SCORE_HPP

// includes

#include <cmath>

#include "common.hpp"
#include "libmy.hpp"

namespace score {

// constants

const Score Inf      {Score(10'000)};
const Score BB_Inf   {Inf - Score(1'000)};
const Score Eval_Inf {Inf - Score(2'000)};
const Score None     {-Inf - Score(1)};

// functions

template <typename T>
inline T side (T sc, Side sd) { return (sd == White) ? +sc : -sc; }

inline bool  is_ok (int sc)  { return std::abs(sc) <= Inf; }
inline Score make  (int sc)  { assert(is_ok(sc)); return Score(sc); }
inline Score win   (Ply ply) { return +Inf - Score(ply); }
inline Score loss  (Ply ply) { return -Inf + Score(ply); }

Score to_tt   (Score sc, Ply ply);
Score from_tt (Score sc, Ply ply);

Score clamp (Score sc);

inline bool is_win  (Score sc) { return sc > +Eval_Inf; }
inline bool is_loss (Score sc) { return sc < -Eval_Inf; }
inline bool is_eval (Score sc) { return std::abs(sc) <= Eval_Inf; }

} // namespace score

#endif // !defined SCORE_HPP

