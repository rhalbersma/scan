
#ifndef BB_BASE_HPP
#define BB_BASE_HPP

// includes

#include <string>

#include "common.hpp"
#include "libmy.hpp"

class Pos;

namespace bb {

// types

enum Value : int { Draw, Loss, Win, Unknown };

// functions

void init ();

bool pos_is_load   (const Pos & pos);
bool pos_is_search (const Pos & pos, int bb_size);

int probe     (const Pos & pos); // QS
int probe_raw (const Pos & pos); // quiet position

int value_update (int node, int child);

int value_age (int val);
int value_max (int v0, int v1);

int value_nega      (int val, Side sd);
int value_from_nega (int val);

std::string value_to_string (int val);

} // namespace bb

#endif // !defined BB_BASE_HPP

