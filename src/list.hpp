
#ifndef LIST_HPP
#define LIST_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

class Pos;

// types

class List {

private:

   static const int Size {128};

   int m_capture_score {0};
   int m_size {0};
   Move m_move[Size];
   int16 m_score[Size];

public:

   List () = default;

   List            (const List & list) { copy(list); }
   void operator = (const List & list) { copy(list); }

   void clear ()        { m_capture_score = 0; m_size = 0; }
   void add   (Move mv) { assert(m_size < Size); m_move[m_size++] = mv; }

   void add_move    (Square from, Square to);
   void add_capture (Square from, Square to, Bit caps, const Pos & pos, int king);

   void set_size  (int size);
   void set_score (int i, int sc);

   void move_to_front (int i);
   void sort          ();
   void sort_static   (const Pos & pos);

   int  size        ()      const { return m_size; }
   Move move        (int i) const { assert(i >= 0 && i < m_size); return m_move[i]; }
   int  score       (int i) const { assert(i >= 0 && i < m_size); return m_score[i]; }
   Move operator [] (int i) const { return move(i); }

   const Move * begin () const { return &m_move[0]; }
   const Move * end   () const { return &m_move[m_size]; }

private:

   static uint64 move_order (Move mv, const Pos & pos);

   void copy (const List & list);
};

// functions

namespace list {

int pick (const List & list, double k);

bool has        (const List & list, Move mv);
int  find       (const List & list, Move mv);
Move find_index (const List & list, Move_Index index, const Pos & pos);

} // namespace list

#endif // !defined LIST_HPP

