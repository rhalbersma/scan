
// pos.h

#ifndef POS_H
#define POS_H

// includes

#include <string>

#include "bit.h"
#include "libmy.hpp"
#include "move.h"

// constants

const int Square_Size = 66;
const int Dir_Size = 4;
const int Side_Size = 2;
const int Piece_Size = 4; // excludes empty #

enum dir_t { NW, NE, SW, SE };

enum inc_t {
   NW_Inc = -6,
   NE_Inc = -5,
   SW_Inc = +5,
   SE_Inc = +6
};

enum side_t { White, Black };

enum piece_t { WM, WK, BM, BK, Empty, Frame };

// "constants"

extern const int Square_From_50[50];
extern const int Square_To_50[Square_Size];

extern const int Square_File[Square_Size];
extern const int Square_Rank[Square_Size];

extern const int Inc[Dir_Size];

// functions

inline bool side_is_ok (int sd) { return sd >= 0 && sd < Side_Size; }
inline int  side_opp   (int sd) { return sd ^ 1; }

inline bool square_is_ok(int sq) {

   if (sq < 0 || sq >= Square_Size) return false;
   if (Square_To_50[sq] < 0) return false;

   return true;
}

inline int square_from_50 (int sq) { return Square_From_50[sq]; }
inline int square_to_50   (int sq) { return Square_To_50[sq]; }
inline int square_file    (int sq) { return Square_File[sq]; }
inline int square_rank    (int sq) { return Square_Rank[sq]; }
inline int square_opp     (int sq) { return (Square_Size - 1) - sq; }

inline int square_rank(int sq, int sd) {

   assert(square_is_ok(sq));
   assert(side_is_ok(sd));

   int rk = square_rank(sq);

   if (sd == White) {
      return 9 - rk;
   } else {
      return rk;
   }
}

inline bool square_is_promotion(int sq, int sd) {

   assert(square_is_ok(sq));
   assert(side_is_ok(sd));

   static int Promotion_Rank[Side_Size] = { 0, 9 };

   return square_rank(sq) == Promotion_Rank[sd];
}

extern std::string square_to_string   (int sq);
extern bool        string_is_square   (const std::string & s);
extern int         square_from_string (const std::string & s);
extern int         square_from_int    (int sq);

inline bool piece_is_ok     (int pc)         { return pc >= 0 && pc < Piece_Size; }
inline int  piece_man       (int sd)         { return sd << 1; }
inline int  piece_king      (int sd)         { return (sd << 1) | 1; }
inline int  piece_promotion (int pc)         { assert(piece_is_ok(pc)); return pc ^ 1; }
inline int  piece_side      (int pc)         { assert(piece_is_ok(pc)); return bit(pc, 1); }
inline bool piece_is_man    (int pc)         { assert(piece_is_ok(pc)); return !bit_test(pc, 0); }
inline bool piece_is_king   (int pc)         { assert(piece_is_ok(pc)); return  bit_test(pc, 0); }
inline bool piece_is_side   (int pc, int sd) { assert(piece_is_ok(pc)); return piece_side(pc) == sd; }

extern void pos_do_move (Pos & dst, const Pos & src, move_t mv);

// classes

class Pos {

   friend class Board;

private :

   bit_t p_piece[Side_Size];
   bit_t p_king;
   int p_turn;

   void copy (const Pos & pos);

public :

   Pos ()                           { }
   Pos (const Pos & pos)            { copy(pos); }
   Pos (const Pos & pos, move_t mv) { pos_do_move(*this, pos, mv); }

   void operator= (const Pos & pos) { copy(pos); }

   void init     ();
   void init     (int turn, bit_t wp, bit_t bp, bit_t k);
   void from_bit (int turn, bit_t wm, bit_t bm, bit_t wk, bit_t bk);

   int turn () const { return p_turn; }

   bit_t wm () const { return man(White); }
   bit_t bm () const { return man(Black); }
   bit_t wk () const { return king(White); }
   bit_t bk () const { return king(Black); }

   bit_t piece (int sd) const { return p_piece[sd]; }
   bit_t man   (int sd) const { return p_piece[sd] & ~p_king; }
   bit_t king  (int sd) const { return p_piece[sd] &  p_king; }

   bit_t man  () const { return all() & ~p_king; }
   bit_t king () const { return all() &  p_king; }

   bit_t all   () const { return p_piece[White] | p_piece[Black]; }
   bit_t empty () const { return Bit_Squares ^ all(); }
};

// functions

extern void pos_rev (Pos & dst, const Pos & src);

extern void pos_do_move (Pos & dst, const Pos & src, move_t mv);

extern bool pos_is_capture (const Pos & pos);

extern int  pos_size (const Pos & pos);

extern bool pos_has_king (const Pos & pos);

extern int pos_square (const Pos & pos, int sq);

#endif // !defined POS_H

// end of pos.h

