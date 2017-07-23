
#ifndef LIBMY_HPP
#define LIBMY_HPP

// includes

#include <cstdint>
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

typedef std::int8_t  int8;
typedef std::int16_t int16;
typedef std::int32_t int32;
typedef long long int int64;

typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef unsigned long long int uint64;

// classes

namespace ml {

   // array

   template <class T, int Size> class Array {

   private:

      int p_size;
      T p_item[Size];

      void copy(const Array<T, Size> & array) {

         int size = array.p_size;

         p_size = size;

         for (int pos = 0; pos < size; pos++) {
            p_item[pos] = array.p_item[pos];
         }
      }

   public:

      Array ()                             { clear(); }
      Array (const Array<T, Size> & array) { copy(array); }

      void operator= (const Array<T, Size> & array) { copy(array); }

      void clear   ()               { p_size = 0; }
      void add     (T item)         { assert(!full()); p_item[p_size++] = item; }
      void add_ref (const T & item) { assert(!full()); p_item[p_size++] = item; }

      T    remove   ()         { assert(!empty()); return p_item[--p_size]; }
      void set_size (int size) { assert(size <= Size); p_size = size; }

      bool empty () const { return p_size == 0; }
      bool full  () const { return p_size == Size; }
      int  size  () const { return p_size; }

      const T & operator[] (int pos) const { assert(pos < p_size); return p_item[pos]; }
      T &       operator[] (int pos)       { assert(pos < p_size); return p_item[pos]; } // direct access!
   };
}

// functions

namespace ml {

   // math

   void   rand_init   ();
   double rand_float  ();
   uint64 rand_int_64 ();
   bool   rand_bool   (double p);

   int  round (double x);

   int  div       (int a, int b);
   int  div_round (int a, int b);

   bool is_power_2 (int64 n);

   inline uint64 bit       (int n) { return uint64(1) << n; }
   inline uint64 bit_mask  (int n) { return bit(n) - 1; }

#ifdef _MSC_VER
   inline int bit_first (uint64 b) { assert(b != 0); unsigned long i; _BitScanForward64(&i, b); return i; }
   inline int bit_count (uint64 b) { return int(__popcnt64(b)); }
#else
   inline int bit_first (uint64 b) { assert(b != 0); return __builtin_ctzll(b); }
   inline int bit_count (uint64 b) { return __builtin_popcountll(b); }
#endif

   int64 stream_size (std::istream & stream);

   int    get_byte  (std::istream & stream);
   uint64 get_bytes (std::istream & stream, int size);

   // string

   std::string ftos (double x, int decimals);
   std::string trim (const std::string & s);
}

#endif // !defined LIBMY_HPP

