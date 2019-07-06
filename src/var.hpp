
#ifndef VAR_HPP
#define VAR_HPP

// includes

#include <string>

#include "libmy.hpp"

namespace var {

// types

enum Variant_Type { Normal, Killer, BT, Frisian, Losing };

// variables

extern Variant_Type Variant;
extern bool Book;
extern int  Book_Ply;
extern int  Book_Margin;
extern bool Ponder;
extern bool SMP;
extern int  Threads;
extern int  TT_Size;
extern bool BB;
extern int  BB_Size;

extern bool DXP_Server;
extern std::string DXP_Host;
extern int  DXP_Port;
extern bool DXP_Initiator;
extern int  DXP_Time;
extern int  DXP_Moves;
extern bool DXP_Board;
extern bool DXP_Search;

// functions

void init   ();
void load   (const std::string & file_name);
void update ();

std::string get (const std::string & name);
void        set (const std::string & name, const std::string & value);

std::string variant_name ();

} // namespace var

#endif // !defined VAR_HPP

