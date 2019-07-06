
#ifndef SEARCH_HPP
#define SEARCH_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "tt.hpp" // for Flag
#include "util.hpp" // for Timer

class List;
class Node;

// constants

const Depth Depth_Max {Depth(99)};

const Ply Ply_Root {Ply(0)};
const Ply Ply_Max  {Ply(99)};
const int Ply_Size {Ply_Max + 1};

// types

enum Output_Type { Output_None, Output_Terminal, Output_Hub };

class Line {

private:

   ml::Array<Move, Ply_Size> m_move;

public:

   Line () = default;

   void clear ()        { m_move.clear(); }
   void add   (Move mv) { m_move.add(mv); }

   void set    (Move mv);
   void concat (Move mv, const Line & pv);

   int  size        ()      const { return m_move.size(); }
   Move operator [] (int i) const { return m_move[i]; }

   auto begin () const { return m_move.begin(); }
   auto end   () const { return m_move.end(); }

   std::string to_string (const Pos & pos, int size_max = 0) const;
   std::string to_hub    (const Pos & pos) const;
};

class Search_Input {

public:

   bool move {true};
   bool book {true};
   Depth depth {Depth_Max};
   int64 nodes {int64(1E12)};
   bool input {false};
   Output_Type output {Output_None};

   bool smart {false};
   int moves {0};
   double time {1E6};
   double inc {0.0};
   bool ponder {false};

public:

   void init ();

   void set_time (int moves, double time, double inc);
};

class Search_Output {

public:

   Move move {move::None};
   Move answer {move::None};
   Score score {score::None};
   Flag flag {Flag::None};
   Depth depth {Depth(0)};
   Line pv;

   int64 node {0};
   int64 leaf {0};
   int64 ply_sum {0};

private:

   const Search_Input * m_si;
   Pos m_pos;
   Timer m_timer;

public:

   void init (const Search_Input & si, const Pos & pos);
   void end  ();

   void start_iter (Depth depth);
   void end_iter   ();

   void new_best_move (Move mv, Score sc = score::None);
   void new_best_move (Move mv, Score sc, Flag flag, Depth depth, const Line & pv);

   double ply_avg () const;
   double time    () const;
};

// functions

void search (Search_Output & so, const Node & node, const Search_Input & si);

Move  quick_move  (const Pos & pos);
Score quick_score (const Pos & pos);

#endif // !defined SEARCH_HPP

