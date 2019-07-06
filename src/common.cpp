
// includes

#include <string>

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "util.hpp"

// "constants"

const int Square_Sparse[Dense_Size] {
      0,  1,  2,  3,  4,
    6,  7,  8,  9, 10,
     13, 14, 15, 16, 17,
   19, 20, 21, 22, 23,
     26, 27, 28, 29, 30,
   32, 33, 34, 35, 36,
     39, 40, 41, 42, 43,
   45, 46, 47, 48, 49,
     52, 53, 54, 55, 56,
   58, 59, 60, 61, 62,
};

const int Square_Dense[Square_Size] {
      0,  1,  2,  3,  4, -1,
    5,  6,  7,  8,  9, -1, -1,
     10, 11, 12, 13, 14, -1,
   15, 16, 17, 18, 19, -1, -1,
     20, 21, 22, 23, 24, -1,
   25, 26, 27, 28, 29, -1, -1,
     30, 31, 32, 33, 34, -1,
   35, 36, 37, 38, 39, -1, -1,
     40, 41, 42, 43, 44, -1,
   45, 46, 47, 48, 49,
};

const int Square_File[Square_Size] {
      1,  3,  5,  7,  9, -1,
    0,  2,  4,  6,  8, -1, -1,
      1,  3,  5,  7,  9, -1,
    0,  2,  4,  6,  8, -1, -1,
      1,  3,  5,  7,  9, -1,
    0,  2,  4,  6,  8, -1, -1,
      1,  3,  5,  7,  9, -1,
    0,  2,  4,  6,  8, -1, -1,
      1,  3,  5,  7,  9, -1,
    0,  2,  4,  6,  8,
};

const int Square_Rank[Square_Size] {
      0,  0,  0,  0,  0, -1,
    1,  1,  1,  1,  1, -1, -1,
      2,  2,  2,  2,  2, -1,
    3,  3,  3,  3,  3, -1, -1,
      4,  4,  4,  4,  4, -1,
    5,  5,  5,  5,  5, -1, -1,
      6,  6,  6,  6,  6, -1,
    7,  7,  7,  7,  7, -1, -1,
      8,  8,  8,  8,  8, -1,
    9,  9,  9,  9,  9,
};

const Inc Dir_Inc[Dir_Size] {
   -J1, -I1, +I1, +J1,
   -L1, -K1, +K1, +L1, // for Frisian draughts
};

// functions

bool square_is_ok(int fl, int rk) {
   return (fl >= 0 && fl < File_Size)
       && (rk >= 0 && rk < Rank_Size)
       && square_is_dark(fl, rk);
}

bool square_is_ok(int sq) {
   return (sq >= 0 && sq < Square_Size) && Square_Dense[sq] >= 0;
}

bool square_is_light(int fl, int rk) {
   return !square_is_dark(fl, rk);
}

bool square_is_dark(int fl, int rk) {
   return (fl + rk) % 2 != 0;
}

Square square_make(int fl, int rk) {
   assert(square_is_ok(fl, rk));
   int dense = (rk * File_Size + fl) / 2;
   return square_sparse(dense);
}

Square square_make(int sq) {
   assert(square_is_ok(sq));
   return Square(sq);
}

Square square_sparse(int dense) {
   assert(dense >= 0 && dense < Dense_Size);
   return Square(Square_Sparse[dense]);
}

int square_dense(Square sq) {
   return Square_Dense[sq];
}

Square square_from_std(int std) { // used for input
   if (std < 1 || std > Dense_Size) throw Bad_Input();
   return square_sparse(std - 1);
}

int square_to_std(Square sq) {
   int std = square_dense(sq) + 1;
   assert(std >= 1 && std <= Dense_Size);
   return std;
}

int square_file(Square sq) {
   return Square_File[sq];
}

int square_rank(Square sq) {
   return Square_Rank[sq];
}

Square square_opp(Square sq) {
   assert(square_sparse(0) == 0);
   return square_make((Square_Size - 1) - sq);
}

int square_rank(Square sq, Side sd) {

   int rk = square_rank(sq);

   if (sd != Black) {
      return (Rank_Size - 1) - rk;
   } else {
      return rk;
   }
}

bool square_is_promotion(Square sq, Side sd) {
   return square_rank(sq, sd) == Rank_Size - 1;
}

std::string square_to_string(Square sq) {
   return std::to_string(square_to_std(sq));
}

bool string_is_square(const std::string & s) {

   if (!string_is_nat(s)) return false;

   int std = std::stoi(s);
   return std >= 1 && std <= Dense_Size;
}

Square square_from_string(const std::string & s) {
   if (!string_is_nat(s)) throw Bad_Input();
   return square_from_std(std::stoi(s));
}

Inc inc_make(int inc) {
   assert(inc != 0);
   return Inc(inc);
}

Inc dir_inc(int dir) {
   assert(dir >= 0 && dir < Dir_Size);
   return Dir_Inc[dir];
}

Side side_make(int sd) {
   assert(sd >= 0 && sd < Side_Size);
   return Side(sd);
}

Side side_opp(Side sd) {
   return side_make(sd ^ 1);
}

std::string side_to_string(Side sd) {
   return (sd == White) ? "white" : "black";
}

Piece_Side piece_side_make(int ps) { // excludes Empty
   assert(ps >= 0 && ps < Piece_Side_Size);
   return Piece_Side(ps);
}

bool piece_side_is_piece(Piece_Side ps, Piece pc) {
   return ps != Empty && (ps >> 1) == pc;
}

bool piece_side_is_side(Piece_Side ps, Side sd) {
   return ps != Empty && (ps & 1) == sd;
}

Square Bit::operator*() const {
   return bit::first(*this);
}

void Bit::operator++() {
   *this = bit::rest(*this);
}

