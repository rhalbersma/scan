
// includes

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "libmy.hpp"

namespace ml {

// variables

static std::mt19937_64 gen;

// functions

// math

void rand_init() {
   gen.seed(std::random_device{}());
}

uint64 rand_int_64() {
   return gen();
}

bool rand_bool(double p) {
   assert(p >= 0.0 && p <= 1.0);
   return std::bernoulli_distribution{p}(gen);
}

int round(double x) {
   return int(std::floor(x + 0.5));
}

int div_round(int a, int b) {

   assert(b > 0);

   a += b / 2;

   int div = a / b;
   if (a < 0 && a != b * div) div -= 1; // fix buggy C semantics

   return div;
}

// stream

int64 stream_size(std::istream & stream) {

   assert(stream.tellg() == 0);

   stream.seekg(0, std::ios::end);
   int64 size = stream.tellg();
   stream.seekg(0);

   return size;
}

int get_byte(std::istream & stream) {

   char c;

   if (!stream.get(c)) {
      std::cerr << "error while reading file" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   return int(c & 0xFF); // HACK
}

uint64 get_bytes(std::istream & stream, int size) {

   assert(size >= 0 && size <= 8);

   uint64 bytes = 0;

   for (int i = 0; i < size; i++) {
      bytes = (bytes << 8) | get_byte(stream);
   }

   return bytes;
}

// string

std::string ftos(double x, int decimals) {
   std::stringstream ss;
   ss << std::fixed << std::setprecision(decimals) << x;
   return ss.str();
}

std::string trim(const std::string & s) {

   int end = int(s.size());

   while (end > 0 && std::isspace(s[end - 1])) {
      end--;
   }

   return s.substr(0, end);
}

} // namespace ml

