
#ifndef LIBMY_HPP
#define LIBMY_HPP

// includes

#include <cassert> // uses NDEBUG
#include <cstdint>
#include <iostream>
#include <string>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// types

using int8  = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using uint8  = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

namespace ml {

// types

template <typename T, int Size> class Array {

private:

   int m_size {0};
   T m_elem[Size];

public:

   Array () = default;

   Array           (const Array<T, Size> & array) { copy(array); }
   void operator = (const Array<T, Size> & array) { copy(array); }

   void clear ()               { m_size = 0; }
   void add   (const T & elem) { assert(!full()); m_elem[m_size++] = elem; }

   T    remove   ()         { assert(!empty()); return m_elem[--m_size]; }
   void set_size (int size) { assert(size <= Size); m_size = size; }

   bool empty () const { return m_size == 0; }
   bool full  () const { return m_size == Size; }
   int  size  () const { return m_size; }

   const T & operator [] (int i) const { assert(i >= 0 && i < m_size); return m_elem[i]; }
   T &       operator [] (int i)       { assert(i >= 0 && i < m_size); return m_elem[i]; } // direct access!

   const T * begin () const { return &m_elem[0]; }
   const T * end   () const { return &m_elem[m_size]; }

private:

   void copy(const Array<T, Size> & array) {

      m_size = array.m_size;

      for (int i = 0; i < array.m_size; i++) {
         m_elem[i] = array.m_elem[i];
      }
   }
};

// functions

// math

void   rand_init   ();
uint64 rand_int_64 ();
bool   rand_bool   (double p);

int round (double x);

int div_round (int a, int b);

inline uint64 bit      (int n) { return uint64(1) << n; }
inline uint64 bit_mask (int n) { return bit(n) - 1; }

#ifdef _MSC_VER
inline int bit_first (uint64 b) { assert(b != 0); unsigned long i; _BitScanForward64(&i, b); return i; }
inline int bit_count (uint64 b) { return int(__popcnt64(b)); }
#else
inline int bit_first (uint64 b) { assert(b != 0); return __builtin_ctzll(b); }
inline int bit_count (uint64 b) { return __builtin_popcountll(b); }
#endif

// stream

int64 stream_size (std::istream & stream);

int    get_byte  (std::istream & stream);
uint64 get_bytes (std::istream & stream, int size);

// string

std::string ftos (double x, int decimals);
std::string trim (const std::string & s);

} // namespace ml

#endif // !defined LIBMY_HPP

