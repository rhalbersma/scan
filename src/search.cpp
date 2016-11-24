
// search.cpp

// includes

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "bb_base.h"
#include "board.h"
#include "book.h"
#include "eval.h"
#include "hash.h"
#include "libmy.hpp"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "pos.h"
#include "score.h"
#include "search.h"
#include "sort.h"
#include "thread.h"
#include "trans.h"
#include "var.h"

// classes

class Time {

private :

   double p_time_0;
   double p_time_1;

public :

   void init (const Board & bd);

   double time_0 () const { return p_time_0; }
   double time_1 () const { return p_time_1; }

private :

   void init (double time);
   void init (int moves, double time, double inc, const Board & bd);
};

struct Local {

   int alpha;
   int beta;
   int depth;
   bool pv_node;
   bool safe;
   bool prune;

   List list;
   int i;
   int j;

   move_t move;
   int score;
   Line pv;
};

class Search_Local;

class Split_Point : public Lockable {

private :

   Split_Point * p_parent;

   Board p_board;
   int p_ply;

   Local p_local;

   std::atomic<int> p_workers;
   std::atomic<bool> p_stop;

public :

   void init_root  ();
   void init       (Split_Point * parent, const Board & bd, int ply, const Local & local);
   void get_result (Local & local);

   void enter ();
   void leave ();

   move_t get_move   (Local & local);
   void   stop_root  ();
   void   put_result (move_t mv, int sc, const Line & pv);

   bool is_child (Split_Point * sp);

   bool stop () const { return p_stop; }
   bool free () const { return p_workers == 0; }

   Split_Point * parent () const { return p_parent; }
   const Board & board  () const { return p_board; }
   int           ply    () const { return p_ply; }
   const Local & local  () const { return p_local; }

private :

   bool high () const { return p_local.score >= p_local.beta; }
};

class Search_Local : public Lockable {

private :

   std::thread p_thread;
   bool p_main;

   std::atomic<Split_Point *> p_work;
   ml::Array<Split_Point *, Ply_Size> p_stack;
   Split_Point p_pool[Ply_Size];
   std::atomic<int> p_pool_size;

   Board p_board;
   int p_ply;

   move_t p_move[Ply_Size];
   Undo p_undo[Ply_Size];

   int64 p_node;
   int64 p_leaf;
   int64 p_ply_sum;

public :

   void init (int id);
   void end  ();

   void new_iter   ();
   void mid_search ();

   void search_all  (const Board & bd, List & list, int depth);
   void search_root (const Board & bd, const List & list, int depth);

   void give_work (Split_Point * sp);

   bool idle (Split_Point * parent) const;
   bool idle () const;

private :

   static void launch (Search_Local * sl);

   void idle_loop (Split_Point * wait_sp);

   void join   (Split_Point * sp);
   void search (Split_Point * sp);

   void search_all  (List & list, int depth);
   void search_root (const List & list, int alpha, int beta, int depth, bool prune);
   int  search      (int alpha, int beta, int depth, bool prune, Line & pv);
   void split       (Local & local);
   int  search_move (move_t mv, const Local & local, Line & pv);
   int  qs          (int alpha, int beta, Line & pv);

   static int extend (move_t mv, const Pos & pos, const Local & local);
   static int reduce (move_t mv, const Pos & pos, const Local & local);

   void do_move   (move_t mv);
   void undo_move (move_t mv);

   void mark_leaf ();

   int probe_bb ();
   int eval     () const;

   void poll ();
   bool stop () const;

   void push_sp (Split_Point * sp);
   void pop_sp  ();

   Split_Point * top_sp () const;
};

class Search_Global : public Lockable {

private :

   trans::Trans p_trans;

   Board p_board;
   List p_list;

   Search_Local p_sl[16];

   std::atomic<bool> p_ponder;
   std::atomic<bool> p_flag;

   bool p_change;
   bool p_first;
   bool p_high;

public :

   void new_search  ();
   void init_search (const Board & bd, const List & list);
   void mid_search  ();
   void end_search  ();

   void start_iter (int depth);
   void end_iter   (int depth);

   void search (int depth);

   void new_best_move (move_t mv, int sc, int depth, const Line & pv);

   void poll  ();
   void abort ();

   bool has_worker () const;
   void broadcast  (Split_Point * sp);

   void set_flag (bool flag) { p_flag = flag; }

   void set_first (bool first) { p_first = first; }
   void set_high  (bool high)  { p_high  = high; }

   int    depth () const { return G_Search.depth(); }
   double time  () const { return G_Search.time(); }

   bool ponder () const { return p_ponder; }

   bool change () const { return p_change; }
   bool first  () const { return p_first; }
   bool high   () const { return p_high; }

   trans::Trans & trans () { return p_trans; }

   const Search_Local & sl (int id) const { return p_sl[id]; }
         Search_Local & sl (int id)       { return p_sl[id]; }
};

struct SMP : public Lockable {
   std::atomic<bool> busy;
};

class Abort : public std::exception {

};

// variables

Search_Info G_Search_Info;
Search G_Search;

static Time G_Time;

static Search_Global SG;
static Split_Point Root_SP[1]; // to make it a "pointer"

static SMP G_SMP; // lock to create and broadcast split points
static Lockable IO;

// prototypes

static void gen_moves_bb (List & list, const Pos & pos);

static void disp_best_move (const Board & bd);

static double alloc_moves (const Board & bd);
static double alloc_early (const Board & bd);

static double lerp (double mg, double eg, double phase);

static double time_lag (double time);

// functions

void search_init() {

   var::update();
   SG.trans().set_size(var::Trans_Size);
}

void search_new_game() {

   SG.trans().clear();
}

void search_id(const Board & bd) {

   // init

   var::update();

   Search_Info & si = G_Search_Info;

   if (pos_size(bd) <= si.bb_size()) { // only use smaller bitbases in search
      SG.trans().clear(); // HACK #
      si.set_bb_size(pos_size(bd) - 1);
   }

   G_Search.init(bd);

   // special cases

   List list;
   gen_moves_bb(list, bd);
   assert(list.size() != 0);

   if (si.unique() && list.size() == 1) {
      G_Search.new_best_move(list.move(0));
      return;
   }

   if (var::Book && si.book()) {

      int ply = 300 - board_pip(bd);
      assert(ply >= 0);

      int margin = (ply < 4) ? var::Book_Margin : 0;
      move_t mv = book::move(bd, margin);

      if (mv != Move_None) {
         G_Search.new_best_move(mv);
         return;
      }
   }

   // more init

   G_Time.init(bd);

   SG.new_search(); // also launches threads

   move_t easy_move = Move_None;

   if (si.smart() && list.size() > 1) {

      SG.sl(0).search_all(bd, list, 1);

      if (list.score(0) - list.score(1) >= +50) {

         easy_move = list.move(0);

         if (easy_move != ponder_move(bd)) {
            easy_move = Move_None;
         }
      }
   }

   // iterative deepening

   try {

      SG.init_search(bd, list);

      for (int depth = 1; depth <= std::min(si.depth(), Depth_Max); depth++) {

         G_Search.start_iter(depth);
         SG.start_iter(depth);
         SG.search(depth);
         SG.end_iter(depth);
         G_Search.end_iter();

         move_t mv = G_Search.move();
         double time = G_Search.time();

         // early exit?

         bool abort = false;

         if (mv == easy_move && !SG.change() && time >= G_Time.time_0() / 16.0) abort = true;

         if (si.smart() && time >= G_Time.time_0() * alloc_early(bd)) abort = true;

         if (depth >= 12 && abort) {
            SG.set_flag(true);
            if (!SG.ponder()) break;
         }
      }

   } catch (const Abort &) {

      // disp_best_move(bd);
   }

   if (si.output() == Output_Terminal) {
      std::cout << std::endl;
   }

   SG.end_search(); // sync with threads

   G_Search.end();
}

move_t ponder_move(const Board & bd) {

   // init

   List list;
   gen_moves_bb(list, bd);

   if (list.size() == 0) return Move_None;
   if (list.size() == 1) return list.move(0);

   // book

   if (var::Book) {
      move_t mv = book::move(bd, 0);
      if (mv != Move_None) return mv;
   }

   // transposition table

   {
      int trans_move = Index_None;

      int trans_depth, trans_flags, trans_score;
      SG.trans().probe(bd.key(), trans_move, trans_depth, trans_flags, trans_score); // updates trans_move

      if (trans_move != Index_None) {
         sort_trans(list, bd, trans_move);
         return list.move(0);
      }
   }

   return Move_None;
}

static void gen_moves_bb(List & list, const Pos & pos) {

   if (var::BB && bb::pos_is_load(pos)) { // filter root moves using BB probes

      list.clear();

      int val = bb::probe(pos);

      List tmp;
      gen_moves(tmp, pos);

      for (int i = 0; i < tmp.size(); i++) {

         move_t mv = tmp.move(i);

         Pos new_pos(pos, mv);
         int new_val = bb::value_age(bb::probe(new_pos));

         if (new_val == val) list.add(mv);
      }

   } else {

      gen_moves(list, pos);
   }
}

static void disp_best_move(const Board & bd) {

   if (var::SMP) IO.lock();

   const Search_Info & si = G_Search_Info;

   double time = G_Search.time();
   double speed = (time < 0.1) ? 0.0 : double(G_Search.node()) / time;
   std::string pv = G_Search.pv().to_string(bd, 7);

   switch (si.output()) {

   case Output_None :

      break;

   case Output_Terminal :

      std::printf("%2d/%4.1f%+7.2f%11lld%7.2f%5.1f  %s\n", G_Search.depth(), G_Search.ply_avg(), double(G_Search.score()) / 100.0, G_Search.node(), time, speed / 1E6, pv.c_str());
      break;

   case Output_Hub :

      std::printf("info %d %.1f %d %lld %.2f %.1f %s\n", G_Search.depth(), G_Search.ply_avg(), G_Search.score(), G_Search.node(), time, speed / 1E6, pv.c_str());
      std::fflush(stdout);
      break;

   default :

      assert(false);
   }

   if (var::SMP) IO.unlock();
}

void Search_Info::init() {

   var::update();

   p_unique = true;
   p_book = true;
   p_depth = Depth_Max;
   p_input = false;
   p_output = Output_None;
   p_bb_size = var::BB_Size;

   p_smart = false;
   p_moves = 0;
   p_time = 1E6;
   p_inc = 0.0;
   p_ponder = false;
}

void Search_Info::set_time(double time) {

   p_smart = false;
   p_time = time;
}

void Search_Info::set_time(int moves, double time, double inc) {

   p_smart = true;
   p_moves = moves;
   p_time = time;
   p_inc = inc;
}

void Search::init(const Board & bd) {

   p_board = bd;

   p_best.move = Move_None;
   p_best.ponder = Move_None;
   p_best.score = 0;
   p_best.depth = 0;
   p_best.pv.clear();

   p_current.timer.reset();
   p_current.timer.start();
   p_current.depth = 0;
   p_current.node_nb = 0;
   p_current.leaf_nb = 0;
   p_current.ply_sum = 0;
}

void Search::end() {

   p_current.timer.stop();
}

void Search::start_iter(int depth) {

   p_current.depth = depth;
}

void Search::end_iter() {

   SG.mid_search();
}

void Search::new_best_move(move_t mv) {

   Line pv;
   pv.set(mv);

   new_best_move(mv, 0, 0, pv);

   const Search_Info & si = G_Search_Info;

   if (si.output() == Output_Terminal) {
      std::cout << std::endl;
   }
}

void Search::new_best_move(move_t mv, int sc, int depth, const Line & pv) {

   assert(depth == p_current.depth);
   if (pv.size() != 0) assert(pv.move(0) == mv);

   p_best.move = mv;
   p_best.ponder = (pv.size() < 2) ? Move_None : pv.move(1);
   p_best.score = sc;
   p_best.depth = depth;
   p_best.pv = pv;

   if (depth == 0 || depth >= 12) disp_best_move(p_board);
}

double Search::time() const {

   return p_current.timer.elapsed();
}

void Time::init(const Board & bd) {

   const Search_Info & si = G_Search_Info;

   if (si.smart()) {
      init(si.moves(), si.time(), si.inc(), bd);
   } else {
      init(si.time());
   }
}

void Time::init(double time) {

   p_time_0 = time;
   p_time_1 = time;
}

void Time::init(int moves, double time, double inc, const Board & bd) {

   double moves_left = alloc_moves(bd);
   if (moves != 0) moves_left = std::min(moves_left, double(moves));

   double factor = 1.2;
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

   assert(0.0 <= p_time_0 && p_time_0 <= p_time_1);
}

static double alloc_moves(const Board & bd) {

   return lerp(30.0, 10.0, board_phase(bd));
}

static double alloc_early(const Board & bd) {

   return lerp(0.4, 0.8, board_phase(bd));
}

static double lerp(double mg, double eg, double phase) {

   assert(phase >= 0.0 && phase <= 1.0);
   return mg + (eg - mg) * phase;
}

static double time_lag(double time) {

   return std::max(time - 0.1, 0.0);
}

void Search_Global::new_search() {

   G_SMP.busy = false;
   Root_SP->init_root();

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).init(id); // also launches a thread if id /= 0
   }

   trans().inc_date();
   sort_clear();
}

void Search_Global::init_search(const Board & bd, const List & list) {

   p_board = bd;
   p_list = list;

   p_ponder = G_Search_Info.ponder();
   p_flag = false;

   p_change = false;
   p_first = false;
   p_high = false;
}

void Search_Global::mid_search() {

   G_Search.p_current.node_nb = 0;
   G_Search.p_current.leaf_nb = 0;
   G_Search.p_current.ply_sum = 0;

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).mid_search();
   }
}

void Search_Global::end_search() {

   abort();

   Root_SP->leave();
   assert(Root_SP->free());

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).end();
   }
}

void Search_Global::start_iter(int /* depth */) {

   for (int id = 0; id < var::SMP_Threads; id++) {
      sl(id).new_iter();
   }
}

void Search_Global::end_iter(int /* depth */) {

}

void Search_Global::search(int depth) {

   sl(0).search_root(p_board, p_list, depth);
}

void Search_Global::new_best_move(move_t mv, int sc, int depth, const Line & pv) {

   if (var::SMP) lock();

   move_t best = G_Search.move();

   mid_search(); // update search info
   G_Search.new_best_move(mv, sc, depth, pv);

   int i = list_find(p_list, mv);
   p_list.mtf(i);

   if (depth > 1 && mv != best) {
      p_flag = false;
      p_change = true;
   }

   if (var::SMP) unlock();
}

void Search_Global::poll() {

   const Search_Info & si = G_Search_Info;

   bool abort = false;

   // input event?

   if (var::SMP) IO.lock();

   if (si.input() && has_input()) {

      std::string line;
      if (!peek_line(line)) { // EOF
         std::exit(EXIT_SUCCESS);
      }

      std::cout << "input \"" << line << "\"" << std::endl;

      std::stringstream ss(line);

      std::string command;
      ss >> command;

      if (command == "ping") {
         get_line(line);
         std::cout << "pong" << std::endl;
      } else if (command == "ponder-hit") {
         get_line(line);
         p_ponder = false;
         if (p_flag) abort = true;
      } else { // other command => abort search
         p_ponder = false;
         abort = true;
      }
   }

   if (var::SMP) IO.unlock();

   // time limit?

   double time = this->time();

   if (depth() <= 12) {
      // no-op
   } else if (time >= G_Time.time_1()) {
      abort = true;
   } else if (si.smart() && !first()) {
      // no-op
   } else if (time >= G_Time.time_0()) {
      abort = true;
   }

   if (abort) {
      p_flag = true;
      if (!p_ponder) this->abort();
   }
}

void Search_Global::abort() {

   Root_SP->stop_root();
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

void Search_Local::init(int id) {

   p_main = id == 0;

   p_work = Root_SP; // to make it non-null
   p_stack.clear();
   p_pool_size = 0;

   p_ply = 0;

   p_node = 0;
   p_leaf = 0;
   p_ply_sum = 0;

   if (var::SMP && !p_main) {
      p_thread = std::thread(launch, this);
   }
}

void Search_Local::launch(Search_Local * sl) {
   sl->idle_loop(Root_SP);
}

void Search_Local::end() {

   if (var::SMP && !p_main) {
      p_thread.join();
   }
}

void Search_Local::new_iter() {

   // p_node = 0;
   p_leaf = 0;
   p_ply_sum = 0;
}

void Search_Local::mid_search() {

   if (var::SMP || p_main) {
      G_Search.p_current.node_nb += p_node;
      G_Search.p_current.leaf_nb += p_leaf;
      G_Search.p_current.ply_sum += p_ply_sum;
   }
}

void Search_Local::idle_loop(Split_Point * wait_sp) {

   push_sp(wait_sp);

   while (true) {

      assert(p_work == Root_SP);
      p_work = NULL;

      while (!wait_sp->free() && p_work.load() == NULL) // spin
         ;

      Split_Point * work = p_work.exchange(Root_SP); // to make it non-null
      if (work == NULL) break;

      join(work);
   }

   pop_sp();

   assert(wait_sp->free());
   assert(p_work == Root_SP);
}

void Search_Local::give_work(Split_Point * sp) {

   if (idle(sp->parent())) {

      sp->enter();

      assert(p_work.load() == NULL);
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

   return p_work.load() == NULL;
}

void Search_Local::search_all(const Board & bd, List & list, int depth) {

   assert(depth > 0 && depth <= Depth_Max);
   assert(list.size() != 0);

   p_board = bd;
   p_ply = 0;

   assert(p_stack.empty());
   push_sp(Root_SP);

   try {
      search_all(list, depth);
   } catch (const Abort &) {
      pop_sp();
      assert(p_stack.empty());
      throw;
   }

   pop_sp();
   assert(p_stack.empty());
}

void Search_Local::search_root(const Board & bd, const List & list, int depth) {

   assert(depth > 0 && depth <= Depth_Max);
   assert(list.size() != 0);

   p_board = bd;
   p_ply = 0;

   assert(p_stack.empty());
   push_sp(Root_SP);

   try {
      search_root(list, -score::Inf, +score::Inf, depth, true);
   } catch (const Abort &) {
      pop_sp();
      assert(p_stack.empty());
      throw;
   }

   pop_sp();
   assert(p_stack.empty());
}

void Search_Local::join(Split_Point * sp) {

   p_board = sp->board();
   p_ply = sp->ply();

   // sp->enter();
   push_sp(sp);

   try {
      search(sp);
   } catch (const Abort &) {
      // no-op
   }

   pop_sp();
   sp->leave();
}

void Search_Local::search(Split_Point * sp) {

   Local local = sp->local();

   while (true) {

      move_t mv = sp->get_move(local); // also updates "local"
      if (mv == Move_None) break;

      Line pv;
      int sc = search_move(mv, local, pv);

      sp->put_result(mv, sc, pv);
   }
}

void Search_Local::search_all(List & list, int depth) {

   assert(depth > 0 && depth <= Depth_Max);

   assert(list.size() != 0);

   // init

   bool prune = true;

   // move loop

   for (int i = 0; i < list.size(); i++) {

      // search move

      move_t mv = list.move(i);

      Line pv;

      do_move(mv);
      int sc = -search(-score::Inf, +score::Inf, depth - 1, prune, pv);
      undo_move(mv);

      // update state

      list.set_score(i, sc);
   }

   list.sort();
}

void Search_Local::search_root(const List & list, int alpha, int beta, int depth, bool prune) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth > 0 && depth <= Depth_Max);

   assert(list.size() != 0);

   // init

   Local local;

   local.alpha = alpha;
   local.beta = beta;
   local.depth = depth;
   local.pv_node = beta != alpha + 1;
   local.safe = local.pv_node;
   local.prune = prune;

   local.list = list;

   local.move = Move_None;
   local.score = score::None;
   local.pv.clear();

   // move loop

   local.j = 0;

   for (local.i = 0; local.i < local.list.size(); local.i++) {

      int searched_size = local.j;

      SG.set_first(searched_size == 0);

      if (var::SMP && local.depth >= 4 && searched_size != 0 && local.list.size() - searched_size >= 3 && SG.has_worker()) {
         split(local);
         break; // share the epilogue
      }

      // search move

      move_t mv = local.list.move(local.i);

      Line pv;
      int sc = search_move(mv, local, pv);

      // update state

      local.j++;

      if (sc > local.score) {

         local.move = mv;
         local.score = sc;
         local.pv.cat(mv, pv);

         assert(p_ply == 0); // root event
         SG.new_best_move(local.move, local.score, local.depth, local.pv);

         if (sc >= local.beta) break;
      }
   }
}

int Search_Local::search(int alpha, int beta, int depth, bool prune, Line & pv) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);
   assert(depth >= 0 && depth <= Depth_Max);

   assert(!p_stack.empty());
   assert(p_stack[0] == Root_SP);

   // QS

   pv.clear();

   Board & bd = p_board;

   if (bd.is_draw(2)) {
      mark_leaf();
      return 0;
   }

   if (depth <= 0) return qs(alpha, beta, pv);

   // init

   Local local;

   local.alpha = alpha;
   local.beta = beta;
   local.depth = depth;
   local.pv_node = beta != alpha + 1;
   local.safe = local.pv_node;
   local.prune = prune;

   local.move = Move_None;
   local.score = score::None;
   local.pv.clear();

   // transposition table

   int trans_move = Index_None;
   uint64 key = bd.key();

   {
      int trans_depth, trans_flags, trans_score;

      if (SG.trans().probe(key, trans_move, trans_depth, trans_flags, trans_score)) {

         if (!local.safe && trans_depth >= local.depth) {

            trans_score = score::from_trans(trans_score, p_ply);

            if ((trans::is_lower(trans_flags) && trans_score >= local.beta)
             || (trans::is_upper(trans_flags) && trans_score <= local.alpha)
             ||  trans::is_exact(trans_flags)) {
               // mark_leaf();
               return trans_score;
            }
         }
      }
   }

   // bitbases

   if (var::BB && bb::pos_is_search(bd)) {
      local.score = qs(local.alpha, local.beta, local.pv);
      goto cont;
   }

   // gen moves

   gen_moves(local.list, bd);

   if (local.list.size() == 0) { // no legal move => loss
      mark_leaf();
      return score::loss(p_ply);
   }

   if (score::loss(p_ply + 2) >= local.beta) { // loss-distance pruning
      mark_leaf();
      return score::loss(p_ply + 2);
   }

   if (p_ply >= Ply_Max) {
      mark_leaf();
      return eval();
   }

   // pruning

   if (local.prune && !local.safe && local.depth >= 3 && local.beta <= +score::Eval_Inf) {

      int margin = local.depth * 5;
      int new_beta = score::add(local.beta, +margin);

      int new_depth = local.depth * 40 / 100;
      assert(new_depth > 0);

      Line new_pv;
      int sc = search(new_beta - 1, new_beta, new_depth, false, new_pv);

      if (sc >= new_beta) {

         sc = score::add(sc, -margin); // fail-soft margin
         assert(sc >= local.beta);

         pv = new_pv;
         return sc;
      }
   }

   // move ordering

   sort_all(local.list, bd, trans_move);

   // move loop

   local.j = 0;

   for (local.i = 0; local.i < local.list.size(); local.i++) {

      int searched_size = local.j;

      if (var::SMP && local.depth >= 4 && searched_size != 0 && local.list.size() - searched_size >= 3 && SG.has_worker()) {
         split(local);
         break; // share the epilogue
      }

      // search move

      move_t mv = local.list.move(local.i);

      Line new_pv;
      int sc = search_move(mv, local, new_pv);

      // update state

      local.j++;

      if (sc > local.score) {

         local.move = mv;
         local.score = sc;
         local.pv.cat(mv, new_pv);

         assert(p_ply != 0); // root event

         if (sc >= local.beta) break;
      }
   }

cont :

   assert(local.score >= -score::Inf && local.score <= +score::Inf);

   // transposition table

   {
      int trans_move = Index_None;
      if (local.score > local.alpha && local.move != Move_None) {
         trans_move = move_index(local.move, bd);
      }

      int trans_flags = 0;
      if (local.score > local.alpha) trans_flags |= trans::Lower_Flag;
      if (local.score < local.beta)  trans_flags |= trans::Upper_Flag;

      int trans_score = score::to_trans(local.score, p_ply);

      SG.trans().store(key, trans_move, local.depth, trans_flags, trans_score);
   }

   // move-ordering statistics

   if (local.score > local.alpha && local.move != Move_None) {

      good_move(move_index(local.move, bd));

      assert(list_has(local.list, local.move));

      for (int i = 0; i < local.list.size(); i++) {

         move_t mv = local.list.move(i);
         if (mv == local.move) break;

         bad_move(move_index(mv, bd));
      }
   }

   pv = local.pv;
   return local.score;
}

void Search_Local::split(Local & local) {

   SG.poll();
   poll();

   G_SMP.lock(); // useful?

   assert(!G_SMP.busy);
   G_SMP.busy = true;

   Split_Point * sp = &p_pool[p_pool_size++];
   sp->init(top_sp(), p_board, p_ply, local);

   SG.broadcast(sp);

   assert(G_SMP.busy);
   G_SMP.busy = false;

   G_SMP.unlock();

   join(sp);
   idle_loop(sp);

   p_board = sp->board();
   p_ply = sp->ply();

   sp->get_result(local);

   p_pool_size--;
   assert(sp == &p_pool[p_pool_size]);

   poll();
}

int Search_Local::search_move(move_t mv, const Local & local, Line & pv) {

   // init

   Board & bd = p_board;

   int searched_size = local.j;

   int ext = extend(mv, bd, local);
   int red = reduce(mv, bd, local);
   assert(ext == 0 || red == 0);

   int new_alpha = std::max(local.alpha, local.score);
   int new_depth = local.depth + ext - 1;

   assert(new_alpha < local.beta);

   // search move

   int sc;

   do_move(mv);

   if ((local.pv_node && searched_size != 0) || red != 0) {

      sc = -search(-new_alpha - 1, -new_alpha, new_depth - red, local.prune, pv);

      if (sc > new_alpha) { // PVS/LMR re-search
         if (p_ply == 0) SG.set_high(true);
         sc = -search(-local.beta, -new_alpha, new_depth, local.prune, pv);
         if (p_ply == 0) SG.set_high(false);
      }

   } else {

      sc = -search(-local.beta, -new_alpha, new_depth, local.prune, pv);
   }

   undo_move(mv);

   return sc;
}

int Search_Local::qs(int alpha, int beta, Line & pv) {

   assert(-score::Inf <= alpha && alpha < beta && beta <= +score::Inf);

   Board & bd = p_board;

   // init

   pv.clear();

   if (board_is_wipe(bd)) {
      mark_leaf();
      return score::loss(p_ply);
   }

   if (p_ply >= Ply_Max) {
      mark_leaf();
      return eval();
   }

   // move-loop init

   int bs = score::None;

   List list;
   gen_captures(list, bd);

   if (list.size() == 0) { // quiet position

      // bitbases

      if (var::BB && bb::pos_is_search(bd)) {
         mark_leaf();
         return probe_bb();
      }

      // stand pat

      int sc = eval();

      bs = sc;

      if (sc > alpha) {

         alpha = sc;

         if (sc >= beta) {
            mark_leaf();
            return sc;
         }
      }

      gen_promotions(list, bd);
      if (!pos_has_king(bd)) add_exchanges(list, bd);
   }

   // move loop

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);

      Line new_pv;

      do_move(mv);
      int sc = -qs(-beta, -alpha, new_pv);
      undo_move(mv);

      if (sc > bs) {

         bs = sc;
         pv.cat(mv, new_pv);

         if (sc > alpha) {
            alpha = sc;
            if (sc >= beta) break;
         }
      }
   }

   if (list.size() == 0) mark_leaf();

   assert(bs >= -score::Inf && bs <= +score::Inf);
   return bs;
}

int Search_Local::extend(move_t /* mv */ , const Pos & /* pos */, const Local & local) {

   return (local.list.size() == 1) ? 1 : 0;
}

int Search_Local::reduce(move_t mv, const Pos & pos, const Local & local) {

   int red = 0;

   if (local.depth >= 3
    && local.j >= (local.safe ? 3 : 1)
    && !move_is_capture(mv, pos)
    && !move_is_promotion(mv, pos)
    ) {

      red++;
      if (!local.safe && local.j >= 6) red++;
   }

   return red;
}

void Search_Local::do_move(move_t mv) {

   p_node++;
   if ((p_node & bit_mask(12)) == 0) SG.poll();
   if ((p_node & bit_mask( 8)) == 0) poll();

   assert(p_ply < Ply_Max);
   p_move[p_ply] = mv;
   p_board.do_move(mv, p_undo[p_ply]);
   p_ply++;
}

void Search_Local::undo_move(move_t mv) {

   assert(p_ply > 0);
   p_ply--;
   assert(mv == p_move[p_ply]);
   p_board.undo_move(mv, p_undo[p_ply]);
}

void Search_Local::mark_leaf() {

   p_leaf++;
   p_ply_sum += p_ply;
}

int Search_Local::probe_bb() {

   int val = bb::probe_raw(p_board);

   if (val == bb::Win) {
      return +score::BB_Inf - p_ply;
   } else if (val == bb::Loss) {
      return -score::BB_Inf + p_ply;
   } else {
      return 0;
   }
}

int Search_Local::eval() const {

   return ::eval(p_board);
}

void Search_Local::poll() {

   if (stop()) throw Abort();
}

bool Search_Local::stop() const {

   for (Split_Point * sp = top_sp(); sp != NULL; sp = sp->parent()) {
      if (sp->stop()) return true;
   }

   return false;
}

void Search_Local::push_sp(Split_Point * sp) {

   lock();
   p_stack.add(sp);
   unlock();
}

void Search_Local::pop_sp() {

   lock();
   p_stack.remove();
   unlock();
}

Split_Point * Search_Local::top_sp() const {

   assert(!p_stack.empty());
   return p_stack[p_stack.size() - 1];
}

void Split_Point::init_root() {

   p_parent = NULL;

   p_ply = -1; // for debug

   p_workers = 1; // master
   p_stop = false;
}

void Split_Point::init(Split_Point * parent, const Board & bd, int ply, const Local & local) {

   assert(ply > parent->ply());

   p_parent = parent;

   p_board = bd;
   p_ply = ply;

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

move_t Split_Point::get_move(Local & local) {

   move_t mv = Move_None;

   lock();

   if (!high() && p_local.i < p_local.list.size()) {
      mv = p_local.list.move(p_local.i++);
      local.score = p_local.score;
      local.j = p_local.j;
   }

   unlock();

   return mv;
}

void Split_Point::stop_root() {

   p_stop = true;
}

void Split_Point::put_result(move_t mv, int sc, const Line & pv) {

   lock();

   if (high()) { // ignore superfluous moves after a fail high
      unlock();
      return;
   }

   p_local.j++;
   assert(p_local.j <= p_local.i);

   if (sc > p_local.score) {

      p_local.move = mv;
      p_local.score = sc;
      p_local.pv.cat(mv, pv);

      if (p_ply == 0) { // root event
         SG.new_best_move(p_local.move, p_local.score, p_local.depth, p_local.pv);
      }

      if (sc >= p_local.beta) p_stop = true;
   }

   unlock();
}

bool Split_Point::is_child(Split_Point * sp) {

   for (Split_Point * s = this; s != NULL; s = s->p_parent) {
      if (s == sp) return true;
   }

   return false;
}

void Line::set(move_t mv) {

   clear();
   add(mv);
}

void Line::cat(move_t mv, const Line & pv) {

   clear();
   add(mv);

   for (int i = 0; i < pv.size(); i++) {
      add(pv.move(i));
   }
}

std::string Line::to_string(const Pos & pos, int size_max) const {

   std::string s = "";

   int size = this->size();
   if (size_max != 0 && size > size_max) size = size_max;

   Pos new_pos(pos);

   for (int i = 0; i < size; i++) {

      move_t mv = move(i);

      if (i != 0) s += " ";
      s += move_to_string(mv, new_pos);

      new_pos = Pos(new_pos, mv);
   }

   return s;
}

// end of search.cpp

