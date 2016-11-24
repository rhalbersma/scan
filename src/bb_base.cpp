
// bb_base.cpp

// includes

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "bb_base.h"
#include "bb_comp.h"
#include "bb_index.h"
#include "libmy.hpp"
#include "list.h"
#include "move_gen.h"
#include "pos.h"
#include "search.h" // for G_Search_Info.bb_size()
#include "var.h"

namespace bb {

// "constants"

static const int Order[Value_Size] = {
   2, 0, 3, 1, // DLWU -> LUDW
};

// variables

static Base G_Base[ID_Size];

// prototypes

static bool id_is_load (int id);
static bool is_load    (int nw, int nb);

// functions

void init() {

   std::cout << "init bitbase" << std::endl;

   for (int id = 0; id < ID_Size; id++) {
      if (id_is_load(id)) {
         load(id);
      }
   }
}

void load(int id) {

   assert(id_is_ok(id));
   assert(!id_is_illegal(id));
   assert(!id_is_loss(id));
   assert(id_is_load(id));

   G_Base[id].load(id);
}

const Base & base_find(int id) {

   assert(id_is_ok(id));
   assert(!id_is_illegal(id));
   assert(!id_is_loss(id));

   return G_Base[id];
}

static bool id_is_load(int id) {

   return !id_is_illegal(id)
       && !id_is_loss(id)
       && is_load(id_wm(id) + id_wk(id), id_bm(id) + id_bk(id));
}

bool pos_is_load(const Pos & pos) {

   return is_load(bit_count(pos.piece(White)), bit_count(pos.piece(Black)));
}

static bool is_load(int nw, int nb) {

   if (!var::BB) return false;

   int n = nw + nb;
   if (n > var::BB_Size) return false;

   return n <= 6;
}

bool pos_is_game(const Pos & pos) { // for game adjudication

   return pos_is_load(pos);
}

bool pos_is_search(const Pos & pos) {

   return pos_size(pos) <= G_Search_Info.bb_size() && pos_is_load(pos);
}

int probe(const Pos & pos) {

   List list;
   gen_captures(list, pos);

   if (list.size() == 0) { // quiet position

      return probe_raw(pos);

   } else { // capture position

      int best_value = Loss;

      for (int i = 0; i < list.size(); i++) {

         move_t mv = list.move(i);

         Pos new_pos(pos, mv);

         int value = value_age(probe(new_pos));

         best_value = value_max(best_value, value);
         if (best_value == Win) break;
      }

      return best_value;
   }
}

int probe_raw(const Pos & pos) {

   assert(!pos_is_capture(pos));

   int id = pos_id(pos);
   assert(!id_is_illegal(id));
   if (id_is_loss(id)) return Loss;

   const Base & base = base_find(id);
   index_t index = pos_index(id, pos);

   int value = base.get(index);
   assert(value != Unknown);

   return value;
}

int value_age(int val) {

   if (val == Win) {
      return Loss;
   } else if (val == Loss) {
      return Win;
   } else {
      return val;
   }
}

int value_max(int v0, int v1) {

   assert(v0 != Win);

   return (Order[v1] > Order[v0]) ? v1 : v0;
}

void Base::load(int id) {

   p_id = id;
   p_size = index_size(id);

   std::string file_name = Dir + id_file(p_id);
   p_index.load(file_name, p_size);
}

std::string value_to_string(int val) {

   switch (val) {
   case Win  : return "win";
   case Loss : return "loss";
   case Draw : return "draw";
   default   : return "unknown";
   }
}

}

// end of bb_base.cpp

