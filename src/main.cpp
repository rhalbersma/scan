
// main.cpp

// includes

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "bb_base.h"
#include "bb_comp.h"
#include "board.h"
#include "book.h"
#include "dxp.h"
#include "eval.h"
#include "fen.h"
#include "game.h"
#include "hash.h"
#include "libmy.hpp"
#include "list.h"
#include "move_gen.h"
#include "pos.h"
#include "search.h"
#include "sort.h"
#include "thread.h"
#include "tuple.h"
#include "util.h"
#include "var.h"

// types

class Terminal {

private :

   bool p_computer[Side_Size];
   Game p_game;
   int p_depth;
   double p_time;

   move_t user_move ();
   void   new_game  (const std::string & fen = Start_FEN);
   void   go_to     (int pos);

public :

   void loop ();
};

// prototypes

static void init     ();
static void hub_loop ();

// functions

int main(int argc, char * argv[]) {

   std::cout << "Scan 2.0 by Fabien Letouzey" << std::endl;

   std::string arg = "";
   if (argc > 1) arg = argv[1];

   bit_init();
   hash_init();
   tuple_init();
   var::init();

   board_init();
   bb::comp_init();

   ml::rand_init(); // after hash keys

   if (arg == "") { // terminal

      listen_input();
      init();

      Terminal term;
      term.loop();

   } else if (arg == "dxp") {

      init();
      dxp_loop();

   } else if (arg == "hub") {

      listen_input();
      hub_loop();

   } else {

      std::cerr << "usage: " << argv[0] << " <command>" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   return EXIT_SUCCESS;
}

static void init() {

   var::load("scan.ini");
   var::update();

   std::cout << std::endl;

   eval_init();
   search_init();
   if (var::Book) book::init();
   if (var::BB) bb::init();

   std::cout << "done" << std::endl;
   std::cout << std::endl;
}

static void hub_loop() {

   var::load("scan.ini");

   Board bd;
   bd.init();

   Search_Info & si = G_Search_Info;
   si.init();

   while (true) {

      std::string line;
      if (!get_line(line)) { // EOF
         std::exit(EXIT_SUCCESS);
      }

      std::stringstream ss(line);

      std::string command;
      ss >> command;

      if (false) {

      } else if (command == "analyse") {

         si.set_unique(false);
         si.set_book(false);
         si.set_input(true);
         si.set_output(Output_Hub);

         search_id(bd);
         move_t mv = G_Search.move();
         std::cout << "move " << move_to_hub(mv) << std::endl;

      } else if (command == "depth") {

         std::string arg;
         ss >> arg;

         int depth = ml::stoi(arg);
         si.set_depth(depth);

      } else if (command == "go") {

         si.set_unique(true);
         si.set_book(true);
         si.set_input(true);
         si.set_output(Output_Hub);

         search_id(bd);
         move_t mv = G_Search.move();
         move_t ponder = G_Search.ponder();

         if (ponder == Move_None) {
            board_do_move(bd, mv);
            ponder = ponder_move(bd);
         }

         std::cout << "move";
         std::cout << " " << move_to_hub(mv);
         if (ponder != Move_None) std::cout << " " << move_to_hub(ponder);
         std::cout << std::endl;

      } else if (command == "hub") {

         std::cout << "name Scan 2.0" << std::endl;
         std::cout << "author Fabien Letouzey" << std::endl;
         std::cout << "country France" << std::endl;
         std::cout << "wait" << std::endl;

      } else if (command == "init") {

         var::update();

         eval_init();
         search_init();
         if (var::Book) book::init();
         if (var::BB) bb::init();

         std::cout << "ready" << std::endl;

      } else if (command == "level") {

         std::string arg;

         ss >> arg;
         int moves = ml::stoi(arg);

         ss >> arg;
         double time = double(ml::stoi(arg)) / 1000.0;

         ss >> arg;
         double inc = double(ml::stoi(arg)) / 1000.0;

         si.set_time(moves, time, inc);

      } else if (command == "move") {

         std::string arg;
         ss >> arg;

         try {

            move_t mv = move_from_hub(arg);

            if (!move_is_legal(mv, bd)) {
               std::cout << "error illegal move" << std::endl;
            } else {
               board_do_move(bd, mv);
            }

         } catch (const Bad_Input &) {

            std::cout << "error bad move" << std::endl;
         }

      } else if (command == "new") {

         search_new_game();

      } else if (command == "ping") {

         std::cout << "pong" << std::endl;

      } else if (command == "ponder") {

         si.set_unique(true);
         si.set_book(true);
         si.set_input(true);
         si.set_output(Output_Hub);
         si.set_ponder(true); // ###

         search_id(bd);
         move_t mv = G_Search.move();
         move_t ponder = G_Search.ponder();

         if (ponder == Move_None) {
            board_do_move(bd, mv);
            ponder = ponder_move(bd);
         }

         std::cout << "move";
         std::cout << " " << move_to_hub(mv);
         if (ponder != Move_None) std::cout << " " << move_to_hub(ponder);
         std::cout << std::endl;

      } else if (command == "ponder-hit") {

         // no-op (handled during search)

      } else if (command == "pos") {

         std::string arg;
         ss >> arg;

         try {
            board_from_hub(bd, arg);
         } catch (const Bad_Input &) {
            std::cout << "error bad position" << std::endl;
         }

         si.init();

      } else if (command == "quit") {

         std::exit(EXIT_SUCCESS);

      } else if (command == "set") {

         std::string name;
         ss >> name;

         std::string value;
         ss >> value;

         var::set(name, value);

      } else if (command == "start") {

         bd.init();
         si.init();

      } else if (command == "stop") {

         // no-op (handled during search)

      } else if (command == "time") {

         std::string arg;
         ss >> arg;

         double time = double(ml::stoi(arg)) / 1000.0;
         si.set_time(time);
      }
   }
}

void Terminal::loop() {

   p_computer[White] = false;
   p_computer[Black] = true;

   p_depth = 99;
   p_time = 10.0;

   Game & game = p_game;
   game.clear();

   const Board & bd = game.board();

   new_game();

   while (true) {

      bool computer = p_computer[bd.turn()];

      move_t mv;

      if (computer && !p_game.is_end(false)) {

         Search_Info & si = G_Search_Info;

         si.init();
         si.set_unique(true);
         si.set_book(true);
         si.set_depth(p_depth);
         si.set_time(p_time);
         si.set_input(true);
         si.set_output(Output_Terminal);

         search_id(bd);
         mv = G_Search.move();

      } else {

         mv = user_move();
         if (mv == Move_None) continue; // assume command
      }

      std::string move_string = move_to_string(mv, bd);

      game.add_move(mv);
      board_disp(bd);

      if (computer) {
         std::cout << "I play " << move_string << std::endl;
         std::cout << std::endl;
      }
   }
}

move_t Terminal::user_move() {

   const Board & bd = p_game.board();

   std::cout << "> " << std::flush;

   std::string line;
   if (!get_line(line)) { // EOF
      std::exit(EXIT_SUCCESS);
   }

   std::cout << std::endl;

   std::stringstream ss(line);

   std::string command;
   ss >> command;

   if (false) {

   } else if (command == "") { // forced move?

      List list;
      gen_moves(list, bd);
      if (list.size() == 1) return list.move(0);

   } else if (command == "0") {

      p_computer[White] = false;
      p_computer[Black] = false;

   } else if (command == "1") {

      p_computer[bd.turn()] = false;
      p_computer[side_opp(bd.turn())] = true;

   } else if (command == "2") {

      p_computer[White] = true;
      p_computer[Black] = true;

   } else if (command == "b") {

      board_disp(bd);

   } else if (command == "depth") {

      std::string arg;
      ss >> arg;

      p_depth = ml::stoi(arg);

   } else if (command == "fen") {

      std::string arg;
      ss >> arg;

      try {
         new_game(arg);
      } catch (const Bad_Input &) {
         std::cout << "bad FEN" << std::endl;
         std::cout << std::endl;
      }

   } else if (command == "g") {

      p_computer[bd.turn()] = true;
      p_computer[side_opp(bd.turn())] = false;

   } else if (command == "h") {

      std::cout << "(0) human players" << std::endl;
      std::cout << "(1) human vs. computer" << std::endl;
      std::cout << "(2) computer players" << std::endl;
      std::cout << "(b)oard" << std::endl;
      std::cout << "(g)o" << std::endl;
      std::cout << "(h)elp" << std::endl;
      std::cout << "(n)ew game" << std::endl;
      std::cout << "(q)uit" << std::endl;
      std::cout << "(r)edo" << std::endl;
      std::cout << "(r)edo (a)ll" << std::endl;
      std::cout << "(u)ndo" << std::endl;
      std::cout << "(u)ndo (a)ll" << std::endl;
      std::cout << std::endl;

      std::cout << "depth <n>" << std::endl;
      std::cout << "fen <s>" << std::endl;
      std::cout << "time <n>" << std::endl;
      std::cout << std::endl;

   } else if (command == "n") {

      new_game();

   } else if (command == "q") {

      std::exit(EXIT_SUCCESS);

   } else if (command == "r") {

      go_to(p_game.pos() + 1);

   } else if (command == "ra") {

      go_to(p_game.size());

   } else if (command == "time") {

      std::string arg;
      ss >> arg;

      p_time = ml::stof(arg);

   } else if (command == "u") {

      go_to(p_game.pos() - 1);

   } else if (command == "ua") {

      go_to(0);

   } else {

      try {

         move_t mv = move_from_string(command, bd);

         if (!move_is_legal(mv, bd)) {
            std::cout << "illegal move" << std::endl;
            std::cout << std::endl;
            return Move_None;
         } else {
            return mv;
         }

      } catch (const Bad_Input &) {

         std::cout << "???" << std::endl;
         std::cout << std::endl;
      }
   }

   return Move_None;
}

void Terminal::new_game(const std::string & fen) {

   const Board & bd = p_game.board();
   bool opp = p_computer[side_opp(bd.turn())];

   p_game.init(fen);
   board_disp(bd);

   int turn = bd.turn();
   p_computer[turn] = false;
   p_computer[side_opp(turn)] = opp;

   search_new_game();
}

void Terminal::go_to(int pos) {

   if (pos >= 0 && pos <= p_game.size() && pos != p_game.pos()) {

      const Board & bd = p_game.board();
      bool opp = p_computer[side_opp(bd.turn())];

      p_game.go_to(pos);
      board_disp(bd);

      int turn = bd.turn();
      p_computer[turn] = false;
      p_computer[side_opp(turn)] = opp;
   }
}

// end of main.cpp

