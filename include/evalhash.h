/*  Leonidas, a program for playing chess variants
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
#ifndef EVALHASH_H
#define EVALHASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "bool.h"

#undef DEBUG_EVHASH

typedef struct {
#ifdef DEBUG_EVHASH
   uint64_t key;
#endif
   uint64_t data;
} eval_hash_t;

typedef struct {
   eval_hash_t *data;
   size_t number_of_elements;
} eval_hash_table_t;

eval_hash_table_t *create_eval_hash_table(size_t nelem);
void destroy_eval_hash_table(eval_hash_table_t *table);
bool query_eval_table_entry(eval_hash_table_t *table, uint64_t key, int16_t *score);
void store_eval_hash_entry(eval_hash_table_t *table, uint64_t key, int16_t score);

#ifdef __cplusplus
}
#endif

#endif

