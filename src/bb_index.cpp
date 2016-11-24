
// bb_index.cpp

// includes

#include <iostream>
#include <string>

#include "bb_index.h"
#include "bit.h"
#include "libmy.hpp"
#include "pos.h"
#include "tuple.h"

namespace bb {

// constants

static const int Size = 6;

static const bit_t WM_Squares = U64(0x0FFDFFBFF7FEF800);
static const bit_t BM_Squares = U64(0x007DFFBFF7FEFFC0);

// prototypes

static index_t pos_index_wtm (int id, bit_t wm, bit_t bm, bit_t wk, bit_t bk);
static index_t pos_index_btm (int id, bit_t wm, bit_t bm, bit_t wk, bit_t bk);

// functions

bool id_is_ok(int id) {

   if ((id & ~07777) != 0) return false;
   if (id_size(id) > Size) return false;

   return true;
}

int id_make(int wm, int bm, int wk, int bk) {

   assert(wm >= 0);
   assert(bm >= 0);
   assert(wk >= 0);
   assert(bk >= 0);
   assert(wm + bm + wk + bk <= Size);

   return (wm << 9) | (bm << 6) | (wk << 3) | (bk << 0);
}

bool id_is_illegal(int id) {

   return (id & 00707) == 0;
}

extern bool id_is_loss(int id) {

   return (id & 07070) == 0;
}

int id_size(int id) {

   return id_wm(id) + id_bm(id) + id_wk(id) + id_bk(id);
}

std::string id_name(int id) {

   std::string name;
   name += '0' + id_wm(id);
   name += '0' + id_bm(id);
   name += '0' + id_wk(id);
   name += '0' + id_bk(id);

   return name;
}

std::string id_file(int id) {

   return ml::itos(id_size(id)) + "/" + id_name(id);
}

int pos_id(const Pos & pos) {

   int nwm = bit_count(pos.wm());
   int nbm = bit_count(pos.bm());
   int nwk = bit_count(pos.wk());
   int nbk = bit_count(pos.bk());

   if (pos.turn() == White) {
      return id_make(nwm, nbm, nwk, nbk);
   } else {
      return id_make(nbm, nwm, nbk, nwk);
   }
}

index_t pos_index(int id, const Pos & pos) {

   assert(id == pos_id(pos));

   if (pos.turn() == White) {
      return pos_index_wtm(id, pos.wm(), pos.bm(), pos.wk(), pos.bk());
   } else {
      return pos_index_btm(id, pos.bm(), pos.wm(), pos.bk(), pos.wk());
   }
}

static index_t pos_index_wtm(int id, bit_t wm, bit_t bm, bit_t wk, bit_t bk) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   index_t index = 0;

   index *= tuple_size(nwm, 45);
   index += tuple_index_rev(wm, WM_Squares, nwm, 45);

   index *= tuple_size(nbm, 45);
   index += tuple_index(bm, BM_Squares, nbm, 45);

   index *= tuple_size(nwk, 50 - nwm - nbm);
   index += tuple_index_rev(wk, Bit_Squares ^ wm ^ bm, nwk, 50 - nwm - nbm);

   index *= tuple_size(nbk, 50 - nwm - nbm - nwk);
   index += tuple_index(bk, Bit_Squares ^ wm ^ bm ^ wk, nbk, 50 - nwm - nbm - nwk);

   assert(index < index_size(id));
   return index;
}

static index_t pos_index_btm(int id, bit_t wm, bit_t bm, bit_t wk, bit_t bk) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   index_t index = 0;

   index *= tuple_size(nwm, 45);
   index += tuple_index(wm, BM_Squares, nwm, 45);

   index *= tuple_size(nbm, 45);
   index += tuple_index_rev(bm, WM_Squares, nbm, 45);

   index *= tuple_size(nwk, 50 - nwm - nbm);
   index += tuple_index(wk, Bit_Squares ^ wm ^ bm, nwk, 50 - nwm - nbm);

   index *= tuple_size(nbk, 50 - nwm - nbm - nwk);
   index += tuple_index_rev(bk, Bit_Squares ^ wm ^ bm ^ wk, nbk, 50 - nwm - nbm - nwk);

   assert(index < index_size(id));
   return index;
}

index_t index_size(int id) {

   int nwm = id_wm(id);
   int nbm = id_bm(id);
   int nwk = id_wk(id);
   int nbk = id_bk(id);

   index_t size = 1;

   size *= tuple_size(nwm, 45);
   size *= tuple_size(nbm, 45);
   size *= tuple_size(nwk, 50 - nwm - nbm);
   size *= tuple_size(nbk, 50 - nwm - nbm - nwk);

   return size;
}

}

// end of bb_index.cpp

