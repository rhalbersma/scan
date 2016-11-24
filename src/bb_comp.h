
// bb_comp.h

#ifndef BB_COMP_H
#define BB_COMP_H

// includes

#include <string>
#include <vector>

#include "bb_index.h"
#include "libmy.hpp"

namespace bb {

// classes

class Index {

private :

   index_t p_size;
   std::vector<uint8> p_table;
   std::vector<index_t> p_index;

public :

   void load (const std::string & file_name, index_t size);

   index_t size () const { return p_size; }
   int get (index_t pos) const;
};

// functions

extern void comp_init ();

}

#endif // !defined BB_COMP_H

// end of bb_comp.h

