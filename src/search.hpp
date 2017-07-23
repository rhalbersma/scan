
#ifndef SEARCH_HPP
#define SEARCH_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "util.hpp" // for Timer

class List;
class Node;

// constants

const Depth Depth_Max { Depth(99) };

const Ply Ply_Max  { Ply(99) };
const int Ply_Size { Ply_Max + 1 };

// types

enum Output_Type { Output_None, Output_Terminal, Output_Hub };

class Line {

private :

   ml::Array<Move, Ply_Size> p_move;

public :

   Line () { clear(); }

   void clear () { p_move.clear(); }
   void add   (Move mv) { assert(mv != move::None); p_move.add(mv); }

   void set    (Move mv);
   void concat (Move mv, const Line & pv);

   int  size ()      const { return p_move.size(); }
   Move move (int i) const { return p_move[i]; }

   Move operator[] (int i) const { return move(i); }

   std::string to_string (const Pos & pos, int size_max = 0) const;
   std::string to_hub    () const;
};

class Search_Input {

public :

   bool move;
   bool book;
   Depth depth;
   bool input;
   Output_Type output;
   bool ponder;

private :

   bool p_smart;
   int p_moves;
   double p_time;
   double p_inc;

public :

   Search_Input () { init(); }

   void init ();

   void set_time (double time);
   void set_time (int moves, double time, double inc);

   bool   smart  () const { return p_smart; }
   int    moves  () const { return p_moves; }
   double time   () const { return p_time; }
   double inc    () const { return p_inc; }
};

class Search_Output {

public :

   Move move;
   Move answer;
   Score score;
   Depth depth;
   Line pv;

   int64 node;
   int64 leaf;
   int64 ply_sum;

private :

   const Search_Input * p_si;
   Pos p_pos;
   Timer p_timer;

public :

   void init (const Search_Input & si, const Pos & pos);
   void end  ();

   void new_best_move  (Move mv, Score sc = score::None);
   void new_best_move  (Move mv, Score sc, Depth depth, const Line & pv);
   void disp_best_move ();

   double ply_avg () const { return (leaf == 0) ? 0.0 : double(ply_sum) / double(leaf); }
   double time    () const { return p_timer.elapsed(); }
};

// functions

void search (Search_Output & so, const Node & node, const Search_Input & si);

Move  quick_move  (const Pos & pos);
Score quick_score (const Pos & pos);

#endif // !defined SEARCH_HPP

