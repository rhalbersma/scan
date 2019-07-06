
#ifndef GEN_HPP
#define GEN_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

class List;
class Pos;

// functions

void gen_moves      (List & list, const Pos & pos);
void gen_captures   (List & list, const Pos & pos);
void gen_promotions (List & list, const Pos & pos);
void add_sacs       (List & list, const Pos & pos);

bool can_move    (const Pos & pos, Side sd);
bool can_capture (const Pos & pos, Side sd);

#endif // !defined GEN_HPP

