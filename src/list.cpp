
// includes

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "var.hpp"

// functions

void List::add_move(Square from, Square to) {
   assert(m_capture_score == 0);
   add(move::make(from, to));
}

void List::add_capture(Square from, Square to, Bit caps, const Pos & pos, int king) {

   assert(caps != 0);
   assert(king >= 0 && king < 2);

   int capture_score = (var::Variant == var::Frisian)
                     ? bit::count(caps & pos.man()) * 64 + bit::count(caps & pos.king()) * 126 + king
                     : bit::count(caps);

   if (capture_score >= m_capture_score) {

      Move mv = move::make(from, to, caps);

      if (!(bit::count(caps) >= 3 && list::has(*this, mv))) { // check for duplicate

         if (capture_score > m_capture_score) {
            m_capture_score = capture_score;
            m_size = 0;
         }

         add(mv);
      }
   }
}

void List::set_size(int size) {
   assert(size <= m_size);
   m_size = size;
}

void List::set_score(int i, int sc) {
   assert(i >= 0 && i < m_size);
   assert(std::abs(sc) < (1 << 15));
   m_score[i] = sc;
}

void List::copy(const List & list) { // HACK: does not copy scores

   m_capture_score = list.m_capture_score;

   m_size = list.m_size;

   for (int i = 0; i < list.m_size; i++) {
      m_move[i] = list.m_move[i];
      // m_score[i] = list.m_score[i];
   }
}

void List::move_to_front(int i) {

   assert(i >= 0 && i < m_size);

   Move mv = m_move [i];
   int  sc = m_score[i];

   for (int j = i; j > 0; j--) {
      m_move [j] = m_move [j - 1];
      m_score[j] = m_score[j - 1];
   }

   m_move [0] = mv;
   m_score[0] = sc;
}

void List::sort() {

   // init

   if (m_size <= 1) return;

   // insert sort (stable)

   m_score[m_size] = -(1 << 15); // HACK: sentinel

   for (int i = m_size - 2; i >= 0; i--) {

      Move mv = m_move [i];
      int  sc = m_score[i];

      int j;

      for (j = i; sc < m_score[j + 1]; j++) {
         m_move [j] = m_move [j + 1];
         m_score[j] = m_score[j + 1];
      }

      assert(j < m_size);
      m_move [j] = mv;
      m_score[j] = sc;
   }
}

void List::sort_static(const Pos & pos) { // for opening book

   // init

   if (m_size <= 1) return;

   // insert sort (stable)

   for (int i = m_size - 2; i >= 0; i--) {

      Move mv = m_move [i];
      int  sc = m_score[i];

      uint64 order = move_order(mv, pos);

      int j;

      for (j = i; j + 1 < m_size && order < move_order(m_move[j + 1], pos); j++) {
         m_move [j] = m_move [j + 1];
         m_score[j] = m_score[j + 1];
      }

      assert(j < m_size);
      m_move [j] = mv;
      m_score[j] = sc;
   }
}

uint64 List::move_order(Move mv, const Pos & pos) { // for opening book

   uint64 from = move::from(mv, pos);
   uint64 to   = move::to(mv, pos);
   uint64 caps = move::captured(mv, pos);

   return (from << (64 - 6)) | (to << (64 - 12)) | (caps >> 12);
}

namespace list { // ###

// functions

int pick(const List & list, double k) {

   assert(list.size() != 0);

   int bs = list.score(0);

   for (int i = 1; i < list.size(); i++) { // skip 0
      bs = std::max(bs, list.score(i));
   }

   int index = -1;
   double tot = 0.0;

   for (int i = 0; i < list.size(); i++) {

      double w = std::exp(double(list.score(i) - bs) / 100.0 * k);
      tot += w;

      assert(tot > 0.0);
      if (ml::rand_bool(w / tot)) index = i;
   }

   assert(index >= 0 && index < list.size());
   return index;
}

bool has(const List & list, Move mv) {

   for (Move m : list) {
      if (m == mv) return true;
   }

   return false;
}

int find(const List & list, Move mv) {

   for (int i = 0; i < list.size(); i++) {
      if (list[i] == mv) return i;
   }

   assert(false);
   return -1;
}

Move find_index(const List & list, Move_Index index, const Pos & pos) {

   for (Move mv : list) {
      if (move::index(mv, pos) == index) return mv;
   }

   return move::None;
}

} // namespace list

