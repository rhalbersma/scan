
#ifndef BB_INDEX_HPP
#define BB_INDEX_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace bb {

// constants

const int ID_Size { 1 << 12 };

// types

typedef uint32 Index; // enough for 6 pieces (7 for BT variant)

enum ID : int;

// functions

void index_init ();

bool id_is_ok (ID id);
ID   id_make  (int wm, int bm, int wk, int bk);

bool id_is_illegal (ID id);
bool id_is_loss    (ID id);

int  id_size (ID id);

int  id_wm (ID id);
int  id_bm (ID id);
int  id_wk (ID id);
int  id_bk (ID id);

std::string id_name (ID id);
std::string id_file (ID id);

ID pos_id (const Pos & pos);

Index pos_index  (ID id, const Pos & pos);
Index index_size (ID id);

}

#endif // !defined BB_INDEX_HPP

