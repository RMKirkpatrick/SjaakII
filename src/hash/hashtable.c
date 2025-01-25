/*  Jazz, a program for playing chess
 *  Copyright (C) 2009, 2011  Evert Glebbeek
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include "hashtable.h"
#include "prefetch.h"

#define HASH_BUCKETS    3

static inline size_t map_key_to_index(uint64_t key, size_t nelem)
{
   return key & ( nelem - 1);
}

static inline uint32_t map_key_to_lock(uint64_t key)
{
   return key >> 32;
}

hash_table_t *create_hash_table(size_t nelem)
{
   hash_table_t *table;
   if (!nelem) return NULL;

   table = calloc(1, sizeof *table);
   table->data = calloc(nelem + HASH_BUCKETS, sizeof *table->data);
   table->number_of_elements = nelem;
   return table;
}

void destroy_hash_table(hash_table_t *table)
{
   if (table) {
      free(table->data);
      free(table);
   }
}

/* Encrypt transposition table entry using the "xor trick" for lockless
 * hashing.
 */
static void crypt(hash_table_entry_t *hash)
{
   uint64_t *h;
   assert(sizeof(hash_table_entry_t) == 3*sizeof(uint64_t));
   h = (uint64_t *)hash;
   //h[1] ^= h[2];
   h[0] ^= h[1];
}

//static lock_t hash_lock = 0;
void store_table_entry(hash_table_t *table, uint64_t key, int depth, int score, unsigned int flags, move_t best_move)
{
   hash_table_entry_t *data;
   hash_table_entry_t *worst_data = NULL;
   size_t index, b;
   uint32_t lock;

   if (!table)
      return;

   /* Map the key onto the array index, check if entry is there */
   index = map_key_to_index(key, table->number_of_elements);
   lock = map_key_to_lock(key);
   worst_data = data = table->data + index;

   //acquire_lock(&hash_lock);

   /* Check all buckets */
   for (b=1; b<HASH_BUCKETS; b++) {
      hash_table_entry_t hash = data[b];
      crypt(&hash);
      if (hash.lock == lock) {
         worst_data = data+b;
         break;
      }

      if (hash.depth < worst_data->depth ||
          hash.generation < table->generation ||
          hash.generation == 0) {
         worst_data = data+b;
      }
   }
   data = worst_data;

   data->lock = lock;
   data->depth = depth;
   data->score = score;
   data->flags = flags;
   data->best_move = best_move;
   data->generation = table->generation;
   crypt(data);
   table->write_count++;
   //release_lock(&hash_lock);
}

bool retrieve_table(hash_table_t *table, uint64_t key, int *depth, int *score, unsigned int *flags, move_t *best_move)
{
   size_t index, b;
   uint32_t lock;

   if (!table)
      return false;

   /* Map the key onto the array index, check if entry is there */
   index = map_key_to_index(key, table->number_of_elements);
   lock = map_key_to_lock(key);

   //acquire_lock(&hash_lock);

   /* Check all buckets */
   b = 0;
   for (b = 0; b<HASH_BUCKETS; b++) {
      hash_table_entry_t hash = table->data[index+b];
      crypt(&hash);
      if (hash.lock == lock) {
         hash.generation = table->generation;

         *depth = hash.depth;
         *score = hash.score;
         *flags = hash.flags;
         *best_move = hash.best_move;
         //release_lock(&hash_lock);
         return true;
      }
   }

   //release_lock(&hash_lock);
   return false;
}

void prepare_hashtable_search(hash_table_t *table)
{
   table->generation++;
   table->write_count = 0;
}

void prefetch_hashtable(hash_table_t *table, uint64_t key)
{
   size_t index;

   if (table) {
      index = map_key_to_index(key, table->number_of_elements);
      prefetch(table->data+index);
   }
}

int count_unused_table_entries(hash_table_t *table)
{
   size_t n;
   int count;
   if (!table)
      return 0;

   count = 0;

   for (n=0; n<table->number_of_elements; n++) {
      if (table->data[n].generation == 0)
         count++;
   }

   return count;
}
