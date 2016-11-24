
// book.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "board.h"
#include "book.h"
#include "libmy.hpp"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "pos.h"
#include "score.h"

namespace book {

// constants

static const int Hash_Bit = 20;
static const int Hash_Size = 1 << Hash_Bit;
static const int Hash_Mask = Hash_Size - 1;

// types

struct Entry {
   uint64 key;
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

   Entry * find_entry (uint64 key, bool create = false);
};

// variables

static Book G_Book;

// prototypes

static void backup ();
static int  backup (Board & bd);

static void load (const std::string & file_name);
static void load (std::istream & file, Board & bd);

static void book_gen    (List & list, Board & bd);
static void book_filter (List & list, int margin);

static bool in_book    (const Board & bd);
static int  book_score (const Board & bd);

// functions

void init() {

   std::cout << "init book" << std::endl;
   G_Book.load("data/book");
}

move_t move(const Board & bd, int margin) {

   if (!in_book(bd)) return Move_None;

   List list;

   Board new_bd; // let Board::Board() initialise frame squares
   new_bd = bd;
   book_gen(list, new_bd);

   book_filter(list, margin);
   if (list.size() == 0) return Move_None;

   return list_pick(list, 30.0);
}

void Book::load(const std::string & file_name) {

   clear();
   book::load(file_name);
   backup();
}

void Book::clear() {

   p_table.resize(Hash_Size);

   Entry entry = { 0, 0, false, false };

   for (int i = 0; i < int(p_table.size()); i++) {
      p_table[i] = entry;
   }
}

void Book::clear_done() {

   for (int i = 0; i < int(p_table.size()); i++) {
      p_table[i].done = false;
   }
}

Entry * Book::find_entry(uint64 key, bool create) {

   assert(key != 0);
   if (key == 0) return NULL;

   for (int index = int(key & Hash_Mask); true; index = (index + 1) & Hash_Mask) {

      Entry * entry = &p_table[index];

      if (entry->key == 0) {

         if (create) {

            entry->key = key;
            entry->score = score::None;
            entry->node = false;
            entry->done = false;

            return entry;

         } else {

            return NULL;
         }

      } else if (entry->key == key) {

         return entry;
      }
   }
}

static void backup() {

   Book & book = G_Book;
   book.clear_done();

   Board bd;
   bd.init();

   (void) backup(bd);
}

static int backup(Board & bd) {

   Book & book = G_Book;

   Entry * entry = book.find_entry(bd.key());
   assert(entry != NULL);

   if (entry->done || !entry->node) return entry->score;

   List list;
   gen_moves(list, bd);

   int bs = score::None;

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);

      Undo undo;

      bd.do_move(mv, undo);
      int sc = -backup(bd);
      bd.undo_move(mv, undo);

      if (sc > bs) bs = sc;
   }

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

   Book & book = G_Book;
   book.clear_done();

   Board bd;
   bd.init();

   load(file, bd);
}

static void load(std::istream & file, Board & bd) {

   Book & book = G_Book;

   Entry * entry = book.find_entry(bd.key(), true);
   assert(entry != NULL);

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
      gen_moves(list, bd);
      list.sort_static();

      for (int i = 0; i < list.size(); i++) {

         move_t mv = list.move(i);

         Undo undo;

         bd.do_move(mv, undo);
         load(file, bd);
         bd.undo_move(mv, undo);
      }
   }
}

static void book_gen(List & list, Board & bd) {

   Book & book = G_Book;

   Entry * entry = book.find_entry(bd.key());
   assert(entry != NULL);
   assert(entry->node);

   gen_moves(list, bd);

   for (int i = 0; i < list.size(); i++) {

      move_t mv = list.move(i);

      Undo undo;

      bd.do_move(mv, undo);
      int sc = -book_score(bd);
      bd.undo_move(mv, undo);

      list.set_score(i, sc);
   }

   list.sort();
}

static void book_filter(List & list, int margin) {

   if (list.size() > 1) {

      int i;

      for (i = 1; i < list.size(); i++) { // skip 0
         if (list.score(i) + margin < list.score(0)) break;
      }

      list.set_size(i);
   }
}

static bool in_book(const Board & bd) {

   Entry * entry = G_Book.find_entry(bd.key());
   return entry != NULL && entry->node;
}

static int book_score(const Board & bd) {

   Entry * entry = G_Book.find_entry(bd.key());
   assert(entry != NULL);

   return entry->score;
}

}

// end of book.cpp

