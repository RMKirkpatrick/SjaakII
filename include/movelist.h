#ifndef MOVELIST_H
#define MOVELIST_H

#include "move.h"

/* datastructure to hold the number of legal moves in a position */
#define MAX_MOVES    256
typedef struct movelist_t {
   move_t *move;
   int *score;
   int num_moves;
   int cur_move;
   int max_moves;

   movelist_t () { 
      cur_move = num_moves = 0;
      max_moves = MAX_MOVES;
      move  = (move_t *)malloc(max_moves * sizeof *move);
      score = (int *)malloc(max_moves * sizeof *score);
   }
   ~movelist_t() {
      free(move);
      free(score);
   }

   void clear()
   {
      cur_move = num_moves = 0;
   }

   void push(move_t m)
   {
      move[num_moves++] = m;

      if (num_moves >= max_moves) {
         max_moves += MAX_MOVES;
         move  = (move_t *)realloc(move, max_moves * sizeof *move);
         score = (int *)realloc(score, max_moves * sizeof *score);
      }
   }

   void print() const
   {
      for (int n=0; n<num_moves; n++) {
         printf("%s ", move_to_string(move[n]));
      }
      printf("\n");
   }

   bool contains(move_t m) const
   {
      for (int n=0; n<num_moves; n++)
         if (move[n] == m) return true;
      return false;
   }

   int get_move_score() const
   {
      assert(cur_move > 0);
      assert(cur_move-1 < num_moves);

      return score[cur_move-1];
   }

   void set_move_score(int s)
   {
      assert(cur_move > 0);
      assert(cur_move-1 < num_moves);

      score[cur_move-1] = s;
   }

   void rewind()
   {
      cur_move = 0;
   }

   move_t next_move()
   {
      move_t m = 0;

      if (cur_move < num_moves) {
         int nm = cur_move;
         int sv = score[nm];
         int n;
         for (n=cur_move+1; n<num_moves; n++) {
            if (score[n] > score[nm])
               nm = n;
         }
         n = cur_move;
         m = move[n]; move[n] = move[nm]; move[nm] = m;
         sv = score[n]; score[n] = score[nm]; score[nm] = sv;

         m = move[cur_move];
         cur_move++;
      }

      return m;
   }

   void show()
   {
      int n = cur_move;

      rewind();
      while (move_t m = next_move()) {
         printf("% 3d. % 6d %s\n", cur_move, get_move_score(), move_to_string(m));
      }
   }

   /* A bit of syntactic sugar: define a functor so we can call movelist()
    * instead of movelist.next_move().
    * Then if we call it "movelist_t next_move" we can write "move =
    * next_move()".
    * Perhaps this is not quite the same as readable code...
    */
   move_t operator()() {
      return next_move();
   }

   void sort() {
   }

   /* Returns the index in the movelist of a valid move corresponding to the
    * given origin/destination. Intended for player interaction.
    * Returns -1 if no moves in the list match (ie, the requested move is
    * invalid). In the case of a promotion, there will be more than one move
    * that matches.
    */
   int validate_move(int from, int to, int ppiece = MAX_PIECE_TYPES) const
   {
      int n;

      for (n=0; n<num_moves; n++) {
         if (get_move_from(move[n]) == from && get_move_to(move[n]) == to) {
            bool promotion_or_gate = is_promotion_move(move[n]) || is_gate_move(move[n]);
            if (!promotion_or_gate && ppiece == MAX_PIECE_TYPES)
               return n;
            else if (promotion_or_gate && get_move_promotion_piece(move[n]) == ppiece)
               return n;
         }
      }
      return -1;
   }
} movelist_t;

#endif

