
#ifndef BIT_HPP
#define BIT_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

namespace bit {

// constants

const Bit Squares    {0x7DF3EF9F7CFBE7DF}; // legal squares
const Bit WM_Squares {0x7DF3EF9F7CFBE7C0};
const Bit BM_Squares {0x01F3EF9F7CFBE7DF};

const Bit Ranks_012 {0x000000000003E7DF};
const Bit Ranks_345 {0x0000001F7CF80000};
const Bit Ranks_678 {0x01F3EF8000000000};
const Bit Ranks_123 {0x0000000000FBE7C0};
const Bit Ranks_456 {0x00000F9F7C000000};
const Bit Ranks_789 {0x7DF3E00000000000};

// functions

void init ();

inline bool is_ok (uint64 b) { return (b & ~0x7DF3EF9F7CFBE7DF) == 0; }

inline Bit bit (Square sq) { return Bit(uint64(1) << sq); }

inline bool has (Bit b, Square sq) { return (b & bit(sq)) != 0; }
inline int  bit (Bit b, Square sq) { return (b >> sq) & 1; }

inline bool is_incl (Bit b0, Bit b1) { return (b0 & ~b1) == 0; }

inline void set   (Bit & b, Square sq) { b |=  bit(sq); }
inline void clear (Bit & b, Square sq) { b &= ~bit(sq); }

inline Bit add    (Bit b, Square sq) { assert(!has(b, sq)); return b ^ bit(sq); }
inline Bit remove (Bit b, Square sq) { assert( has(b, sq)); return b ^ bit(sq); }

inline Square first (Bit b) { assert(b != 0); return Square(ml::bit_first(b)); }
inline Bit    rest  (Bit b) { assert(b != 0); return b & (b - 1); }
inline int    count (Bit b) { return ml::bit_count(b); }

Bit file (int fl);
Bit rank (int rk);
Bit rank (int rk, Side sd);

Bit capture_mask (Square from, Square to);
Bit beyond       (Square from, Square to);
Inc line_inc     (Square from, Square to);

Bit man_moves     (Square from);
Bit man_captures  (Square from);
Bit king_captures (Square from);

Bit king_moves    (Square from, Bit empty);
Bit king_captures (Square from, Bit empty);
Bit attack        (Square from, Bit tos, Bit empty);

} // namespace bit

#endif // !defined BIT_HPP

