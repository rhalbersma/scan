
#ifndef MOVE_HPP
#define MOVE_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace move {

// constants

const Move None {Move(0)};

// functions

Move make (Square from, Square to, Bit captured = Bit(0));

Square from     (Move mv, const Pos & pos);
Square to       (Move mv, const Pos & pos);
Bit    captured (Move mv, const Pos & pos);

Move_Index index (Move mv, const Pos & pos);

bool is_capture    (Move mv, const Pos & pos);
bool is_promotion  (Move mv, const Pos & pos);
bool is_conversion (Move mv, const Pos & pos);
bool is_forcing    (Move mv, const Pos & pos);

bool is_legal (Move mv, const Pos & pos);

std::string to_string (Move mv, const Pos & pos);
std::string to_hub    (Move mv, const Pos & pos);

Move from_string (const std::string & s, const Pos & pos);
Move from_hub    (const std::string & s, const Pos & pos);

} // namespace move

#endif // !defined MOVE_HPP

