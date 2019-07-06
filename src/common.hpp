
#ifndef COMMON_HPP
#define COMMON_HPP

// includes

#include <string>

#include "libmy.hpp"

// constants

const int File_Size {10};
const int Rank_Size {10};
const int Dense_Size {File_Size * Rank_Size / 2};
const int Square_Size {63}; // 13x10 board
const int Dir_Size {8}; // HACK for Frisian draughts
const int Side_Size {2};
const int Piece_Size {2};
const int Piece_Side_Size {4}; // excludes Empty #

const int Move_Index_Size {1 << 12};

const int Stage_Size {300};

const std::string Engine_Name    {"Scan"};
const std::string Engine_Version {"3.1"};

// types

enum Square : int;

enum Inc : int {
   I1 = 6, J1 = 7,
   I2 = I1 * 2, J2 = J1 * 2,
   K1 = 1, L1 = I1 + J1,
   K2 = K1 * 2, L2 = L1 * 2,
};

enum Side  : int { White, Black };
enum Piece : int { Man, King };

enum Piece_Side : int {
   White_Man,
   Black_Man,
   White_King,
   Black_King,
   Empty,
};

enum class Key  : uint64;
enum class Move : uint64;

enum Move_Index : int { Move_Index_None = 0 };

enum Depth : int;
enum Ply   : int;
enum Score : int;

class Bit {

private:

   uint64 m_bit {0};

public:

   Bit () = default;
   explicit Bit (uint64 b) { assert((b & ~0x7DF3EF9F7CFBE7DF) == 0); m_bit = b; }

   operator uint64 () const { return m_bit; }

   void operator |= (Bit    b) { m_bit |= uint64(b); }
   void operator &= (uint64 b) { m_bit &= b; }
   void operator ^= (Bit    b) { m_bit ^= uint64(b); }

   Bit begin () const { return *this; }
   Bit end   () const { return Bit(0); }

   Square operator *  () const;
   void   operator ++ ();
};

// operators

inline Inc operator + (Inc inc) { return Inc(+int(inc)); }
inline Inc operator - (Inc inc) { return Inc(-int(inc)); }

inline void operator ^= (Key & k0, Key k1) { k0 = Key(uint64(k0) ^ uint64(k1)); }

inline Depth operator + (Depth d0, Depth d1) { return Depth(int(d0) + int(d1)); }
inline Depth operator - (Depth d0, Depth d1) { return Depth(int(d0) - int(d1)); }

inline void operator += (Depth & d0, Depth d1) { d0 = d0 + d1; }
inline void operator -= (Depth & d0, Depth d1) { d0 = d0 - d1; }

inline Ply operator + (Ply p0, Ply p1) { return Ply(int(p0) + int(p1)); }
inline Ply operator - (Ply p0, Ply p1) { return Ply(int(p0) - int(p1)); }

inline void operator += (Ply & p0, Ply p1) { p0 = p0 + p1; }
inline void operator -= (Ply & p0, Ply p1) { p0 = p0 - p1; }

inline Score operator + (Score sc) { return Score(+int(sc)); }
inline Score operator - (Score sc) { return Score(-int(sc)); }

inline Score operator + (Score s0, Score s1) { return Score(int(s0) + int(s1)); }
inline Score operator - (Score s0, Score s1) { return Score(int(s0) - int(s1)); }

inline void operator += (Score & s0, Score s1) { s0 = s0 + s1; }
inline void operator -= (Score & s0, Score s1) { s0 = s0 - s1; }

inline Bit operator | (Bit b0, Bit    b1) { return Bit(uint64(b0) | uint64(b1)); }
inline Bit operator & (Bit b0, uint64 b1) { return Bit(uint64(b0) & b1); }
inline Bit operator ^ (Bit b0, Bit    b1) { return Bit(uint64(b0) ^ uint64(b1)); }

// functions

bool   square_is_light (int fl, int rk);
bool   square_is_dark  (int fl, int rk);
bool   square_is_ok    (int fl, int rk);
bool   square_is_ok    (int sq);
Square square_make     (int fl, int rk);
Square square_make     (int sq);

Square square_sparse   (int dense);
int    square_dense    (Square sq);
Square square_from_std (int std);
int    square_to_std   (Square sq);
int    square_file     (Square sq);
int    square_rank     (Square sq);
Square square_opp      (Square sq);

int  square_rank         (Square sq, Side sd);
bool square_is_promotion (Square sq, Side sd);

std::string square_to_string   (Square sq);
bool        string_is_square   (const std::string & s);
Square      square_from_string (const std::string & s);

Inc  inc_make (int inc);
Inc  dir_inc  (int dir);

Side side_make (int sd);
Side side_opp  (Side sd);

std::string side_to_string (Side sd);

Piece_Side piece_side_make     (int ps);
bool       piece_side_is_piece (Piece_Side ps, Piece pc);
bool       piece_side_is_side  (Piece_Side ps, Side sd);

#endif // !defined COMMON_HPP

