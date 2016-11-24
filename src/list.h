
// list.h

#ifndef LIST_H
#define LIST_H

// includes

#include "libmy.hpp"
#include "move.h"

class Pos;

// types

class List {

private :

   static const int Size = 128;

   int p_capture_size;

   int p_size;
   move_t p_move[Size];
   int p_score[Size];

   void copy(const List & list) { // does not copy scores

      p_size = list.p_size;

      for (int i = 0; i < list.p_size; i++) {
         p_move[i] = list.p_move[i];
      }
   }

   void slide (int from, int to);

public :

   List ()                  { clear(); }
   List (const List & list) { copy(list); }

   void operator= (const List & list) { copy(list); }

   void clear       ();
   void add_move    (move_t mv);
   void add_capture (move_t mv);

   void add (move_t mv);

   void set_size  (int size)        { assert(size <= p_size); p_size = size; }
   void set_score (int pos, int sc) { p_score[pos] = sc; }

   void mtf         (int pos); // move to front
   void sort        ();
   void sort_static ();

   int capture_size () const { return p_capture_size; }

   int    size  ()        const { return p_size; }
   move_t move  (int pos) const { return p_move[pos]; }
   int    score (int pos) const { return p_score[pos]; }
};

// functions

extern move_t list_pick (List & list, double k);

extern bool list_has  (const List & list, move_t mv);
extern int  list_find (const List & list, move_t mv);

#endif // !defined LIST_H

// end of list.h

