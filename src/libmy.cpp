
// libmy.cpp

// includes

#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "libmy.hpp"

namespace ml {

// functions

// math

void rand_init() {

   std::srand(std::time(NULL));
}

double rand_float() {

   return double(std::rand()) / (double(RAND_MAX) + 1.0);
}

int rand_int(int n) {

   assert(n > 0);
   return int(floor(rand_float() * double(n))); // TODO: remove "floor"?
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

int stoi(const std::string & s) {

   std::stringstream ss(s);
   int n;
   ss >> n;
   return n;   
}

double stof(const std::string & s) {

   std::stringstream ss(s);
   double x;
   ss >> x;
   return x;   
}

std::string itos(int n) {

   std::stringstream ss;
   ss << n;
   return ss.str();
}

std::string ftos(double x) {

   std::stringstream ss;
   ss << x;
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

// end of libmy.cpp

