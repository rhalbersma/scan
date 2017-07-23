
#ifndef POS_HPP
#define POS_HPP

// includes

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "move_gen.hpp" // for can_capture

// types

class Pos {

private :

   Bit p_piece[Piece_Size];
   Bit p_side[Side_Size];
   Bit p_all;
   Side p_turn;

public :

   Pos () { }
   Pos (Side turn, Bit wm, Bit bm, Bit wk, Bit bk);

   Pos succ (Move mv) const;

   Side turn () const { return p_turn; }

   Bit  all   () const { return p_all; }
   Bit  empty () const { return bit::Squares ^ all(); }

   Bit  piece (Piece pc) const { return p_piece[pc]; }
   Bit  side  (Side sd)  const { return p_side[sd]; }

   Bit  piece_side (Piece pc, Side sd) const { return piece(pc) & side(sd); }

   Bit  man  () const { return piece(Man); }
   Bit  king () const { return piece(King); }

   Bit  man  (Side sd) const { return piece_side(Man, sd); }
   Bit  king (Side sd) const { return piece_side(King, sd); }

   Bit  white () const { return side(White); }
   Bit  black () const { return side(Black); }

   Bit  wm () const { return man(White); }
   Bit  bm () const { return man(Black); }
   Bit  wk () const { return king(White); }
   Bit  bk () const { return king(Black); }

   bool is_piece (Square sq, Piece pc) const { return bit::has(piece(pc), sq); }
   bool is_side  (Square sq, Side sd)  const { return bit::has(side(sd), sq); }
   bool is_empty (Square sq)           const { return bit::has(empty(), sq); }

private :

   Pos (Bit man, Bit king, Bit white, Bit black, Bit all, Side turn);
};

class Node {

private :

   Pos p_pos;
   int p_ply;
   const Node * p_parent;

public :

   Node () { }
   explicit Node (const Pos & pos);

   Node succ (Move mv) const;

   operator const Pos & () const { return p_pos; }

   bool is_end  ()        const;
   bool is_draw (int rep) const;

private :

   Node (const Pos & pos, int ply, const Node * parent);
};

namespace pos { // ###

// variables

extern Pos Start;

// functions

void init ();

bool is_loss (const Pos & pos);
bool is_wipe (const Pos & pos);

inline bool is_quiet   (const Pos & pos) { return !can_capture(pos, White) && !can_capture(pos, Black); }
inline bool is_capture (const Pos & pos) { return can_capture(pos, pos.turn()); }
inline bool is_threat  (const Pos & pos) { return can_capture(pos, side_opp(pos.turn())); }

inline int  size (const Pos & pos) { return bit::count(pos.all()); }

inline bool has_king (const Pos & pos)          { return pos.king()   != 0; }
inline bool has_king (const Pos & pos, Side sd) { return pos.king(sd) != 0; }

Piece_Side piece_side (const Pos & pos, Square sq);

double phase (const Pos & pos);
int    stage (const Pos & pos);
int    tempo (const Pos & pos);
int    skew  (const Pos & pos, Side sd);

void disp (const Pos & pos);

}

#endif // !defined POS_HPP

