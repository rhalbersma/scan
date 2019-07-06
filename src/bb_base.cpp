
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
#include "gen.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "var.hpp"

namespace bb {

// constants

const int Value_Size {4};

// types

class Base {

private:

   ID m_id;
   Index m_size;
   Index_ m_index;

public:

   void load (ID id);

   ID    id   () const { return m_id; }
   Index size () const { return m_size; }

   int operator [] (Index index) const { return m_index[index]; }
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

      if (!id_is_illegal(id) && !id_is_end(id) && is_load(id_size(id))) {
         G_Base[id].load(id);
      }
   }
}

static bool is_load(int size) {
   return var::BB && size <= var::BB_Size;
}

bool pos_is_load(const Pos & pos) {
   return is_load(pos::size(pos));
}

bool pos_is_search(const Pos & pos, int bb_size) {
   return pos::size(pos) <= bb_size && pos_is_load(pos);
}

int probe(const Pos & pos) {

   if (pos::is_wipe(pos)) return value_from_nega(pos::result(pos, pos.turn())); // for BT variant

   List list;
   gen_captures(list, pos);

   if (list.size() == 0) { // quiet position

      return probe_raw(pos);

   } else { // capture position

      int node = Loss;

      for (Move mv : list) {
         node = value_update(node, probe(pos.succ(mv)));
         if (node == Win) break;
      }

      return node;
   }
}

int probe_raw(const Pos & pos) {

   assert(!pos::is_capture(pos));

   ID id = pos_id(pos);
   assert(!id_is_illegal(id));
   if (id_is_end(id)) return (var::Variant == var::Losing) ? Win : Loss;

   const Base & base = G_Base[id];
   Index index = pos_index(id, pos);

   int value = base[index];
   assert(value != Unknown);

   return value;
}

void Base::load(ID id) {

   m_id = id;
   m_size = index_size(id);

   std::string file_name = std::string("data/bb") + var::variant_name() + "/" + std::to_string(id_size(id)) + "/" + id_name(id);
   m_index.load(file_name, m_size);
}

int value_update(int node, int child) {
   return value_max(node, value_age(child));
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

int value_nega(int val, Side sd) {

   assert(val != Unknown);

   if (val == Win) {
      return score::side(+1, sd);
   } else if (val == Loss) {
      return score::side(-1, sd);
   } else { // draw or unknown
      return 0;
   }
}

int value_from_nega(int val) {

   if (val > 0) {
      return Win;
   } else if (val < 0) {
      return Loss;
   } else {
      return Draw;
   }
}

std::string value_to_string(int val) {

   switch (val) {
      case Win :  return "win";
      case Loss : return "loss";
      case Draw : return "draw";
      default :   return "unknown";
   }
}

} // namespace bb

