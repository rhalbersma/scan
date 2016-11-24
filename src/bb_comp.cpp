
// bb_comp.cpp

// includes

#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "bb_base.h"
#include "bb_comp.h"
#include "libmy.hpp"
#include "util.h"

namespace bb {

// constants

static const int RLE_Size = 85; // 255 / 3

static const int Block_Size = 1 << 8;

// variables

static int RLE[RLE_Size];

static int Code_Value[256];
static int Code_Length[256];

// prototypes

static int code_value  (int byte);
static int code_length (int byte);

// functions

void comp_init() {

   for (int i = 0; i < RLE_Size; i++) {
      RLE[i] = ml::round(std::pow(1.2, double(i)));
   }

   for (int byte = 0; byte < 256; byte++) {
      Code_Value[byte]  = code_value(byte);
      Code_Length[byte] = code_length(byte);
   }
}

static int code_value(int byte) {

   assert(byte >= 0 && byte < 256);
   return byte % 3;
}

static int code_length(int byte) {

   assert(byte >= 0 && byte < 256);
   return RLE[byte / 3];
}

void Index::load(const std::string & file_name, index_t size) {

   std::ifstream file(file_name.c_str(), std::ios::binary);

   if (!file) {
      std::cerr << "unable to open file \"" << file_name << "\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   p_size = size;
   load_file(p_table, file);

   // create index table for on-line decompression

   index_t table_size = index_t(p_table.size());
   index_t index_size = (table_size + Block_Size - 1) / Block_Size;

   p_index.clear();
   p_index.reserve(index_size + 1); // sentinel for debug

   index_t pos = 0;

   for (index_t i = 0; i < table_size; i++) {
      if (i % Block_Size == 0) p_index.push_back(pos);
      pos += code_length(p_table[i]);
   }

   if (pos != p_size) {
      std::cerr << "unmatched uncompressed size: " << file_name << ": " << p_size << " -> " << pos << std::endl;
      std::exit(EXIT_FAILURE);
   }

   assert(index_t(p_index.size()) == index_size);
   p_index.push_back(p_size); // sentinel for debug
}

int Index::get(index_t pos) const {

   assert(pos < p_size);

   // find the compressed block using the index table

   int low = 0;
   int high = int(p_index.size()) - 1;
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

   for (index_t i = low * Block_Size; true; i++) {

      int byte = p_table[i];

      int len = Code_Length[byte];
      if (pos < index_t(len)) return Code_Value[byte];
      pos -= len;
   }
}

}

// end of bb_comp.cpp

