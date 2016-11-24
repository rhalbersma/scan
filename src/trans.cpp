
// trans.cpp

// includes

#include <cstdio>
#include <cstring>
#include <vector>

#include "libmy.hpp"
#include "move.h"
#include "trans.h"

namespace trans {

// constants

static const int Cluster_Size = 4;

// functions

void Trans::set_size(int size) {

   assert(ml::is_power_2(size));

   p_size = size;
   p_mask = (size - 1) & -Cluster_Size;

   p_table.resize(p_size);

   clear();
}

void Trans::clear() {

   assert(sizeof(Entry) == 16);

   Entry entry = { 0, 0, 0, 0, 0, 0, 0, 0 };

   for (int i = 0; i < p_size; i++) {
      p_table[i] = entry;
   }

   set_date(0);
}

void Trans::inc_date() {

   set_date((p_date + 1) % Date_Size);
}

void Trans::set_date(int date) {

   assert(date >= 0 && date < Date_Size);

   p_date = date;

   for (date = 0; date < Date_Size; date++) {
      p_age[date] = age(date);
   }
}

int Trans::age(int date) const {

   assert(date >= 0 && date < Date_Size);

   int age = p_date - date;
   if (age < 0) age += Date_Size;

   assert(age >= 0 && age < Date_Size);
   return age;
}

void Trans::store(uint64 key, int move, int depth, int flags, int score) {

   assert(move >= 0 && move < (1 << 16));
   assert(depth >= 0 && depth < (1 << 8));
   assert((flags & ~Flags) == 0);
   assert(score >= -32767 && score <= +32767);

   // probe

   int index = int(key & p_mask);
   uint32 lock = uint32(key >> 32);

   Entry * be = NULL;
   int bs = -1000000000;

   for (int i = 0; i < Cluster_Size; i++) {

      Entry & entry = p_table[index + i];

      if (entry.lock == lock) { // hash hit

         if (entry.depth <= depth) {

            if (move == Index_None) move = entry.move;
            if (entry.depth == depth && entry.score == score) {
               flags |= entry.flags; // HACK
            }

            assert(entry.lock == lock);
            entry.move = move;
            entry.depth = depth;
            entry.date = p_date;
            entry.flags = flags;
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

      if (sc > bs) {
         be = &entry;
         bs = sc;
      }
   }

   // "best" entry found

   assert(be != NULL);
   Entry & entry = *be;

   // store

   entry.lock = lock;
   entry.move = move;
   entry.depth = depth;
   entry.date = p_date;
   entry.flags = flags;
   entry.score = score;
}

bool Trans::probe(uint64 key, int & move, int & depth, int & flags, int & score) {

   // probe

   int index = int(key & p_mask);
   uint32 lock = uint32(key >> 32);

   for (int i = 0; i < Cluster_Size; i++) {

      const Entry & entry = p_table[index + i];

      if (entry.lock == lock) {

         // found

         move = entry.move;
         depth = entry.depth;
         flags = entry.flags;
         score = entry.score;

         return true;
      }
   }

   // not found

   return false;
}

}

// end of trans.cpp

