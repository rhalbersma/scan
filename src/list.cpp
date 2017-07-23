
// includes

#include <cstdio>
#include <cmath>
#include <string>

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "pos.hpp"

// functions

List::List() {
   clear();
}

List::List(const List & list) {
   copy(list);
}

void List::operator=(const List & list) {
   copy(list);
}

void List::clear() {
   p_capture_size = 0;
   p_size = 0;
}

void List::add_move(Square from, Square to) {
   assert(p_capture_size == 0);
   Move mv = move::make(from, to);
   add(mv);
}

void List::add_capture(Square from, Square to, Bit caps) {

   assert(caps != 0);

   int size = bit::count(caps);

   if (size >= p_capture_size) {

      Move mv = move::make(from, to, caps);

      if (!(size >= 4 && list::has(*this, mv))) { // check for duplicate

         if (size > p_capture_size) {
            p_capture_size = size;
            p_size = 0;
         }

         add(mv);
      }
   }
}

void List::add(Move mv) {

   assert(!(list::has(*this, mv)));

   assert(p_size < Size);
   p_move[p_size] = mv;
   // p_score[p_size] = 0;
   p_size++;
}

void List::copy(const List & list) { // does not copy scores

   p_size = list.p_size;

   for (int i = 0; i < list.p_size; i++) {
      p_move[i] = list.p_move[i];
   }
}

void List::mtf(int i) {

   assert(i >= 0 && i < p_size);

   Move mv = p_move[i];
   int  sc = p_score[i];

   for (int j = i; j > 0; j--) {
      p_move[j]  = p_move[j - 1];
      p_score[j] = p_score[j - 1];
   }

   p_move[0]  = mv;
   p_score[0] = sc;
}

void List::sort() {

   // init

   int size = p_size;
   if (size <= 1) return;

   p_score[size] = -(1 << 30); // HACK: sentinel

   // insert sort (stable)

   for (int i = size - 2; i >= 0; i--) {

      Move mv = p_move[i];
      int  sc = p_score[i];

      int j;

      for (j = i; sc < p_score[j + 1]; j++) {
         p_move[j]  = p_move[j + 1];
         p_score[j] = p_score[j + 1];
      }

      assert(j < size);

      p_move[j]  = mv;
      p_score[j] = sc;
   }

   // debug

   if (DEBUG) {
      for (int i = 0; i < size - 1; i++) {
         assert(p_score[i] >= p_score[i + 1]);
      }
   }
}

void List::sort_static() { // for opening book

   // init

   int size = p_size;
   if (size <= 1) return;

   // insert sort (stable)

   for (int i = size - 2; i >= 0; i--) {

      Move mv = p_move[i];
      int  sc = p_score[i];
      uint64 order = move_order(mv);

      int j;

      for (j = i; j + 1 < size && order < move_order(p_move[j + 1]); j++) {
         p_move[j]  = p_move[j + 1];
         p_score[j] = p_score[j + 1];
      }

      assert(j < size);

      p_move[j]  = mv;
      p_score[j] = sc;
   }

   // debug

   if (DEBUG) {
      for (int i = 0; i < size - 1; i++) {
         assert(move_order(p_move[i]) >= move_order(p_move[i + 1]));
      }
   }
}

uint64 List::move_order(Move mv) { // for opening book

   uint64 from = move::from(mv);
   uint64 to   = move::to(mv);
   uint64 caps = move::captured(mv);

   return (from << (64 - 6)) | (to << (64 - 12)) | (caps >> 12);
}

namespace list {

int pick(const List & list, double k) {

   assert(list.size() != 0);

   int bs = list.score(0);

   for (int i = 1; i < list.size(); i++) { // skip 0
      int sc = list.score(i);
      if (sc > bs) bs = sc;
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

   for (int i = 0; i < list.size(); i++) {
      if (list.move(i) == mv) return true;
   }

   return false;
}

int find(const List & list, Move mv) {

   for (int i = 0; i < list.size(); i++) {
      if (list.move(i) == mv) return i;
   }

   assert(false);
   return -1;
}

Move find_index(const List & list, Move_Index index, const Pos & pos) {

   for (int i = 0; i < list.size(); i++) {

      Move mv = list.move(i);

      if (move::index(mv, pos) == index) {
         return mv;
      }
   }

   return move::None;
}

}

