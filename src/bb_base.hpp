
#ifndef BB_BASE_HPP
#define BB_BASE_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace bb {

// types

enum Value { Draw, Loss, Win, Unknown };

// functions

void init ();

bool pos_is_load   (const Pos & pos);
bool pos_is_search (const Pos & pos, int bb_size);

Value probe     (const Pos & pos);
Value probe_raw (const Pos & pos);

Value value_update (Value node, Value child);

Value value_age (Value val);
Value value_max (Value v0, Value v1);

int value_nega (Value val, Side sd);

std::string value_to_string (Value val);

}

#endif // !defined BB_BASE_HPP

