/*  Sjaak, a program for playing chess
 *  Copyright (C) 2011, 2014, 2015  Evert Glebbeek
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
#include <stdlib.h>
#include <stdint.h>
#include "evalhash.h"
#include "bool.h"

/* FIXME: tune this */
#define NUM_BUCKETS 2
static inline size_t map_key_to_index(uint64_t key, size_t nelem)
{
   return key & (nelem - 1);
}

static inline uint64_t map_key_to_lock(uint64_t key)
{
   return key & ~0xffffll;
}

static int16_t extract_score(uint64_t data)
{
   uint16_t uscore = data & 0xffff;
   return (int16_t)uscore;
}

eval_hash_table_t *create_eval_hash_table(size_t nelem)
{
   eval_hash_table_t *table = calloc(1, sizeof *table);

   table->number_of_elements = nelem;
   table->data = calloc(nelem + NUM_BUCKETS, sizeof *table->data);
   return table; 
}

void destroy_eval_hash_table(eval_hash_table_t *table)
{
   if (table) {
      free(table->data);
      free(table);
   }
}

bool query_eval_table_entry(eval_hash_table_t *table, uint64_t key, int16_t *score)
{
   size_t index, b;
   uint64_t lock;

   if (!table)
      return false;

   /* Map the key onto the array index, check if entry is there */
   index = map_key_to_index(key, table->number_of_elements);
   lock = map_key_to_lock(key);
   //printf("0x%016llx 0x%016llx\n", key, lock);

   for (b=0; b<NUM_BUCKETS; b++) {
      uint64_t data = table->data[index + b].data;
      if (map_key_to_lock(data) == lock) {
         *score = extract_score(data);
         return true;
      }
   }
   return false;
}


void store_eval_hash_entry(eval_hash_table_t *table, uint64_t key, int16_t score)
{
   size_t index, b;
   uint64_t lock, data;

   if (!table)
      return;

   /* Map the key onto the array index, check if entry is there */
   index = map_key_to_index(key, table->number_of_elements);
   lock = map_key_to_lock(key);

   /* Find out where to store the entry.
    * If it already exists in one of the buckets we simply store it there
    * after moving the bucket to the start of the queue.
    * If it isn't present yet we shift the entire queue back one place and
    * create a new entry at the start.
    */
   for (b = 0; b<NUM_BUCKETS; b++) {
      if (map_key_to_lock(table->data[index+b].data) == lock) {
         if (b) {
            eval_hash_t h = table->data[index];
            table->data[index] = table->data[index+b];
            table->data[index+b] = h;
         }
         goto store;
      }
   }

   /* If we made it here we need to create a new entry at the start of the
    * queue.
    */
   for (b=NUM_BUCKETS-1; b>0; b--)
      table->data[index+b] = table->data[index+b-1];

store:
   data = lock | ((uint16_t)score);
   table->data[index].data = data;
#ifdef DEBUG_EVHASH
   table->data[index].key = key;
#endif
}

