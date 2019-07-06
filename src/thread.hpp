
#ifndef THREAD_HPP
#define THREAD_HPP

// includes

#include <string>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "libmy.hpp"

// types

class Lockable {

protected: // HACK for Waitable::wait()

   mutable std::mutex m_mutex;

public:

   void lock   () const { m_mutex.lock(); }
   void unlock () const { m_mutex.unlock(); }
};

class Waitable : public Lockable {

private:

   std::condition_variable_any m_cond;

public:

   void wait   () { m_cond.wait(m_mutex); } // HACK: direct access
   void signal () { m_cond.notify_one(); }
};

// functions

void listen_input ();

bool has_input ();
bool peek_line (std::string & line);
bool get_line  (std::string & line);

#endif // !defined THREAD_HPP

