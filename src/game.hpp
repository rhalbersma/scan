
#ifndef GAME_HPP
#define GAME_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "pos.hpp"

// types

class Game {

private :

   static const int Size { 1024 };

   struct Move_Info {
      Move move;
      float time;
   };

   Pos p_pos_start;
   int p_moves;
   double p_time;
   double p_inc;

   ml::Array<Node, Size> p_node;
   ml::Array<Move_Info, Size> p_move;
   int p_ply;

   double p_clock[Side_Size];
   bool p_flag[Side_Size];

public :

   Game ();

   void init     (const Pos & pos);
   void init     (const Pos & pos, int moves, double time, double inc);
   void add_move (Move mv, double time = 0.0);

   void go_to (int ply);

   Side turn   ()                     const { return pos().turn(); }
   bool is_end (bool use_bb = false)  const;
   int  result (bool use_bb, Side sd) const;

   int  size ()      const { return p_move.size(); }
   int  ply  ()      const { return p_ply; }
   Move move (int i) const { return p_move[i].move; }

   Move operator[] (int i) const { return move(i); }

   int    moves (Side sd) const { assert(sd == turn()); return (p_moves == 0) ? 0 : p_moves - (p_ply / 2) % p_moves; }
   double time  (Side sd) const { return p_clock[sd]; }
   double inc   ()        const { return p_inc; }

   Pos          start_pos () const { return p_pos_start; }
   Pos          pos       () const { return Pos(node()); }
   const Node & node      () const { assert(p_node.size() > 0); return p_node[p_node.size() - 1]; }

private :

   void reset     ();
   void play_move ();
};

// functions

std::string result_to_string (int result);

#endif // !defined GAME_HPP

