
// bit.h

#ifndef BIT_H
#define BIT_H

// includes

#include "libmy.hpp"

class Pos;

// types

typedef uint64 bit_t;

// constants

const bit_t Bit_Squares = U64(0x0FFDFFBFF7FEFFC0); // legal squares

// functions

void bit_init ();

inline bit_t bit      (int n) { return bit_t(U64(1) << n); }
inline bit_t bit_mask (int n) { return bit(n) - 1; }

inline void  bit_set    (bit_t & b, int n) { b |=  bit(n); }
inline void  bit_clear  (bit_t & b, int n) { b &= ~bit(n); }
inline void  bit_switch (bit_t & b, int n) { b ^=  bit(n); }

inline int   bit      (bit_t b, int n) { return (b >> n) & 1; }
inline bool  bit_test (bit_t b, int n) { return (b & bit(n)) != 0; }

inline bit_t bit_add    (bit_t b, int n) { assert(!bit_test(b, n)); return b ^ bit(n); }
inline bit_t bit_remove (bit_t b, int n) { assert( bit_test(b, n)); return b ^ bit(n); }

inline bool  bit_incl (bit_t b0, bit_t b1) { return (b0 & ~b1) == 0; }

#ifdef _MSC_VER

#include <intrin.h>
inline int bit_first (bit_t b) { assert(b != 0); unsigned long n; _BitScanForward64(&n, b); return n; }
inline int bit_count (bit_t b) { return int(__popcnt64(b)); }

#else // assume GCC

inline int bit_first (bit_t b) { assert(b != 0); return __builtin_ctzll(b); }
inline int bit_count (bit_t b) { return __builtin_popcountll(b); }

#endif

inline bit_t bit_rest (bit_t b) { assert(b != 0); return bit_t(b & (b - 1)); }

extern bit_t bit_file (int fl);
extern bit_t bit_rank (int rk);

extern bit_t bit_rev (bit_t b);

extern int   line_inc    (int from, int to);
extern bit_t bit_between (int from, int to);

extern bit_t king_attack_one    (int from);
extern bit_t king_attack_almost (int from);
extern bit_t king_attack        (int from, bit_t blockers);

#endif // !defined BIT_H

// end of bit.h

