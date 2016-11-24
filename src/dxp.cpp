
// dxp.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bit.h"
#include "board.h"
#include "dxp.h"
#include "fen.h"
#include "game.h"
#include "libmy.hpp"
#include "move.h"
#include "pos.h"
#include "search.h"
#include "socket.h"
#include "util.h" // for Bad_Input and Bad_Output
#include "var.h"

// constants

static const std::string Engine_Name = "Scan 2.0";

static const bool BB_Adj = false;
static const bool Disp_DXP = false;

static const int Result_Unknown = -2;

// types

struct Game_Req { // 'R'
   int version;
   std::string initiator_name;
   char follower_color;
   double thinking_time;
   int number_of_moves;
   char starting_position;
   char color_to_move_first;
   std::string position;
};

struct Game_Acc { // 'A'
   std::string follower_name;
   char acceptance_code;
};

struct Move { // 'M'
   double time;
   int from_field;
   int to_field;
   int number_captured;
   int captured_pieces[20];
};

struct Game_End { // 'E'
   char reason;
   char stop_code;
};

struct Chat { // 'C'
   std::string text;
};

struct Back_Req { // 'B'
   int move_number;
   char color_to_move;
};

struct Back_Acc { // 'K'
   char acceptance_code;
};

struct Message { // avoid "union" because of std::string constructor
   Game_Req game_req;
   Game_Acc game_acc;
   Move move;
   Game_End game_end;
   Chat chat;
   Back_Req back_req;
   Back_Acc back_acc;
};

class Scanner_DXP {

private :

   std::string p_string;
   int p_pos;

public :

   Scanner_DXP (const std::string & string);

   std::string get_field ();
   std::string get_field (int size);

   bool eos () const;
   char get_char ();
};

// variables

static std::string Opp_Name;
static int My_Side;
static double Time;
static int Moves;
static Pos Start_Pos;

static bool Playing;
static Game G_Game;
static int Games;
static int Score;

// prototypes

static void handle_string (const std::string & s);

static void handle_game_req (const Game_Req & game_req);
static void handle_game_acc (const Game_Acc & game_acc);
static void handle_move     (const Move & move);
static void handle_game_end (const Game_End & game_end);
static void handle_chat     (const Chat & chat);
static void handle_back_req (const Back_Req & back_req);
static void handle_back_acc (const Back_Acc & back_acc);

static void first_game ();
static void next_game  ();
static void start_game ();
static void end_game   (int result);
static void go_to      (int ply);
static void play_move  (move_t mv, double time);
static void update     ();

static void put_game_req (int sd, double time, int moves, const Pos & pos);
static void put_game_acc (char acceptance_code);
static void put_move     (move_t mv, double time);
static void put_game_end (int result, char stop_code);
static void put_chat     (const std::string & s);
static void put_back_req (int ply, int carry);
static void put_back_acc (char acceptance_code);

static std::string encode_message (char header, const Message & msg);
static void        decode_message (char & header, Message & msg, const std::string & s);

static std::string encode_game_req (const Game_Req & game_req);
static std::string encode_game_acc (const Game_Acc & game_acc);
static std::string encode_move     (const Move & move);
static std::string encode_game_end (const Game_End & game_end);
static std::string encode_chat     (const Chat & chat);
static std::string encode_back_req (const Back_Req & back_req);
static std::string encode_back_acc (const Back_Acc & back_acc);

static void decode_game_req (Game_Req & game_req, Scanner_DXP & scan);
static void decode_game_acc (Game_Acc & game_acc, Scanner_DXP & scan);
static void decode_move     (Move & move,         Scanner_DXP & scan);
static void decode_game_end (Game_End & game_end, Scanner_DXP & scan);
static void decode_chat     (Chat & chat,         Scanner_DXP & scan);
static void decode_back_req (Back_Req & back_req, Scanner_DXP & scan);
static void decode_back_acc (Back_Acc & back_acc, Scanner_DXP & scan);

static std::string encode_int    (int n, int size);
static std::string encode_string (const std::string & s, int size);

static int         decode_int    (Scanner_DXP & scan, int size);
static std::string decode_string (Scanner_DXP & scan, int size);

static std::string get_string ();
static void        put_string (const std::string & s);

// functions

void dxp_loop() {

   socket_init();

   Opp_Name = "Opponent";
   Time = double(var::DXP_Time) * 60.0;
   Moves = var::DXP_Moves;

   Playing = false;
   Games = 0;
   Score = 0;

   if (var::DXP_Initiator) first_game();

   while (true) {
      std::string s = get_string();
      handle_string(s);
   }
}

static void handle_string(const std::string & s) {

   char header;
   Message msg;

   decode_message(header, msg, s);

   switch (header) {
   case 'R' : handle_game_req(msg.game_req); break;
   case 'A' : handle_game_acc(msg.game_acc); break;
   case 'M' : handle_move(msg.move);         break;
   case 'E' : handle_game_end(msg.game_end); break;
   case 'C' : handle_chat(msg.chat);         break;
   case 'B' : handle_back_req(msg.back_req); break;
   case 'K' : handle_back_acc(msg.back_acc); break;
   default  : throw Bad_Input();             break;
   }
}

static void handle_game_req(const Game_Req & game_req) {

   if (game_req.version > 1) {
      put_game_acc('1');
      return;
   }

   Opp_Name = game_req.initiator_name;
   My_Side = (game_req.follower_color == 'W') ? White : Black;
   Time = game_req.thinking_time * 60.0;
   Moves = game_req.number_of_moves;

   if (game_req.starting_position == 'A') {
      Start_Pos.init();
   } else if (game_req.starting_position == 'B') {
      std::string dxp = game_req.color_to_move_first + game_req.position;
      pos_from_dxp(Start_Pos, dxp);
   } else {
      throw Bad_Input();
   }

   put_game_acc('0');

   start_game();
}

static void handle_game_acc(const Game_Acc & game_acc) {

   assert(!Playing);

   if (game_acc.acceptance_code == '0') {
      Opp_Name = game_acc.follower_name;
      start_game();
   }
}

static void handle_move(const Move & move) {

   double time = move.time;

   int from = square_from_50(move.from_field - 1);
   int to = square_from_50(move.to_field - 1);

   bit_t caps = 0;

   for (int i = 0; i < move.number_captured; i++) {
      bit_set(caps, square_from_50(move.captured_pieces[i] - 1));
   }

   move_t mv = move_make(from, to, caps);

   const Board & bd = G_Game.board();

   std::string move_string = move_to_string(mv, bd);

   play_move(mv, time);

   if (var::DXP_Board) {

      board_disp(bd);

      std::cout << Opp_Name << " plays " << move_string << std::endl;
      std::cout << std::endl;
   }

   update();
}

static void handle_game_end(const Game_End & game_end) {

   if (Playing) {
      put_game_end(Result_Unknown, game_end.stop_code);
      end_game(-(game_end.reason - '2'));
   }

   if (game_end.stop_code == '1') {
      std::exit(EXIT_SUCCESS);
   }

   if (var::DXP_Initiator) next_game();
}

static void handle_chat(const Chat & chat) {

   std::cout << "chat: " << chat.text << std::endl;
   if (var::DXP_Board) std::cout << std::endl;
}

static void handle_back_req(const Back_Req & back_req) {

   int ply = back_req.move_number * 2;
   if (back_req.color_to_move != 'W') ply++; // TODO: use Start_Pos

   put_back_acc('0');

   go_to(ply);
}

static void handle_back_acc(const Back_Acc & /* back_acc */) {

   // ignore
}

static void first_game() {

   assert(!Playing);

   My_Side = White;
   Start_Pos.init();

   put_game_req(side_opp(My_Side), Time, Moves, Start_Pos);
}

static void next_game() {

   assert(!Playing);

   My_Side = side_opp(My_Side);
   Start_Pos.init();

   put_game_req(side_opp(My_Side), Time, Moves, Start_Pos);
}

static void start_game() {

   G_Game.init(pos_fen(Start_Pos), Moves, Time, 0.0);
   Playing = true;
   update();
}

static void end_game(int result) {

   assert(std::abs(result) <= 2); // +/- 2 for "unknown"

   // TODO: save game

   if (std::abs(result) == 2) result = 0; // assume unknown is a draw

   if (std::abs(result) <= 1) {

      Games++;
      Score += result;

      std::printf("%s  games %3d  score %+5.2f\n", result_to_string(result).c_str(), Games, double(Score) / double(Games));
      if (var::DXP_Board) std::cout << std::endl;
   }

   Playing = false;
}

static void go_to(int ply) {

   G_Game.go_to(ply);
   Playing = true;
   update();
}

static void play_move(move_t mv, double time) {

   G_Game.add_move(mv, time);
}

static void update() { // TODO: pondering and time-out?

   if (!Playing) return; // global variables might be "uninitialised"

   const Game & game = G_Game;
   const Board & bd = game.board();

   if (bd.turn() != My_Side) return; // TODO: pondering

   if (game.is_end(BB_Adj)) {

      int result = game.result(BB_Adj);
      if (My_Side != White) result = -result;

      std::cout << "game ended (" << result_to_string(result) << ")" << std::endl;
      if (var::DXP_Board) std::cout << std::endl;

      put_game_end(result, '0');
      end_game(+result);

   } else if (Moves != 0 && game.pos() >= Moves * 2) { // all moves played

      int result;
      std::string comment = ml::itos(game.pos() / 2) + " moves played";

      if (game.is_end(true)) { // always adjudicate using bitbases

         result = game.result(true);
         if (My_Side != White) result = -result;

         comment += ", bitbase " + result_to_string(result);

      } else {

         result = Result_Unknown;
         comment += ", unknown result";
      }

      std::cout << "game ended (" << comment << ")" << std::endl;
      if (var::DXP_Board) std::cout << std::endl;

      put_game_end(result, '0');
      put_chat("game ended (" + comment + ")");
      end_game(+result);

   } else { // search

      Search_Info & si = G_Search_Info;

      si.init();
      si.set_unique(true);
      si.set_book(true);
      si.set_time(game.moves(bd.turn()), game.time(bd.turn()), game.inc());
      si.set_input(false);
      si.set_output(var::DXP_Search ? Output_Terminal : Output_None);

      search_id(bd);
      move_t mv = G_Search.move();
      double sc = double(G_Search.score()) / 100.0;
      double time = G_Search.time();

      std::string move_string = move_to_string(mv, bd);

      put_move(mv, time);
      play_move(mv, time);

      if (var::DXP_Board) {

         board_disp(bd);

         std::printf("%s plays %s (%+.2f)\n", Engine_Name.c_str(), move_string.c_str(), sc);
         std::cout << std::endl;
      }
   }
}

static void put_game_req(int sd, double time, int moves, const Pos & pos) {

   Message msg;

   msg.game_req.version = 1;
   msg.game_req.initiator_name = Engine_Name;
   msg.game_req.follower_color = (sd == White) ? 'W' : 'Z';
   msg.game_req.thinking_time = time / 60.0;
   msg.game_req.number_of_moves = moves;

   std::string dxp = pos_dxp(pos);

   if (dxp == Start_DXP) {
      msg.game_req.starting_position = 'A';
   } else {
      msg.game_req.starting_position = 'B';
      msg.game_req.color_to_move_first = dxp[0];
      msg.game_req.position = dxp.substr(1);
   }

   put_string(encode_message('R', msg));
}

static void put_game_acc(char acceptance_code) {

   Message msg;

   msg.game_acc.follower_name = Engine_Name;
   msg.game_acc.acceptance_code = acceptance_code;

   put_string(encode_message('A', msg));
}

static void put_move(move_t mv, double time) {

   Message msg;

   int   from = move_from(mv);
   int   to   = move_to(mv);
   bit_t caps = move_captured(mv);

   msg.move.time = time;
   msg.move.from_field = square_to_50(from) + 1;
   msg.move.to_field = square_to_50(to) + 1;
   msg.move.number_captured = bit_count(caps);
   int i = 0;
   for (bit_t b = caps; b != 0; b = bit_rest(b)) {
      int sq = bit_first(b);
      msg.move.captured_pieces[i++] = square_to_50(sq) + 1;
   }
   assert(i == msg.move.number_captured);

   put_string(encode_message('M', msg));
}

static void put_game_end(int result, char stop_code) {

   assert(result >= -2 && result <= +1); // -2 for "unknown"

   Message msg;

   msg.game_end.reason = '2' + result;
   msg.game_end.stop_code = stop_code;

   put_string(encode_message('E', msg));
}

static void put_chat(const std::string & s) {

   std::cout << "chat: " << s << std::endl;
   if (var::DXP_Board) std::cout << std::endl;

   Message msg;

   msg.chat.text = s;

   put_string(encode_message('C', msg));
}

static void put_back_req(int ply, int carry) {

   Message msg;

   msg.back_req.move_number = (ply + carry) / 2 + 1;
   msg.back_req.color_to_move = ((ply + carry) % 2 == 0) ? 'W' : 'Z';

   put_string(encode_message('B', msg));
}

static void put_back_acc(char acceptance_code) {

   Message msg;

   msg.back_acc.acceptance_code = acceptance_code;

   put_string(encode_message('K', msg));
}

static std::string encode_message(char header, const Message & msg) {

   std::string s;
   s += header;

   switch (header) {
   case 'R' : s += encode_game_req(msg.game_req); break;
   case 'A' : s += encode_game_acc(msg.game_acc); break;
   case 'M' : s += encode_move(msg.move);         break;
   case 'E' : s += encode_game_end(msg.game_end); break;
   case 'C' : s += encode_chat(msg.chat);         break;
   case 'B' : s += encode_back_req(msg.back_req); break;
   case 'K' : s += encode_back_acc(msg.back_acc); break;
   default  : throw Bad_Output();                 break;
   }

   return s;
}

static void decode_message(char & header, Message & msg, const std::string & s) {

   Scanner_DXP scan(s);
   header = scan.get_char();

   switch (header) {
   case 'R' : decode_game_req(msg.game_req, scan); break;
   case 'A' : decode_game_acc(msg.game_acc, scan); break;
   case 'M' : decode_move(msg.move, scan);         break;
   case 'E' : decode_game_end(msg.game_end, scan); break;
   case 'C' : decode_chat(msg.chat, scan);         break;
   case 'B' : decode_back_req(msg.back_req, scan); break;
   case 'K' : decode_back_acc(msg.back_acc, scan); break;
   default  : throw Bad_Input();                   break;
   }
}

static std::string encode_game_req(const Game_Req & game_req) {

   std::string s;

   s += encode_int(game_req.version, 2);
   s += encode_string(game_req.initiator_name, 32);
   s += game_req.follower_color;
   s += encode_int(ml::round(game_req.thinking_time), 3); // TODO: float
   s += encode_int(game_req.number_of_moves, 3);
   s += game_req.starting_position;

   if (game_req.starting_position == 'A') {
      // no-op
   } else if (game_req.starting_position == 'B') {
      s += game_req.color_to_move_first;
      s += game_req.position;
   } else {
      throw Bad_Output();
   }

   return s;
}

static std::string encode_game_acc(const Game_Acc & game_acc) {

   std::string s;

   s += encode_string(game_acc.follower_name, 32);
   s += game_acc.acceptance_code;

   return s;
}

static std::string encode_move(const Move & move) {

   std::string s;

   s += encode_int(ml::round(move.time), 4); // TODO: float
   s += encode_int(move.from_field, 2);
   s += encode_int(move.to_field, 2);
   s += encode_int(move.number_captured, 2);

   for (int i = 0; i < move.number_captured; i++) {
      s += encode_int(move.captured_pieces[i], 2);
   }

   return s;
}

static std::string encode_game_end(const Game_End & game_end) {

   std::string s;

   s += game_end.reason;
   s += game_end.stop_code;

   return s;
}

static std::string encode_chat(const Chat & chat) {

   std::string s;

   s += chat.text;

   return s;
}

static std::string encode_back_req(const Back_Req & back_req) {

   std::string s;

   s += encode_int(back_req.move_number, 3);
   s += back_req.color_to_move;

   return s;
}

static std::string encode_back_acc(const Back_Acc & back_acc) {

   std::string s;

   s += back_acc.acceptance_code;

   return s;
}

static void decode_game_req(Game_Req & game_req, Scanner_DXP & scan) {

   game_req.version = decode_int(scan, 2);
   game_req.initiator_name = decode_string(scan, 32);
   game_req.follower_color = scan.get_char();
   game_req.thinking_time = double(decode_int(scan, 3)); // TODO: float
   game_req.number_of_moves = decode_int(scan, 3);

   game_req.starting_position = scan.get_char();

   if (game_req.starting_position == 'A') {
      // no-op
   } else if (game_req.starting_position == 'B') {
      game_req.color_to_move_first = scan.get_char();
      game_req.position = scan.get_field(50);
   } else {
      throw Bad_Input();
   }

   if (!scan.eos()) throw Bad_Input();
}

static void decode_game_acc(Game_Acc & game_acc, Scanner_DXP & scan) {

   game_acc.follower_name = decode_string(scan, 32);
   game_acc.acceptance_code = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_move(Move & move, Scanner_DXP & scan) {

   move.time = double(decode_int(scan, 4)); // TODO: float
   move.from_field = decode_int(scan, 2);
   move.to_field = decode_int(scan, 2);
   move.number_captured = decode_int(scan, 2);

   for (int i = 0; i < move.number_captured; i++) {
      move.captured_pieces[i] = decode_int(scan, 2);
   }

   if (!scan.eos()) throw Bad_Input();
}

static void decode_game_end(Game_End & game_end, Scanner_DXP & scan) {

   game_end.reason = scan.get_char();
   game_end.stop_code = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_chat(Chat & chat, Scanner_DXP & scan) {

   chat.text = scan.get_field();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_back_req(Back_Req & back_req, Scanner_DXP & scan) {

   back_req.move_number = decode_int(scan, 3);
   back_req.color_to_move = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_back_acc(Back_Acc & back_acc, Scanner_DXP & scan) {

   back_acc.acceptance_code = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static std::string encode_int(int n, int size) { // fill with '0'

   if (n < 0) throw Bad_Output();
   assert(size <= 10);

   std::string s;
   s.resize(size);

   for (int i = 0; i < size; i++) {
      s[size - i - 1] = '0' + n % 10;
      n /= 10;
   }

   if (n != 0) throw Bad_Output();

   assert(int(s.size()) == size);
   return s;
}

static std::string encode_string(const std::string & s, int size) { // fill with ' '

   if (int(s.size()) > size) throw Bad_Output();

   std::string ns = s;

   for (int i = 0; i < size - int(s.size()); i++) {
      ns += ' ';
   }

   assert(int(ns.size()) == size);
   return ns;
}

static int decode_int(Scanner_DXP & scan, int size) {

   std::string s = scan.get_field(size);
   return ml::stoi(s);
}

static std::string decode_string(Scanner_DXP & scan, int size) {

   std::string s = scan.get_field(size);
   return ml::trim(s);
}

Scanner_DXP::Scanner_DXP(const std::string & string) {

   p_string = string;
   p_pos = 0;
}

std::string Scanner_DXP::get_field() { // remaining characters

   return get_field(int(p_string.size()) - p_pos);
}

std::string Scanner_DXP::get_field(int size) {

   std::string s;

   for (int i = 0; i < size; i++) {
      s += get_char();
   }

   return s;
}

bool Scanner_DXP::eos() const {

   return p_pos >= int(p_string.size());
}

char Scanner_DXP::get_char() {

   if (eos()) throw Bad_Input();
   return p_string[p_pos++];
}

static std::string get_string() {

   std::string s = socket_read();

   if (Disp_DXP) {
      std::cout << "< " << s << std::endl;
      if (var::DXP_Board) std::cout << std::endl;
   }

   return s;
}

static void put_string(const std::string & s) {

   if (Disp_DXP) {
      std::cout << "> " << s << std::endl;
      if (var::DXP_Board) std::cout << std::endl;
   }

   socket_write(s);
}

// end of dxp.cpp

