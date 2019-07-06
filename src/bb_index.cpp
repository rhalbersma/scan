
// includes

#include <algorithm>
#include <string>

#include "bb_index.hpp"
#include "bit.hpp"
#include "common.hpp"
#include "libmy.hpp"
#include "pos.hpp"
#include "var.hpp"

namespace bb {

// constants

const int Man_Squares  {Dense_Size - File_Size / 2};
const int King_Squares {Dense_Size};

const int N_Max {Dense_Size};
const int P_Max {7};

const bool Rev {true}; // argument literal for "reverse"

// types

using Tuple = uint32; // enough for 9 pieces

// variables

static Tuple Tuple_Size[P_Max + 1][64];

// prototypes

static Index pos_index_wtm (ID id, const Pos & pos);
static Index pos_index_btm (ID id, const Pos & pos);

static Tuple tuple_index     (Bit pieces, Bit squares, int p, int n);
static Tuple tuple_index_rev (Bit pieces, Bit squares, int p, int n);

static Tuple tuple_size (int p, int n);

static int wolf_index_white (ID id, const Pos & pos, bool rev = false);
static int wolf_index_black (ID id, const Pos & pos, bool rev = false);

static int wolf_size_white (ID id);
static int wolf_size_black (ID id);

static int bit_index     (Bit b, Square sq);
static int bit_index_rev (Bit b, Square sq);

// functions

void index_init() {

   assert(N_Max < 64);

   // Pascal's triangle

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

ID id_make(int wm, int bm, int wk, int bk) {
   assert(wm + bm + wk + bk < 8);
   return ID((wm << 9) | (bm << 6) | (wk << 3) | (bk << 0));
}

bool id_is_illegal(ID id) {
   return (id & 00707) == 0
       || (var::Variant == var::BT && (id & 00070) != 0);
}

bool id_is_end(ID id) {
   return (id & 07070) == 0
       || (var::Variant == var::BT && (id & 00007) != 0);
}

int id_size(ID id) {
   return id_wm(id) + id_bm(id) + id_wk(id) + id_bk(id);
}

std::string id_name(ID id) {

   std::string name;
   name += char('0' + id_wm(id));
   name += char('0' + id_bm(id));
   name += char('0' + id_wk(id));
   name += char('0' + id_bk(id));

   return name;
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
      return pos_index_wtm(id, pos);
   } else {
      return pos_index_btm(id, pos);
   }
}

static Index pos_index_wtm(ID id, const Pos & pos) {

   Bit wm = pos.wm();
   Bit bm = pos.bm();
   Bit wk = pos.wk();
   Bit bk = pos.bk();

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   Index index = 0;

   index *= tuple_size(nwm, Man_Squares);
   index += tuple_index_rev(wm, bit::WM_Squares, nwm, Man_Squares);

   index *= tuple_size(nbm, Man_Squares);
   index += tuple_index(bm, bit::BM_Squares, nbm, Man_Squares);

   index *= tuple_size(nwk, King_Squares - nwm - nbm);
   index += tuple_index_rev(wk, bit::Squares ^ wm ^ bm, nwk, King_Squares - nwm - nbm);

   index *= tuple_size(nbk, King_Squares - nwm - nbm - nwk);
   index += tuple_index(bk, bit::Squares ^ wm ^ bm ^ wk, nbk, King_Squares - nwm - nbm - nwk);

   if (var::Variant == var::Frisian) {

      index *= wolf_size_white(id);
      index += wolf_index_white(id, pos);

      index *= wolf_size_black(id);
      index += wolf_index_black(id, pos);
   }

   assert(index < index_size(id));
   return index;
}

static Index pos_index_btm(ID id, const Pos & pos) {

   Bit wm = pos.bm();
   Bit bm = pos.wm();
   Bit wk = pos.bk();
   Bit bk = pos.wk();

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   Index index = 0;

   index *= tuple_size(nwm, Man_Squares);
   index += tuple_index(wm, bit::BM_Squares, nwm, Man_Squares);

   index *= tuple_size(nbm, Man_Squares);
   index += tuple_index_rev(bm, bit::WM_Squares, nbm, Man_Squares);

   index *= tuple_size(nwk, King_Squares - nwm - nbm);
   index += tuple_index(wk, bit::Squares ^ wm ^ bm, nwk, King_Squares - nwm - nbm);

   index *= tuple_size(nbk, King_Squares - nwm - nbm - nwk);
   index += tuple_index_rev(bk, bit::Squares ^ wm ^ bm ^ wk, nbk, King_Squares - nwm - nbm - nwk);

   if (var::Variant == var::Frisian) {

      index *= wolf_size_white(id);
      index += wolf_index_white(id, pos, Rev);

      index *= wolf_size_black(id);
      index += wolf_index_black(id, pos, Rev);
   }

   assert(index < index_size(id));
   return index;
}

Index index_size(ID id) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   Index size = 1;

   size *= tuple_size(nwm, Man_Squares);
   size *= tuple_size(nbm, Man_Squares);
   size *= tuple_size(nwk, King_Squares - nwm - nbm);
   size *= tuple_size(nbk, King_Squares - nwm - nbm - nwk);

   if (var::Variant == var::Frisian) {
      size *= wolf_size_white(id);
      size *= wolf_size_black(id);
   }

   return size;
}

static Tuple tuple_index(Bit pieces, Bit squares, int p, int n) {

   assert(p >= 0 && p <= P_Max);
   assert(n >= p && n <= N_Max);

   assert(bit::count(pieces) == p);
   assert(bit::count(squares) == n);
   assert(bit::is_incl(pieces, squares));

   Tuple index = 0;

   int i = 0;

   for (Square sq : pieces) {
      int pos = bit_index(squares, sq);
      index += tuple_size(i + 1, pos);
      i++;
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

   int i = 0;

   for (Square sq : pieces) {
      assert(i < p);
      square[(p - 1) - i] = sq;
      i++;
   }

   assert(i == p);

   for (int i = 0; i < p; i++) {
      Square sq = square[i];
      int pos = bit_index_rev(squares, sq);
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

static int wolf_index_white(ID id, const Pos & pos, bool rev) {

   if (var::Variant == var::Frisian) {

      Side sd = rev ? Black : White;
      int index = pos.count(sd);

      if (index != 0) { // sd has a wolf
         int wolf = rev ? bit_index    (pos.king(sd), pos.wolf(sd))
                        : bit_index_rev(pos.king(sd), pos.wolf(sd));
         index += wolf * 3;
      }

      assert(index >= 0 && index <= id_wk(id) * 3);
      return index;
   }

   return 0;
}

static int wolf_index_black(ID id, const Pos & pos, bool rev) {

   if (var::Variant == var::Frisian) {

      Side sd = rev ? White : Black;
      int index = pos.count(sd);

      if (index != 0) { // sd has a wolf
         int wolf = rev ? bit_index_rev(pos.king(sd), pos.wolf(sd))
                        : bit_index    (pos.king(sd), pos.wolf(sd));
         index += wolf * 3;
      }

      assert(index >= 0 && index <= id_bk(id) * 3);
      return index;
   }

   return 0;
}

static int wolf_size_white(ID id) {

   int size = 1;

   if (var::Variant == var::Frisian && id_wm(id) != 0) {
      size += id_wk(id) * 3;
   }

   return size;
}

static int wolf_size_black(ID id) {

   int size = 1;

   if (var::Variant == var::Frisian && id_bm(id) != 0) {
      size += id_bk(id) * 3;
   }

   return size;
}

static int bit_index(Bit b, Square sq) {
   assert(bit::has(b, sq));
   return bit::count(b & (ml::bit(sq) - 1));
}

static int bit_index_rev(Bit b, Square sq) {
   assert(bit::has(b, sq));
   return bit::count(b & (0 - ml::bit(sq + 1)));
}

} // namespace bb

