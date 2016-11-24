
// search.h

#ifndef SEARCH_H
#define SEARCH_H

// includes

#include <string>

#include "board.h"
#include "libmy.hpp"
#include "move.h"
#include "util.h"

class List;

// constants

const int Depth_Max = 79;
const int Ply_Max   = 99;

const int Depth_Size = Depth_Max + 1;
const int Ply_Size   = Ply_Max + 1;

// types

enum output_t {
   Output_None, Output_Terminal, Output_Hub
};

class Line {

private :

   ml::Array<move_t, Ply_Size> p_move;

   void copy (const Line & pv) { p_move = pv.p_move; }

public :

   Line ()                { clear(); }
   Line (const Line & pv) { copy(pv); }

   void operator= (const Line & pv) { copy(pv); }

   void clear ()          { p_move.clear(); }
   void add   (move_t mv) { p_move.add(mv); }

   void set (move_t mv);
   void cat (move_t mv, const Line & pv);

   int    size ()        const { return p_move.size(); }
   move_t move (int pos) const { return p_move[pos]; }

   std::string to_string (const Pos & pos, int size_max = 0) const;
};

struct Search_Best {
   move_t move;
   move_t ponder;
   int score;
   int depth;
   Line pv;
};

struct Search_Current {
   Timer timer;
   int depth;
   int64 node_nb;
   int64 leaf_nb;
   int64 ply_sum;
};

class Search_Info {

private :

   bool p_unique;
   bool p_book;
   int p_depth;
   bool p_input;
   output_t p_output;
   int p_bb_size;

   bool p_smart;
   int p_moves;
   double p_time;
   double p_inc;
   bool p_ponder;

public :

   void init ();

   void set_unique  (bool unique)     { p_unique = unique; }
   void set_book    (bool book)       { p_book = book; }
   void set_depth   (int depth)       { p_depth = depth; }
   void set_input   (bool input)      { p_input = input; }
   void set_output  (output_t output) { p_output = output; }
   void set_bb_size (int bb_size)     { p_bb_size = bb_size; }

   void set_time   (double time);
   void set_time   (int moves, double time, double inc);
   void set_ponder (bool ponder) { p_ponder = ponder; }

   bool     unique  () const { return p_unique; }
   bool     book    () const { return p_book; }
   int      depth   () const { return p_depth; }
   bool     input   () const { return p_input; }
   output_t output  () const { return p_output; }
   int      bb_size () const { return p_bb_size; }

   bool   smart  () const { return p_smart; }
   int    moves  () const { return p_moves; }
   double time   () const { return p_time; }
   double inc    () const { return p_inc; }
   bool   ponder () const { return p_ponder; }
};

class Search {

public : // HACK

   Search_Best p_best;
   Search_Current p_current;

private :

   Board p_board;

public :

   void init (const Board & bd);
   void end  ();

   void start_iter (int depth);
   void end_iter   ();

   void set_score     (int sc) { p_best.score = sc; }
   void new_best_move (move_t mv);
   void new_best_move (move_t mv, int sc, int depth, const Line & pv);

   move_t move     () const { return p_best.move; }
   move_t ponder   () const { return p_best.ponder; }
   int    score    () const { return p_best.score; }
   int    depth    () const { return p_best.depth; }
   double ply_avg  () const { return (p_current.leaf_nb == 0) ? 0.0 : double(p_current.ply_sum) / double(p_current.leaf_nb); }
   const Line & pv () const { return p_best.pv; }
   int64  node     () const { return p_current.node_nb; }
   double time     () const;
};

// variables

extern Search_Info G_Search_Info;
extern Search G_Search;

// functions

extern void search_init     ();
extern void search_new_game ();

extern void search_id (const Board & bd);

extern move_t ponder_move (const Board & bd);

#endif // !defined SEARCH_H

// end of search.h

