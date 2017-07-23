
#ifndef BB_COMP_HPP
#define BB_COMP_HPP

// includes

#include <string>
#include <vector>

#include "bb_index.hpp"
#include "common.hpp"
#include "libmy.hpp"

namespace bb {

// types

class Index_ {

private :

   Index p_size;
   std::vector<uint8> p_table;
   std::vector<Index> p_index;

public :

   void load (const std::string & file_name, Index size);

   Index size ()          const { return p_size; }
   int   get  (Index pos) const;
};

// functions

void comp_init ();

}

#endif // !defined BB_COMP_HPP

