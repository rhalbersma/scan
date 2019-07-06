
#ifndef SOCKET_HPP
#define SOCKET_HPP

// includes

#include <string>

#include "libmy.hpp"

namespace socket_ { // HACK: "socket" creates a conflict on macOS

// functions

void init ();

std::string read  ();
void        write (const std::string & s);

} // namespace socket_

#endif // !defined SOCKET_HPP

