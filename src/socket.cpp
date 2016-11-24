
// socket.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32

#include <winsock2.h> // or just <winsock.h>?

#else // assume Posix

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef int SOCKET;
static const SOCKET INVALID_SOCKET = -1;

#endif

#include "libmy.hpp"
#include "socket.h"
#include "var.h"

// constants

static const int Buffer_Size_Max = 256;

// variables

static SOCKET Socket;
static char Buffer[Buffer_Size_Max];
static int Buffer_Size;

// prototypes

static bool socket_has_input ();

// functions

void socket_init() {

#ifdef _WIN32

   WSADATA wd;

   if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
      std::cerr << "WSAStartup failed (bad winsock DLL?)" << std::endl;
      std::exit(EXIT_FAILURE);
   }

#endif

   Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (Socket == INVALID_SOCKET) {
      std::perror("socket");
      std::exit(EXIT_FAILURE);
   }

   struct sockaddr_in sa;

   if (var::DXP_Server) {

      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(INADDR_ANY);
      sa.sin_port = htons(var::DXP_Port);

      if (bind(Socket, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
         std::perror("bind");
         std::exit(EXIT_FAILURE);
      }

      if (listen(Socket, 1) < 0) {
         std::perror("listen");
         std::exit(EXIT_FAILURE);
      }

      std::cout << "waiting for a connection" << std::endl;

      Socket = accept(Socket, NULL, NULL);
      if (Socket == INVALID_SOCKET) {
         std::perror("accept");
         std::exit(EXIT_FAILURE);
      }

   } else { // client

      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = inet_addr(var::DXP_Host.c_str());
      sa.sin_port = htons(var::DXP_Port);

      if (connect(Socket, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
         std::perror("connect");
         std::exit(EXIT_FAILURE);
      }
   }

   std::cout << "connection established" << std::endl;
   std::cout << std::endl;

   Buffer_Size = 0;
}

std::string socket_read() {

   // fill buffer until a NUL byte is read

   while (!socket_has_input()) {

      if (Buffer_Size >= Buffer_Size_Max) {
         std::cerr << "socket buffer overflow" << std::endl;
         std::exit(EXIT_FAILURE);
      }

      int len = recv(Socket, &Buffer[Buffer_Size], Buffer_Size_Max - Buffer_Size, 0);

      if (len < 0) { // error
         std::perror("recv");
         std::exit(EXIT_FAILURE);
      } else if (len == 0) { // EOF
         std::cerr << "connection closed" << std::endl;
         std::exit(EXIT_SUCCESS);
      } else { // success
         Buffer_Size += len;
         assert(Buffer_Size <= Buffer_Size_Max);
      }
   }

   // extract the first "line"

   std::string s;

   int pos = 0;

   while (true) {
      assert(pos < Buffer_Size);
      char c = Buffer[pos++];
      if (c == '\0') break;
      s += c;
   }

   // shift the rest of the buffer

   int size = 0;

   while (pos < Buffer_Size) {
      Buffer[size++] = Buffer[pos++];
   }

   Buffer_Size = size;

   return s;
}

void socket_write(const std::string & s) {

   const char * buffer = s.c_str();
   int size = int(s.size()) + 1;

   for (int pos = 0; pos < size;) {

      int len = send(Socket, &buffer[pos], size - pos, 0);

      if (len < 0) { // error
         std::perror("send");
         std::exit(EXIT_FAILURE);
      } else { // success
         pos += len;
         assert(pos <= size);
      }
   }
}

static bool socket_has_input() {

   for (int pos = 0; pos < Buffer_Size; pos++) {
      if (Buffer[pos] == '\0') return true;
   }

   return false;
}

// end of socket.cpp

