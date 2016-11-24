
// trans.h

#ifndef TRANS_H
#define TRANS_H

// includes

#include <vector>

#include "libmy.hpp"

namespace trans {

// constants

const int Date_Size = 16;

const int Upper_Flag = 1 << 0;
const int Lower_Flag = 1 << 1;

const int Unknown = 0;
const int Upper   = Upper_Flag;
const int Lower   = Lower_Flag;
const int Exact   = Upper_Flag | Lower_Flag;
const int Flags   = Upper_Flag | Lower_Flag;

// types

struct Entry {
   uint32 lock;
   uint32 pad_4; // #
   uint16 move;
   int16 score;
   uint8 depth;
   uint8 date;
   uint8 flags;
   uint8 pad_1; // #
};

class Trans {

private :

   std::vector<Entry> p_table;

   int p_size;
   int p_mask;
   int p_date;
   int p_age[Date_Size];

public :

   void set_size (int size);

   void clear    ();
   void inc_date ();

   void store (uint64 key, int move, int depth, int flags, int score);
   bool probe (uint64 key, int & move, int & depth, int & flags, int & score);

private :

   void set_date (int date);
   int  age      (int date) const;
};

// functions

inline bool is_upper (int flags) { return (flags & Upper_Flag) != 0; }
inline bool is_lower (int flags) { return (flags & Lower_Flag) != 0; }
inline bool is_exact (int flags) { return flags == Exact; }

}

#endif // !defined TRANS_H

// end of trans.h

