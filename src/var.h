
// var.h

#ifndef VAR_H
#define VAR_H

// includes

#include <string>

#include "libmy.hpp"

namespace var {

// variables

extern bool Book;
extern int  Book_Margin;
extern bool Ponder;
extern bool SMP;
extern int  SMP_Threads;
extern int  Trans_Size;
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

extern void init   ();
extern void load   (const std::string & file_name);
extern void update ();

extern std::string get (const std::string & name);
extern void        set (const std::string & name, const std::string & value);

extern bool   get_bool  (const std::string & name);
extern int    get_int   (const std::string & name);
extern double get_float (const std::string & name);

}

#endif // !defined VAR_H

// end of var.h

