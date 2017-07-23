
// includes

#include <cstdlib>
#include <iostream>
#include <string>

#include "bb_base.hpp"
#include "common.hpp"
#include "game.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "score.hpp"

// functions

Game::Game() {
   init(pos::Start);
}

void Game::init(const Pos & pos) {
   init(pos, 0, 0.0, 0.0);
}

void Game::init(const Pos & pos, int moves, double time, double inc) {

   p_pos_start = pos;
   p_moves = moves;
   p_time = time;
   p_inc = inc;

   p_move.clear();

   reset();
}

void Game::reset() {

   p_ply = 0;

   p_node.clear();

   p_node.add_ref(Node(p_pos_start));
   assert(p_node.size() == p_ply + 1);

   p_clock[White] = p_time;
   p_clock[Black] = p_time;
   p_flag[White] = false;
   p_flag[Black] = false;
}

void Game::add_move(Move mv, double time) {

   if (!move::is_legal(mv, pos())) {
      pos::disp(pos());
      std::cout << "illegal move: " << move::to_hub(mv) << std::endl;
      std::cout << std::endl;
      std::exit(EXIT_FAILURE);
   }

   Move_Info mi;
   mi.move = mv;
   mi.time = time;

   p_move.set_size(p_ply); // truncate move list
   p_move.add_ref(mi);

   play_move();
}

void Game::play_move() {

   assert(p_ply < size());
   Move   mv   = p_move[p_ply].move;
   double time = p_move[p_ply].time;
   p_ply++;

   Side turn = this->turn();

   p_clock[turn] += p_inc; // pre-increment #
   p_clock[turn] -= time;

   if (p_clock[turn] < 0.0 && !p_flag[turn]) {
      // std::cerr << "LOSS ON TIME ###" << std::endl;
      p_flag[turn] = true;
   }

   if (p_moves != 0 && p_ply % (p_moves * 2) == 0) {
      p_clock[White] += p_time;
      p_clock[Black] += p_time;
   }

   p_node.add_ref(node().succ(mv));
   assert(p_node.size() == p_ply + 1);
}

void Game::go_to(int ply) {

   assert(ply >= 0 && ply <= size());

   reset();

   for (int i = 0; i < ply; i++) {
      play_move();
   }

   assert(p_ply == ply);
}

bool Game::is_end(bool use_bb) const {
   return node().is_end() || (use_bb && bb::pos_is_load(pos()));
}

int Game::result(bool use_bb, Side sd) const {

   assert(is_end(use_bb));

   int res;

   if (pos::is_loss(pos())) {

      res = score::side(-1, turn()); // for white

   } else if (node().is_draw(3)) {

      res = 0;

   } else if (use_bb && bb::pos_is_load(pos())) {

      bb::Value val = bb::probe(pos());
      res = bb::value_nega(val, turn()); // for white

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

