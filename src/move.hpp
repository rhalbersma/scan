
#ifndef MOVE_HPP
#define MOVE_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace move {

// constants

const Move None { Move(0) };

// functions

Move make (Square from, Square to, Bit captured = Bit(0));

inline Square from     (Move mv) { return Square((uint64(mv) >> 6) & 077); }
inline Square to       (Move mv) { return Square((uint64(mv) >> 0) & 077); }
inline Bit    captured (Move mv) { return Bit((uint64(mv) & ~uint64(07777)) >> 5); }

inline Move_Index index (Move mv, const Pos & /* pos */) { return Move_Index(uint64(mv) & uint64(07777)); }

bool is_capture    (Move mv, const Pos & pos);
bool is_promotion  (Move mv, const Pos & pos);
bool is_man        (Move mv, const Pos & pos);
bool is_conversion (Move mv, const Pos & pos);

bool is_legal (Move mv, const Pos & pos);

std::string to_string   (Move mv, const Pos & pos);
std::string to_hub      (Move mv);
Move        from_string (const std::string & s, const Pos & pos);
Move        from_hub    (const std::string & s);

}

#endif // !defined MOVE_HPP

