
#ifndef GAME_HPP
#define GAME_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"
#include "pos.hpp"

// types

class Game {

private:

   static const int Size {1024};

   struct Move_Info {
      Move move;
      float time {0.0};
   };

   Pos m_pos_start;
   int m_moves {0};
   double m_time {0.0};
   double m_inc {0.0};

   ml::Array<Node, Size> m_node;
   ml::Array<Move_Info, Size> m_move;
   int m_ply;

   double m_clock[Side_Size];

public:

   Game () { clear(); }

   void clear    ()                { init(pos::Start); }
   void init     (const Pos & pos) { init(pos, 0, 0.0, 0.0); }
   void init     (const Pos & pos, int moves, double time, double inc);
   void add_move (Move mv, double time = 0.0);

   void go_to (int ply);

   Side turn   ()                     const { return pos().turn(); }
   bool is_end (bool use_bb = false)  const;
   int  result (bool use_bb, Side sd) const;

   int  size        ()      const { return m_move.size(); }
   int  ply         ()      const { return m_ply; }
   Move move        (int i) const { return m_move[i].move; }
   Move operator [] (int i) const { return m_move[i].move; }

   int    moves ()        const { return (m_moves == 0) ? 0 : m_moves - (m_ply / 2) % m_moves; }
   double time  (Side sd) const { return m_clock[sd]; }
   double inc   ()        const { return m_inc; }

   Pos          start_pos () const { return m_pos_start; }
   Pos          pos       () const { return Pos(node()); }
   const Node & node      () const { assert(m_node.size() > 0); return m_node[m_node.size() - 1]; }

private:

   void reset     ();
   void play_move ();
};

// functions

std::string result_to_string (int result);

#endif // !defined GAME_HPP

