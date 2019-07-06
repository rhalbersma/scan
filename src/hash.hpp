
#ifndef HASH_HPP
#define HASH_HPP

// includes

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace hash {

// functions

void init ();

Key key (const Pos & pos);

inline int    index (Key key, int mask) { return uint64(key) & mask; }
inline uint32 lock  (Key key)           { return uint64(key) >> 32; }

} // namespace hash

#endif // !defined HASH_HPP

