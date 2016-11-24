
// move.h

#ifndef MOVE_H
#define MOVE_H

// includes

#include <string>

#include "bit.h"
#include "libmy.hpp"

class Pos;

// types

typedef uint64 move_t;

// constants

const move_t Move_None = 0;
const int Index_None = 0;

const int Index_Size = 1 << 12;

// functions

extern move_t move_make (int from, int to, bit_t captured = 0);

extern bool move_is_promotion  (move_t mv, const Pos & pos);
extern bool move_is_man        (move_t mv, const Pos & pos);
extern bool move_is_conversion (move_t mv, const Pos & pos);

extern bool move_is_legal (move_t mv, const Pos & pos);

extern std::string move_to_string   (move_t mv, const Pos & pos);
extern std::string move_to_hub      (move_t mv);
extern move_t      move_from_string (const std::string & s, const Pos & pos);
extern move_t      move_from_hub    (const std::string & s);

inline int   move_from       (move_t mv) { return int((mv >> 6) & 077); }
inline int   move_to         (move_t mv) { return int(mv & 077); }
inline bit_t move_captured   (move_t mv) { return bit_t(mv & ~07777); }
inline int   move_index      (move_t mv, const Pos & /* pos */)  { return int(mv & 07777); }

inline bool  move_is_capture (move_t mv, const Pos & /* pos */)  { return move_captured(mv) != 0; }

#endif // !defined MOVE_H

// end of move.h

