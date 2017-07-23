
// includes

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "bb_base.hpp"
#include "book.hpp"
#include "common.hpp"
#include "eval.hpp"
#include "hash.hpp"
#include "hub.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "move_gen.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "thread.hpp"
#include "tt.hpp"
#include "var.hpp"

// types

class Time {

private :

   double p_time_0; // target
   double p_time_1; // extended
   double p_time_2; // maximum

public :

   void init (const Search_Input & si, const Pos & pos);

   double time_0 () const { return p_time_0; }
   double time_1 () const { return p_time_1; }
   double time_2 () const { return p_time_2; }

private :

   void init (double time);
   void init (int moves, double time, double inc, const Pos & pos);
};

struct Local {

   const Node * p_node; // HACK: should be private

   Score alpha;
   Score beta;
   Depth depth;
   Ply ply;
   bool pv_node;
   bool prune;

   List list;
   int i;
   int j;

   Move move;
   Score score;
   Line pv;

   const Node & node () const { return *p_node; }
};

class Search_Global;
class Search_Local;

class Split_Point : public Lockable {

private :

   Split_Point * p_parent;
   Search_Global * p_sg;

   Local p_local;

   std::atomic<int> p_workers;
   std::atomic<bool> p_stop;

public :

   void init_root  ();
   void init       (Split_Point * parent, Search_Global & sg, const Local & local);
   void get_result (Local & local);

   void enter ();
   void leave ();

   Move get_move (Local & local);
   void update   (Move mv, Score sc, const Line & pv);

   void stop_root ();

   bool is_child (Split_Point * sp);

   bool stop () const { return p_stop; }
   bool free () const { return p_workers == 0; }

   Split_Point * parent () const { return p_parent; }
   const Local & local  () const { return p_local; }
};

class Search_Local : public Lockable {

private :

   static const int Pool_Size { 10 };

   std::thread p_thread;
   bool p_main;

   std::atomic<Split_Point *> p_work;
   ml::Array<Split_Point *, Ply_Size> p_stack;
   Split_Point p_pool[Pool_Size];
   std::atomic<int> p_pool_size;

   Search_Global * p_sg;

   int64 p_node;
   int64 p_leaf;
   int64 p_ply_sum;

public :

   void init (int id, Search_Global & sg);
   void end  ();

   void start_iter ();
   void end_iter   (Search_Output & so);

   void search_all_try  (const Node & node, List & list, Depth depth);
   void search_root_try (const Node & node, const List & list, Depth depth);

   void give_work (Split_Point * sp);

   bool idle (Split_Point * parent) const;
   bool idle () const;

private :

   static void launch (Search_Local * sl, Split_Point * root_sp);

   void idle_loop (Split_Point * wait_sp);

   void join      (Split_Point * sp);
   void move_loop (Split_Point * sp);

   void  search_all  (const Node & node, List & list, Depth depth, Ply ply);
   void  search_root (const Node & node, const List & list, Score alpha, Score beta, Depth depth, Ply ply, bool prune);
   Score search      (const Node & node, Score alpha, Score beta, Depth depth, Ply ply, bool prune, Line & pv);
   Score qs          (const Node & node, Score alpha, Score beta, Depth depth, Ply ply, Line & pv);

   void  move_loop   (Local & local);
   Score search_move (Move mv, const Local & local, Line & pv);

   void split (Local & local);

   static Depth extend (Move mv, const Local & local);
   static Depth reduce (Move mv, const Local & local);

   void inc_node ();

   Score leaf      (Score sc, Ply ply);
   void  mark_leaf (Ply ply);

   static Score bb_probe (const Pos & pos, Ply ply);
   static Score eval     (const Pos & pos);

   void poll ();
   bool stop () const;

   void push_sp (Split_Point * sp);
   void pop_sp  (Split_Point * sp);

   Split_Point * top_sp () const;
};

class Search_Global : public Lockable {

private :

   const Search_Input * p_si;
   Search_Output * p_so;

   const Node * p_node;
   List p_list;

   int p_bb_size;

   Search_Local p_sl[16];

   Split_Point p_root_sp;

   std::atomic<bool> p_ponder;
   std::atomic<bool> p_flag;

   Move p_last_move;
   Score p_last_score;

   bool p_change;
   bool p_first;
   bool p_high;
   bool p_drop;
   double p_factor;

public :

   void new_search    (int bb_size);
   void init_search   (const Search_Input & si, Search_Output & so, const Node & node, const List & list);
   void collect_stats ();
   void end_search    ();

   void start_iter (Depth depth);
   void end_iter   (Depth depth);

   void search (Depth depth);

   void new_best_move (Move mv, Score sc, Depth depth, const Line & pv);

   void poll  ();
   void abort ();

   bool has_worker () const;
   void broadcast  (Split_Point * sp);

   Split_Point * root_sp () { return &p_root_sp; }

   void set_flag (bool flag) { p_flag = flag; }

   void set_first (bool first) { p_first = first; }
   void set_high  (bool high)  { p_high  = high; }

   int    depth () const { return p_so->depth; }
   double time  () const { return p_so->time(); }

   bool ponder () const { return p_ponder; }

   Move  last_move  () const { return p_last_move; }
   Score last_score () const { return p_last_score; }

   bool   change () const { return p_change; }
   bool   first  () const { return p_first; }
   bool   high   () const { return p_high; }
   bool   drop   () const { return p_drop; }
   double factor () const { return p_factor; }

   int bb_size () const { return p_bb_size; }

   const Search_Local & sl (int id) const { return p_sl[id]; }
         Search_Local & sl (int id)       { return p_sl[id]; }
};

struct SMP : public Lockable {
   std::atomic<bool> busy;
};

class Abort : public std::exception {
};

// variables

static Time G_Time; // TODO: move to SG

static SMP G_SMP; // lock to create and broadcast split points // MOVE ME to SG?
static Lockable G_IO;

// prototypes

static void gen_moves_bb (List & list, const Pos & pos);

static double lerp (double mg, double eg, double phase);

static double time_lag (double time);

static void local_update (Local & local, Move mv, Score sc, const Line & pv, Search_Global & sg); // MOVE ME

// functions

void search(Search_Output & so, const Node & node, const Search_Input & si) {

   // init

   var::update();

   so.init(si, node);

   int bb_size = var::BB_Size;

   if (bb::pos_is_load(node)) { // root position already in bitbases => only use smaller bitbases in search
      tt::G_TT.clear();
      bb_size = pos::size(node) - 1;
   }

   // special cases

   List list;
   gen_moves_bb(list, node);
   assert(list.size() != 0);

   if (si.move && !si.ponder && list.size() == 1) {

      Move mv = list.move(0);
      Score sc = quick_score(node);

      so.new_best_move(mv, sc);
      return;
   }

   if (var::Book && si.book) {

      int ply = pos::stage(node);
      assert(ply >= 0);

      int margin = (ply < var::Book_Ply) ? var::Book_Margin : 0;

      Move mv;
      Score sc;

      if (book::probe(node, Score(margin), mv, sc)) {
         so.new_best_move(mv, sc);
         return;
      }
   }

   // more init

   G_Time.init(si, node);

   Search_Global sg;
   sg.new_search(bb_size); // also launches threads
   sg.init_search(si, so, node, list);

   Move easy_move = move::None;

   if (si.smart() && list.size() > 1) {

      sg.sl(0).search_all_try(node, list, Depth(1));

      if (list.score(0) - list.score(1) >= +100 && list.move(0) == quick_move(node)) {
         easy_move = list.move(0);
      }
   }

   // iterative deepening

   try {

      for (int d = 1; d <= si.depth; d++) {

         Depth depth = Depth(d);

         sg.start_iter(depth);
         sg.search(depth);
         sg.end_iter(depth);
         sg.collect_stats();

         Move mv = so.move;
         double time = so.time();
         // if (true || time >= 0.01) so.disp_best_move();

         // early exit?

         bool abort = false;

         if (mv == easy_move && !sg.change() && time >= G_Time.time_0() / 16.0) abort = true;

         if (si.smart() && time >= G_Time.time_0() * sg.factor() * lerp(0.4, 0.8, pos::phase(node))) abort = true;

         if (si.smart() && sg.drop()) abort = false;

         if (depth >= 12 && abort) {
            sg.set_flag(true);
            if (!sg.ponder()) break;
         }
      }

   } catch (const Abort &) {

      // so.disp_best_move();
   }

   if (si.output == Output_Terminal) std::cout << std::endl;

   sg.end_search(); // sync with threads

   so.end();
}

Move quick_move(const Pos & pos) {

   // init

   if (pos::is_wipe(pos)) return move::None; // for BT variant

   List list;
   gen_moves_bb(list, pos);

   if (list.size() == 0) return move::None;
   if (list.size() == 1) return list.move(0);

   // book

   if (var::Book) {

      Move mv;
      Score sc;

      if (book::probe(pos, Score(0), mv, sc)) {
         return mv;
      }
   }

   // transposition table

   Move_Index tt_move = Move_Index_None;

   Depth tt_depth;
   tt::Flags tt_flags;
   Score tt_score;
   tt::G_TT.probe(hash::key(pos), tt_move, tt_depth, tt_flags, tt_score); // updates tt_move

   if (tt_move != Move_Index_None) {
      Move mv = list::find_index(list, tt_move, pos);
      if (mv != move::None) return mv;
   }

   return move::None;
}

Score quick_score(const Pos & pos) {

   // book

   if (var::Book) {

      Move mv;
      Score sc;

      if (book::probe(pos, Score(0), mv, sc)) {
         return sc;
      }
   }

   // TODO: BB (but: win/loss have no distance)

   // transposition table

   Move_Index tt_move = Move_Index_None;

   Depth tt_depth;
   tt::Flags tt_flags;
   Score tt_score;

   if (tt::G_TT.probe(hash::key(pos), tt_move, tt_depth, tt_flags, tt_score)) {
      return score::from_tt(tt_score, Ply(0));
   }

   return score::None;
}

static void gen_moves_bb(List & list, const Pos & pos) {

   if (bb::pos_is_load(pos)) { // root position already in bitbases

      // filter root moves using BB probes

      list.clear();

      bb::Value node = bb::probe(pos);

      List tmp;
      gen_moves(tmp, pos);

      for (int i = 0; i < tmp.size(); i++) {

         Move mv = tmp.move(i);

         Pos new_pos = pos.succ(mv);

         bb::Value child = bb::probe(new_pos);
         if (bb::value_age(child) == node) list.add(mv); // optimal move
      }

   } else {

      gen_moves(list, pos);
   }
}

static double lerp(double mg, double eg, double phase) {
   assert(phase >= 0.0 && phase <= 1.0);
   return mg + (eg - mg) * phase;
}

void Search_Input::init() {

   var::update();

   move = true;
   book = true;
   depth = Depth_Max;
   input = false;
   output = Output_None;
   ponder = false;

   p_smart = false;
   p_moves = 0;
   p_time = 1E6;
   p_inc = 0.0;
}

void Search_Input::set_time(double time) {
   p_smart = false;
   p_time = time;
}

void Search_Input::set_time(int moves, double time, double inc) {
   p_smart = true;
   p_moves = moves;
   p_time = time;
   p_inc = inc;
}

void Search_Output::init(const Search_Input & si, const Pos & pos) {

   move = move::None;
   answer = move::None;
   score = Score(0);
   depth = Depth(0);
   pv.clear();

   node = 0;
   leaf = 0;
   ply_sum = 0;

   p_si = &si;
   p_pos = pos;
   p_timer.reset();
   p_timer.start();
}

void Search_Output::end() {
   p_timer.stop();
}

void Search_Output::new_best_move(Move mv, Score sc) {

   Line pv;
   pv.set(mv);

   new_best_move(mv, sc, Depth(0), pv);

   if (p_si->output == Output_Terminal) std::cout << std::endl;
}

void Search_Output::new_best_move(Move mv, Score sc, Depth depth, const Line & pv) {

   if (pv.size() != 0) assert(pv[0] == mv);

   move = mv;
   answer = (pv.size() < 2) ? move::None : pv[1];
   score = sc;
   this->depth = depth;
   this->pv = pv;

   if (depth == 0 || depth >= 12) disp_best_move();
}

void Search_Output::disp_best_move() {

   if (var::SMP) G_IO.lock();

   double time = this->time();
   double speed = (time < 0.01) ? 0.0 : double(node) / time;

   switch (p_si->output) {

      case Output_None :

         // no-op
         break;

      case Output_Terminal :

         std::printf("%2d/%4.1f%+7.2f%11lld%7.2f%5.1f  %s\n", depth, ply_avg(), double(score) / 100.0, node, time, speed / 1E6, pv.to_string(p_pos, 7).c_str());
         std::fflush(stdout);
         break;

      case Output_Hub : {

         std::string line = "info";
         if (depth != 0)           hub::add_pair(line, "depth", std::to_string(depth));
         if (ply_avg() != 0.0)     hub::add_pair(line, "mean-depth", ml::ftos(ply_avg(), 1));
         if (score != score::None) hub::add_pair(line, "score", ml::ftos(double(score) / 100.0, 2));
         if (node != 0)            hub::add_pair(line, "nodes", std::to_string(node));
         if (time >= 0.001)        hub::add_pair(line, "time", ml::ftos(time, 3));
         if (speed != 0.0)         hub::add_pair(line, "nps", ml::ftos(speed / 1E6, 1));
         if (pv.size() != 0)       hub::add_pair(line, "pv", pv.to_hub());
         hub::write(line);

         break;
      }
   }

   if (var::SMP) G_IO.unlock();
}

void Time::init(const Search_Input & si, const Pos & pos) {

   if (si.smart()) {
      init(si.moves(), si.time(), si.inc(), pos);
   } else {
      init(si.time());
   }
}

void Time::init(double time) {
   p_time_0 = time;
   p_time_1 = time;
   p_time_2 = time;
}

void Time::init(int moves, double time, double inc, const Pos & pos) {

   double moves_left = lerp(30.0, 10.0, pos::phase(pos));
   if (moves != 0) moves_left = std::min(moves_left, double(moves));

   double factor = 1.3;
   if (var::Ponder) factor *= 1.2;

   double total = std::max(time + inc * moves_left, 0.0);
   double alloc = total / moves_left * factor;

   if (moves == 0 || moves >= 10) { // don't use > 1/4 total time
      double total_safe = std::max(time / 4.0 + inc, 0.0);
      total = std::min(total, total_safe);
   }

   double max = time_lag(std::min(total, time + inc) * 0.95);

   p_time_0 = std::min(time_lag(alloc), max);
   p_time_1 = std::min(time_lag(alloc * 4.0), max);
   p_time_2 = max;

   assert(0.0 <= p_time_0 && p_time_0 <= p_time_1 && p_time_1 <= p_time_2);
}

static double time_lag(double time) {
   return std::max(time - 0.1, 0.0);
}

void Search_Global::new_search(int bb_size) {

   p_bb_size = bb_size;

   G_SMP.busy = false;
   p_root_sp.init_root();

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).init(id, *this); // also launches a thread if id /= 0
   }

   tt::G_TT.inc_date();
   sort_clear();
}

void Search_Global::init_search(const Search_Input & si, Search_Output & so, const Node & node, const List & list) {

   p_si = &si;
   p_so = &so;

   p_node = &node;
   p_list = list;

   p_ponder = si.ponder;
   p_flag = false;

   p_last_move = move::None;
   p_last_score = score::None;

   p_change = false;
   p_first = false;
   p_high = false;
   p_drop = false;
   p_factor = 1.0;
}

void Search_Global::collect_stats() {

   p_so->node = 0;
   p_so->leaf = 0;
   p_so->ply_sum = 0;

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).end_iter(*p_so);
   }
}

void Search_Global::end_search() {

   abort();

   p_root_sp.leave();
   assert(p_root_sp.free());

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).end();
   }
}

void Search_Global::start_iter(Depth /* depth */) {

   p_drop = false;

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).start_iter();
   }
}

void Search_Global::end_iter(Depth depth) {

   // time management

   Move  mv = p_so->move;
   Score sc = p_so->score;

   if (depth > 1 && mv == p_last_move) {
      p_factor = std::max(p_factor * 0.9, 0.6);
   }

   p_last_move = mv;
   p_last_score = sc;
}

void Search_Global::search(Depth depth) {
   sl(0).search_root_try(*p_node, p_list, depth);
}

void Search_Global::new_best_move(Move mv, Score sc, Depth depth, const Line & pv) {

   if (var::SMP) lock();

   Move best = p_so->move;

   collect_stats(); // update search info
   p_so->new_best_move(mv, sc, depth, pv);

   int i = list::find(p_list, mv);
   p_list.mtf(i);

   if (depth > 1 && mv != best) {

      p_flag = false;
      p_change = true;

      p_factor = std::max(p_factor, 1.0);
      p_factor = std::min(p_factor * 1.2, 2.0);
   }

   p_drop = sc - p_last_score <= ((var::Variant == var::Normal) ? -10 : -20);

   if (p_drop) {
      p_flag = false;
      p_change = true;
   }

   if (var::SMP) unlock();
}

void Search_Global::poll() {

   bool abort = false;

   // input event?

   if (var::SMP) G_IO.lock();

   if (p_si->input && has_input()) {

      std::string line;
      if (!peek_line(line)) std::exit(EXIT_SUCCESS); // EOF

      std::stringstream ss(line);

      std::string command;
      ss >> command;

      if (command == "ping") {
         get_line(line);
         std::cout << "pong" << std::endl;
      } else if (command == "ponder-hit") {
         get_line(line);
         p_ponder = false;
         if (p_flag || p_list.size() == 1) abort = true;
      } else { // other command => abort search
         p_ponder = false;
         abort = true;
      }
   }

   if (var::SMP) G_IO.unlock();

   // time limit?

   double time = this->time();

   if (depth() <= 12) {
      // no-op
   } else if (time >= G_Time.time_1()) {
      abort = true;
   } else if (p_si->smart() && !first()) {
      // no-op
   } else if (time >= G_Time.time_0() * factor()) {
      abort = true;
   }

   if (abort) {
      p_flag = true;
      if (!p_ponder) this->abort();
   }
}

void Search_Global::abort() {
   p_root_sp.stop_root();
}

bool Search_Global::has_worker() const {

   if (G_SMP.busy) return false;

   for (int id = 0; id < var::SMP_Threads; id++) {
      if (sl(id).idle()) return true;
   }

   return false;
}

void Search_Global::broadcast(Split_Point * sp) {

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).give_work(sp);
   }
}

void Search_Local::init(int id, Search_Global & sg) {

   p_main = id == 0;

   p_work = sg.root_sp(); // to make it non-null
   p_stack.clear();
   p_pool_size = 0;

   p_sg = &sg;

   p_node = 0;
   p_leaf = 0;
   p_ply_sum = 0;

   if (var::SMP && !p_main) {
      p_thread = std::thread(launch, this, sg.root_sp());
   }
}

void Search_Local::launch(Search_Local * sl, Split_Point * root_sp) {
   sl->idle_loop(root_sp);
}

void Search_Local::end() {
   if (var::SMP && !p_main) p_thread.join();
}

void Search_Local::start_iter() {

   // p_node = 0;
   p_leaf = 0;
   p_ply_sum = 0;
}

void Search_Local::end_iter(Search_Output & so) {

   if (var::SMP || p_main) {
      so.node += p_node;
      so.leaf += p_leaf;
      so.ply_sum += p_ply_sum;
   }
}

void Search_Local::idle_loop(Split_Point * wait_sp) {

   push_sp(wait_sp);

   while (true) {

      assert(p_work == p_sg->root_sp());
      p_work = nullptr;

      while (!wait_sp->free() && p_work.load() == nullptr) // spin
         ;

      Split_Point * work = p_work.exchange(p_sg->root_sp()); // to make it non-null
      if (work == nullptr) break;

      join(work);
   }

   pop_sp(wait_sp);

   assert(wait_sp->free());
   assert(p_work == p_sg->root_sp());
}

void Search_Local::give_work(Split_Point * sp) {

   if (idle(sp->parent())) {

      sp->enter();

      assert(p_work.load() == nullptr);
      p_work = sp;
   }
}

bool Search_Local::idle(Split_Point * parent) const {

   lock();
   bool idle = this->idle() && parent->is_child(top_sp());
   unlock();

   return idle;
}

bool Search_Local::idle() const {
   return p_work.load() == nullptr;
}

void Search_Local::search_all_try(const Node & node, List & list, Depth depth) {

   assert(depth > 0 && depth <= Depth_Max);
   assert(list.size() != 0);

   assert(p_stack.empty());
   push_sp(p_sg->root_sp());

   try {
      search_all(node, list, depth, Ply(0));
   } catch (const Abort &) {
      pop_sp(p_sg->root_sp());
      assert(p_stack.empty());
      throw;
   }

   pop_sp(p_sg->root_sp());
   assert(p_stack.empty());
}

void Search_Local::search_root_try(const Node & node, const List & list, Depth depth) {

   assert(depth > 0 && depth <= Depth_Max);
   assert(list.size() != 0);

   assert(p_stack.empty());
   push_sp(p_sg->root_sp());

   try {
      search_root(node, list, -score::Inf, +score::Inf, depth, Ply(0), true);
   } catch (const Abort &) {
      pop_sp(p_sg->root_sp());
      assert(p_stack.empty());
      throw;
   }

   pop_sp(p_sg->root_sp());
   assert(p_stack.empty());
}

void Search_Local::join(Split_Point * sp) {

   // sp->enter();
   push_sp(sp);

   try {
      move_loop(sp);
   } catch (const Abort &) {
      // no-op
   }

   pop_sp(sp);
   sp->leave();
}

void Search_Local::move_loop(Split_Point * sp) {

   Local local = sp->local(); // local copy

   while (true) {

      Move mv = sp->get_move(local); // also updates "local"
      if (mv == move::None) break;

      Line pv;
      Score sc = search_move(mv, local, pv);

      sp->update(mv, sc, pv);
   }
}

void Search_Local::search_all(const Node & node, List & list, Depth depth, Ply ply) {

   assert(depth > 0 && depth <= Depth_Max);

   assert(list.size() != 0);

   // init

   bool prune = true;

   // move loop

   for (int i = 0; i < list.size(); i++) {

      // search move

      Move mv = list.move(i);

      Line pv;

      inc_node();
      Node new_node = node.succ(mv);
      Score sc = -search(new_node, -score::Inf, +score::Inf, depth - Depth(1), ply + Ply(1), prune, pv);

      // update state

      list.set_score(i, sc);
   }

   list.sort();
}

void Search_Local::search_root(const Node & node, const List & list, Score alpha, Score beta, Depth depth, Ply ply, bool prune) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth > 0 && depth <= Depth_Max);

   assert(list.size() != 0);

   // init

   Local local;

   local.p_node = &node;
   local.alpha = alpha;
   local.beta = beta;
   local.depth = depth;
   local.ply = ply;
   local.pv_node = beta != alpha + Score(1);
   local.prune = prune;

   local.move = move::None;
   local.score = score::None;
   local.pv.clear();

   // move loop

   local.list = list;

   move_loop(local);
}

Score Search_Local::search(const Node & node, Score alpha, Score beta, Depth depth, Ply ply, bool prune, Line & pv) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth <= Depth_Max);

   assert(!p_stack.empty());
   assert(p_stack[0] == p_sg->root_sp());

   // QS

   pv.clear();

   if (node.is_draw(2)) return leaf(Score(0), ply);

   if (depth <= 0) return qs(node, alpha, beta, Depth(0), ply, pv);

   if (pos::is_wipe(node)) return leaf(score::loss(ply), ply); // for BT variant

   // init

   Local local;

   local.p_node = &node;
   local.alpha = alpha;
   local.beta = beta;
   local.depth = depth;
   local.ply = ply;
   local.pv_node = beta != alpha + Score(1);
   local.prune = prune;

   local.move = move::None;
   local.score = score::None;
   local.pv.clear();

   // transposition table

   Move_Index tt_move = Move_Index_None;
   Key key = hash::key(node);

   {
      Depth tt_depth;
      tt::Flags tt_flags;
      Score tt_score;

      if (tt::G_TT.probe(key, tt_move, tt_depth, tt_flags, tt_score)) {

         if (!local.pv_node && tt_depth >= local.depth) {

            tt_score = score::from_tt(tt_score, local.ply);

            if ((tt::is_lower(tt_flags) && tt_score >= local.beta)
             || (tt::is_upper(tt_flags) && tt_score <= local.alpha)
             ||  tt::is_exact(tt_flags)) {
               return tt_score;
            }
         }
      }
   }

   // bitbases

   if (bb::pos_is_search(node, p_sg->bb_size())) {

      Line new_pv;
      Score sc = qs(node, local.alpha, local.beta, Depth(0), local.ply, new_pv); // captures + BB probe

      if ((sc < 0 && sc <= local.alpha) || sc == 0 || (sc > 0 && sc >= local.beta)) {
         local.score = sc;
         local.pv = new_pv;
         goto cont;
      }

      if (sc > 0) { // win => lower bound
         local.score = sc;
         local.pv = new_pv;
      }
   }

   // gen moves

   gen_moves(local.list, node);

   if (local.list.size() == 0) { // no legal moves => loss
      return leaf(score::loss(local.ply), local.ply);
   }

   if (score::loss(local.ply + Ply(2)) >= local.beta) { // loss-distance pruning
      return leaf(score::loss(local.ply + Ply(2)), local.ply);
   }

   if (local.ply >= Ply_Max) return leaf(eval(node), local.ply);

   // pruning

   if (local.prune && !local.pv_node && local.depth >= 3 && local.beta <= +score::Eval_Inf) {

      Score margin = Score(10 * local.depth);
      Score new_beta = score::add_safe(local.beta, +margin);
      Depth new_depth = Depth(local.depth * 40 / 100);

      Line new_pv;
      Score sc = search(node, new_beta - Score(1), new_beta, new_depth, local.ply + Ply(1), false, new_pv);

      if (sc >= new_beta) {

         sc = score::add_safe(sc, -margin); // fail-soft margin
         assert(sc >= local.beta);

         pv = new_pv;
         return sc;
      }
   }

   // move loop

   sort_all(local.list, node, tt_move);
   move_loop(local);

cont : // epilogue

   assert(score::is_ok(local.score));

   // transposition table

   {
      Move_Index tt_move = Move_Index_None;
      if (local.score > local.alpha && local.move != move::None) {
         tt_move = move::index(local.move, node);
      }

      Depth tt_depth = local.depth;

      tt::Flags tt_flags = tt::Flags_None;
      if (local.score > local.alpha) tt_flags |= tt::Flags_Lower;
      if (local.score < local.beta)  tt_flags |= tt::Flags_Upper;

      Score tt_score = score::to_tt(local.score, local.ply);

      tt::G_TT.store(key, tt_move, tt_depth, tt_flags, tt_score);
   }

   // move-ordering statistics

   if (local.score > local.alpha && local.move != move::None && local.list.size() > 1) {

      good_move(move::index(local.move, node));

      assert(list::has(local.list, local.move));

      for (int i = 0; i < local.list.size(); i++) {

         Move mv = local.list.move(i);
         if (mv == local.move) break;

         bad_move(move::index(mv, node));
      }
   }

   pv = local.pv;
   return local.score;
}

void Search_Local::move_loop(Local & local) {

   local.i = 0;
   local.j = 0;

   while (local.score < local.beta && local.i < local.list.size()) {

      int searched_size = local.j;

      if (local.ply == 0) { // root event
         p_sg->set_first(searched_size == 0);
      }

      // SMP

      if (var::SMP && local.depth >= 6 && searched_size != 0 && local.list.size() - searched_size >= 5 && p_sg->has_worker() && p_pool_size < Pool_Size) {
         split(local);
         break; // share the epilogue
      }

      // search move

      Move mv = local.list.move(local.i++);

      Line pv;
      Score sc = search_move(mv, local, pv);

      local_update(local, mv, sc, pv, *p_sg);
   }
}

Score Search_Local::search_move(Move mv, const Local & local, Line & pv) {

   // init

   int searched_size = local.j;

   Depth ext = extend(mv, local);
   Depth red = reduce(mv, local);
   assert(ext == 0 || red == 0);

   Score new_alpha = std::max(local.alpha, local.score);
   Depth new_depth = local.depth + ext - Depth(1);

   assert(new_alpha < local.beta);

   // search move

   Score sc;

   inc_node();
   Node new_node = local.node().succ(mv);

   if ((local.pv_node && searched_size != 0) || red != 0) {

      sc = -search(new_node, -new_alpha - Score(1), -new_alpha, new_depth - red, local.ply + Ply(1), local.prune, pv);

      if (sc > new_alpha) { // PVS/LMR re-search

         if (local.ply == 0) p_sg->set_high(true);
         sc = -search(new_node, -local.beta, -new_alpha, new_depth, local.ply + Ply(1), local.prune, pv);
         if (local.ply == 0) p_sg->set_high(false);
      }

   } else {

      sc = -search(new_node, -local.beta, -new_alpha, new_depth, local.ply + Ply(1), local.prune, pv);
   }

   return sc;
}

Score Search_Local::qs(const Node & node, Score alpha, Score beta, Depth depth, Ply ply, Line & pv) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth <= 0);

   // init

   pv.clear();

   if (pos::is_wipe(node)) return leaf(score::loss(ply), ply);

   if (ply >= Ply_Max) return leaf(eval(node), ply);

   // move-loop init

   Score bs = score::None;

   List list;
   gen_captures(list, node);

   if (list.size() == 0) { // quiet position

      // bitbases

      if (bb::pos_is_search(node, p_sg->bb_size())) {
         return leaf(bb_probe(node, ply), ply);
      }

      // threat position?

      if (depth == 0 && pos::is_threat(node)) {
         return search(node, alpha, beta, Depth(1), ply + Ply(1), false, pv); // one-ply search
      }

      // stand pat

      Score sc = eval(node);

      bs = sc;

      if (sc >= beta) return leaf(sc, ply);

      list.clear();
      if (var::Variant == var::BT) gen_promotions(list, node);
      if (!pos::has_king(node) && !pos::is_threat(node)) add_sacs(list, node);
   }

   // move loop

   for (int i = 0; i < list.size(); i++) {

      Move mv = list.move(i);

      Line new_pv;

      inc_node();
      Node new_node = node.succ(mv);
      Score sc = -qs(new_node, -beta, -std::max(alpha, bs), depth - Depth(1), ply + Ply(1), new_pv);

      if (sc > bs) {

         bs = sc;
         pv.concat(mv, new_pv);

         if (sc >= beta) break;
      }
   }

   if (list.size() == 0) mark_leaf(ply);

   assert(score::is_ok(bs));
   return bs;
}

void Search_Local::split(Local & local) {

   p_sg->poll();
   poll();

   G_SMP.lock(); // useful?

   assert(!G_SMP.busy);
   G_SMP.busy = true;

   assert(p_pool_size < Pool_Size);
   Split_Point * sp = &p_pool[p_pool_size++];
   sp->init(top_sp(), *p_sg, local);

   p_sg->broadcast(sp);

   assert(G_SMP.busy);
   G_SMP.busy = false;

   G_SMP.unlock();

   join(sp);
   idle_loop(sp);

   sp->get_result(local);

   assert(p_pool_size > 0);
   p_pool_size--;
   assert(sp == &p_pool[p_pool_size]);

   poll();
}

Depth Search_Local::extend(Move mv, const Local & local) {
   return (local.list.size() == 1) ? Depth(1) : Depth(0);
}

Depth Search_Local::reduce(Move mv, const Local & local) {

   int red = 0;

   if (local.depth >= 2
    && local.j >= (local.pv_node ? 3 : 1)
    && !move::is_capture(mv, local.node())
    && !move::is_promotion(mv, local.node())
    ) {

      if (!local.pv_node && local.j >= 4) {
         red = 2;
      } else { // includes PV nodes
         red = 1;
      }
   }

   return Depth(red);
}

void Search_Local::inc_node() {
   p_node++;
   if ((p_node & ml::bit_mask(12)) == 0) p_sg->poll();
   if ((p_node & ml::bit_mask( 8)) == 0) poll();
}

Score Search_Local::leaf(Score sc, Ply ply) {
   assert(score::is_ok(sc));
   mark_leaf(ply);
   return sc;
}

void Search_Local::mark_leaf(Ply ply) {
   p_leaf++;
   p_ply_sum += ply;
}

Score Search_Local::bb_probe(const Pos & pos, Ply ply) {

   switch (bb::probe_raw(pos)) {
      case bb::Win  : return +score::BB_Inf - Score(ply);
      case bb::Loss : return -score::BB_Inf + Score(ply);
      case bb::Draw : return Score(0);
      default :       assert(false); return Score(0);
   }
}

Score Search_Local::eval(const Pos & pos) {
   return ::eval(pos, pos.turn());
}

void Search_Local::poll() {
   if (stop()) throw Abort();
}

bool Search_Local::stop() const {

   for (Split_Point * sp = top_sp(); sp != nullptr; sp = sp->parent()) {
      if (sp->stop()) return true;
   }

   return false;
}

void Search_Local::push_sp(Split_Point * sp) {

   lock();

   if (!p_stack.empty()) { // for debug
      Split_Point * top = top_sp();
      assert(sp->is_child(top));
   }

   p_stack.add(sp);

   unlock();
}

void Search_Local::pop_sp(Split_Point * sp) { // sp for debug

   lock();
   assert(top_sp() == sp);
   p_stack.remove();
   unlock();
}

Split_Point * Search_Local::top_sp() const {

   assert(!p_stack.empty());
   return p_stack[p_stack.size() - 1];
}

void Split_Point::init_root() {

   p_parent = nullptr;

   p_workers = 1; // master
   p_stop = false;
}

void Split_Point::init(Split_Point * parent, Search_Global & sg, const Local & local) {

   assert(parent != nullptr);

   p_parent = parent;
   p_sg = &sg;

   p_local = local;

   p_workers = 1; // master
   p_stop = false;
}

void Split_Point::get_result(Local & local) {
   local = p_local;
}

void Split_Point::enter() {
   assert(p_workers != 0);
   p_workers++;
}

void Split_Point::leave() {
   assert(p_workers != 0);
   p_workers--;
}

Move Split_Point::get_move(Local & local) {

   Move mv = move::None;

   lock();

   if (p_local.score < p_local.beta && p_local.i < p_local.list.size()) {

      mv = p_local.list.move(p_local.i++);

      local.score = p_local.score;
      local.j = p_local.j;
   }

   unlock();

   return mv;
}

void Split_Point::update(Move mv, Score sc, const Line & pv) {

   lock();

   if (p_local.score < p_local.beta) { // ignore superfluous moves after a fail high
      local_update(p_local, mv, sc, pv, *p_sg);
      if (p_local.score >= p_local.beta) p_stop = true;
   }

   unlock();
}

static void local_update(Local & local, Move mv, Score sc, const Line & pv, Search_Global & sg) { // MOVE ME

   local.j++;
   assert(local.j <= local.i);

   if (sc > local.score) {

      local.move = mv;
      local.score = sc;
      local.pv.concat(mv, pv);

      if (local.ply == 0) { // root event
         sg.new_best_move(local.move, local.score, local.depth, local.pv);
      }
   }
}

void Split_Point::stop_root() {
   p_stop = true;
}

bool Split_Point::is_child(Split_Point * sp) {

   for (Split_Point * s = this; s != nullptr; s = s->p_parent) {
      if (s == sp) return true;
   }

   return false;
}

void Line::set(Move mv) {
   clear();
   add(mv);
}

void Line::concat(Move mv, const Line & pv) {

   clear();
   add(mv);

   for (int i = 0; i < pv.size(); i++) {
      add(pv[i]);
   }
}

std::string Line::to_string(const Pos & pos, int size_max) const {

   std::string s;

   int size = this->size();
   if (size_max != 0 && size > size_max) size = size_max;

   Pos new_pos = pos;

   for (int i = 0; i < size; i++) {

      Move mv = move(i);

      if (s != "") s += " ";
      s += move::to_string(mv, new_pos);

      new_pos = new_pos.succ(mv);
   }

   return s;
}

std::string Line::to_hub() const {

   std::string s;

   for (int i = 0; i < this->size(); i++) {

      Move mv = move(i);

      if (s != "") s += " ";
      s += move::to_hub(mv);
   }

   return s;
}

