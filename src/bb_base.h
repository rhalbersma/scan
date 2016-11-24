
// bb_base.h

#ifndef BB_BASE_H
#define BB_BASE_H

// includes

#include <string>

#include "bb_comp.h"
#include "bb_index.h"
#include "libmy.hpp"

class Pos;

namespace bb {

// constants

const std::string Dir = "data/bb/";

const int Draw       = 0;
const int Loss       = 1;
const int Win        = 2;
const int Unknown    = 3;
const int Value_Size = 4;

// classes

class Base {

private :

   int p_id;
   index_t p_size;
   Index p_index;

public :

   void load (int id);

   int     id   () const { return p_id; }
   index_t size () const { return p_size; }

   int get (index_t index) const { return p_index.get(index); }
};

// functions

extern void init ();
extern void load (int id);

extern const Base & base_find (int id);

extern bool pos_is_load   (const Pos & pos);
extern bool pos_is_game   (const Pos & pos);
extern bool pos_is_search (const Pos & pos);

extern int probe     (const Pos & pos);
extern int probe_raw (const Pos & pos);

extern int value_age (int v);
extern int value_max (int v0, int v1);

extern std::string value_to_string (int val);

}

#endif // !defined BB_BASE_H

// end of bb_base.h

