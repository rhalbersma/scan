
// includes

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "book.hpp"
#include "common.hpp"
#include "gen.hpp"
#include "hash.hpp"
#include "libmy.hpp"
#include "list.hpp"
#include "move.hpp"
#include "pos.hpp"
#include "score.hpp"
#include "var.hpp"

namespace book {

// constants

const int Hash_Bit  {21};
const int Hash_Size {1 << Hash_Bit};
const int Hash_Mask {Hash_Size - 1};

const Key Key_None {Key(0)};

// types

struct Entry { // 16 bytes (with alignment)
   Key key {Key_None};
   int score {0};
   bool node {false};
   bool done {false};
};

class Book {

private:

   std::vector<Entry> m_table;

public:

   void load  (const std::string & file_name);
   void clear ();

   void clear_done ();

   Entry * find_entry (const Pos & pos, bool create = false);
};

// variables

static Book G_Book;

// prototypes

static void backup ();
static int  backup (const Pos & pos);

static void load (const std::string & file_name);
static void load (std::istream & file, const Pos & pos);

// functions

void init() {

   static_assert(sizeof(Entry) == 16, "");

   std::cout << "init book" << std::endl;
   G_Book.load(std::string("data/book") + var::variant_name());
}

bool probe(const Pos & pos, Score margin, Move & move, Score & score) {

   move = move::None;
   score = score::None;

   const Entry * entry = G_Book.find_entry(pos);
   if (entry == nullptr || !entry->node) return false;

   List list;
   gen_moves(list, pos);

   for (int i = 0; i < list.size(); i++) {
      Move mv = list[i];
      list.set_score(i, -G_Book.find_entry(pos.succ(mv))->score);
   }

   list.sort();

   if (list.size() > 1) {

      int i;

      for (i = 1; i < list.size(); i++) { // skip 0
         if (list.score(i) + margin < list.score(0)) break;
      }

      list.set_size(i);
   }

   if (list.size() == 0) return false;

   int i = list::pick(list, (var::Variant == var::Frisian) ? 10.0 : 20.0);

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
   m_table.resize(Hash_Size);
   Entry entry {};
   std::fill(m_table.begin(), m_table.end(), entry);
}

void Book::clear_done() {
   for (Entry & entry : m_table) {
      entry.done = false;
   }
}

Entry * Book::find_entry(const Pos & pos, bool create) {

   Key key = hash::key(pos);
   if (key == Key_None) return nullptr;

   for (int index = hash::index(key, Hash_Mask); true; index = (index + 1) & Hash_Mask) {

      Entry * entry = &m_table[index];

      if (entry->key == Key_None) { // free entry

         if (create) {
            *entry = {key, 0, false, false};
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

static int backup(const Pos & pos) {

   Entry * entry = G_Book.find_entry(pos);
   assert(entry != nullptr);

   if (entry->done || !entry->node) return entry->score;

   List list;
   gen_moves(list, pos);

   int bs = score::None;

   for (Move mv : list) {
      bs = std::max(bs, -backup(pos.succ(mv)));
   }

   if (bs == score::None) bs = -score::Inf; // no legal moves

   assert(bs >= -score::Inf && bs <= +score::Inf);
   entry->score = bs;
   entry->done = true;

   return bs;
}

static void load(const std::string & file_name) {

   std::ifstream file(file_name);

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
      list.sort_static(pos);

      for (Move mv : list) {
         load(file, pos.succ(mv));
      }
   }

   entry->done = true;
}

} // namespace book

