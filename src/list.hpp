
#ifndef LIST_HPP
#define LIST_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

class Pos;

// types

class List {

private :

   static const int Size { 128 };

   int p_capture_size;

   int p_size;
   Move p_move[Size];
   int p_score[Size];

public :

   List ();
   List (const List & list);

   void operator= (const List & list);

   void clear       ();
   void add_move    (Square from, Square to);
   void add_capture (Square from, Square to, Bit caps);

   void add (Move mv);

   void set_size  (int size)      { assert(size <= p_size); p_size = size; }
   void set_score (int i, int sc) { assert(i >= 0 && i < p_size); p_score[i] = sc; }

   void mtf         (int i); // move to front
   void sort        ();
   void sort_static ();

   int  size  ()      const { return p_size; }
   Move move  (int i) const { assert(i >= 0 && i < p_size); return p_move[i]; }
   int  score (int i) const { assert(i >= 0 && i < p_size); return p_score[i]; }

   Move operator[] (int i) const { return move(i); }

private :

   static uint64 move_order (Move mv);

   void copy (const List & list);
};

// functions

namespace list {

int  pick (const List & list, double k);

bool has        (const List & list, Move mv);
int  find       (const List & list, Move mv);
Move find_index (const List & list, Move_Index index, const Pos & pos);

}

#endif // !defined LIST_HPP

