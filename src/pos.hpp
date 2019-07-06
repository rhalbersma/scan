
#ifndef POS_HPP
#define POS_HPP

// includes

#include <array>

#include "bit.hpp"
#include "common.hpp"
#include "gen.hpp" // for can_capture
#include "libmy.hpp"

// types

class Pos {

private:

   std::array<Bit, Piece_Size> m_piece;
   std::array<Bit, Side_Size> m_side;
   Bit m_all;
   Side m_turn;

   int m_wolf[Side_Size];
   int m_count[Side_Size];

public:

   Pos () = default;
   Pos (Side turn, Bit wm, Bit bm, Bit wk, Bit bk);

   friend bool operator == (const Pos & p0, const Pos & p1);

   Pos succ (Move mv) const;

   Side turn () const { return m_turn; }

   Bit all   () const { return m_all; }
   Bit empty () const { return bit::Squares ^ all(); }

   Bit piece (Piece pc) const { return m_piece[pc]; }
   Bit side  (Side sd)  const { return m_side[sd]; }

   Bit piece_side (Piece pc, Side sd) const { return piece(pc) & side(sd); }

   Bit man  () const { return piece(Man); }
   Bit king () const { return piece(King); }

   Bit man  (Side sd) const { return piece_side(Man, sd); }
   Bit king (Side sd) const { return piece_side(King, sd); }

   Bit white () const { return side(White); }
   Bit black () const { return side(Black); }

   Bit wm () const { return man(White); }
   Bit bm () const { return man(Black); }
   Bit wk () const { return king(White); }
   Bit bk () const { return king(Black); }

   bool is_empty (Square sq)           const { return bit::has(empty(),   sq); }
   bool is_piece (Square sq, Piece pc) const { return bit::has(piece(pc), sq); }
   bool is_side  (Square sq, Side sd)  const { return bit::has(side(sd),  sq); }

   Square wolf  (Side sd) const { return square_make(m_wolf[sd]); }
   int    count (Side sd) const { return m_count[sd]; }

private:

   Pos (Bit man, Bit king, Bit white, Bit black, Bit all, Side turn);
};

bool operator == (const Pos & p0, const Pos & p1);

class Node {

private:

   Pos m_pos;
   int m_ply;
   const Node * m_parent;

public:

   Node () = default;
   explicit Node (const Pos & pos);

   operator const Pos & () const { return m_pos; }

   Node succ (Move mv) const;

   bool is_end  ()        const;
   bool is_draw (int rep) const;

private:

   Node (const Pos & pos, int ply, const Node * parent);
};

namespace pos { // ###

// variables

extern Pos Start;

// functions

void init ();

bool is_end  (const Pos & pos);
bool is_wipe (const Pos & pos);
int  result  (const Pos & pos, Side sd);

inline bool is_capture (const Pos & pos) { return can_capture(pos, pos.turn()); }
inline bool is_threat  (const Pos & pos) { return can_capture(pos, side_opp(pos.turn())); }

inline int size (const Pos & pos) { return bit::count(pos.all()); }

inline bool has_king (const Pos & pos)          { return pos.king()   != 0; }
inline bool has_king (const Pos & pos, Side sd) { return pos.king(sd) != 0; }

Piece_Side piece_side (const Pos & pos, Square sq);

int tempo (const Pos & pos);
int skew  (const Pos & pos, Side sd);

inline int    stage (const Pos & pos) { return Stage_Size - tempo(pos); }
inline double phase (const Pos & pos) { return double(stage(pos)) / double(Stage_Size); }

void disp (const Pos & pos);

} // namespace pos

#endif // !defined POS_HPP

