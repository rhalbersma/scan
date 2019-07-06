
// includes

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "bb_comp.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "util.hpp"

namespace bb {

// constants

const int RLE_Size {255 / 3};

const Index Block_Size {1 << 8};

// variables

static Index RLE[RLE_Size + 1];

static int Code_Value[256];
static Index Code_Length[256];

// functions

void comp_init() {

   for (int i = 0; i < RLE_Size; i++) {
      RLE[i] = ml::round(std::pow(1.2, double(i)));
   }

   for (int byte = 0; byte < 256; byte++) {
      Code_Value[byte]  = byte % 3;
      Code_Length[byte] = RLE[byte / 3];
   }
}

void Index_::load(const std::string & file_name, Index size) {

   std::ifstream file(file_name, std::ios::binary);

   if (!file) {
      std::cerr << "unable to open file \"" << file_name << "\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   m_size = size;
   load_file(m_table, file);

   // create index table for on-line decompression

   Index table_size = Index(m_table.size());
   Index index_size = (table_size + Block_Size - 1) / Block_Size;

   m_index.clear();
   m_index.reserve(index_size + 1); // sentinel for debug

   Index pos = 0;

   for (Index i = 0; i < table_size;) {

      assert(i % Block_Size == 0);
      m_index.push_back(pos);

      Index next = std::min(i + Block_Size, table_size);

      for (; i < next; i++) {
         pos += Code_Length[m_table[i]];
      }
   }

   if (pos != m_size) {
      std::cerr << "unmatched uncompressed size: " << file_name << ": " << m_size << " -> " << pos << std::endl;
      std::exit(EXIT_FAILURE);
   }

   assert(Index(m_index.size()) == index_size);
   m_index.push_back(m_size); // sentinel for debug
}

int Index_::operator[](Index pos) const {

   assert(pos < m_size);

   // find the compressed block using the index table

   Index low = 0;
   Index high = m_index.size() - 1;
   assert(low <= high);

   while (low < high) {

      Index mid = (low + high + 1) / 2;
      assert(mid > low && mid <= high);

      if (m_index[mid] <= pos) {
         low = mid;
         assert(low <= high);
      } else {
         high = mid - 1;
         assert(low <= high);
      }
   }

   assert(low == high);
   assert(m_index[low] <= pos);
   assert(m_index[low + 1] > pos);

   // find the value using on-line RLE

   assert(pos >= m_index[low]);
   pos -= m_index[low];

   for (Index i = low * Block_Size; true; i++) {

      int byte = m_table[i];

      Index len = Code_Length[byte];
      if (pos < len) return Code_Value[byte];
      pos -= len;
   }
}

} // namespace bb

