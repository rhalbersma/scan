
// includes

#include <cstdio>
#include <cstring>
#include <vector>

#include "common.hpp"
#include "hash.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "score.hpp"
#include "tt.hpp"

namespace tt {

// constants

const int Cluster_Size { 4 };

// variables

TT G_TT;

// functions

void TT::set_size(int size) {

   assert(ml::is_power_2(size));

   p_size = size;
   p_mask = (size - 1) & -Cluster_Size;

   p_table.resize(p_size);

   clear();
}

void TT::clear() {

   assert(sizeof(Entry) == 16);

   Entry entry { 0, 0, 0, 0, 0, 0, 0, 0 };

   for (int i = 0; i < p_size; i++) {
      p_table[i] = entry;
   }

   set_date(0);
}

void TT::inc_date() {
   set_date((p_date + 1) % Date_Size);
}

void TT::set_date(int date) {

   assert(date >= 0 && date < Date_Size);

   p_date = date;

   for (date = 0; date < Date_Size; date++) {
      p_age[date] = age(date);
   }
}

int TT::age(int date) const {

   assert(date >= 0 && date < Date_Size);

   int age = p_date - date;
   if (age < 0) age += Date_Size;

   assert(age >= 0 && age < Date_Size);
   return age;
}

void TT::store(Key key, Move_Index move, Depth depth, Flags flags, Score score) {

   // assert(move >= 0 && move < (1 << 16));
   assert(depth >= 0 && depth < (1 << 8));
   assert((int(flags) & ~int(Flags_Mask)) == 0);
   assert(score > -(1 << 15) && score < +(1 << 15));
   assert(score != score::None);

   // probe

   int    index = hash::index(key, p_mask);
   uint32 lock  = hash::lock(key);

   Entry * be = nullptr;
   int bs = -256;

   for (int i = 0; i < Cluster_Size; i++) {

      assert(index + i < p_size);
      Entry & entry = p_table[index + i];

      if (entry.lock == lock) { // hash hit

         if (entry.depth <= depth) {

            assert(entry.lock == lock);
            if (move != Move_Index_None) entry.move = move;
            entry.depth = depth;
            entry.date = p_date;
            entry.flags = int(flags);
            entry.score = score;

         } else { // deeper entry

            entry.date = p_date;
         }

         return;
      }

      // evaluate replacement score

      int sc = 0;
      sc = sc * Date_Size + p_age[entry.date];
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
   entry.move = move;
   entry.depth = depth;
   entry.date = p_date;
   entry.flags = int(flags);
   entry.score = score;
}

bool TT::probe(Key key, Move_Index & move, Depth & depth, Flags & flags, Score & score) {

   // init

   // probe

   int    index = hash::index(key, p_mask);
   uint32 lock  = hash::lock(key);

   for (int i = 0; i < Cluster_Size; i++) {

      assert(index + i < p_size);
      const Entry & entry = p_table[index + i];

      if (entry.lock == lock) {

         // found

         move = Move_Index(entry.move);
         depth = Depth(entry.depth);
         flags = Flags(entry.flags);
         score = Score(entry.score);

         return true;
      }
   }

   // not found

   return false;
}

}

