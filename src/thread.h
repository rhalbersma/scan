
// thread.h

#ifndef THREAD_H
#define THREAD_H

// includes

#include <string>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "libmy.hpp"

// classes

class Lockable {

protected : // HACK for Waitable::wait()

   mutable std::mutex p_mutex;

public :

   void lock   () const { p_mutex.lock(); }
   void unlock () const { p_mutex.unlock(); }
};

class Waitable : public Lockable {

private :

   std::condition_variable_any p_cond;

public :

   void wait   () { p_cond.wait(p_mutex); } // HACK: direct access
   void signal () { p_cond.notify_one(); }
};

// functions

extern void listen_input ();

extern bool has_input ();
extern bool peek_line (std::string & line);
extern bool get_line  (std::string & line);

#endif // !defined THREAD_H

// end of thread.h

