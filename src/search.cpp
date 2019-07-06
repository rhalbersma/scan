
// includes

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "bb_base.hpp"
#include "book.hpp"
#include "common.hpp"
#include "eval.hpp"
#include "gen.hpp"
#include "hash.hpp"
#include "hub.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "thread.hpp"
#include "tt.hpp"
#include "var.hpp"

// types

enum ID : int { ID_Main = 0 };

class Time {

private:

   double m_time_0; // target
   double m_time_1; // extended

public:

   void init (const Search_Input & si, const Pos & pos);

   double time_0 () const { return m_time_0; }
   double time_1 () const { return m_time_1; }

private:

   void init (double time);
   void init (int moves, double time, double inc, const Pos & pos);
};

struct Local {

private:

   const Node * m_node {nullptr};

public:

   Local () = default;
   explicit Local (const Node & node) { m_node = &node; }

   Score alpha;
   Score beta;
   Depth depth;
   Ply ply;
   bool pv_node;
   bool prune;

   Move skip_move {move::None};
   Move sing_move {move::None};
   Score sing_score {score::None};

   List list;
   int i {0};
   int j {0};

   Move move {move::None};
   Score score {score::None};
   Line pv;

   const Node & node () const { assert(m_node != nullptr); return *m_node; }
};

class Search_Global;
class Search_Local;

class Split_Point : public Lockable {

private:

   Split_Point * m_parent;
   Search_Global * m_sg;

   Local m_local;

   std::atomic<int> m_workers;
   std::atomic<bool> m_stop;

public:

   void init_root  ();
   void init       (Split_Point * parent, Search_Global & sg, const Local & local);
   void get_result (Local & local);

   void enter ();
   void leave ();

   Move get_move (Local & local);
   void update   (Move mv, Score sc, const Line & pv);

   void stop_root ();

   bool is_child (Split_Point * sp);

   bool stop () const { return m_stop; }
   bool free () const { return m_workers == 0; }

   Split_Point * parent () const { return m_parent; }
   const Local & local  () const { return m_local; }
};

class Search_Local : public Lockable {

private:

   static const int Pool_Size {10};

   std::thread m_thread;
   ID m_id;

   std::atomic<Split_Point *> m_work;
   ml::Array<Split_Point *, Ply_Size> m_stack;
   Split_Point m_pool[Pool_Size];
   std::atomic<int> m_pool_size;

   Search_Global * m_sg;

   int64 m_node;
   int64 m_leaf;
   int64 m_ply_sum;

public:

   void init (ID id, Search_Global & sg);
   void end  ();

   void start_iter ();
   void end_iter   (Search_Output & so);

   void search_root_try (const Node & node, const List & list, Depth depth);

   void give_work (Split_Point * sp);

   bool idle (Split_Point * parent) const;
   bool idle () const;

private:

   static void launch (Search_Local * sl, Split_Point * root_sp);

   void idle_loop (Split_Point * wait_sp);

   void join      (Split_Point * sp);
   void move_loop (Split_Point * sp);

   void  search_asp  (const Node & node, const List & list, Depth depth, Ply ply, bool prune);
   void  search_root (const Node & node, const List & list, Score alpha, Score beta, Depth depth, Ply ply, bool prune);
   Score search      (const Node & node, Score alpha, Score beta, Depth depth, Ply ply, bool prune, Move skip_move, Line & pv);
   Score qs          (const Node & node, Score alpha, Score beta, Depth depth, Ply ply, Line & pv);

   void  move_loop   (Local & local);
   Score search_move (Move mv, const Local & local, Line & pv);

   void split (Local & local);

   static Depth extend (Move mv, const Local & local);
   static Depth reduce (Move mv, const Local & local);

   void inc_node ();

   Score end_score (const Pos & pos, Ply ply);
   Score leaf      (Score sc, Ply ply);
   void  mark_leaf (Ply ply);

   static Score bb_probe (const Pos & pos, Ply ply);

   void poll ();
   bool stop () const;

   void push_sp (Split_Point * sp);
   void pop_sp  (Split_Point * sp);

   Split_Point * top_sp () const;
};

class Search_Global : public Lockable {

private:

   const Search_Input * m_si;
   Search_Output * m_so;

   const Node * m_node;
   List m_list;

   int m_bb_size;

   Search_Local m_sl[16];

   Split_Point m_root_sp;

   Depth m_depth;

   std::atomic<bool> m_ponder;
   std::atomic<bool> m_flag;

   Move m_last_move;
   Score m_last_score;

   double m_factor;

public:

   void init (const Search_Input & si, Search_Output & so, const Node & node, const List & list, int bb_size);
   void end  ();

   void search        (Depth depth);
   void collect_stats ();

   void new_best_move (Move mv, Score sc, Flag flag, Depth depth, const Line & pv);

   void poll  ();
   void abort ();

   bool has_worker () const;
   void broadcast  (Split_Point * sp);

   List & list () { return m_list; } // HACK

   Split_Point * root_sp () { return &m_root_sp; }

   void set_flag () { m_flag = true; }

   Depth depth () const { return m_depth; }

   bool ponder () const { return m_ponder; }

   Move  last_move  () const { return m_last_move; }
   Score last_score () const { return m_last_score; }

   double factor () const { return m_factor; }

   int bb_size () const { return m_bb_size; }

   const Search_Local & sl (ID id) const { return m_sl[id]; }
         Search_Local & sl (ID id)       { return m_sl[id]; }

   const Search_Input  & si () const { assert(m_si != nullptr); return *m_si; }
   const Search_Output & so () const { assert(m_so != nullptr); return *m_so; }
};

struct SMP : public Lockable {
   std::atomic<bool> busy {false};
};

class Abort : public std::exception {};

// variables

static Time G_Time;

static SMP G_SMP; // lock to create and broadcast split points
static Lockable G_IO;

// prototypes

inline int depth_min () { return (var::Variant == var::Losing) ? 6 : 12; }

static void gen_moves_bb (List & list, const Pos & pos);

static double lerp (double mg, double eg, double phase);

static double time_lag (double time);

static void local_update (Local & local, Move mv, Score sc, const Line & pv, Search_Global & sg);

static Flag flag (Score sc, Score alpha, Score beta);

// functions

void search(Search_Output & so, const Node & node, const Search_Input & si) {

   // init

   var::update();

   so.init(si, node);

   int bb_size = var::BB_Size;

   if (bb::pos_is_load(node)) { // root position already in bitbases => only use smaller bitbases in search
      G_TT.clear();
      bb_size = pos::size(node) - 1;
   }

   // special cases

   List list;
   gen_moves_bb(list, node);
   assert(list.size() != 0);

   if (si.move && !si.ponder && list.size() == 1) {

      Move mv = list[0];
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
   sg.init(si, so, node, list, bb_size); // also launches threads

   // iterative deepening

   try {

      for (int d = 1; d <= si.depth; d++) {

         Depth depth = Depth(d);

         sg.search(depth);
         sg.collect_stats();

         // early exit?

         bool abort = false;

         if (si.smart && so.time() >= G_Time.time_0() * sg.factor() * lerp(0.4, 0.8, pos::phase(node))) {
            abort = true;
         }

         if (depth >= depth_min() && abort) {
            sg.set_flag();
            if (!sg.ponder()) break;
         }
      }

   } catch (const Abort &) {

   }

   if (si.output == Output_Terminal) std::cout << std::endl;

   sg.end(); // sync with threads
   so.end();
}

Move quick_move(const Pos & pos) {

   // init

   if (pos::is_wipe(pos)) return move::None; // for BT variant

   List list;
   gen_moves_bb(list, pos);

   if (list.size() == 0) return move::None;
   if (list.size() == 1) return list[0];

   // book

   if (var::Book) {

      Move mv;
      Score sc;

      if (book::probe(pos, Score(0), mv, sc)) return mv;
   }

   // transposition table

   Move_Index tt_move = Move_Index_None;

   Score tt_score;
   Flag tt_flag;
   Depth tt_depth;

   G_TT.probe(hash::key(pos), tt_move, tt_score, tt_flag, tt_depth); // updates tt_move

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

      if (book::probe(pos, Score(0), mv, sc)) return sc;
   }

   // transposition table

   Move_Index tt_move = Move_Index_None;

   Score tt_score;
   Flag tt_flag;
   Depth tt_depth;

   if (G_TT.probe(hash::key(pos), tt_move, tt_score, tt_flag, tt_depth)) {
      return score::from_tt(tt_score, Ply_Root);
   }

   return score::None;
}

static void gen_moves_bb(List & list, const Pos & pos) {

   if (bb::pos_is_load(pos)) { // root position already in bitbases

      // filter root moves using BB probes

      list.clear();

      int node = bb::probe(pos);

      List tmp;
      gen_moves(tmp, pos);

      for (Move mv : tmp) {
         if (bb::value_age(bb::probe(pos.succ(mv))) == node) list.add(mv); // optimal move
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

   move = true;
   book = true;
   depth = Depth_Max;
   nodes = 1E12;
   input = false;
   output = Output_None;

   smart = false;
   moves = 0;
   time = 1E6;
   inc = 0.0;
   ponder = false;
}

void Search_Input::set_time(int moves, double time, double inc) {
   smart = true;
   this->moves = moves;
   this->time = time;
   this->inc = inc;
}

void Search_Output::init(const Search_Input & si, const Pos & pos) {

   m_si = &si;
   m_pos = pos;

   move = move::None;
   answer = move::None;
   score = score::None;
   flag = Flag::None;
   depth = Depth(0);
   pv.clear();

   m_timer.reset();
   m_timer.start();

   node = 0;
   leaf = 0;
   ply_sum = 0;
}

void Search_Output::end() {
   m_timer.stop();
}

void Search_Output::new_best_move(Move mv, Score sc) {

   Line pv;
   pv.set(mv);

   new_best_move(mv, sc, Flag::Exact, Depth(0), pv);

   if (m_si->output == Output_Terminal) std::cout << std::endl;
}

void Search_Output::new_best_move(Move mv, Score sc, Flag flag, Depth depth, const Line & pv) {

   if (pv.size() != 0) assert(pv[0] == mv);

   move = mv;
   answer = (pv.size() < 2) ? move::None : pv[1];
   score = sc;
   this->flag = flag;
   this->depth = depth;
   this->pv = pv;

   if (var::SMP) G_IO.lock();

   double time = this->time();
   double speed = (time < 0.01) ? 0.0 : double(node) / time;

   switch (m_si->output) {

      case Output_None :

         // no-op
         break;

      case Output_Terminal :

         std::printf("%2d/%4.1f%+7.2f%11ld%7.2f%5.1f  %s\n", depth, ply_avg(), double(score) / 100.0, node, time, speed / 1E6, pv.to_string(m_pos, 7).c_str());
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
         if (pv.size() != 0)       hub::add_pair(line, "pv", pv.to_hub(m_pos));
         hub::write(line);

         break;
      }
   }

   if (var::SMP) G_IO.unlock();
}

double Search_Output::ply_avg() const {
   return (leaf == 0) ? 0.0 : double(ply_sum) / double(leaf);
}

double Search_Output::time() const {
   return m_timer.elapsed();
}

void Time::init(const Search_Input & si, const Pos & pos) {

   if (si.smart) {
      init(si.moves, si.time, si.inc, pos);
   } else {
      init(si.time);
   }
}

void Time::init(double time) {
   m_time_0 = time;
   m_time_1 = time;
}

void Time::init(int moves, double time, double inc, const Pos & pos) {

   double moves_left = lerp(30.0, 10.0, pos::phase(pos));
   if (moves != 0) moves_left = std::min(moves_left, double(moves));

   double factor = 1.3;
   if (var::Ponder) factor *= 1.2;

   double total = std::max(time + inc * moves_left, 0.0);
   double alloc = total / moves_left * factor;

   if (moves > 1) { // save some time for the following moves
      double total_safe = std::max((time / double(moves - 1) + inc - (time / double(moves) + inc) * 0.5) * double(moves - 1), 0.0);
      total = std::min(total, total_safe);
   }

   double max = time_lag(std::min(total, time + inc) * 0.95);

   m_time_0 = std::min(time_lag(alloc), max);
   m_time_1 = std::min(time_lag(alloc * 4.0), max);

   assert(0.0 <= m_time_0 && m_time_0 <= m_time_1);
}

static double time_lag(double time) {
   return std::max(time - 0.1, 0.0);
}

void Search_Global::init(const Search_Input & si, Search_Output & so, const Node & node, const List & list, int bb_size) {

   m_si = &si;
   m_so = &so;

   m_node = &node;
   m_list = list;

   m_depth = Depth(0);

   m_ponder = si.ponder;
   m_flag = false;

   m_last_move = move::None;
   m_last_score = score::None;

   m_factor = 1.0;

   // new search

   m_bb_size = bb_size;

   G_SMP.busy = false;
   m_root_sp.init_root();

   for (int id = 0; id < var::Threads; id++) {
      sl(ID(id)).init(ID(id), *this); // also launches a thread if id /= 0
   }

   G_TT.inc_date();
   sort_clear();
}

void Search_Global::collect_stats() {

   m_so->node = 0;
   m_so->leaf = 0;
   m_so->ply_sum = 0;

   for (int id = 0; id < var::Threads; id++) {
      sl(ID(id)).end_iter(*m_so);
   }
}

void Search_Global::end() {

   abort();

   m_root_sp.leave();
   assert(m_root_sp.free());

   for (int id = 0; id < var::Threads; id++) {
      sl(ID(id)).end();
   }
}

void Search_Global::search(Depth depth) {

   m_depth = depth;

   for (int id = 0; id < var::Threads; id++) {
      sl(ID(id)).start_iter();
   }

   sl(ID_Main).search_root_try(*m_node, m_list, depth);

   // time management

   Move  mv = m_so->move;
   Score sc = m_so->score;

   if (m_si->smart && depth > 1 && mv == m_last_move) {
      m_factor = std::max(m_factor * 0.9, 0.6);
   }

   m_last_move = mv;
   m_last_score = sc;
}

void Search_Global::new_best_move(Move mv, Score sc, Flag flag, Depth depth, const Line & pv) {

   if (var::SMP) lock();

   Move bm = m_so->move;

   collect_stats(); // update search info
   m_so->new_best_move(mv, sc, flag, depth, pv);

   int i = list::find(m_list, mv);
   m_list.move_to_front(i);

   if (depth > 1 && mv != bm) {

      m_flag = false;

      if (m_si->smart) {
         m_factor = std::max(m_factor, 1.0);
         m_factor = std::min(m_factor * 1.2, 2.0);
      }
   }

   if (var::SMP) unlock();
}

void Search_Global::poll() {

   bool abort = false;

   // input event?

   if (var::SMP) G_IO.lock();

   if (m_si->input && has_input()) {

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
         m_ponder = false;
         if (m_flag || m_list.size() == 1) abort = true;
      } else { // other command => abort search
         m_ponder = false;
         abort = true;
      }
   }

   if (var::SMP) G_IO.unlock();

   // time limit?

   double time = m_so->time();

   if (m_depth <= depth_min()) {
      // no-op
   } else if (time >= G_Time.time_1()) {
      abort = true;
   } else if (m_si->smart) {
      // no-op
   } else if (time >= G_Time.time_0() * factor()) {
      abort = true;
   }

   if (abort) {
      m_flag = true;
      if (!m_ponder) this->abort();
   }
}

void Search_Global::abort() {
   m_root_sp.stop_root();
}

bool Search_Global::has_worker() const {

   if (G_SMP.busy) return false;

   for (int id = 0; id < var::Threads; id++) {
      if (sl(ID(id)).idle()) return true;
   }

   return false;
}

void Search_Global::broadcast(Split_Point * sp) {

   for (int id = 0; id < var::Threads; id++) {
      sl(ID(id)).give_work(sp);
   }
}

void Search_Local::init(ID id, Search_Global & sg) {

   m_id = id;

   m_work = sg.root_sp(); // to make it non-null
   m_stack.clear();
   m_pool_size = 0;

   m_sg = &sg;

   m_node = 0;
   m_leaf = 0;
   m_ply_sum = 0;

   if (var::SMP && m_id != ID_Main) m_thread = std::thread(launch, this, sg.root_sp());
}

void Search_Local::launch(Search_Local * sl, Split_Point * root_sp) {
   sl->idle_loop(root_sp);
}

void Search_Local::end() {
   if (var::SMP && m_id != ID_Main) m_thread.join();
}

void Search_Local::start_iter() {
   // m_node = 0;
   m_leaf = 0;
   m_ply_sum = 0;
}

void Search_Local::end_iter(Search_Output & so) {

   if (var::SMP || m_id == ID_Main) {
      so.node += m_node;
      so.leaf += m_leaf;
      so.ply_sum += m_ply_sum;
   }
}

void Search_Local::idle_loop(Split_Point * wait_sp) {

   push_sp(wait_sp);

   while (true) {

      assert(m_work == m_sg->root_sp());
      m_work = nullptr;

      while (!wait_sp->free() && m_work.load() == nullptr) // spin
         ;

      Split_Point * work = m_work.exchange(m_sg->root_sp()); // to make it non-null
      if (work == nullptr) break;

      join(work);
   }

   pop_sp(wait_sp);

   assert(wait_sp->free());
   assert(m_work == m_sg->root_sp());
}

void Search_Local::give_work(Split_Point * sp) {

   if (idle(sp->parent())) {

      sp->enter();

      assert(m_work.load() == nullptr);
      m_work = sp;
   }
}

bool Search_Local::idle(Split_Point * parent) const {

   lock();
   bool idle = this->idle() && parent->is_child(top_sp());
   unlock();

   return idle;
}

bool Search_Local::idle() const {
   return m_work.load() == nullptr;
}

void Search_Local::search_root_try(const Node & node, const List & list, Depth depth) {

   assert(list.size() != 0);
   assert(depth > 0 && depth <= Depth_Max);

   assert(m_stack.empty());
   push_sp(m_sg->root_sp());

   try {
      search_asp(node, list, depth, Ply_Root, true);
   } catch (const Abort &) {
      pop_sp(m_sg->root_sp());
      assert(m_stack.empty());
      throw;
   }

   pop_sp(m_sg->root_sp());
   assert(m_stack.empty());
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

      if (mv != local.skip_move) {

         Line pv;
         Score sc = search_move(mv, local, pv);

         sp->update(mv, sc, pv);
      }
   }
}

void Search_Local::search_asp(const Node & node, const List & list, Depth depth, Ply ply, bool prune) {

   assert(list.size() != 0);
   assert(depth > 0 && depth <= Depth_Max);
   assert(ply == Ply_Root);

   Score last_score = m_sg->last_score();

   // window loop

   if (depth >= 4 && score::is_eval(last_score)) {

      int margin = (var::Variant == var::Normal) ? 10 : 20;

      int alpha_margin = margin;
      int beta_margin  = margin;

      while (std::max(alpha_margin, beta_margin) < 500) {

         Score alpha = last_score - Score(alpha_margin);
         Score beta  = last_score + Score(beta_margin);
         assert(-score::Eval_Inf <= alpha && alpha < beta && beta <= +score::Eval_Inf);

         search_root(node, list, alpha, beta, depth, ply, prune);
         Score sc = m_sg->so().score;

         if (!score::is_eval(sc)) {
            break;
         } else if (sc <= alpha) {
            alpha_margin *= 2;
         } else if (sc >= beta) {
            beta_margin *= 2;
         } else {
            assert(sc > alpha && sc < beta);
            return;
         }
      }
   }

   search_root(node, list, -score::Inf, +score::Inf, depth, ply, prune);
}

void Search_Local::search_root(const Node & node, const List & list, Score alpha, Score beta, Depth depth, Ply ply, bool prune) {

   assert(list.size() != 0);
   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth > 0 && depth <= Depth_Max);
   assert(ply == Ply_Root);

   // init

   Local local(node);

   local.alpha = alpha;
   local.beta = beta;
   local.depth = depth;
   local.ply = ply;
   local.pv_node = beta != alpha + Score(1);
   local.prune = prune;

   local.skip_move = move::None;
   local.sing_move = move::None;
   local.sing_score = score::None;

   local.move = move::None;
   local.score = score::None;
   local.pv.clear();

   // move loop

   local.list = list;

   move_loop(local);
}

Score Search_Local::search(const Node & node, Score alpha, Score beta, Depth depth, Ply ply, bool prune, Move skip_move, Line & pv) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth <= Depth_Max);
   assert(ply <= Ply_Max);

   assert(!m_stack.empty());
   assert(m_stack[0] == m_sg->root_sp());

   // QS

   pv.clear();

   if (node.is_draw(2)) return leaf(Score(0), ply);

   if (depth <= 0) return qs(node, alpha, beta, Depth(0), ply, pv);

   if (pos::is_wipe(node)) return end_score(node, ply); // for BT variant

   // init

   Local local(node);

   local.alpha = alpha;
   local.beta = beta;
   local.depth = depth;
   local.ply = ply;
   local.pv_node = beta != alpha + Score(1);
   local.prune = prune;

   local.skip_move = skip_move;
   local.sing_move = move::None;
   local.sing_score = score::None;

   local.move = move::None;
   local.score = score::None;
   local.pv.clear();

   // transposition table

   Move_Index tt_move = Move_Index_None;
   Key key = hash::key(node);

   if (local.skip_move != move::None) key ^= Key(local.skip_move);

   {
      Score tt_score;
      Flag tt_flag;
      Depth tt_depth;

      if (G_TT.probe(key, tt_move, tt_score, tt_flag, tt_depth)) {

         tt_score = score::from_tt(tt_score, local.ply);

         if (tt_depth >= local.depth && tt_score != score::None) {

            if ((is_lower(tt_flag) && tt_score >= local.beta)
             || (is_upper(tt_flag) && tt_score <= local.alpha)
             ||  is_exact(tt_flag)
             ) {
               return tt_score;
            }
         }

         if ((is_lower(tt_flag) && score::is_win (tt_score) && tt_score >= local.beta)
          || (is_upper(tt_flag) && score::is_loss(tt_score) && tt_score <= local.alpha)
          ) {
            return tt_score;
         }

         if (tt_depth >= local.depth - 4 && is_lower(tt_flag) && score::is_eval(tt_score)) {
            gen_moves(local.list, node); // HACK ###
            local.sing_move  = list::find_index(local.list, tt_move, node);
            local.sing_score = tt_score;
         }
      }
   }

   // bitbases

   if (bb::pos_is_search(node, m_sg->bb_size())) {

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

   if (local.list.size() == 0) return end_score(node, local.ply); // no legal moves => end

   if (score::loss(local.ply + Ply(2)) >= local.beta) { // loss-distance pruning
      return leaf(score::loss(local.ply + Ply(2)), local.ply);
   }

   if (local.ply >= Ply_Max) return leaf(eval(node), local.ply);

   // pruning

   if (local.prune
    && !local.pv_node
    && local.depth >= 3
    && score::is_eval(local.beta)
    ) {

      Score margin = Score(local.depth * 10);
      Score new_beta = local.beta + margin;
      Depth new_depth = Depth(local.depth * 40 / 100);

      Line new_pv;
      Score sc = search(node, new_beta - Score(1), new_beta, new_depth, local.ply + Ply(1), false, move::None, new_pv);

      if (sc >= new_beta) {

         sc -= margin; // fail-soft margin
         assert(sc >= local.beta);

         pv = new_pv;
         return sc;
      }
   }

   // move loop

   sort_moves(local.list, node, tt_move);
   move_loop(local);

cont : // epilogue

   assert(score::is_ok(local.score));

   // transposition table

   {
      Move_Index tt_move = (local.score > local.alpha) ? move::index(local.move, node) : Move_Index_None;
      Score tt_score = score::to_tt(local.score, local.ply);
      Flag tt_flag = flag(local.score, local.alpha, local.beta);
      Depth tt_depth = local.depth;

      G_TT.store(key, tt_move, tt_score, tt_flag, tt_depth);
   }

   // move-ordering statistics

   if (local.score > local.alpha
    && local.move != move::None
    && local.list.size() > 1
    && local.skip_move == move::None
    ) {

      good_move(local.move, node);

      assert(list::has(local.list, local.move));

      for (Move mv : local.list) {
         if (mv == local.move) break;
         bad_move(mv, node);
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

      // SMP

      if (var::SMP
       && local.depth >= 6
       && searched_size != 0
       && local.list.size() - searched_size >= 5
       && m_sg->has_worker()
       && m_pool_size < Pool_Size
       ) {
         split(local);
         return;
      }

      // search move

      Move mv = local.list[local.i++];

      if (mv != local.skip_move) {

         Line pv;
         Score sc = search_move(mv, local, pv);

         local_update(local, mv, sc, pv, *m_sg);
      }
   }
}

Score Search_Local::search_move(Move mv, const Local & local, Line & pv) {

   // init

   const Node & node = local.node();

   int searched_size = local.j;

   Depth ext = extend(mv, local);
   Depth red = reduce(mv, local);
   if (ext != 0 && red != 0) red = Depth(0);

   if (local.pv_node
    && local.depth >= 8
    && mv == local.sing_move
    && local.skip_move == move::None
    && ext == 0
    ) {

      assert(red == 0);

      Score new_alpha = local.sing_score - Score(40);

      Line new_pv;
      Score sc = search(node, new_alpha, new_alpha + Score(1), local.depth - Depth(4), local.ply, local.prune, mv, new_pv);

      if (sc <= new_alpha) ext = Depth(1);
   }

   Score new_alpha = std::max(local.alpha, local.score);
   Depth new_depth = local.depth + ext - Depth(1);

   assert(new_alpha < local.beta);

   // search move

   Score sc;

   inc_node();

   Node new_node = local.node().succ(mv);

   if ((local.pv_node && searched_size != 0) || red != 0) {

      sc = -search(new_node, -new_alpha - Score(1), -new_alpha, new_depth - red, local.ply + Ply(1), local.prune, move::None, pv);

      if (sc > new_alpha) { // PVS/LMR re-search
         sc = -search(new_node, -local.beta, -new_alpha, new_depth, local.ply + Ply(1), local.prune, move::None, pv);
      }

   } else {

      sc = -search(new_node, -local.beta, -new_alpha, new_depth, local.ply + Ply(1), local.prune, move::None, pv);
   }

   assert(score::is_ok(sc));
   return sc;
}

Score Search_Local::qs(const Node & node, Score alpha, Score beta, Depth depth, Ply ply, Line & pv) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth <= 0);
   assert(ply <= Ply_Max);

   // init

   pv.clear();

   if (pos::is_wipe(node)) return end_score(node, ply); // for BT variant

   if (score::loss(ply + Ply(2)) >= beta) { // loss-distance pruning
      return leaf(score::loss(ply + Ply(2)), ply);
   }

   if (ply >= Ply_Max) return leaf(eval(node), ply);

   // move-loop init

   Score bs = score::None;

   List list;
   gen_captures(list, node);

   if (list.size() == 0) { // quiet position

      // bitbases

      if (bb::pos_is_search(node, m_sg->bb_size())) {
         return leaf(bb_probe(node, ply), ply);
      }

      // threat position?

      if (depth == 0 && pos::is_threat(node)) {
         return search(node, alpha, beta, Depth(1), ply + Ply(1), false, move::None, pv); // one-ply search
      }

      // stand pat

      bs = eval(node);
      if (bs >= beta) return leaf(bs, ply);

      list.clear();
      if (var::Variant == var::BT) gen_promotions(list, node);
      if (!pos::has_king(node) && !pos::is_threat(node)) add_sacs(list, node);
   }

   // move loop

   for (Move mv : list) {

      inc_node();

      Line new_pv;
      Score sc = -qs(node.succ(mv), -beta, -std::max(alpha, bs), depth - Depth(1), ply + Ply(1), new_pv);

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

   m_sg->poll();
   poll();

   G_SMP.lock(); // useful?

   assert(!G_SMP.busy);
   G_SMP.busy = true;

   assert(m_pool_size < Pool_Size);
   Split_Point * sp = &m_pool[m_pool_size++];
   sp->init(top_sp(), *m_sg, local);

   m_sg->broadcast(sp);

   assert(G_SMP.busy);
   G_SMP.busy = false;

   G_SMP.unlock();

   join(sp);
   idle_loop(sp);

   sp->get_result(local);

   assert(m_pool_size > 0);
   m_pool_size -= 1;
   assert(sp == &m_pool[m_pool_size]);

   poll();
}

Depth Search_Local::extend(Move mv, const Local & local) {

   int ext = 0;

   const Node & node = local.node();

   if (local.list.size() == 1) ext += 1;

   if (var::Variant == var::Losing) {

      if (move::is_capture(mv, node)) {
         ext += 1;
      } else if (move::is_forcing(mv, node)) {
         if (local.pv_node || local.depth <= 4) ext += 1;
      }
   }

   return Depth(std::min(ext, 1));
}

Depth Search_Local::reduce(Move mv, const Local & local) { // LMR

   int red = 0;

   const Node & node = local.node();

   if (local.depth >= 2
    && local.j >= (local.pv_node ? 3 : 1)
    && !move::is_capture  (mv, node)
    && !move::is_promotion(mv, node)
    ) {
      red = (!local.pv_node && local.j >= 4) ? 2 : 1;
   }

   return Depth(red);
}

void Search_Local::inc_node() {

   m_node += 1;
   if (m_node >= m_sg->si().nodes && m_sg->depth() > 1) m_sg->abort();

   if ((m_node & ml::bit_mask(12)) == 0) m_sg->poll();
   if ((m_node & ml::bit_mask( 4)) == 0) poll();
}

Score Search_Local::end_score(const Pos & pos, Ply ply) { // pos for debug
   assert(pos::is_end(pos));
   Score sc = (var::Variant == var::Losing) ? score::win(ply) : score::loss(ply);
   return leaf(sc, ply);
}

Score Search_Local::leaf(Score sc, Ply ply) {
   assert(score::is_ok(sc));
   mark_leaf(ply);
   return sc;
}

void Search_Local::mark_leaf(Ply ply) {
   m_leaf += 1;
   m_ply_sum += ply;
}

Score Search_Local::bb_probe(const Pos & pos, Ply ply) {

   switch (bb::probe_raw(pos)) {
      case bb::Win :  return +score::BB_Inf - Score(ply);
      case bb::Loss : return -score::BB_Inf + Score(ply);
      case bb::Draw : return Score(0);
      default :       assert(false); return Score(0);
   }
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

   if (!m_stack.empty()) assert(sp->is_child(top_sp()));
   m_stack.add(sp);

   unlock();
}

void Search_Local::pop_sp(Split_Point * sp) { // sp for debug

   lock();
   assert(top_sp() == sp);
   m_stack.remove();
   unlock();
}

Split_Point * Search_Local::top_sp() const {

   assert(!m_stack.empty());
   return m_stack[m_stack.size() - 1];
}

void Split_Point::init_root() {

   m_parent = nullptr;

   m_workers = 1; // master
   m_stop = false;
}

void Split_Point::init(Split_Point * parent, Search_Global & sg, const Local & local) {

   assert(parent != nullptr);

   m_parent = parent;
   m_sg = &sg;

   m_local = local;

   m_workers = 1; // master
   m_stop = false;
}

void Split_Point::get_result(Local & local) {
   local = m_local;
}

void Split_Point::enter() {
   assert(m_workers != 0);
   m_workers += 1;
}

void Split_Point::leave() {
   assert(m_workers != 0);
   m_workers -= 1;
}

Move Split_Point::get_move(Local & local) {

   Move mv = move::None;

   lock();

   if (m_local.score < m_local.beta && m_local.i < m_local.list.size()) {

      mv = m_local.list[m_local.i++];

      local.score = m_local.score;
      local.j = m_local.j;
   }

   unlock();

   return mv;
}

void Split_Point::update(Move mv, Score sc, const Line & pv) {

   lock();

   if (m_local.score < m_local.beta) { // ignore superfluous moves after a fail high
      local_update(m_local, mv, sc, pv, *m_sg);
      if (m_local.score >= m_local.beta) m_stop = true;
   }

   unlock();
}

static void local_update(Local & local, Move mv, Score sc, const Line & pv, Search_Global & sg) {

   assert(score::is_ok(sc));

   local.j += 1;
   assert(local.j <= local.i);

   if (sc > local.score) {

      local.move = mv;
      local.score = sc;
      local.pv.concat(mv, pv);

      if (local.ply == Ply_Root && (local.j == 1 || sc > local.alpha)) {
         sg.new_best_move(local.move, local.score, flag(local.score, local.alpha, local.beta), local.depth, local.pv);
      }
   }

   assert(score::is_ok(local.score));
}

void Split_Point::stop_root() {
   m_stop = true;
}

bool Split_Point::is_child(Split_Point * sp) {

   for (Split_Point * s = this; s != nullptr; s = s->m_parent) {
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

   for (Move mv : pv) {
      add(mv);
   }
}

std::string Line::to_string(const Pos & pos, int size_max) const {

   std::string s;

   int size = m_move.size();
   if (size_max != 0 && size > size_max) size = size_max;

   Pos new_pos = pos;

   for (int i = 0; i < size; i++) {

      Move mv = m_move[i];

      if (!s.empty()) s += " ";
      s += move::to_string(mv, new_pos);

      new_pos = new_pos.succ(mv);
   }

   return s;
}

std::string Line::to_hub(const Pos & pos) const {

   std::string s;

   Pos new_pos = pos;

   for (Move mv : m_move) {

      if (!s.empty()) s += " ";
      s += move::to_hub(mv, new_pos);

      new_pos = new_pos.succ(mv);
   }

   return s;
}

static Flag flag(Score sc, Score alpha, Score beta) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);

   Flag flag = Flag::None;
   if (sc > alpha) flag |= Flag::Lower;
   if (sc < beta)  flag |= Flag::Upper;

   return flag;
}

