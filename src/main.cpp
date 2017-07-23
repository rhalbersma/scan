
// includes

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bb_base.hpp"
#include "bb_comp.hpp"
#include "bb_index.hpp"
#include "bit.hpp"
#include "book.hpp"
#include "dxp.hpp"
#include "eval.hpp"
#include "fen.hpp"
#include "game.hpp"
#include "hash.hpp"
#include "hub.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "move_gen.hpp"
#include "pos.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "thread.hpp"
#include "tt.hpp"
#include "util.hpp"
#include "var.hpp"

// types

class Terminal {

private :

   static constexpr double Time { 10.0 };

   bool p_computer[Side_Size];
   Game p_game;
   Depth p_depth;
   double p_time;

   Move user_move ();
   void new_game  (const Pos & pos = pos::Start);
   void go_to     (int ply);

public :

   void loop ();
};

// prototypes

static void hub_loop ();

static void disp_game (const Game & game);

static void init_high ();
static void init_low  ();

// functions

int main(int argc, char * argv[]) {

   std::string arg = "";
   if (argc > 1) arg = argv[1];

   common_init();
   bit::init();
   hash::init();
   pos::init();
   var::init();

   bb::comp_init();
   bb::index_init();

   ml::rand_init(); // after hash keys

   var::load("scan.ini");

   if (arg == "") { // terminal

      listen_input();

      init_high();

      Terminal term;
      term.loop();

   } else if (arg == "dxp") {

      init_high();

      dxp::loop();

   } else if (arg == "hub") {

      listen_input();

      hub_loop();

   } else {

      std::cerr << "usage: " << argv[0] << " <command>" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   return EXIT_SUCCESS;
}

static void hub_loop() {

   Game game;

   Search_Input si;
   si.init();

   while (true) {

      std::string line = hub::read();
      hub::Scanner scan(line);

      if (scan.eos()) { // empty line
         hub::error("missing command");
         continue;
      }

      std::string command = scan.get_command();

      if (false) {

      } else if (command == "go") {

         bool think = false; // ignored
         bool ponder = false;
         bool analyze = false;

         while (!scan.eos()) {

            hub::Pair p = scan.get_pair();

            if (false) {
            } else if (p.first == "think") {
               think = true;
            } else if (p.first == "ponder") {
               ponder = true;
            } else if (p.first == "analyze") {
               analyze = true;
            }
         }

         si.move = !analyze;
         si.book = !analyze;
         si.input = true;
         si.output = Output_Hub;
         si.ponder = ponder;

         Search_Output so;
         search(so, game.node(), si);

         Move move = so.move;
         Move answer = so.answer;

         if (move == move::None) {
            move = quick_move(game.node());
         }

         if (move != move::None && answer == move::None) {
            Node new_node = game.node().succ(move);
            answer = quick_move(new_node);
         }

         std::string line = "done";
         if (move   != move::None) hub::add_pair(line, "move",   move::to_hub(move));
         if (answer != move::None) hub::add_pair(line, "ponder", move::to_hub(answer));
         hub::write(line);

         si.init(); // reset level

      } else if (command == "hub") {

         std::string line = "id";
         hub::add_pair(line, "name", Engine_Name);
         hub::add_pair(line, "version", Engine_Version);
         hub::add_pair(line, "author", "Fabien Letouzey");
         hub::add_pair(line, "country", "France");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "variant");
         hub::add_pair(line, "value", var::get("variant"));
         hub::add_pair(line, "type", "enum");
         hub::add_pair(line, "values", "normal killer bt");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "book");
         hub::add_pair(line, "value", var::get("book"));
         hub::add_pair(line, "type", "bool");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "book-ply");
         hub::add_pair(line, "value", var::get("book-ply"));
         hub::add_pair(line, "type", "int");
         hub::add_pair(line, "min", "0");
         hub::add_pair(line, "max", "20");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "book-margin");
         hub::add_pair(line, "value", var::get("book-margin"));
         hub::add_pair(line, "type", "int");
         hub::add_pair(line, "min", "0");
         hub::add_pair(line, "max", "20");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "ponder");
         hub::add_pair(line, "value", var::get("ponder"));
         hub::add_pair(line, "type", "bool");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "threads");
         hub::add_pair(line, "value", var::get("threads"));
         hub::add_pair(line, "type", "int");
         hub::add_pair(line, "min", "1");
         hub::add_pair(line, "max", "16");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "tt-size");
         hub::add_pair(line, "value", var::get("tt-size"));
         hub::add_pair(line, "type", "int");
         hub::add_pair(line, "min", "16");
         hub::add_pair(line, "max", "30");
         hub::write(line);

         line = "param";
         hub::add_pair(line, "name", "bb-size");
         hub::add_pair(line, "value", var::get("bb-size"));
         hub::add_pair(line, "type", "int");
         hub::add_pair(line, "min", "0");
         hub::add_pair(line, "max", "8");
         hub::write(line);

         hub::write("wait");

      } else if (command == "init") {

         init_low();

         hub::write("ready");

      } else if (command == "level") {

         int depth = -1;
         double move_time = -1.0;

         bool smart = false;
         int moves = 0;
         double game_time = 30.0;
         double inc = 0.0;

         bool infinite = false; // ignored
         bool ponder = false; // ignored

         while (!scan.eos()) {

            hub::Pair p = scan.get_pair();

            if (false) {
            } else if (p.first == "depth") {
               depth = std::stoi(p.second);
            } else if (p.first == "move-time") {
               move_time = std::stod(p.second);
            } else if (p.first == "moves") {
               smart = true;
               moves = std::stoi(p.second);
            } else if (p.first == "time") {
               smart = true;
               game_time = std::stod(p.second);
            } else if (p.first == "inc") {
               smart = true;
               inc = std::stod(p.second);
            } else if (p.first == "infinite") {
               infinite = true;
            } else if (p.first == "ponder") {
               ponder = true;
            }
         }

         if (depth >= 0) si.depth = Depth(depth);
         if (move_time >= 0.0) si.set_time(move_time);

         if (smart) si.set_time(moves, game_time, inc);

      } else if (command == "new-game") {

         tt::G_TT.clear();

      } else if (command == "ping") {

         hub::write("pong");

      } else if (command == "ponder-hit") {

         // no-op (handled during search)

      } else if (command == "pos") {

         std::string pos = Start_Hub;
         std::string moves;

         while (!scan.eos()) {

            hub::Pair p = scan.get_pair();

            if (false) {
            } else if (p.first == "start") {
               pos = Start_Hub;
            } else if (p.first == "pos") {
               pos = p.second;
            } else if (p.first == "moves") {
               moves = p.second;
            }
         }

         // position

         try {
            game.init(pos_from_hub(pos));
         } catch (const Bad_Input &) {
            hub::error("bad position");
            continue;
         }

         // moves

         std::stringstream ss(moves);

         std::string arg;

         while (ss >> arg) {

            try {

               Move mv = move::from_hub(arg);

               if (!move::is_legal(mv, game.pos())) {
                  hub::error("illegal move");
                  break;
               } else {
                  game.add_move(mv);
               }

            } catch (const Bad_Input &) {

               hub::error("bad move");
               break;
            }
         }

         si.init(); // reset level

      } else if (command == "quit") {

         std::exit(EXIT_SUCCESS);

      } else if (command == "set-param") {

         std::string name;
         std::string value;

         while (!scan.eos()) {

            hub::Pair p = scan.get_pair();

            if (false) {
            } else if (p.first == "name") {
               name = p.second;
            } else if (p.first == "value") {
               value = p.second;
            }
         }

         if (name == "") {
            hub::error("missing name");
            continue;
         }

         var::set(name, value);
         var::update();

      } else if (command == "stop") {

         // no-op (handled during search)

      } else { // unknown command

         hub::error("bad command");
         continue;
      }
   }
}

void Terminal::loop() {

   p_computer[White] = false;
   p_computer[Black] = true;

   p_depth = Depth_Max;
   p_time = Time;

   Game & game = p_game;

   new_game();

   while (true) {

      bool computer = p_computer[game.turn()];

      Move mv;

      if (computer && !p_game.is_end()) {

         Search_Input si;
         si.move = true;
         si.book = true;
         si.depth = p_depth;
         si.set_time(p_time);
         si.input = true;
         si.output = Output_Terminal;

         Search_Output so;
         search(so, game.node(), si);

         mv = so.move;

      } else {

         mv = user_move();
         if (mv == move::None) continue; // assume command
      }

      std::string move_string = move::to_string(mv, game.pos());

      game.add_move(mv);
      pos::disp(game.pos());

      if (computer) {
         std::cout << "I play " << move_string << std::endl;
         std::cout << std::endl;
      }
   }
}

Move Terminal::user_move() {

   std::cout << "> " << std::flush;

   std::string line;
   if (!get_line(line)) std::exit(EXIT_SUCCESS); // EOF

   std::cout << std::endl;

   std::stringstream ss(line);

   std::string command;
   ss >> command;

   if (false) {

   } else if (command == "" && !p_game.is_end()) { // forced move?

      List list;
      gen_moves(list, p_game.pos());
      if (list.size() == 1) return list.move(0);

   } else if (command == "0") {

      p_computer[White] = false;
      p_computer[Black] = false;

   } else if (command == "1") {

      p_computer[p_game.turn()] = false;
      p_computer[side_opp(p_game.turn())] = true;

   } else if (command == "2") {

      p_computer[White] = true;
      p_computer[Black] = true;

   } else if (command == "b") {

      pos::disp(p_game.pos());

   } else if (command == "depth") {

      std::string arg;
      ss >> arg;

      p_depth = Depth(std::stoi(arg));

   } else if (command == "fen") {

      std::string arg;
      ss >> arg;

      if (arg == "") {

         std::cout << pos_fen(p_game.pos()) << std::endl;
         std::cout << std::endl;

      } else {

         try {
            new_game(pos_from_fen(arg));
         } catch (const Bad_Input &) {
            std::cout << "bad FEN" << std::endl;
            std::cout << std::endl;
         }
      }

   } else if (command == "g") {

      p_computer[p_game.turn()] = true;
      p_computer[side_opp(p_game.turn())] = false;

   } else if (command == "game") {

      disp_game(p_game);
      std::cout << std::endl;

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
      std::cout << "fen [<FEN>]" << std::endl;
      std::cout << "game" << std::endl;
      std::cout << "time <n>" << std::endl;
      std::cout << std::endl;

   } else if (command == "n") {

      new_game();

   } else if (command == "q") {

      std::exit(EXIT_SUCCESS);

   } else if (command == "r") {

      go_to(p_game.ply() + 1);

   } else if (command == "ra") {

      go_to(p_game.size());

   } else if (command == "time") {

      std::string arg;
      ss >> arg;

      p_time = std::stod(arg);

   } else if (command == "u") {

      go_to(p_game.ply() - 1);

   } else if (command == "ua") {

      go_to(0);

   } else if (!p_game.is_end()) {

      try {

         Move mv = move::from_string(command, p_game.pos());

         if (!move::is_legal(mv, p_game.pos())) {
            std::cout << "illegal move" << std::endl;
            std::cout << std::endl;
            return move::None;
         } else {
            return mv;
         }

      } catch (const Bad_Input &) {

         std::cout << "???" << std::endl;
         std::cout << std::endl;
      }
   }

   return move::None;
}

void Terminal::new_game(const Pos & pos) {

   bool opp = p_computer[side_opp(p_game.turn())];

   p_game.init(pos);
   pos::disp(p_game.pos());

   p_computer[p_game.turn()] = false;
   p_computer[side_opp(p_game.turn())] = opp;

   tt::G_TT.clear();
}

void Terminal::go_to(int ply) {

   if (ply >= 0 && ply <= p_game.size() && ply != p_game.ply()) {

      bool opp = p_computer[side_opp(p_game.turn())];

      p_game.go_to(ply);
      pos::disp(p_game.pos());

      p_computer[p_game.turn()] = false;
      p_computer[side_opp(p_game.turn())] = opp;
   }
}

static void disp_game(const Game & game) {

   Pos pos = game.start_pos();

   for (int i = 0; i < game.ply(); i++) {

      Move mv = game.move(i);

      if (i != 0) std::cout << " ";
      std::cout << move::to_string(mv, pos);

      pos = pos.succ(mv);
   }

   std::cout << std::endl;
}

static void init_high() {

   std::cout << std::endl;

   init_low();

   std::cout << "done" << std::endl;
   std::cout << std::endl;
}

static void init_low() {

   var::update();

   if (var::Book) book::init();
   if (var::BB) bb::init();

   std::string file_name = std::string("data/eval") + var::variant("", "_killer", "_bt");
   eval_init(file_name);

   tt::G_TT.set_size(var::TT_Size);
}

