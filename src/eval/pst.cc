#include <stdint.h>
#include "pieces.h"
#include "board.h"
#include "pst.h"

int psq_map[NUM_SIDES][128];

/* Base centralisation score for each square */
int centre_table[128];

/* Base advancement score for each square */
int advance_table[128];

/* Extra advancement score for proximity to promotion zone, for pawns */
int promo_table[128];

/* End game bonus for driving the defending king to the edge */
int lone_king_table[128];

/* Base shield score, for pawn shields */
int shield_table[128];

/* Initialise the base tables for the current board size */
void initialise_base_evaluation_tables(int files, int ranks)
{
   int r, f;
   int c = files;

   if (ranks > c)
      c = ranks;

   int aa = 0;
   for (r = 0; r<ranks; r++) {
      for (f = 0; f<files; f++) {
         int s = f + files * r;

         /* Advance table, a progressive piece advancement */
         advance_table[s] = r;

         centre_table[s] = ((c-1)&~0x1) - (abs(2*r - c + 1)/2 + abs(2*f - c + 1)/2);
         if (files & 1)
            centre_table[s] = ((c-1)&~0x1) - (abs(2*r - c + 1)/2 + abs(2*f - c + 2 - (c&1))/2);
         aa += centre_table[s];

         promo_table[s] = 0;

         shield_table[s] = 0;
         if (f < files/2 && (r == 1)) shield_table[s] = 1;
         if (f > files/2+1 && (r == 1)) shield_table[s] = 1;

         lone_king_table[s] = 6*(abs(2*r - ranks + 1) + abs(2*f - files + 1));

         psq_map[WHITE][s] = s;
         psq_map[BLACK][s] = f + (ranks-1 - r) * files;
      }
   }
   for (r = 0; r<ranks; r++) {
      for (f = 0; f<files; f++) {
         int s = f + files * r;
         centre_table[s] -= aa / (files*ranks);
      }
   }

#if 0
   for (r = 0; r<ranks; r++) {
      for (f = 0; f<files; f++) {
         printf(" % 2d ", centre_table[f+files*r]);
      }
      printf("\n");
   }
   exit(0);
#endif
}


