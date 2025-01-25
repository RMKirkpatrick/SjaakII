#include <stdlib.h>
#include <stdint.h>
#include "aligned_malloc.h"
#include "assert.h"

typedef struct {
   void *real_ptr;
   void *align_ptr;
} aligned_memory_pool_t;

static aligned_memory_pool_t *pool = NULL;
static int pool_size = 0;
static int pool_free = 0;

/* Allocate aligned memory blocks. */
void *aligned_malloc(size_t size, size_t align)
{
   uint8_t *real_ptr;
   void *align_ptr;
   assert((align & (align-1)) == 0);
   if (pool_free == pool_size) {
      pool_size = 2*(pool_size)+1;
      pool = realloc(pool, pool_size * sizeof *pool);
   }

   real_ptr  = malloc(size + align-1);
   align_ptr = (void *) ((uint64_t)(real_ptr + align-1) & ~(align-1));

   pool[pool_free].real_ptr  = real_ptr;
   pool[pool_free].align_ptr = align_ptr;
   pool_free++;
   return align_ptr;
}

/* Free an aligned memory block */
void aligned_free(void *ptr)
{
   int n;
   for (n=0; n<pool_free; n++) {
      if (pool[n].align_ptr == ptr) {
         free(pool[n].real_ptr);
         pool_free--;
         pool[n] = pool[pool_free];
         return;
      }
   }
}
