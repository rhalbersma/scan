
#ifndef TT_HPP
#define TT_HPP

// includes

#include <vector>

#include "common.hpp"
#include "libmy.hpp"

namespace tt {

// types

enum class Flags;

inline Flags operator | (Flags f0, Flags f1) { return Flags(int(f0) | int(f1)); }

inline void  operator |= (Flags & f0, Flags f1) { f0 = f0 | f1; }

// constants

const Flags Flags_None  { Flags(0) };
const Flags Flags_Lower { Flags(1 << 0) };
const Flags Flags_Upper { Flags(1 << 1) };
const Flags Flags_Exact { Flags_Lower | Flags_Upper };
const Flags Flags_Mask  { Flags_Lower | Flags_Upper };

// types

class TT {

private :

   static const int Date_Size { 16 };

   struct Entry { // 16 bytes
      uint32 lock;
      uint32 pad_4; // #
      uint16 move;
      int16 score;
      uint8 depth;
      uint8 date;
      uint8 flags;
      uint8 pad_1; // #
   };

   std::vector<Entry> p_table;

   int p_size;
   int p_mask;
   int p_date;
   int p_age[Date_Size];

public :

   void set_size (int size);

   void clear    ();
   void inc_date ();

   void store (Key key, Move_Index move, Depth depth, Flags flags, Score score);
   bool probe (Key key, Move_Index & move, Depth & depth, Flags & flags, Score & score);

private :

   void set_date (int date);
   int  age      (int date) const;
};

// variables

extern TT G_TT;

// functions

inline bool is_lower (Flags flags) { return (int(flags) & int(Flags_Lower)) != 0; }
inline bool is_upper (Flags flags) { return (int(flags) & int(Flags_Upper)) != 0; }
inline bool is_exact (Flags flags) { return flags == Flags_Exact; }

}

#endif // !defined TT_HPP

