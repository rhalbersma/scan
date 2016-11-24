
// libmy.hpp

#ifndef LIBMY_HPP
#define LIBMY_HPP

// includes

#include <iostream>
#include <string>

// constants

#undef FALSE
#define FALSE 0

#undef TRUE
#define TRUE 1

#ifdef DEBUG
#  undef DEBUG
#  define DEBUG TRUE
#else
#  define DEBUG FALSE
#endif

#ifdef _MSC_VER
#  define I64_FORMAT "%I64d"
#  define U64_FORMAT "%016I64X"
#else
#  define I64_FORMAT "%lld"
#  define U64_FORMAT "%016llX"
#endif

// macros

#if DEBUG
#  undef NDEBUG
#else
#  define NDEBUG
#endif

#include <cassert> // needs NDEBUG

#ifdef _MSC_VER
#  define I64(n) (n##i64)
#  define U64(u) (u##ui64)
#else
#  define I64(n) (n##LL)
#  define U64(u) (u##ULL)
#endif

// types

typedef signed   char int8;
typedef unsigned char uint8;

typedef signed   short int16;
typedef unsigned short uint16;

typedef signed   int int32;
typedef unsigned int uint32;

typedef signed   long long int int64;
typedef unsigned long long int uint64;

// classes

namespace ml {

   // array

   template <class T, int SIZE> class Array {

   private:

      int p_size;
      T p_item[SIZE];

      void copy(const Array<T, SIZE> & array) {

         int size = array.p_size;

         p_size = size;

         for (int pos = 0; pos < size; pos++) {
            p_item[pos] = array.p_item[pos];
         }
      }

   public:

      Array ()                             { clear(); }
      Array (const Array<T, SIZE> & array) { copy(array); }

      void operator= (const Array<T, SIZE> & array) { copy(array); }

      void clear ()       { p_size = 0; }
      void add   (T item) { assert(!full()); p_item[p_size++] = item; }

      T    remove   ()         { assert(!empty()); return p_item[--p_size]; }
      void set_size (int size) { assert(size <= SIZE); p_size = size; }

      bool empty () const { return p_size == 0; }
      bool full  () const { return p_size == SIZE; }
      int  size  () const { return p_size; }

      const T & operator[] (int pos) const { return p_item[pos]; }
      T &       operator[] (int pos)       { return p_item[pos]; } // direct access!
   };
}

// functions

namespace ml {

   // math

   void   rand_init  ();
   double rand_float ();
   int    rand_int   (int n);
   bool   rand_bool  (double p);

   int    round (double x);

   int    div       (int a, int b);
   int    div_round (int a, int b);

   bool   is_power_2 (int64 n);

   // stream

   int64 stream_size (std::istream & stream);

   int    get_byte  (std::istream & stream);
   uint64 get_bytes (std::istream & stream, int size);

   // string

   int         stoi (const std::string & s);
   double      stof (const std::string & s);
   std::string itos (int n);
   std::string ftos (double x);

   std::string trim (const std::string & s);
}

#endif // !defined LIBMY_HPP

// end of libmy.hpp

