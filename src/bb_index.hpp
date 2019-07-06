
#ifndef BB_INDEX_HPP
#define BB_INDEX_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace bb {

// constants

const int ID_Size {1 << 12};

// types

using Index = uint32; // enough for 6 pieces (7 for BT variant)

enum ID : int;

// functions

void index_init ();

ID id_make (int wm, int bm, int wk, int bk);

bool id_is_illegal (ID id);
bool id_is_end     (ID id);

int id_size (ID id);

inline int id_wm (ID id) { return (id >> 9) & 07; }
inline int id_bm (ID id) { return (id >> 6) & 07; }
inline int id_wk (ID id) { return (id >> 3) & 07; }
inline int id_bk (ID id) { return (id >> 0) & 07; }

std::string id_name (ID id);

ID pos_id (const Pos & pos);

Index pos_index (ID id, const Pos & pos);

Index index_size (ID id);

} // namespace bb

#endif // !defined BB_INDEX_HPP

