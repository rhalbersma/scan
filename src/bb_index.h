
// bb_index.h

#ifndef BB_INDEX_H
#define BB_INDEX_H

// includes

#include <string>

#include "libmy.hpp"

class Pos;

namespace bb {

// types

typedef uint32 index_t; // enough for 6 pieces

// constants

const int ID_Size = 1 << 12;

// functions

extern void index_init ();

extern bool id_is_ok  (int id);
extern int  id_make   (int wm, int bm, int wk, int bk);

extern bool id_is_illegal (int id);
extern bool id_is_loss    (int id);

extern int id_size (int id);

inline int id_wm (int id) { return (id >> 9) & 07; }
inline int id_bm (int id) { return (id >> 6) & 07; }
inline int id_wk (int id) { return (id >> 3) & 07; }
inline int id_bk (int id) { return (id >> 0) & 07; }

extern std::string id_name (int id);
extern std::string id_file (int id);

extern int pos_id (const Pos & pos);

extern index_t pos_index (int id, const Pos & pos);

extern index_t index_size (int id);

}

#endif // !defined BB_INDEX_H

// end of bb_index.h

