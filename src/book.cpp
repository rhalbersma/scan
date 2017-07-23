
// includes

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "book.hpp"
#include "common.hpp"
#include "hash.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "move_gen.hpp"
#include "pos.hpp"
#include "score.hpp"

namespace book {

// constants

const int Hash_Bit  { 20 };
const int Hash_Size { 1 << Hash_Bit };
const int Hash_Mask { Hash_Size - 1 };

const Key Key_None { Key(0) };

// types

struct Entry {
   Key key;
   int16 score;
   bool node;
   bool done;
};

class Book {

private :

   std::vector<Entry> p_table;

public :

   void load  (const std::string & file_name);
   void clear ();

   void clear_done ();

   Entry * find_entry (const Pos & pos, bool create = false);
};

// variables

static Book G_Book;

// prototypes

static void  backup ();
static Score backup (const Pos & pos);

static void load (const std::string & file_name);
static void load (std::istream & file, const Pos & pos);

static void book_gen    (List & list, const Pos & pos);
static void book_filter (List & list, Score margin);

static bool  in_book    (const Pos & pos);
static Score book_score (const Pos & pos);

// functions

void init() {
   std::cout << "init book" << std::endl;
   G_Book.load("data/book");
}

bool probe(const Pos & pos, Score margin, Move & move, Score & score) {

   move = move::None;
   score = score::None;

   if (!in_book(pos)) return false;

   List list;
   book_gen(list, pos);

   book_filter(list, margin);
   if (list.size() == 0) return false;

   int i = list::pick(list, 20.0);
   move = list.move(i);
   score = score::make(list.score(i));
   return true;
}

void Book::load(const std::string & file_name) {
   clear();
   book::load(file_name);
   backup();
}

void Book::clear() {

   p_table.resize(Hash_Size);

   Entry entry { Key_None, 0, false, false };

   for (int i = 0; i < int(p_table.size()); i++) {
      p_table[i] = entry;
   }
}

void Book::clear_done() {
   for (int i = 0; i < int(p_table.size()); i++) {
      p_table[i].done = false;
   }
}

Entry * Book::find_entry(const Pos & pos, bool create) {

   Key key = hash::key(pos);
   if (key == Key_None) return nullptr;

   for (int index = hash::index(key, Hash_Mask); true; index = (index + 1) & Hash_Mask) {

      Entry * entry = &p_table[index];

      if (entry->key == Key_None) { // free entry

         if (create) {
            *entry = { key, 0, false, false };
            return entry;
         } else {
            return nullptr;
         }

      } else if (entry->key == key) {

         return entry;
      }
   }
}

static void backup() {
   G_Book.clear_done();
   (void) backup(pos::Start);
}

static Score backup(const Pos & pos) {

   Entry * entry = G_Book.find_entry(pos);
   assert(entry != nullptr);

   if (entry->done || !entry->node) return Score(entry->score);

   List list;
   gen_moves(list, pos);

   Score bs = score::None;

   for (int i = 0; i < list.size(); i++) {

      Move mv = list.move(i);

      Pos new_pos = pos.succ(mv);
      Score sc = -backup(new_pos);

      if (sc > bs) bs = sc;
   }

   if (bs == score::None) bs = -score::Inf; // no legal moves

   assert(bs >= -score::Inf && bs <= +score::Inf);
   entry->score = bs;
   entry->done = true;

   return bs;
}

static void load(const std::string & file_name) {

   std::ifstream file(file_name.c_str());

   if (!file) {
      std::cerr << "unable to open file \"" << file_name << "\"" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   G_Book.clear_done();
   load(file, pos::Start);
}

static void load(std::istream & file, const Pos & pos) {

   Entry * entry = G_Book.find_entry(pos, true);
   assert(entry != nullptr);

   if (entry->done) return;
   entry->done = true;

   bool node;
   file >> node;

   if (file.eof()) {
      std::cerr << "load(): EOF" << std::endl;
      std::exit(EXIT_FAILURE);
   }

   if (!node) { // leaf

      file >> entry->score;

      if (file.eof()) {
         std::cerr << "load(): EOF" << std::endl;
         std::exit(EXIT_FAILURE);
      }

   } else {

      entry->node = true;

      List list;
      gen_moves(list, pos);
      list.sort_static();

      for (int i = 0; i < list.size(); i++) {

         Move mv = list.move(i);

         Pos new_pos = pos.succ(mv);
         load(file, new_pos);
      }
   }
}

static void book_gen(List & list, const Pos & pos) {

   gen_moves(list, pos);

   for (int i = 0; i < list.size(); i++) {

      Move mv = list.move(i);

      Pos new_pos = pos.succ(mv);
      Score sc = -book_score(new_pos);

      list.set_score(i, sc);
   }

   list.sort();
}

static void book_filter(List & list, Score margin) {

   if (list.size() > 1) {

      int i;

      for (i = 1; i < list.size(); i++) { // skip 0
         if (list.score(i) + margin < list.score(0)) break;
      }

      list.set_size(i);
   }
}

static bool in_book(const Pos & pos) {
   const Entry * entry = G_Book.find_entry(pos);
   return entry != nullptr && entry->node;
}

static Score book_score(const Pos & pos) {

   const Entry * entry = G_Book.find_entry(pos);
   assert(entry != nullptr);

   return Score(entry->score);
}

}

