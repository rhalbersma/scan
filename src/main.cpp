
// includes

#include <cmath>
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
#include "common.hpp"
#include "dxp.hpp"
#include "eval.hpp"
#include "fen.hpp"
#include "game.hpp"
#include "gen.hpp"
#include "hash.hpp"
#include "hub.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "search.hpp"
#include "sort.hpp"
#include "thread.hpp"
#include "tt.hpp"
#include "util.hpp"
#include "var.hpp"

// types

class Terminal {

private:

   static constexpr double Time {5.0};

   bool m_computer[Side_Size];
   Game m_game;
   Depth m_depth;
   int64 m_nodes;
   double m_time;

   Move user_move ();
   void new_game  (const Pos & pos = pos::Start);
   void go_to     (int ply);

public:

   void loop ();
};

// variables

static Terminal G_Terminal;

// prototypes

static void hub_loop ();

static void disp_game (const Game & game);

static void init_high ();
static void init_low  ();

static void param_bool (const std::string & name);
static void param_int  (const std::string & name, int min, int max);
static void param_enum (const std::string & name, const std::string & values);

// functions

int main(int argc, char * argv[]) {

   std::string arg {};
   if (argc > 1) arg = argv[1];

   bit::init();
   hash::init();
   pos::init();
   var::init();

   bb::index_init();
   bb::comp_init();

   ml::rand_init(); // after hash keys

   var::load("scan.ini");

   if (arg.empty()) { // terminal

      listen_input();
      init_high();

      G_Terminal.loop();

   } else if (arg == "dxp") {

      init_high();

      dxp::loop();

   } else if (arg == "hub") {

      listen_input();
      bit::init(); // depends on the variant

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

            auto p = scan.get_pair();

            if (false) {
            } else if (p.name == "think") {
               think = true;
            } else if (p.name == "ponder") {
               ponder = true;
            } else if (p.name == "analyze") {
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

         if (move == move::None) move = quick_move(game.node());

         if (move != move::None && answer == move::None) {
            answer = quick_move(game.node().succ(move));
         }

         Pos p0 = game.pos();
         Pos p1 = p0;
         if (move != move::None) p1 = p0.succ(move);

         std::string line = "done";
         if (move   != move::None) hub::add_pair(line, "move",   move::to_hub(move, p0));
         if (answer != move::None) hub::add_pair(line, "ponder", move::to_hub(answer, p1));
         hub::write(line);

         si.init(); // reset level

      } else if (command == "hub") {

         std::string line = "id";
         hub::add_pair(line, "name", Engine_Name);
         hub::add_pair(line, "version", Engine_Version);
         hub::add_pair(line, "author", "Fabien Letouzey");
         hub::add_pair(line, "country", "France");
         hub::write(line);

         param_enum("variant", "normal killer bt frisian losing");
         param_bool("book");
         param_int ("book-ply", 0, 20);
         param_int ("book-margin", 0, 100);
         param_bool("ponder");
         param_int ("threads", 1, 16);
         param_int ("tt-size", 16, 30);
         param_int ("bb-size", 0, 7);

         hub::write("wait");

      } else if (command == "init") {

         init_low();
         hub::write("ready");

      } else if (command == "level") {

         int depth = -1;
         int64 nodes = -1;
         double move_time = -1.0;

         bool smart = false;
         int moves = 0;
         double game_time = 30.0;
         double inc = 0.0;

         bool infinite = false; // ignored
         bool ponder = false; // ignored

         while (!scan.eos()) {

            auto p = scan.get_pair();

            if (false) {
            } else if (p.name == "depth") {
               depth = std::stoi(p.value);
            } else if (p.name == "nodes") {
               nodes = std::stoll(p.value);
            } else if (p.name == "move-time") {
               move_time = std::stod(p.value);
            } else if (p.name == "moves") {
               smart = true;
               moves = std::stoi(p.value);
            } else if (p.name == "time") {
               smart = true;
               game_time = std::stod(p.value);
            } else if (p.name == "inc") {
               smart = true;
               inc = std::stod(p.value);
            } else if (p.name == "infinite") {
               infinite = true;
            } else if (p.name == "ponder") {
               ponder = true;
            }
         }

         if (depth >= 0) si.depth = Depth(depth);
         if (nodes >= 0) si.nodes = nodes;
         if (move_time >= 0.0) si.time = move_time;

         if (smart) si.set_time(moves, game_time, inc);

      } else if (command == "new-game") {

         G_TT.clear();

      } else if (command == "ping") {

         hub::write("pong");

      } else if (command == "ponder-hit") {

         // no-op (handled during search)

      } else if (command == "pos") {

         std::string pos = pos_hub(pos::Start);
         std::string moves;

         while (!scan.eos()) {

            auto p = scan.get_pair();

            if (false) {
            } else if (p.name == "start") {
               pos = pos_hub(pos::Start);
            } else if (p.name == "pos") {
               pos = p.value;
            } else if (p.name == "moves") {
               moves = p.value;
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

               Move mv = move::from_hub(arg, game.pos());

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

            auto p = scan.get_pair();

            if (false) {
            } else if (p.name == "name") {
               name = p.value;
            } else if (p.name == "value") {
               value = p.value;
            }
         }

         if (name.empty()) {
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

   m_computer[White] = false;
   m_computer[Black] = true;

   m_depth = Depth_Max;
   m_nodes = 1E12;
   m_time = Time;

   Game & game = m_game;
   game.clear();

   new_game();

   while (true) {

      bool computer = m_computer[game.turn()];

      Move mv;

      if (computer && !m_game.is_end()) {

         Search_Input si;
         si.move = true;
         si.book = true;
         si.depth = m_depth;
         si.nodes = m_nodes;
         si.time = m_time;
         si.input = true;
         si.output = Output_Terminal;

         Search_Output so;
         search(so, game.node(), si);

         mv = so.move;
         if (mv == move::None) mv = quick_move(game.node());

      } else {

         mv = user_move();
         if (mv == move::None) continue; // assume command
      }

      std::string move_string = move::to_string(mv, game.pos());

      game.add_move(mv);
      pos::disp(game.pos());

      if (computer) {
         std::cout << "I play " << move_string << '\n';
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

   } else if (command.empty() && !m_game.is_end()) { // forced move?

      List list;
      gen_moves(list, m_game.pos());
      if (list.size() == 1) return list[0];

   } else if (command == "0") {

      m_computer[White] = false;
      m_computer[Black] = false;

   } else if (command == "1") {

      m_computer[m_game.turn()] = false;
      m_computer[side_opp(m_game.turn())] = true;

   } else if (command == "2") {

      m_computer[White] = true;
      m_computer[Black] = true;

   } else if (command == "b") {

      pos::disp(m_game.pos());

   } else if (command == "depth") {

      std::string arg;
      ss >> arg;

      m_depth = Depth(std::stoi(arg));

   } else if (command == "fen") {

      std::string arg;
      ss >> arg;

      if (arg.empty()) {

         std::cout << pos_fen(m_game.pos()) << '\n';
         std::cout << std::endl;

      } else {

         try {
            new_game(pos_from_fen(arg));
         } catch (const Bad_Input &) {
            std::cout << "bad FEN\n";
            std::cout << std::endl;
         }
      }

   } else if (command == "g") {

      m_computer[m_game.turn()] = true;
      m_computer[side_opp(m_game.turn())] = false;

   } else if (command == "game") {

      disp_game(m_game);

   } else if (command == "h") {

      std::cout << "(0) human players\n";
      std::cout << "(1) human vs. computer\n";
      std::cout << "(2) computer players\n";
      std::cout << "(b)oard\n";
      std::cout << "(g)o\n";
      std::cout << "(h)elp\n";
      std::cout << "(n)ew game\n";
      std::cout << "(q)uit\n";
      std::cout << "(r)edo\n";
      std::cout << "(r)edo (a)ll\n";
      std::cout << "(u)ndo\n";
      std::cout << "(u)ndo (a)ll\n";
      std::cout << '\n';

      std::cout << "depth <n>\n";
      std::cout << "fen [<FEN>]\n";
      std::cout << "game\n";
      std::cout << "nodes <n>\n";
      std::cout << "time <seconds per move>\n";
      std::cout << std::endl;

   } else if (command == "n") {

      new_game();

   } else if (command == "nodes") {

      std::string arg;
      ss >> arg;

      m_nodes = std::stoll(arg);

   } else if (command == "q") {

      std::exit(EXIT_SUCCESS);

   } else if (command == "r") {

      go_to(m_game.ply() + 1);

   } else if (command == "ra") {

      go_to(m_game.size());

   } else if (command == "time") {

      std::string arg;
      ss >> arg;

      m_time = std::stod(arg);

   } else if (command == "u") {

      go_to(m_game.ply() - 1);

   } else if (command == "ua") {

      go_to(0);

   } else if (!m_game.is_end()) {

      try {

         Move mv = move::from_string(command, m_game.pos());

         if (!move::is_legal(mv, m_game.pos())) {
            std::cout << "illegal move\n";
            std::cout << std::endl;
            return move::None;
         } else {
            return mv;
         }

      } catch (const Bad_Input &) {

         std::cout << "???\n";
         std::cout << std::endl;
      }
   }

   return move::None;
}

void Terminal::new_game(const Pos & pos) {

   bool opp = m_computer[side_opp(m_game.turn())];

   m_game.init(pos);
   pos::disp(m_game.pos());

   m_computer[m_game.turn()] = false;
   m_computer[side_opp(m_game.turn())] = opp;

   G_TT.clear();
}

void Terminal::go_to(int ply) {

   if (ply >= 0 && ply <= m_game.size() && ply != m_game.ply()) {

      bool opp = m_computer[side_opp(m_game.turn())];

      m_game.go_to(ply);
      pos::disp(m_game.pos());

      m_computer[m_game.turn()] = false;
      m_computer[side_opp(m_game.turn())] = opp;
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

   std::cout << '\n';
   std::cout << std::endl;
}

static void init_high() {

   std::cout << std::endl;

   init_low();

   std::cout << "done\n";
   std::cout << std::endl;
}

static void init_low() {

   bit::init(); // depends on the variant
   if (var::Book) book::init();
   if (var::BB) bb::init();

   eval_init();
   G_TT.set_size(var::TT_Size);
}

static void param_bool(const std::string & name) {

   std::string line = "param";
   hub::add_pair(line, "name", name);
   hub::add_pair(line, "value", var::get(name));
   hub::add_pair(line, "type", "bool");
   hub::write(line);
}

static void param_int(const std::string & name, int min, int max) {

   std::string line = "param";
   hub::add_pair(line, "name", name);
   hub::add_pair(line, "value", var::get(name));
   hub::add_pair(line, "type", "int");
   hub::add_pair(line, "min", std::to_string(min));
   hub::add_pair(line, "max", std::to_string(max));
   hub::write(line);
}

static void param_enum(const std::string & name, const std::string & values) {

   std::string line = "param";
   hub::add_pair(line, "name", name);
   hub::add_pair(line, "value", var::get(name));
   hub::add_pair(line, "type", "enum");
   hub::add_pair(line, "values", values);
   hub::write(line);
}

