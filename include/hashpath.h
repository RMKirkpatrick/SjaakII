#ifndef HASHPATH_H
#define HASHPATH_H

#include <stdint.h>
#include "move.h"

#define MAX_HASH_PATH_LENGTH  128

typedef struct {
   uint64_t lock;
   move_t moves[MAX_HASH_PATH_LENGTH];
   int length;
   int generation;
} hash_path_entry_t;

typedef struct {
   /* We keep two parallel hash tables: a "depth priority" table and an
    * "always replace" table.
    */
   hash_path_table_entry_t *data;
   size_t number_of_elements;
   size_t write_count;
   uint8_t generation;
} hash_path_table_t;

hash_path_table_t *create_hash_path_table(size_t nelem);
void destroy_hash_path_table(hash_path_table_t *table);

#endif
