
// game.h

#ifndef GAME_H
#define GAME_H

// includes

#include <string>

#include "board.h"
#include "libmy.hpp"
#include "move.h"

// types

struct Move_Info {
   move_t move;
   float time;
};

class Game {

private :

   static const int Size = 1024;

   std::string p_fen;
   int p_moves;
   double p_time;
   double p_inc;

   ml::Array<Move_Info, Size> p_move;
   int p_pos;

   Board p_board;
   double p_clock[Side_Size];
   bool p_flag[Side_Size];

public :

   void clear    ();
   void clear    (int moves, double time, double inc);
   void init     (const std::string & fen);
   void init     (const std::string & fen, int moves, double time, double inc);
   void add_move (move_t mv, double time = 0.0);

   void go_to (int pos);

   bool is_end (bool use_bb) const;
   int  result (bool use_bb) const; // for white

   int    size ()        const { return p_move.size(); }
   int    pos  ()        const { return p_pos; }
   move_t move (int pos) const { return p_move[pos].move; }

   int    moves (int sd) const;
   double time  (int sd) const { return p_clock[sd]; }
   double inc   ()       const { return p_inc; }

   const Board & board () const { return p_board; }

private :

   void reset     ();
   void play_move ();
};

// functions

extern std::string result_to_string (int result);

#endif // !defined GAME_H

// end of game.h

