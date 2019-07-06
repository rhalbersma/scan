
#ifndef TT_HPP
#define TT_HPP

// includes

#include <vector>

#include "common.hpp"
#include "libmy.hpp"

// types

enum class Flag : int {
   None  = 0,
   Upper = 1 << 0,
   Lower = 1 << 1,
   Exact = Upper | Lower,
};

class TT {

private:

   static const int Date_Size {16};

   struct Entry { // 16 bytes
      uint32 lock;
      uint32 pad_4; // #
      uint16 move;
      int16 score;
      uint8 depth;
      uint8 date;
      uint8 flag;
      uint8 pad_1; // #
   };

   std::vector<Entry> m_table;

   int m_size {0};
   int m_mask {0};
   int m_date {0};
   int m_age[Date_Size] {};

public:

   void set_size (int size);

   void clear    ();
   void inc_date ();

   void store (Key key, Move_Index move, Score score, Flag flag, Depth depth);
   bool probe (Key key, Move_Index & move, Score & score, Flag & flag, Depth & depth);

private:

   void set_date (int date);
};

// variables

extern TT G_TT;

// operators

inline Flag operator |  (Flag   f0, Flag f1) { return Flag(int(f0) | int(f1)); }
inline void operator |= (Flag & f0, Flag f1) { f0 = f0 | f1; }

// functions

inline bool is_lower (Flag flag) { return (int(flag) & int(Flag::Lower)) != 0; }
inline bool is_upper (Flag flag) { return (int(flag) & int(Flag::Upper)) != 0; }
inline bool is_exact (Flag flag) { return flag == Flag::Exact; }

#endif // !defined TT_HPP

