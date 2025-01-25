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
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
#include "move.h"

/* Types of entries that may occur in the table (flags) */
#define HASH_TYPE_EXACT       0x00000001
#define HASH_TYPE_LOWER       0x00000002
#define HASH_TYPE_UPPER       0x00000004

#ifdef __cplusplus
extern "C" {
#endif

/* Entries in the hash table */
typedef struct {
   uint32_t lock;          /* Store the full key (for position verification) */
   move_t best_move;       /* The best move returned from the previous search */
   int16_t score;          /* The value of this node */
   int16_t depth;          /* The depth to which this position was searched; distance from horizon */
   uint8_t flags;          /* Properties of this entry */
   uint8_t generation;     /* Generation of this entry */
} hash_table_entry_t;

typedef struct {
   /* We keep two parallel hash tables: a "depth priority" table and an
    * "always replace" table.
    */
   hash_table_entry_t *data;
   size_t number_of_elements;
   size_t write_count;
   uint8_t generation;
} hash_table_t;

hash_table_t *create_hash_table(size_t nelem);
void destroy_hash_table(hash_table_t *table);
void store_table_entry(hash_table_t *table, uint64_t key, int depth,  int score, unsigned int flags, move_t best_move);
bool retrieve_table(hash_table_t *table, uint64_t key, int *depth, int *score, unsigned int *flags, move_t *best_move);
void prefetch_hashtable(hash_table_t *table, uint64_t key);
void prepare_hashtable_search(hash_table_t *table);
int count_unused_table_entries(hash_table_t *table);

#ifdef __cplusplus
}
#endif

#endif
