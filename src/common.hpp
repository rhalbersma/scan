
#ifndef COMMON_HPP
#define COMMON_HPP

// includes

#include <string>

#include "libmy.hpp"

// constants

const int Line_Size { 10 };
const int Dense_Size { Line_Size * Line_Size / 2 };
const int Square_Size { 63 };
const int Dir_Size { 4 };
const int Side_Size { 2 };
const int Piece_Size { 2 };
const int Piece_Side_Size { 4 }; // excludes Empty #

const int Move_Index_Size { 1 << 12 };

const int Stage_Size { 300 };

const std::string Engine_Name { "Scan" };
const std::string Engine_Version { "3.0" };

// types

enum Square : int;

enum class Dir : int;
enum Inc : int {
   I1 = 6, J1 = 7,
   I2 = I1 * 2, J2 = J1 * 2,
};

enum Side { White, Black };
enum Piece { Man, King };

enum Piece_Side {
   White_Man,
   Black_Man,
   White_King,
   Black_King,
   Empty,
};

enum class Key  : uint64;
enum class Move : uint64;

enum Move_Index { Move_Index_None = 0 };

enum Trit {
   Trit_White = -1,
   Trit_Empty = 0,
   Trit_Black = +1,
};

enum Depth : int;
enum Ply   : int;
enum Score : int;

class Bit {

private :

   uint64 p_bit;

public :

   Bit () { p_bit = 0; }
   explicit Bit (uint64 b) { assert((b & ~U64(0x7DF3EF9F7CFBE7DF)) == 0); p_bit = b; }

   operator uint64 () const { return p_bit; }

   void operator |= (Bit b)    { p_bit |= uint64(b); }
   void operator &= (uint64 b) { p_bit &= b; }
   void operator ^= (Bit b)    { p_bit ^= uint64(b); }
};

// operators

inline Inc operator + (Inc sc) { return Inc(+int(sc)); }
inline Inc operator - (Inc sc) { return Inc(-int(sc)); }

inline void operator ^= (Key & k0, Key k1) { k0 = Key(uint64(k0) ^ uint64(k1)); }

inline Depth operator + (Depth d0, Depth d1) { return Depth(int(d0) + int(d1)); }
inline Depth operator - (Depth d0, Depth d1) { return Depth(int(d0) - int(d1)); }

inline void operator += (Depth & d0, Depth d1) { d0 = d0 + d1; }
inline void operator -= (Depth & d0, Depth d1) { d0 = d0 - d1; }

inline Ply  operator + (Ply p0, Ply p1) { return Ply(int(p0) + int(p1)); }
inline Ply  operator - (Ply p0, Ply p1) { return Ply(int(p0) - int(p1)); }

inline void operator += (Ply & p0, Ply p1) { p0 = p0 + p1; }
inline void operator -= (Ply & p0, Ply p1) { p0 = p0 - p1; }

inline Score operator + (Score sc) { return Score(+int(sc)); }
inline Score operator - (Score sc) { return Score(-int(sc)); }

inline Score operator + (Score s0, Score s1) { return Score(int(s0) + int(s1)); }
inline Score operator - (Score s0, Score s1) { return Score(int(s0) - int(s1)); }

inline void operator += (Score & s0, Score s1) { s0 = s0 + s1; }
inline void operator -= (Score & s0, Score s1) { s0 = s0 - s1; }

inline Bit  operator | (Bit b0, Bit    b1) { return Bit(uint64(b0) | uint64(b1)); }
inline Bit  operator & (Bit b0, uint64 b1) { return Bit(uint64(b0) & b1); }
inline Bit  operator ^ (Bit b0, Bit    b1) { return Bit(uint64(b0) ^ uint64(b1)); }

// functions

void common_init ();

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

Piece_Side piece_side_make  (int ps);
Piece      piece_side_piece (Piece_Side ps);
Side       piece_side_side  (Piece_Side ps);

#endif // !defined COMMON_HPP

