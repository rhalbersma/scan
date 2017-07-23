
// includes

#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bb_comp.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "util.hpp"

namespace bb {

// constants

const int RLE_Size { 255 / 3 };

const int Block_Size { 1 << 8 };

// variables

static int RLE[RLE_Size + 1];

static int Code_Value[256];
static int Code_Length[256];

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

   std::ifstream file(file_name.c_str(), std::ios::binary);

   if (!file) {
      std::cerr << "unable to open file \"" << file_name << "\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   p_size = size;
   load_file(p_table, file);

   // create index table for on-line decompression

   Index table_size = Index(p_table.size());
   Index index_size = (table_size + Block_Size - 1) / Block_Size;

   p_index.clear();
   p_index.reserve(index_size + 1); // sentinel for debug

   Index pos = 0;

   for (Index i = 0; i < table_size; i++) {
      if (i % Block_Size == 0) p_index.push_back(pos);
      pos += Code_Length[p_table[i]];
   }

   if (pos != p_size) {
      std::cerr << "unmatched uncompressed size: " << file_name << ": " << p_size << " -> " << pos << std::endl;
      std::exit(EXIT_FAILURE);
   }

   assert(Index(p_index.size()) == index_size);
   p_index.push_back(p_size); // sentinel for debug
}

int Index_::get(Index pos) const {

   assert(pos < p_size);

   // find the compressed block using the index table

   int low = 0;
   int high = p_index.size() - 1;
   assert(low <= high);

   while (low < high) {

      int mid = (low + high + 1) / 2;
      assert(mid > low && mid <= high);

      if (p_index[mid] <= pos) {
         low = mid;
         assert(low <= high);
      } else {
         high = mid - 1;
         assert(low <= high);
      }
   }

   assert(low == high);
   assert(p_index[low] <= pos);
   assert(p_index[low + 1] > pos);

   // find the value using on-line RLE

   assert(pos >= p_index[low]);
   pos -= p_index[low];

   for (Index i = low * Block_Size; true; i++) {

      int byte = p_table[i];

      int len = Code_Length[byte];
      if (pos < Index(len)) return Code_Value[byte];
      pos -= len;
   }
}

}

