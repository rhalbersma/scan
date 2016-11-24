
// socket.h

#ifndef SOCKET_H
#define SOCKET_H

// includes

#include <string>

#include "libmy.hpp"

// functions

extern void socket_init ();

extern std::string socket_read  ();
extern void        socket_write (const std::string & s);

#endif // !defined SOCKET_H

// end of socket.h

