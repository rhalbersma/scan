
// libmy.cpp

// includes

#include <cmath>
#include <cctype>
#include <cstdio>
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

// functions

// math

void rand_init() {
   std::srand(std::time(nullptr));
}

double rand_float() {
   return double(std::rand()) / (double(RAND_MAX) + 1.0);
}

uint64 rand_int_64() {
   static std::mt19937_64 gen;
   return gen();
}

bool rand_bool(double p) {
   assert(p >= 0.0 && p <= 1.0);
   return rand_float() < p;
}

int round(double x) {
   return int(floor(x + 0.5));
}

int div(int a, int b) {

   assert(b > 0);

   if (b <= 0) {
      std::cerr << "ml::div(): divide error" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   int div = a / b;
   if (a < 0 && a != b * div) div--; // fix buggy C semantics

   return div;
}

int div_round(int a, int b) {
   assert(b > 0);
   return div(a + b / 2, b);
}

bool is_power_2(int64 n) {
   assert(n >= 0);
   return (n & (n - 1)) == 0 && n != 0;
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

   int begin = 0;

   while (begin < int(s.size()) && std::isspace(s[begin])) {
      begin++;
   }

   int end = int(s.size());

   while (end > begin && std::isspace(s[end - 1])) {
      end--;
   }

   assert(begin <= end);

   return s.substr(begin, end - begin);
}

}

