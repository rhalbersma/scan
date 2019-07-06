
// includes

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bit.hpp"
#include "common.hpp"
#include "dxp.hpp"
#include "fen.hpp"
#include "game.hpp"
#include "libmy.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "search.hpp"
#include "socket.hpp"
#include "util.hpp" // for Bad_Input and Bad_Output
#include "var.hpp"

namespace dxp {

// types

enum Result {
   Unknown = -2,
   Loss    = -1,
   Draw    =  0,
   Win     = +1,
};

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

struct Move_ { // 'M'
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
   Move_ move;
   Game_End game_end;
   Chat chat;
   Back_Req back_req;
   Back_Acc back_acc;
};

class Scanner {

private:

   const std::string m_string;
   int m_pos {0};

public:

   explicit Scanner (const std::string & s);

   std::string get_field ();
   std::string get_field (int size);

   bool eos () const;
   char get_char ();
};

// variables

static std::string Opp_Name;
static Side My_Side;
static double Time;
static int Moves;
static Pos Start_Pos;
static double Last_Score;

static bool Playing;
static Game G_Game;
static int Games;
static int Total;

// prototypes

static void handle_string (const std::string & s);

static void handle_game_req (const Game_Req & game_req);
static void handle_game_acc (const Game_Acc & game_acc);
static void handle_move     (const Move_ & move);
static void handle_game_end (const Game_End & game_end);
static void handle_chat     (const Chat & chat);
static void handle_back_req (const Back_Req & back_req);
static void handle_back_acc (const Back_Acc & back_acc);

static void first_game ();
static void next_game  ();
static void start_game ();
static void end_game   (int result);
static void go_to      (int ply);
static void play_move  (Move mv, double time);
static void update     ();
static void error      (const std::string & msg);

static void put_game_req (Side sd, double time, int moves, const Pos & pos);
static void put_game_acc (char acceptance_code);
static void put_move     (Move mv, double time, const Pos & pos);
static void put_game_end (int result, char stop_code);
static void put_chat     (const std::string & s);
static void put_back_acc (char acceptance_code);

static std::string encode_message (char header, const Message & msg);
static void        decode_message (char & header, Message & msg, const std::string & s);

static std::string encode_game_req (const Game_Req & game_req);
static std::string encode_game_acc (const Game_Acc & game_acc);
static std::string encode_move     (const Move_ & move);
static std::string encode_game_end (const Game_End & game_end);
static std::string encode_chat     (const Chat & chat);
static std::string encode_back_req (const Back_Req & back_req);
static std::string encode_back_acc (const Back_Acc & back_acc);

static void decode_game_req (Game_Req & game_req, Scanner & scan);
static void decode_game_acc (Game_Acc & game_acc, Scanner & scan);
static void decode_move     (Move_ & move,        Scanner & scan);
static void decode_game_end (Game_End & game_end, Scanner & scan);
static void decode_chat     (Chat & chat,         Scanner & scan);
static void decode_back_req (Back_Req & back_req, Scanner & scan);
static void decode_back_acc (Back_Acc & back_acc, Scanner & scan);

static std::string score_to_string (double sc);

static std::string encode_int    (int n, int size);
static std::string encode_string (const std::string & s, int size);

static Side        decode_side   (char c);
static int         decode_int    (Scanner & scan, int size);
static double      decode_real   (Scanner & scan, int size);
static std::string decode_string (Scanner & scan, int size);

static std::string get_string ();
static void        put_string (const std::string & s);

// functions

void loop() {

   socket_::init();

   Opp_Name = "Opponent";
   Time = double(var::DXP_Time) * 60.0;
   Moves = var::DXP_Moves;

   Playing = false;
   Games = 0;
   Total = 0;

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
      default :  throw Bad_Input();
   }
}

static void handle_game_req(const Game_Req & game_req) {

   if (Playing) error("game in progress");

   if (game_req.version > 1) {
      put_game_acc('1');
      return;
   }

   Opp_Name = game_req.initiator_name;
   My_Side = decode_side(game_req.follower_color);
   Time = game_req.thinking_time * 60.0;
   Moves = game_req.number_of_moves;

   if (game_req.starting_position == 'A') {

      Start_Pos = pos::Start;

   } else if (game_req.starting_position == 'B') {

      std::string dxp = game_req.color_to_move_first + game_req.position;

      try {
         Start_Pos = pos_from_dxp(dxp);
      } catch (const Bad_Input &) {
         error("illegal position: \"" + dxp + "\"");
      }

   } else {

      throw Bad_Input();
   }

   put_game_acc('0');

   start_game();
}

static void handle_game_acc(const Game_Acc & game_acc) {

   if (Playing) error("game in progress");

   if (game_acc.acceptance_code == '0') {
      Opp_Name = game_acc.follower_name;
      start_game();
   }
}

static void handle_move(const Move_ & move) {

   if (!Playing) error("no game in progress");

   double time = move.time;

   Square from = square_from_std(move.from_field);
   Square to   = square_from_std(move.to_field);

   Bit caps {};

   for (int i = 0; i < move.number_captured; i++) {
      bit::set(caps, square_from_std(move.captured_pieces[i]));
   }

   Move mv = move::make(from, to, caps);

   if (!move::is_legal(mv, G_Game.pos())) {
      pos::disp(G_Game.pos());
      error("illegal move: \"" + move::to_hub(mv, G_Game.pos()) + "\"");
   }

   std::string move_string = move::to_string(mv, G_Game.pos());

   play_move(mv, time);

   if (var::DXP_Board) {

      pos::disp(G_Game.pos());

      std::cout << Opp_Name << " plays " << move_string << '\n';
      std::cout << std::endl;
   }

   update();
}

static void handle_game_end(const Game_End & game_end) {

   if (Playing) {
      put_game_end(Result::Unknown, game_end.stop_code);
      end_game(-(game_end.reason - '2'));
   }

   if (game_end.stop_code == '1') std::exit(EXIT_SUCCESS);

   if (var::DXP_Initiator) next_game();
}

static void handle_chat(const Chat & chat) {
   std::cout << "chat: " << chat.text << std::endl;
   if (var::DXP_Board) std::cout << std::endl;
}

static void handle_back_req(const Back_Req & back_req) {

   int ply = (back_req.move_number - 1) * 2;
   if (decode_side(back_req.color_to_move) != White) ply++;
   if (Start_Pos.turn() != White) ply -= 1; // shift if black played the first move

   if (ply < 0 || ply > G_Game.size()) error("bad move number");

   put_back_acc('0');

   go_to(ply);
}

static void handle_back_acc(const Back_Acc & /* back_acc */) {
   // ignore
}

static void first_game() {

   assert(!Playing);

   My_Side = White;
   Start_Pos = pos::Start;

   put_game_req(side_opp(My_Side), Time, Moves, Start_Pos);
}

static void next_game() {

   assert(!Playing);

   My_Side = side_opp(My_Side);
   Start_Pos = pos::Start;

   put_game_req(side_opp(My_Side), Time, Moves, Start_Pos);
}

static void start_game() {

   assert(!Playing);

   G_Game.init(Start_Pos, Moves, Time, 0.0);
   Playing = true;
   Last_Score = 0.0;
   update();
}

static void end_game(int result) {

   assert(std::abs(result) <= 2); // +/- 2 for "unknown"

   assert(Playing);

   // TODO: save game

   if (std::abs(result) == 2) result = Result::Draw; // assume unknown is a draw

   if (std::abs(result) <= 1) { // finished

      Games += 1;
      Total += result;

      std::printf("result %-4s  games %3d  score %+5.2f\n", result_to_string(result).c_str(), Games, double(Total) / double(Games));
      if (var::DXP_Board) std::printf("\n");
      std::fflush(stdout);

   } else { // unfinished

      pos::disp(G_Game.pos());

      std::cout << "unfinished game; last eval = " << score_to_string(Last_Score) << " ###" << std::endl;
      if (var::DXP_Board) std::cout << std::endl;
   }

   Playing = false;
   Last_Score = 0.0;
}

static void go_to(int ply) {
   G_Game.go_to(ply);
   Playing = true;
   update();
}

static void play_move(Move mv, double time) {
   assert(Playing);
   G_Game.add_move(mv, time);
}

static void update() {

   assert(Playing);

   const Game & game = G_Game;

   if (game.turn() != My_Side) return; // no pondering in DXP mode

   if (game.is_end(false)) {

      int result = game.result(false, My_Side);

      std::cout << "game ended (" << result_to_string(result) << ")" << std::endl;
      if (var::DXP_Board) std::cout << std::endl;

      put_game_end(result, '0');
      end_game(+result);

   } else if (Moves != 0 && game.ply() >= Moves * 2) { // all moves played

      int result;
      std::string comment = std::to_string(game.ply() / 2) + " moves played";

      if (game.is_end(true)) { // always adjudicate using bitbases

         result = game.result(true, My_Side);
         comment += ", endgame " + result_to_string(result);

      } else if (Last_Score >= +3.0) {

         result = Result::Win;
         comment += ", adjudication " + result_to_string(result) + ", last eval = " + score_to_string(Last_Score);

      } else if (Last_Score <= -3.0) {

         result = Result::Loss;
         comment += ", adjudication " + result_to_string(result) + ", last eval = " + score_to_string(Last_Score);

      } else {

         result = Result::Unknown;
         comment += ", unknown result, last eval = " + score_to_string(Last_Score);
      }

      std::cout << "game ended (" << comment << ")" << std::endl;
      if (var::DXP_Board) std::cout << std::endl;

      put_game_end(result, '0');
      put_chat("game ended (" + comment + ")");
      end_game(+result);

   } else { // search

      Search_Input si;
      si.move = true;
      si.book = true;
      si.set_time(game.moves(), game.time(game.turn()), game.inc());
      si.input = false;
      si.output = var::DXP_Search ? Output_Terminal : Output_None;

      Search_Output so;
      search(so, game.node(), si);

      Move mv = so.move;
      double sc = (so.score == score::None) ? 0.0 : double(so.score) / 100.0;
      double time = so.time();

      std::string move_string = move::to_string(mv, game.pos());

      put_move(mv, time, game.pos());
      play_move(mv, time);

      if (var::DXP_Board) {

         pos::disp(game.pos());

         std::cout << Engine_Name << " plays " << move_string << " (" << score_to_string(sc) << ")\n";
         std::cout << std::endl;
      }

      if (sc != 0.0) Last_Score = sc; // ignore forced moves
   }
}

static void error(const std::string & msg) {
   put_chat("error: " + msg);
   throw Bad_Input();
}

static void put_game_req(Side sd, double time, int moves, const Pos & pos) {

   Message msg;

   msg.game_req.version = 1;
   msg.game_req.initiator_name = Engine_Name + " " + Engine_Version;
   msg.game_req.follower_color = (sd == White) ? 'W' : 'Z';
   msg.game_req.thinking_time = time / 60.0;
   msg.game_req.number_of_moves = moves;

   std::string dxp = pos_dxp(pos);

   if (pos == pos::Start) {
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

   msg.game_acc.follower_name = Engine_Name + " " + Engine_Version;
   msg.game_acc.acceptance_code = acceptance_code;

   put_string(encode_message('A', msg));
}

static void put_move(Move mv, double time, const Pos & pos) {

   Message msg;

   Square from = move::from(mv, pos);
   Square to   = move::to(mv, pos);
   Bit    caps = move::captured(mv, pos);

   msg.move.time = time;
   msg.move.from_field = square_to_std(from);
   msg.move.to_field   = square_to_std(to);
   msg.move.number_captured = bit::count(caps);
   int i = 0;
   for (Square sq : caps) {
      msg.move.captured_pieces[i++] = square_to_std(sq);
   }
   assert(i == msg.move.number_captured);

   put_string(encode_message('M', msg));
}

static void put_game_end(int result, char stop_code) {

   assert(result >= -2 && result <= +1); // -2 for "unknown"

   Message msg;

   msg.game_end.reason = char('2' + result);
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
      default :  throw Bad_Output();
   }

   return s;
}

static void decode_message(char & header, Message & msg, const std::string & s) {

   Scanner scan(s);
   header = scan.get_char();

   switch (header) {
      case 'R' : decode_game_req(msg.game_req, scan); break;
      case 'A' : decode_game_acc(msg.game_acc, scan); break;
      case 'M' : decode_move(msg.move, scan);         break;
      case 'E' : decode_game_end(msg.game_end, scan); break;
      case 'C' : decode_chat(msg.chat, scan);         break;
      case 'B' : decode_back_req(msg.back_req, scan); break;
      case 'K' : decode_back_acc(msg.back_acc, scan); break;
      default :  throw Bad_Input();
   }
}

static std::string encode_game_req(const Game_Req & game_req) {

   std::string s;

   s += encode_int(game_req.version, 2);
   s += encode_string(game_req.initiator_name, 32);
   s += game_req.follower_color;
   s += encode_int(ml::round(game_req.thinking_time), 3); // TODO: real?
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

static std::string encode_move(const Move_ & move) {

   std::string s;

   s += encode_int(ml::round(move.time), 4); // TODO: real?
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

static void decode_game_req(Game_Req & game_req, Scanner & scan) {

   game_req.version = decode_int(scan, 2);
   game_req.initiator_name = decode_string(scan, 32);
   game_req.follower_color = scan.get_char();
   game_req.thinking_time = decode_real(scan, 3);
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

static void decode_game_acc(Game_Acc & game_acc, Scanner & scan) {

   game_acc.follower_name = decode_string(scan, 32);
   game_acc.acceptance_code = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_move(Move_ & move, Scanner & scan) {

   move.time = decode_real(scan, 4);
   move.from_field = decode_int(scan, 2);
   move.to_field = decode_int(scan, 2);
   move.number_captured = decode_int(scan, 2);

   if (move.number_captured < 0 || move.number_captured > 20) {
      throw Bad_Input();
   }

   for (int i = 0; i < move.number_captured; i++) {
      move.captured_pieces[i] = decode_int(scan, 2);
   }

   if (!scan.eos()) throw Bad_Input();
}

static void decode_game_end(Game_End & game_end, Scanner & scan) {

   game_end.reason = scan.get_char();
   game_end.stop_code = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_chat(Chat & chat, Scanner & scan) {

   chat.text = scan.get_field();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_back_req(Back_Req & back_req, Scanner & scan) {

   back_req.move_number = decode_int(scan, 3);
   back_req.color_to_move = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static void decode_back_acc(Back_Acc & back_acc, Scanner & scan) {

   back_acc.acceptance_code = scan.get_char();

   if (!scan.eos()) throw Bad_Input();
}

static std::string score_to_string(double sc) {
   char s[256];
   std::sprintf(s, "%+.2f", sc);
   return s;
}

static std::string encode_int(int n, int size) { // fill with '0'

   if (n < 0) throw Bad_Output();
   assert(size <= 10);

   std::string s;
   s.resize(size);

   for (int i = 0; i < size; i++) {
      s[size - i - 1] = char('0' + n % 10);
      n /= 10;
   }

   if (n != 0) throw Bad_Output();

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

static Side decode_side(char c) {

   switch (c) {
      case 'W' : return White;
      case 'Z' : return Black;
      default :  throw Bad_Input();
   }
}

static int decode_int(Scanner & scan, int size) {
   std::string s = scan.get_field(size);
   return std::stoi(s);
}

static double decode_real(Scanner & scan, int size) {
   std::string s = scan.get_field(size);
   return std::stod(s);
}

static std::string decode_string(Scanner & scan, int size) {
   std::string s = scan.get_field(size);
   return ml::trim(s);
}

Scanner::Scanner(const std::string & s) : m_string{s} {}

std::string Scanner::get_field() { // remaining characters
   return get_field(int(m_string.size()) - m_pos);
}

std::string Scanner::get_field(int size) {

   std::string s;

   for (int i = 0; i < size; i++) {
      s += get_char();
   }

   return s;
}

bool Scanner::eos() const {
   return m_pos == int(m_string.size());
}

char Scanner::get_char() {
   if (eos()) throw Bad_Input();
   return m_string[m_pos++];
}

static std::string get_string() {
   return socket_::read();
}

static void put_string(const std::string & s) {
   socket_::write(s);
}

} // namespace dxp

