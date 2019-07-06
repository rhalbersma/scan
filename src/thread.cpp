
// includes

#include <iostream>
#include <string>

#include "libmy.hpp"
#include "thread.hpp"

// types

class Input : public Waitable {

private:

   std::atomic<bool> m_has_input {false};
   bool m_eof {false};
   std::string m_line;

public:

   bool peek_line (std::string & line);
   bool get_line  (std::string & line);

   void put_eof  ();
   void put_line (const std::string & line);

   bool has_input () const { return m_has_input; }
};

// variables

static Input G_Input;
static std::thread G_Thread;

// prototypes

static void input_program (Input * input);

// functions

void listen_input() {
   G_Thread = std::thread(input_program, &G_Input);
   G_Thread.detach();
}

static void input_program(Input * input) {

   std::string line;

   while (std::getline(std::cin, line)) {
      input->put_line(line);
   }

   input->put_eof();
}

bool has_input() {
   return G_Input.has_input();
}

bool peek_line(std::string & line) {
   return G_Input.peek_line(line);
}

bool get_line(std::string & line) {
   return G_Input.get_line(line);
}

bool Input::peek_line(std::string & line) {

   lock();

   assert(m_has_input);

   bool line_ok = !m_eof;
   if (line_ok) line = m_line;

   unlock();

   return line_ok;
}

bool Input::get_line(std::string & line) {

   lock();

   while (!m_has_input) {
      wait();
   }

   bool line_ok = !m_eof;
   if (line_ok) line = m_line;

   m_has_input = false;
   signal();

   unlock();

   return line_ok;
}

void Input::put_eof() {

   lock();

   while (m_has_input) {
      wait();
   }

   m_eof = true;

   m_has_input = true;
   signal();

   unlock();
}

void Input::put_line(const std::string & line) {

   lock();

   while (m_has_input) {
      wait();
   }

   m_line = line;

   m_has_input = true;
   signal();

   unlock();
}

