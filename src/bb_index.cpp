
// includes

#include <algorithm>
#include <iostream>
#include <string>

#include "bb_index.hpp"
#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "pos.hpp"
#include "var.hpp"

namespace bb {

// constants

const int Size { 7 };

const int N_Max { Dense_Size };
const int P_Max { 8 };

// types

typedef uint32 Tuple;

// variables

static Tuple Tuple_Size[P_Max + 1][64];

// prototypes

static Index pos_index_wtm (ID is, Bit wm, Bit bm, Bit wk, Bit bk);
static Index pos_index_btm (ID id, Bit wm, Bit bm, Bit wk, Bit bk);

static Tuple tuple_index     (Bit pieces, Bit squares, int p, int n);
static Tuple tuple_index_rev (Bit pieces, Bit squares, int p, int n);
static Tuple tuple_size      (int p, int n);

static int bit_pos     (Square sq, Bit squares);
static int bit_pos_rev (Square sq, Bit squares);

static uint64 before (Square sq);
static uint64 after  (Square sq);

// functions

void index_init() {

   // Pascal triangle

   for (int n = 0; n <= N_Max; n++) {

      Tuple_Size[0][n] = 1;

      for (int p = 1; p <= P_Max; p++) {
         Tuple_Size[p][n] = 0;
      }
   }

   for (int n = 1; n <= N_Max; n++) {
      for (int p = 1; p <= std::min(n, P_Max); p++) {
         Tuple_Size[p][n] = Tuple_Size[p][n - 1] + Tuple_Size[p - 1][n - 1];
      }
   }
}

bool id_is_ok(ID id) {
   return (id & ~07777) == 0 && id_size(id) <= Size;
}

ID id_make(int wm, int bm, int wk, int bk) {

   assert(wm >= 0);
   assert(bm >= 0);
   assert(wk >= 0);
   assert(bk >= 0);
   assert(wm + bm + wk + bk <= Size);

   return ID((wm << 9) | (bm << 6) | (wk << 3) | (bk << 0));
}

bool id_is_illegal(ID id) {
   return (id & 00707) == 0
       || (var::Variant == var::BT && (id & 00070) != 0);
}

bool id_is_loss(ID id) {
   return (id & 07070) == 0
       || (var::Variant == var::BT && (id & 00007) != 0);
}

int id_size(ID id) {
   return id_wm(id) + id_bm(id) + id_wk(id) + id_bk(id);
}

int id_wm(ID id) {
   return (id >> 9) & 07;
}

int id_bm(ID id) {
   return (id >> 6) & 07;
}

int id_wk(ID id) {
   return (id >> 3) & 07;
}

int id_bk(ID id) {
   return (id >> 0) & 07;
}

std::string id_name(ID id) {

   std::string name;
   name += '0' + id_wm(id);
   name += '0' + id_bm(id);
   name += '0' + id_wk(id);
   name += '0' + id_bk(id);

   return name;
}

std::string id_file(ID id) {
   return std::to_string(id_size(id)) + "/" + id_name(id);
}

ID pos_id(const Pos & pos) {

   int nwm = bit::count(pos.wm());
   int nbm = bit::count(pos.bm());
   int nwk = bit::count(pos.wk());
   int nbk = bit::count(pos.bk());

   if (pos.turn() == White) {
      return id_make(nwm, nbm, nwk, nbk);
   } else {
      return id_make(nbm, nwm, nbk, nwk);
   }
}

Index pos_index(ID id, const Pos & pos) {

   assert(id == pos_id(pos));

   if (pos.turn() == White) {
      return pos_index_wtm(id, pos.wm(), pos.bm(), pos.wk(), pos.bk());
   } else {
      return pos_index_btm(id, pos.bm(), pos.wm(), pos.bk(), pos.wk());
   }
}

static Index pos_index_wtm(ID id, Bit wm, Bit bm, Bit wk, Bit bk) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   Index index = 0;

   index *= tuple_size(nwm, 45);
   index += tuple_index_rev(wm, bit::WM_Squares, nwm, 45);

   index *= tuple_size(nbm, 45);
   index += tuple_index(bm, bit::BM_Squares, nbm, 45);

   index *= tuple_size(nwk, 50 - nwm - nbm);
   index += tuple_index_rev(wk, Bit(bit::Squares ^ wm ^ bm), nwk, 50 - nwm - nbm);

   index *= tuple_size(nbk, 50 - nwm - nbm - nwk);
   index += tuple_index(bk, Bit(bit::Squares ^ wm ^ bm ^ wk), nbk, 50 - nwm - nbm - nwk);

   assert(index < index_size(id));
   return index;
}

static Index pos_index_btm(ID id, Bit wm, Bit bm, Bit wk, Bit bk) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   Index index = 0;

   index *= tuple_size(nwm, 45);
   index += tuple_index(wm, bit::BM_Squares, nwm, 45);

   index *= tuple_size(nbm, 45);
   index += tuple_index_rev(bm, bit::WM_Squares, nbm, 45);

   index *= tuple_size(nwk, 50 - nwm - nbm);
   index += tuple_index(wk, Bit(bit::Squares ^ wm ^ bm), nwk, 50 - nwm - nbm);

   index *= tuple_size(nbk, 50 - nwm - nbm - nwk);
   index += tuple_index_rev(bk, Bit(bit::Squares ^ wm ^ bm ^ wk), nbk, 50 - nwm - nbm - nwk);

   assert(index < index_size(id));
   return index;
}

Index index_size(ID id) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   Index size = 1;

   size *= tuple_size(nwm, 45);
   size *= tuple_size(nbm, 45);
   size *= tuple_size(nwk, 50 - nwm - nbm);
   size *= tuple_size(nbk, 50 - nwm - nbm - nwk);

   return size;
}

static Tuple tuple_index(Bit pieces, Bit squares, int p, int n) {

   assert(p >= 0 && p <= P_Max);
   assert(n >= p && n <= N_Max);

   assert(bit::count(pieces) == p);
   assert(bit::count(squares) == n);
   assert(bit::is_incl(pieces, squares));

   Tuple index = 0;

   Bit b;
   int i;

   for (b = pieces, i = 0; b != 0; b = bit::rest(b), i++) {
      Square sq = bit::first(b);
      int pos = bit_pos(sq, squares);
      index += tuple_size(i + 1, pos);
   }

   assert(i == p);

   assert(index < tuple_size(p, n));
   return index;
}

static Tuple tuple_index_rev(Bit pieces, Bit squares, int p, int n) {

   assert(p >= 0 && p <= P_Max);
   assert(n >= p && n <= N_Max);

   assert(bit::count(pieces) == p);
   assert(bit::count(squares) == n);
   assert(bit::is_incl(pieces, squares));

   Tuple index = 0;

   Square square[P_Max + 1];

   Bit b;
   int i;

   for (b = pieces, i = 0; b != 0; b = bit::rest(b), i++) {
      Square sq = bit::first(b);
      assert(i < p);
      square[p - i - 1] = sq;
   }

   assert(i == p);

   for (int i = 0; i < p; i++) {
      Square sq = square[i];
      int pos = bit_pos_rev(sq, squares);
      index += tuple_size(i + 1, pos);
   }

   assert(index < tuple_size(p, n));
   return index;
}

static Tuple tuple_size(int p, int n) {
   assert(p >= 0 && p <= P_Max);
   assert(n >= 0 && n <= N_Max);
   return Tuple_Size[p][n];
}

static int bit_pos(Square sq, Bit squares) {
   assert(bit::has(squares, sq));
   return bit::count(squares & before(sq));
}

static int bit_pos_rev(Square sq, Bit squares) {
   assert(bit::has(squares, sq));
   return bit::count(squares & after(sq));
}

static uint64 before(Square sq) {
   return ml::bit(sq) - 1;
}

static uint64 after(Square sq) {
   assert(sq < 63);
   return 0 - ml::bit(sq + 1); // HACK: some compilers complain if "0" is omitted
}

}

