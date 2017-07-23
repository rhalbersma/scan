
// includes

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "util.hpp"

// "constants"

const Inc Dir_Inc[Dir_Size] { -J1, -I1, +I1, +J1 };

// variables

static int Square_Sparse[Dense_Size];
static int Square_Dense[Square_Size];

static int Square_File[Square_Size];
static int Square_Rank[Square_Size];

// functions

void common_init() {

   // square numbering

   int dense = 0;
   int sparse = 0;

   for (int rk = 0; rk < Line_Size; rk++) {

      for (int fl = 0; fl < Line_Size + 3; fl++) { // 3 ghost files

         assert(dense < Dense_Size);
         assert(sparse < Square_Size);

         if (square_is_light(fl, rk)) { // invalid square

         } else if (square_is_ok(fl, rk)) { // board square

            Square_Sparse[dense] = sparse;
            Square_Dense[sparse] = dense;

            Square_File[sparse] = fl;
            Square_Rank[sparse] = rk;

            assert(square_make(fl, rk) == sparse);

            dense++;
            sparse++;

            if (dense >= Dense_Size) break; // all squares have been covered

         } else { // ghost square

            Square_Dense[sparse] = -1;

            Square_File[sparse] = -1;
            Square_Rank[sparse] = -1;

            sparse++;
         }
      }
   }
}

bool square_is_ok(int fl, int rk) {
   return (fl >= 0 && fl < Line_Size)
       && (rk >= 0 && rk < Line_Size)
       && square_is_dark(fl, rk);
}

bool square_is_ok(int sq) {
   return (sq >= 0 && sq < Square_Size)
       && Square_Dense[sq] >= 0;
}

bool square_is_light(int fl, int rk) {
   return !square_is_dark(fl, rk);
}

bool square_is_dark(int fl, int rk) {
   return (fl + rk) % 2 != 0;
}

Square square_make(int fl, int rk) {
   assert(square_is_ok(fl, rk));
   int dense = (rk * Line_Size + fl) / 2;
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
   return square_make((Square_Size - 1) - sq);
}

int square_rank(Square sq, Side sd) {

   int rk = square_rank(sq);

   if (sd == White) {
      return (Line_Size - 1) - rk;
   } else {
      return rk;
   }
}

bool square_is_promotion(Square sq, Side sd) {
   return square_rank(sq, sd) == Line_Size - 1;
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

Piece_Side piece_side_make(int ps) {
   assert(ps >= 0 && ps < Piece_Side_Size);
   return Piece_Side(ps);
}

Piece piece_side_piece(Piece_Side ps) {
   assert(ps != Empty);
   return Piece(ps >> 1);
}

Side piece_side_side(Piece_Side ps) {
   assert(ps != Empty);
   return side_make(ps & 1);
}

