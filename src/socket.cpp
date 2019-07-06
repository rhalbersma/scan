
// includes

#include <cstdio> // for perror
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32

#include <winsock2.h> // or just <winsock.h>?

#else // assume Posix

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using SOCKET = int;
const SOCKET INVALID_SOCKET {-1};
void closesocket(int socket) { close(socket); }

#endif

#include "libmy.hpp"
#include "socket.hpp"
#include "var.hpp"

namespace socket_ { // HACK: "socket" creates a conflict on macOS

// constants

const int Buffer_Size_Max {4096};

const char EOL {'\0'}; // DXP protocol

// variables

static SOCKET G_Socket;
static char G_Buffer[Buffer_Size_Max];
static int G_Buffer_Size;

// prototypes

static bool has_input ();

// functions

void init() {

#ifdef _WIN32

   WSADATA wd;

   if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
      std::cerr << "WSAStartup failed (bad winsock DLL?)" << std::endl;
      std::exit(EXIT_FAILURE);
   }

#endif

   struct sockaddr_in sa;

   if (var::DXP_Server) {

      SOCKET listen_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (listen_socket == INVALID_SOCKET) {
         std::perror("socket");
         std::exit(EXIT_FAILURE);
      }

      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(INADDR_ANY);
      sa.sin_port = htons(var::DXP_Port);

      if (bind(listen_socket, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
         std::perror("bind");
         std::exit(EXIT_FAILURE);
      }

      if (listen(listen_socket, 1) < 0) {
         std::perror("listen");
         std::exit(EXIT_FAILURE);
      }

      std::cout << "waiting for a connection" << std::endl;

      G_Socket = accept(listen_socket, nullptr, nullptr);
      if (G_Socket == INVALID_SOCKET) {
         std::perror("accept");
         std::exit(EXIT_FAILURE);
      }

      closesocket(listen_socket);

   } else { // client

      G_Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (G_Socket == INVALID_SOCKET) {
         std::perror("socket");
         std::exit(EXIT_FAILURE);
      }

      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = inet_addr(var::DXP_Host.c_str());
      sa.sin_port = htons(var::DXP_Port);

      if (connect(G_Socket, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
         std::perror("connect");
         std::exit(EXIT_FAILURE);
      }
   }

   std::cout << "connection established\n";
   std::cout << std::endl;

   G_Buffer_Size = 0;
}

std::string read() {

   // fill buffer until a NUL byte is read

   while (!has_input()) {

      if (G_Buffer_Size >= Buffer_Size_Max) {
         std::cerr << "socket buffer overflow" << std::endl;
         std::exit(EXIT_FAILURE);
      }

      int len = recv(G_Socket, &G_Buffer[G_Buffer_Size], Buffer_Size_Max - G_Buffer_Size, 0);

      if (len < 0) { // error
         std::perror("recv");
         std::exit(EXIT_FAILURE);
      } else if (len == 0) { // EOF
         std::cerr << "connection closed" << std::endl;
         std::exit(EXIT_SUCCESS);
      }

      G_Buffer_Size += len;
      assert(G_Buffer_Size <= Buffer_Size_Max);
   }

   // extract the first "line"

   std::string s;

   int i = 0;

   while (true) {
      assert(i < G_Buffer_Size);
      char c = G_Buffer[i++];
      if (c == EOL) break;
      s += c;
   }

   // shift the rest of the buffer

   int size = 0;

   while (i < G_Buffer_Size) {
      G_Buffer[size++] = G_Buffer[i++];
   }

   G_Buffer_Size = size;

   return s;
}

void write(const std::string & s) {

   const char * buffer = s.c_str();
   int size = s.size() + 1; // includes '\0'

   for (int i = 0; i < size;) {

      int len = send(G_Socket, &buffer[i], size - i, 0);

      if (len < 0) { // error
         std::perror("send");
         std::exit(EXIT_FAILURE);
      }

      i += len;
      assert(i <= size);
   }
}

static bool has_input() {

   for (int i = 0; i < G_Buffer_Size; i++) {
      if (G_Buffer[i] == EOL) return true;
   }

   return false;
}

} // namespace socket_

