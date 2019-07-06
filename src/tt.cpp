
// includes

#include <algorithm>
#include <cmath>

#include "common.hpp"
#include "hash.hpp"
#include "libmy.hpp"
#include "score.hpp"
#include "tt.hpp"

// constants

const int Cluster_Size {4};

// variables

TT G_TT;

// functions

void TT::set_size(int size) {

   m_size = size;
   m_mask = (size - 1) & -Cluster_Size;

   m_table.resize(m_size);

   clear();
}

void TT::clear() {

   static_assert(sizeof(Entry) == 16, "");

   Entry entry {};
   entry.move = Move_Index_None;
   entry.score = score::None;
   entry.flag = int(Flag::None);

   std::fill(m_table.begin(), m_table.end(), entry);

   set_date(0);
}

void TT::inc_date() {
   set_date((m_date + 1) % Date_Size);
}

void TT::set_date(int date) {

   assert(date >= 0 && date < Date_Size);

   m_date = date;

   for (date = 0; date < Date_Size; date++) {

      int age = m_date - date;
      if (age < 0) age += Date_Size;

      m_age[date] = age;
   }
}

void TT::store(Key key, Move_Index move, Score score, Flag flag, Depth depth) {

   assert(move >= 0 && move < (1 << 16));
   assert(score != score::None);
   assert(std::abs(score) < (1 << 15));
   assert(depth > 0 && depth < (1 << 8));

   // probe

   int    index = hash::index(key, m_mask);
   uint32 lock  = hash::lock(key);

   Entry * be = nullptr;
   int bs = -256;

   for (int i = 0; i < Cluster_Size; i++) {

      assert(index + i < m_size);
      Entry & entry = m_table[index + i];

      if (entry.lock == lock) { // hash hit

         if (entry.depth <= depth) {

            assert(entry.lock == lock);
            entry.date = m_date;
            if (move != Move_Index_None) entry.move = move;
            entry.score = score;
            entry.flag = int(flag);
            entry.depth = depth;

         } else { // deeper entry

            entry.date = m_date;
         }

         return;
      }

      // evaluate replacement score

      int sc = 0;
      sc = sc * Date_Size + m_age[entry.date];
      sc = sc * 256 - entry.depth;
      assert(sc > -256);

      if (sc > bs) {
         be = &entry;
         bs = sc;
      }
   }

   // "best" entry found

   assert(be != nullptr);
   Entry & entry = *be;
   // assert(entry.lock != lock); // triggers in SMP

   // store

   entry.lock = lock;
   entry.date = m_date;
   entry.move = move;
   entry.score = score;
   entry.flag = int(flag);
   entry.depth = depth;
}

bool TT::probe(Key key, Move_Index & move, Score & score, Flag & flag, Depth & depth) {

   // probe

   int    index = hash::index(key, m_mask);
   uint32 lock  = hash::lock(key);

   for (int i = 0; i < Cluster_Size; i++) {

      assert(index + i < m_size);
      const Entry & entry = m_table[index + i];

      if (entry.lock == lock) {

         // found

         move = Move_Index(entry.move);
         score = Score(entry.score);
         flag = Flag(entry.flag);
         depth = Depth(entry.depth);

         return true;
      }
   }

   // not found

   move = Move_Index_None;
   return false;
}

