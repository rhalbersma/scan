
// includes

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "bb_base.hpp"
#include "bb_comp.hpp"
#include "bb_index.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "move_gen.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "var.hpp"

namespace bb {

// constants

const int Value_Size { 4 };

// types

class Base {

private :

   ID p_id;
   Index p_size;
   Index_ p_index;

public :

   void load (ID id);

   ID    id   () const { return p_id; }
   Index size () const { return p_size; }

   Value get (Index index) const { return Value(p_index.get(index)); }
};

// "constants"

const int Order[Value_Size] { 2, 0, 3, 1 }; // DLWU -> LUDW

// variables

static Base G_Base[ID_Size];

// prototypes

static bool is_load (int size);

// functions

void init() {

   std::cout << "init bitbase" << std::endl;

   for (int i = 0; i < ID_Size; i++) {

      ID id = ID(i);

      if (!id_is_illegal(id) && !id_is_loss(id) && is_load(id_size(id))) {
         G_Base[id].load(id);
      }
   }
}

bool pos_is_load(const Pos & pos) {
   return is_load(pos::size(pos));
}

static bool is_load(int size) {
   return var::BB && size <= var::BB_Size;
}

bool pos_is_search(const Pos & pos, int bb_size) {
   return pos::size(pos) <= bb_size && pos_is_load(pos);
}

Value probe(const Pos & pos) {

   if (pos::is_wipe(pos)) return Loss; // for BT variant

   List list;
   gen_captures(list, pos);

   if (list.size() == 0) { // quiet position

      return probe_raw(pos);

   } else { // capture position

      Value node = Loss;

      for (int i = 0; i < list.size(); i++) {

         Move mv = list.move(i);

         Pos new_pos = pos.succ(mv);

         Value child = probe(new_pos);

         node = value_update(node, child);
         if (node == Win) break;
      }

      return node;
   }
}

Value probe_raw(const Pos & pos) {

   assert(!pos::is_capture(pos));

   ID id = pos_id(pos);
   assert(!id_is_illegal(id));
   if (id_is_loss(id)) return Loss;

   const Base & base = G_Base[id];
   Index index = pos_index(id, pos);

   Value value = base.get(index);
   assert(value != Unknown);

   return value;
}

void Base::load(ID id) {

   p_id = id;
   p_size = index_size(id);

   std::string file_name = std::string("data/bb") + var::variant("", "_killer", "_bt") + "/" + id_file(p_id);
   p_index.load(file_name, p_size);
}

Value value_update(Value node, Value child) {
   return value_max(node, value_age(child));
}

Value value_age(Value val) {

   if (val == Win) {
      return Loss;
   } else if (val == Loss) {
      return Win;
   } else {
      return val;
   }
}

Value value_max(Value v0, Value v1) {
   assert(v0 != Win);
   return (Order[v1] > Order[v0]) ? v1 : v0;
}

int value_nega(Value val, Side sd) {

   assert(val != Unknown);

   if (val == Win) {
      return score::side(+1, sd);
   } else if (val == Loss) {
      return score::side(-1, sd);
   } else { // draw or unknown
      return 0;
   }
}

std::string value_to_string(Value val) {

   switch (val) {
      case Win  : return "win";
      case Loss : return "loss";
      case Draw : return "draw";
      default   : return "unknown";
   }
}

}

