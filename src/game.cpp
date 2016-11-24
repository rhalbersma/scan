
// game.cpp

// includes

#include <cstdlib>
#include <iostream>
#include <string>

#include "bb_base.h"
#include "board.h"
#include "fen.h"
#include "game.h"
#include "libmy.hpp"
#include "move.h"
#include "pos.h"

// functions

void Game::clear() {

   init(Start_FEN);
}

void Game::clear(int moves, double time, double inc) {

   init(Start_FEN, moves, time, inc);
}

void Game::init(const std::string & fen) {

   init(fen, 0, 0.0, 0.0);
}

void Game::init(const std::string & fen, int moves, double time, double inc) {

   p_fen = fen;
   p_moves = moves;
   p_time = time;
   p_inc = inc;

   p_move.clear();

   reset();
}

void Game::reset() {

   p_pos = 0;

   board_from_fen(p_board, p_fen);

   p_clock[White] = p_time;
   p_clock[Black] = p_time;
   p_flag[White] = false;
   p_flag[Black] = false;
}

void Game::add_move(move_t mv, double time) {

   Board & bd = p_board;

   if (!move_is_legal(mv, bd)) {
      board_disp(bd);
      std::cout << "illegal move: " << move_to_hub(mv) << std::endl;
      std::cout << std::endl;
      std::exit(EXIT_FAILURE);
   }

   Move_Info mi;
   mi.move = mv;
   mi.time = time;

   p_move.set_size(p_pos); // truncate move list
   p_move.add(mi);

   play_move();
}

void Game::play_move() {

   assert(p_pos < size());
   move_t mv   = p_move[p_pos].move;
   double time = p_move[p_pos].time;
   p_pos++;

   Board & bd = p_board;

   int turn = bd.turn();

   p_clock[turn] += p_inc; // pre-increment #
   p_clock[turn] -= time;

   if (p_clock[turn] < 0.0 && !p_flag[turn]) {
      // std::cerr << "LOSS ON TIME ###" << std::endl;
      p_flag[turn] = true;
   }

   if (p_moves != 0 && p_pos % (p_moves * 2) == 0) {
      p_clock[White] += p_time;
      p_clock[Black] += p_time;
   }

   board_do_move(bd, mv);
}

void Game::go_to(int pos) {

   assert(pos >= 0 && pos <= size());

   reset();

   for (int i = 0; i < pos; i++) {
      play_move();
   }

   assert(p_pos == pos);
}

bool Game::is_end(bool use_bb) const {

   const Board & bd = p_board;

   return board_is_end(bd, 3) || (use_bb && bb::pos_is_game(bd));
}

int Game::result(bool use_bb) const {

   assert(is_end(use_bb));

   const Board & bd = p_board;

   if (board_is_loss(bd)) {

      return (bd.turn() == White) ? -1 : +1;

   } else if (bd.is_draw(3)) {

      return 0;

   } else if (use_bb && bb::pos_is_game(bd)) {

      int val = bb::probe(bd);

      if (val == bb::Win) {
         return (bd.turn() == White) ? +1 : -1;
      } else if (val == bb::Loss) {
         return (bd.turn() == White) ? -1 : +1;
      } else {
         return 0;
      }

   } else {

      assert(false);
      return 0;
   }
}

int Game::moves(int sd) const {

   assert(sd == p_board.turn());

   return (p_moves == 0) ? 0 : p_moves - (p_pos / 2) % p_moves;
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

// end of game.cpp

