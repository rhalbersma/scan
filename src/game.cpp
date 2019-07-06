
// includes

#include <string>

#include "bb_base.hpp"
#include "common.hpp"
#include "game.hpp"
#include "libmy.hpp"
#include "pos.hpp"
#include "score.hpp"

// functions

void Game::init(const Pos & pos, int moves, double time, double inc) {

   m_pos_start = pos;
   m_moves = moves;
   m_time = time;
   m_inc = inc;

   m_move.clear();

   reset();
}

void Game::reset() {

   m_ply = 0;

   m_node.clear();

   m_node.add(Node(m_pos_start));
   assert(m_node.size() == m_ply + 1);

   m_clock[White] = m_time;
   m_clock[Black] = m_time;
}

void Game::add_move(Move mv, double time) {

   m_move.set_size(m_ply); // truncate move list
   m_move.add({mv, float(time)});

   play_move();
}

void Game::play_move() {

   assert(m_ply < size());
   Move   mv   = m_move[m_ply].move;
   double time = m_move[m_ply].time;
   m_ply += 1;

   Side turn = this->turn();

   m_clock[turn] += m_inc; // pre-increment #
   m_clock[turn] -= time;

   if (m_moves != 0 && m_ply % (m_moves * 2) == 0) {
      m_clock[White] += m_time;
      m_clock[Black] += m_time;
   }

   m_node.add(node().succ(mv));
   assert(m_node.size() == m_ply + 1);
}

void Game::go_to(int ply) {

   assert(ply >= 0 && ply <= size());

   reset();

   for (int i = 0; i < ply; i++) {
      play_move();
   }

   assert(m_ply == ply);
}

bool Game::is_end(bool use_bb) const {
   return node().is_end() || (use_bb && bb::pos_is_load(pos()));
}

int Game::result(bool use_bb, Side sd) const {

   assert(is_end(use_bb));

   int res;

   if (pos::is_end(pos())) {
      res = pos::result(pos(), White);
   } else if (node().is_draw(3)) {
      res = 0;
   } else if (use_bb && bb::pos_is_load(pos())) {
      res = bb::value_nega(bb::probe(pos()), turn()); // for white
   } else {
      assert(false);
      res = 0;
   }

   return score::side(res, sd); // for sd
}

std::string result_to_string(int result) {

   if (result > 0) {
      return "win";
   } else if (result < 0) {
      return "loss";
   } else {
      return "draw";
   }
}

