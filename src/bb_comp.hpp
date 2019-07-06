
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

class Index_ { // "Index" is already taken

private:

   Index m_size;
   std::vector<uint8> m_table;
   std::vector<Index> m_index;

public:

   void load (const std::string & file_name, Index size);

   Index size        ()          const { return m_size; }
   int   operator [] (Index pos) const;
};

// functions

void comp_init ();

} // namespace bb

#endif // !defined BB_COMP_HPP

