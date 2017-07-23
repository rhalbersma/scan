
#ifndef SCORE_HPP
#define SCORE_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

namespace score {

// constants

const Score Inf      { Score(10000) };
const Score BB_Inf   { Score( 9000) };
const Score Eval_Inf { Score( 8000) };

const Score None { -Inf - Score(1) };

// functions

inline int side(int sc, Side sd) { return (sd == White) ? +sc : -sc; }

inline bool  is_ok (int sc)  { return sc >= -Inf && sc <= +Inf; }
inline Score make  (int sc)  { assert(is_ok(sc)); return Score(sc); }
inline Score loss  (Ply ply) { return -Inf + Score(ply); }

Score to_tt   (Score sc, Ply ply);
Score from_tt (Score sc, Ply ply);

Score clamp    (Score sc);
Score add_safe (Score sc, Score inc);

}

#endif // !defined SCORE_HPP

