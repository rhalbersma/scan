
// board.h

#ifndef BOARD_H
#define BOARD_H

// includes

#include <string>

#include "bit.h"
#include "libmy.hpp"
#include "move.h"
#include "pos.h"

// types

struct Undo {
   bool promotion;
   int ply;
   bit_t wp;
   bit_t bp;
   bit_t king;
};

// classes

class Board {

private :

   Pos p_pos;

   bit_t p_bit[Piece_Size];
   int p_square[Square_Size];
   int p_trit[Square_Size];

   uint64 p_key;
   int p_pip;
   int p_skew[Side_Size];

   int p_ply;
   ml::Array<uint64, 256> p_rep;

   bool is_ok ();

   void copy (const Board & bd);

   void switch_turn ();

   void add_piece    (int pc, int sq);
   void remove_piece (int pc, int sq);
   void move_piece   (int pc, int from, int to);

   Board (const Board & bd) { copy(bd); }

public :

   Board ();

   void operator= (const Board & bd) { copy(bd); }

   operator const Pos & () const { return p_pos; }

   void init     ();
   void from_bit (int turn, bit_t wm, bit_t bm, bit_t wk, bit_t bk);
   void from_pos (const Pos & pos);

   void do_move   (move_t mv, Undo & undo);
   void undo_move (move_t mv, const Undo & undo);

   void set_root ();

   int    turn ()       const { return p_pos.p_turn; }
   uint64 key  ()       const { return p_key; }
   int    pip  ()       const { return p_pip; }
   int    skew (int sd) const { return p_skew[sd]; }

   int   size (int pc) const { return bit_count(p_bit[pc]); }
   bit_t bit  (int pc) const { return p_bit[pc]; }

   bit_t bit_wm () const { return p_bit[WM]; }
   bit_t bit_bm () const { return p_bit[BM]; }
   bit_t bit_wk () const { return p_bit[WK]; }
   bit_t bit_bk () const { return p_bit[BK]; }

   bit_t bit_piece (int sd) const { return bit_man(sd) | bit_king(sd); }
   bit_t bit_man   (int sd) const { return p_bit[piece_man(sd)]; }
   bit_t bit_king  (int sd) const { return p_bit[piece_king(sd)]; }

   bit_t bit_man  () const { return p_bit[WM] | p_bit[BM]; }
   bit_t bit_king () const { return p_bit[WK] | p_bit[BK]; }

   bit_t bit_all   () const { return p_bit[WM] | p_bit[WK] | p_bit[BM] | p_bit[BK]; }
   bit_t bit_empty () const { return Bit_Squares ^ bit_all(); }

   int square (int sq) const { return p_square[sq]; }
   int trit   (int sq) const { return p_trit[sq]; }

   bool is_draw (int rep) const;
};

// functions

extern void board_init ();

extern void board_from_fen (Board & bd, const std::string & s);
extern void board_from_hub (Board & bd, const std::string & s);
extern void board_do_move  (Board & bd, move_t mv);

extern bool board_is_end  (const Board & bd, int rep);
extern bool board_is_loss (const Board & bd);
extern bool board_is_wipe (const Board & bd);

extern int    board_pip   (const Board & bd);
extern int    board_skew  (const Board & bd, int sd);
extern double board_phase (const Board & bd);

extern void board_disp (const Board & bd);

#endif // !defined BOARD_H

// end of board.h

