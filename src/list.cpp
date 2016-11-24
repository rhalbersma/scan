
// list.cpp

// includes

#include <cstdio>
#include <cmath>
#include <string>

#include "libmy.hpp"
#include "list.h"
#include "move.h"
#include "pos.h"

// prototypes

static int list_best (const List & list);

static uint64 move_order (move_t mv);

static double weight (int sc, int best, double k);

// functions

void List::clear() {

   p_capture_size = 0;
   p_size = 0;
}

void List::add_move(move_t mv) {

   assert(move_captured(mv) == 0);

   if (p_capture_size == 0) {
      add(mv);
   }
}

void List::add_capture(move_t mv) {

   bit_t captured = move_captured(mv);
   assert(captured != 0);

   int size = bit_count(captured);

   if (size < p_capture_size) return;

   if (size >= 4 && list_has(*this, mv)) return; // duplicate capture

   if (size > p_capture_size) {
      p_capture_size = size;
      p_size = 0;
   }

   add(mv);
}

void List::add(move_t mv) {

   assert(p_size < Size);

   assert(!(list_has(*this, mv)));

   p_move[p_size] = mv;
   // p_score[p_size] = 0;
   p_size++;
}

void List::mtf(int pos) {

   assert(pos >= 0 && pos < p_size);

   move_t mv = p_move[pos];
   int    sc = p_score[pos];

   for (int i = pos; i > 0; i--) {
      p_move[i]  = p_move[i - 1];
      p_score[i] = p_score[i - 1];
   }

   p_move[0]  = mv;
   p_score[0] = sc;
}

void List::sort() {

   // init

   int size = p_size;
   if (size <= 1) return;

   p_score[size] = -1000000000; // HACK: sentinel

   // insert sort (stable)

   for (int i = size - 2; i >= 0; i--) {

      move_t mv = p_move[i];
      int    sc = p_score[i];

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

      move_t mv = p_move[i];
      int    sc = p_score[i];
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

move_t list_pick(List & list, double k) {

   assert(list.size() != 0);

   int best = list_best(list);

   move_t mv = Move_None;
   double tot = 0.0;

   for (int i = 0; i < list.size(); i++) {

      double w = weight(list.score(i), best, k);
      tot += w;

      if (ml::rand_bool(w / tot)) mv = list.move(i);
   }

   assert(mv != Move_None);
   return mv;
}

bool list_has(const List & list, move_t mv) {

   for (int i = 0; i < list.size(); i++) {
      if (list.move(i) == mv) return true;
   }

   return false;
}

int list_find(const List & list, move_t mv) {

   for (int i = 0; i < list.size(); i++) {
      if (list.move(i) == mv) return i;
   }

   assert(false);
   return -1;
}

static int list_best(const List & list) {

   assert(list.size() != 0);

   int bs = list.score(0);

   for (int i = 1; i < list.size(); i++) { // skip 0
      int sc = list.score(i);
      if (sc > bs) bs = sc;
   }

   return bs;
}

static uint64 move_order(move_t mv) {

   uint64 from = move_from(mv);
   uint64 to   = move_to(mv);
   uint64 caps = move_captured(mv);

   return (from << (64 - 6)) | (to << (64 - 12)) | (caps >> 12);
}

static double weight(int sc, int best, double k) {

   assert(sc <= best);

   return std::exp(double(sc - best) / 100.0 * k);
}

// end of list.cpp

