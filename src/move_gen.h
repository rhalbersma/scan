
// move_gen.h

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

// includes

#include "libmy.hpp"

class List;
class Pos;

// functions

extern void gen_moves      (List & list, const Pos & pos);
extern void gen_captures   (List & list, const Pos & pos);
extern void gen_promotions (List & list, const Pos & pos);
extern void gen_quiets     (List & list, const Pos & pos);
extern void add_exchanges  (List & list, const Pos & pos);

extern bool can_move    (const Pos & pos, int sd);
extern bool can_capture (const Pos & pos, int sd);

#endif // !defined MOVE_GEN_H

// end of move_gen.h

