/*  Sjaak, a program for playing chess
 *  Copyright (C) 2011, 2014  Evert Glebbeek
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
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <ctype.h>
#include <algorithm>
#include "assert.h"
#include "bitboard.h"
#include "pieces.h"
#include "board.h"
#include "move.h"
#include "movelist.h"
#include "aligned_malloc.h"

/* Stages for staged move generation */
typedef enum stage_t { STAGE_START=0,
                       STAGE_DROP=STAGE_START, STAGE_NORMAL,      /* Normal move generation */
                       STAGE_CHECKING_DROP, STAGE_CHECKING_MOVE,  /* Mate/Tsume search */
                       STAGE_CHECK_EVADE,                         /* Check evasion */
                       STAGE_DONE } stage_t;
static const stage_t next_stage[STAGE_DONE+1] = {
   // STAGE_DROP, STAGE_NORMAL,                    /* Normal move generation */
   STAGE_NORMAL, STAGE_DONE,
   // STAGE_CHECKING_DROP, STAGE_CHECKING_MOVE,    /* Mate/Tsume search */
   STAGE_CHECKING_MOVE, STAGE_DONE,
   // STAGE_CHECK_EVADE,                           /* Check evasion */
   STAGE_DONE,

   // STAGE_DONE
   STAGE_DONE
};


inline stage_t& operator++(stage_t& stage, int)
{
   assert(stage >= STAGE_START);
   assert(stage <  STAGE_DONE);
	const int i = static_cast<int>(stage)+1;
	stage = static_cast<stage_t>(i);
	return stage;
}

template<typename kind>
struct movegen_t {
   /* Leapers and asymmetric leapers. */
   bitboard_t<kind> leaper[MAX_LEAPER_TYPES][sizeof(kind)*8];
   bitboard_t<kind> aleaper[NUM_SIDES][MAX_LEAPER_TYPES][sizeof(kind)*8];
   int number_of_leapers;
   int number_of_aleapers;

   /* Rider descriptions */
   int number_of_riders;
   struct {
      int dx, dy;
   } rider_step[MAX_RIDER_TYPES][4];
   bitboard_t<kind> rider_ray[MAX_RIDER_TYPES][sizeof(kind)*8][sizeof(kind)*8];

   /* Stepper descriptions */
   uint32_t stepper_description[MAX_STEPPER_TYPES][NUM_SIDES]; // 8 directions, with repeat counts (0-15) for each->32 bits
   bitboard_t<kind> stepper_step[MAX_STEPPER_TYPES][NUM_SIDES][sizeof(kind)*8];
   bitboard_t<kind> step_mask[8];
   int inverse_step[8];
   int step_shift[8];
   int number_of_steppers;

   /* Castling move bitboards */
   bitboard_t<kind> castle_mask[NUM_CASTLE_MOVES][NUM_SIDES];
   bitboard_t<kind> castle_free[NUM_CASTLE_MOVES][NUM_SIDES];
   bitboard_t<kind> castle_safe[NUM_CASTLE_MOVES][NUM_SIDES];
   bitboard_t<kind> castle_king_dest[NUM_CASTLE_MOVES][NUM_SIDES];
   bitboard_t<kind> castle_rook_dest[NUM_CASTLE_MOVES][NUM_SIDES];
   int castle_king_from[NUM_CASTLE_MOVES][NUM_SIDES];

   /* Normal slider tables */
   bitboard_t<kind> **horizontal_slider_move;
   bitboard_t<kind> **vertical_slider_move;
   bitboard_t<kind> **horizontal_hopper_move;
   bitboard_t<kind> **vertical_hopper_move;
   move_flag_t super_slider_flags;
   move_flag_t super_hopper_flags;

   /* Super piece */
   bitboard_t<kind> super[sizeof(kind)*8];
   bitboard_t<kind> super_slider[sizeof(kind)*8];
   bitboard_t<kind> super_hopper[sizeof(kind)*8];
   bitboard_t<kind> super_leaper[sizeof(kind)*8];
   bitboard_t<kind> super_stepper[sizeof(kind)*8];
   bitboard_t<kind> super_rider[sizeof(kind)*8];

   void initialise() {
      /* Clear leaper/stepper descriptions */
      number_of_leapers = 0;
      number_of_aleapers = 0;
      number_of_steppers = 1;
      number_of_riders = 1;
      super_slider_flags = 0;
      super_hopper_flags = 0;

      memset(leaper, 0, sizeof leaper);
      memset(aleaper, 0, sizeof aleaper);

      memset(step_mask, 0, sizeof step_mask);
      memset(stepper_step, 0, sizeof stepper_step);

      for (int n = 0; n<NUM_SIDES; n++) {
         castle_mask[SHORT][n].clear();
         castle_free[SHORT][n].clear();
         castle_safe[SHORT][n].clear();
         castle_king_dest[SHORT][n].clear();
         castle_rook_dest[SHORT][n].clear();
         castle_mask[LONG][n].clear();
         castle_free[LONG][n].clear();
         castle_safe[LONG][n].clear();
         castle_king_dest[LONG][n].clear();
         castle_rook_dest[LONG][n].clear();
      }

      /* Free tables if previously allocated */
      if (horizontal_slider_move) aligned_free(horizontal_slider_move);
      if (vertical_slider_move)   aligned_free(vertical_slider_move  );
      if (horizontal_hopper_move) aligned_free(horizontal_hopper_move);
      if (vertical_hopper_move)   aligned_free(vertical_hopper_move  );

      horizontal_slider_move = NULL;
      vertical_slider_move   = NULL;
      horizontal_hopper_move = NULL;
      vertical_hopper_move   = NULL;

      /* Bitshifts for steppers */
      /* Bitshifts for all directions: N   NE  E   SE    S   SW    W   NW */
      step_shift[0] = bitboard_t<kind>::board_files;        // N
      step_shift[1] = bitboard_t<kind>::board_files+1;      // NE
      step_shift[2] = 1;                                    // E
      step_shift[3] =-bitboard_t<kind>::board_files+1;      // SE
      step_shift[4] =-bitboard_t<kind>::board_files;        // S
      step_shift[5] =-bitboard_t<kind>::board_files-1;      // SW
      step_shift[6] =-1;                                    // W
      step_shift[7] = bitboard_t<kind>::board_files-1;      // NW

      /* N NE E SE S SW W NW
       * 0 1  2 3  4 5  6 7
       * S SW W NW N NE E SE
       * 4 5  6 7  0 1  2 3
       */
      for (int n = 0; n<8; n++) 
         inverse_step[n] = (n+4)&7;

      step_mask[0] = ~ bitboard_t<kind>::board_north_edge;
      step_mask[1] = ~(bitboard_t<kind>::board_east_edge | bitboard_t<kind>::board_north_edge);
      step_mask[2] = ~ bitboard_t<kind>::board_east_edge;
      step_mask[3] = ~(bitboard_t<kind>::board_east_edge | bitboard_t<kind>::board_south_edge);
      step_mask[4] = ~ bitboard_t<kind>::board_south_edge;
      step_mask[5] = ~(bitboard_t<kind>::board_west_edge | bitboard_t<kind>::board_south_edge);;
      step_mask[6] = ~ bitboard_t<kind>::board_west_edge;
      step_mask[7] = ~(bitboard_t<kind>::board_west_edge | bitboard_t<kind>::board_north_edge);

      for (int n = 0; n<NUM_CASTLE_MOVES; n++)
      for (side_t side = WHITE; side<=BLACK; side++) {
         castle_king_from[n][side] = -1;
      }
   }

   void initialise_slider_tables()
   {
      int board_files = bitboard_t<kind>::board_files;
      int board_ranks = bitboard_t<kind>::board_ranks;
      int board_size = board_files * board_ranks;
      int file, rank;
      int occ;
      int n;

      /* Allocate memory for tables */
      size_t file_table_size;
      size_t rank_table_size;
      size_t file_table_start;
      size_t rank_table_start;
      size_t file_table_offset;
      size_t rank_table_offset;
      uint8_t *memory;

      /* Determine size of tables: rank attacks, so index by file */
      rank_table_size  = board_files*sizeof(bitboard_t<kind> *);
      if (rank_table_size & 15) rank_table_size += 16 - (rank_table_size & 15);
      rank_table_start = rank_table_size;
      rank_table_size += board_files*(1<<board_files)*sizeof(bitboard_t<kind>);
      rank_table_offset = ((size_t)1<<board_files)*sizeof(bitboard_t<kind>);

      /* Determine size of tables: file attacks, so index by rank */
      file_table_size  = board_ranks*sizeof(bitboard_t<kind> *);
      if (file_table_size & 15) file_table_size += 16 - (file_table_size & 15);
      file_table_start = file_table_size;
      file_table_size += board_ranks*(1<<board_ranks)*sizeof(bitboard_t<kind>);
      file_table_offset = ((size_t)1<<board_ranks)*sizeof(bitboard_t<kind>);

      /* Free tables if previously allocated */
      if (horizontal_slider_move) aligned_free(horizontal_slider_move);
      if (vertical_slider_move)   aligned_free(vertical_slider_move  );
      if (horizontal_hopper_move) aligned_free(horizontal_hopper_move);
      if (vertical_hopper_move)   aligned_free(vertical_hopper_move  );

      /* Allocate tables for horizontal (rank) attacks */
      memory = (uint8_t *)aligned_malloc(rank_table_size, 16);
      assert(memory);
      memset(memory, 0, rank_table_size);
      horizontal_slider_move = (bitboard_t<kind> **)memory;
      for(n = 0; n<board_files; n++) {
         horizontal_slider_move[n] = (bitboard_t<kind> *)(memory + rank_table_start + n*rank_table_offset);
      }

      memory = (uint8_t *)aligned_malloc(rank_table_size, 16);
      assert(memory);
      memset(memory, 0, rank_table_size);
      horizontal_hopper_move = (bitboard_t<kind> **)memory;
      for(n = 0; n<board_files; n++) {
         horizontal_hopper_move[n] = (bitboard_t<kind> *)(memory + rank_table_start + n*rank_table_offset);
      }

      /* Allocate tables for vertical (file) attacks */
      memory = (uint8_t *)aligned_malloc(file_table_size, 16);
      assert(memory);
      memset(memory, 0, file_table_size);
      vertical_slider_move = (bitboard_t<kind> **)memory;
      for(n = 0; n<board_ranks; n++) {
         vertical_slider_move[n] = (bitboard_t<kind> *)(memory + file_table_start + n*file_table_offset);
      }

      memory = (uint8_t *)aligned_malloc(file_table_size, 16);
      assert(memory);
      memset(memory, 0, file_table_size);
      vertical_hopper_move = (bitboard_t<kind> **)memory;
      for(n = 0; n<board_ranks; n++) {
         vertical_hopper_move[n] = (bitboard_t<kind> *)(memory + file_table_start + n*file_table_offset);
      }

      /* Rank attacks */
      for (file = 0; file<board_files; file++) {
         for (occ = 0; occ < 1<<board_files; occ++) {
            /* Left of slider position, rook and cannon moves, rook
             * attacks.
             */
            for (n=file-1; n>=0; n--) {
               horizontal_slider_move[file][occ] |= bitboard_t<kind>::board_file[n];//bitboard_t<kind>::square_bitboards[n];
               if ( occ & (1 << n) )
                  break;
            }
            n--;
            /* Cannon attacks */
            for (; n>=0; n--) {
               horizontal_hopper_move[file][occ] |= bitboard_t<kind>::board_file[n];//bitboard_t<kind>::square_bitboards[n];
               if ( occ & (1 << n) )
                  break;
            }

            /* Right of slider position */
            for (n=file+1; n<board_files; n++) {
               horizontal_slider_move[file][occ] |= bitboard_t<kind>::board_file[n];//bitboard_t<kind>::square_bitboards[n];

               if ( occ & (1 << n) )
                  break;
            }
            n++;
            /* Cannon attacks */
            for (; n<board_files; n++) {
               horizontal_hopper_move[file][occ] |= bitboard_t<kind>::board_file[n];//bitboard_t<kind>::square_bitboards[n];
               if ( occ & (1 << n) )
                  break;
            }
         }
      }

      /* File attacks */
      for (rank = 0; rank < board_ranks; rank++) {
         for (occ = 0; occ < 1<<board_ranks; occ++) {
            /* South of slider position, rook and cannon moves, rook
             * attacks.
             */
            for (n=rank-1; n>=0; n--) {
               vertical_slider_move[rank][occ] |= bitboard_t<kind>::board_rank[n];//bitboard_t<kind>::square_bitboards[board_files*n];
               if ( occ & (1 << n) )
                  break;
            }
            n--;
            /* Cannon attacks */
            for (; n>=0; n--) {
               vertical_hopper_move[rank][occ] |= bitboard_t<kind>::board_rank[n];//bitboard_t<kind>::square_bitboards[board_files*n];
               if ( occ & (1 << n) )
                  break;
            }

            /* North of slider position */
            for (n=rank+1; n<board_ranks; n++) {
               vertical_slider_move[rank][occ] |= bitboard_t<kind>::board_rank[n];//bitboard_t<kind>::square_bitboards[board_files*n];
               if ( occ & (1 << n) )
                  break;
            }
            n++;
            /* Cannon attacks */
            for (; n<board_ranks; n++) {
               vertical_hopper_move[rank][occ] |= bitboard_t<kind>::board_rank[n];//bitboard_t<kind>::square_bitboards[board_files*n];
               if ( occ & (1 << n) )
                  break;
            }
         }
      }

      /* Initialise superpiece attacks to a full board */
      for (n=0; n<board_size; n++) {
         super_slider[n] = super_hopper[n] = super_leaper[n] = super[n] = bitboard_t<kind>::board_all;
      }
   }

   void apply_board_masks(void)
   {
      int size = bitboard_t<kind>::board_files * bitboard_t<kind>::board_ranks;

      for (int n=0; n<8; n++)
         step_mask[n] &= bitboard_t<kind>::board_all;

      for (int n=0; n<number_of_leapers; n++) {
         for (int s=0; s<size; s++)
            leaper[n][s] &= bitboard_t<kind>::board_all;
      }

      for (int n=0; n<number_of_aleapers; n++) {
         for (int s=0; s<size; s++) {
            aleaper[WHITE][n][s] &= bitboard_t<kind>::board_all;
            aleaper[BLACK][n][s] &= bitboard_t<kind>::board_all;
         }
      }

      for (int n=0; n<number_of_steppers; n++) {
         for (int s=0; s<size; s++) {
            stepper_step[n][WHITE][s] &= bitboard_t<kind>::board_all;
            stepper_step[n][BLACK][s] &= bitboard_t<kind>::board_all;
         }
      }
   }


   void initialise_super_tables(void)
   {
      int board_files = bitboard_t<kind>::board_files;
      int board_ranks = bitboard_t<kind>::board_ranks;
      int board_size = board_files * board_ranks;
      int n, c;

      /* Initialise stepper masks */
      for (int c = 1; c<number_of_steppers; c++) {
         for(n=0; n<board_size; n++) {
            bitboard_t<kind> stepper;
            stepper.set(n);

            stepper_step[c][WHITE][n] = generate_stepper_move_bitboard(make_stepper_index(c), WHITE, bitboard_t<kind>::board_empty, stepper);
            stepper_step[c][BLACK][n] = generate_stepper_move_bitboard(make_stepper_index(c), BLACK, bitboard_t<kind>::board_empty, stepper);
         }
      }

      for(n=0; n<board_size; n++)
         super_stepper[n].clear();

      /* The super-stepper can reverse-step as well as step. This is needed
       * for cases where we need to do a reverse lookup.
       */
      for (int c = 1; c<number_of_steppers; c++) {
         for (int side=0; side<2; side++) {
            for(n=0; n<board_size; n++) {
               bitboard_t<kind> bb = stepper_step[c][side][n];
               super_stepper[n] |= bb;
               while(!bb.is_empty()) {
                  int s = bb.bitscan();
                  bb.reset(s);

                  super_stepper[s].set(n);
               }
            }
         }
      }

      for(n=0; n<board_size; n++) {
         super_stepper[n] &= bitboard_t<kind>::board_all;

         super_leaper[n].clear();
         for (c=0; c<number_of_leapers; c++)
            super_leaper[n] |= leaper[c][n];
         for (c=0; c<number_of_aleapers; c++) {
            super_leaper[n] |= aleaper[WHITE][c][n];
            super_leaper[n] |= aleaper[BLACK][c][n];
         }
         super_leaper[n] &= bitboard_t<kind>::board_all;

         super_rider[n].clear();
         for (c=1; c<number_of_riders; c++) {
            super_rider[n] |= generate_rider_move_bitboard(make_rider_index(c), WHITE, n, bitboard_t<kind>::board_empty);
            super_rider[n] |= generate_rider_move_bitboard(make_rider_index(c), BLACK, n, bitboard_t<kind>::board_empty);
         }
         super_rider[n] &= bitboard_t<kind>::board_all;

         super_slider[n].clear();
         super_slider[n] |= generate_slider_move_bitboard(super_slider_flags, WHITE, n, bitboard_t<kind>::board_empty);
         super_slider[n] &= bitboard_t<kind>::board_all;

         super_hopper[n].clear();
         if (super_hopper_flags)
         super_hopper[n] |= generate_slider_move_bitboard(super_hopper_flags>>4, WHITE, n, bitboard_t<kind>::board_empty);
         super_hopper[n] &= bitboard_t<kind>::board_all;

         super[n] = super_hopper[n] | super_leaper[n] | super_slider[n] | super_stepper[n] | super_rider[n];
      }
   }


   /* Move generator, per piece type */
   bitboard_t<kind> generate_leaper_move_bitboard(move_flag_t flags, side_t side, int square, bitboard_t<kind> occ) const {
      assert(is_leaper(flags));
      bitboard_t<kind> moves;
      int index = get_leaper_index(flags);

      moves = is_aleaper(flags) ? aleaper[side][index][square]
                                :  leaper      [index][square];

      /* Simple leaper? */
      if (is_simple_leaper(flags))
         return moves;

      /* Double-step leaper */
      if (is_double_leaper(flags)) {
         bitboard_t<kind> bb = moves;
         if (is_masked_leaper(flags)) bb &= ~occ;
         index = get_leaper_index2(flags);
         while (!bb.is_empty()) {
            int square = bb.bitscan();
            bb.reset(square);
            moves |= leaper[index][square];
         }
      }

      /* Masked leaper */
      if (is_masked_leaper(flags)) {
         index = get_leaper_indexm(flags);
         moves &= leaper[index][square];
      }

      return moves;
   }

   bitboard_t<kind> generate_rider_move_bitboard(move_flag_t flags, side_t /* side */, int from, bitboard_t<kind> occ) const {
      assert(is_rider(flags));
      bitboard_t<kind> moves;
      int index = get_rider_index(flags);

      int file = unpack_file(from);
      int rank = unpack_rank(from);

      for (int k=0; k<4; k++) {
         int sx = rider_step[index][k].dx;
         int sy = rider_step[index][k].dy;

         if (sx == 0 && sy == 0) break;

         int dx[8] = {  sx,  sx, -sx, -sx,  sy,  sy, -sy, -sy };
         int dy[8] = {  sy, -sy,  sy, -sy,  sx, -sx,  sx, -sx };

         for (int n = 0; n<8; n++) {
            int f = file + dx[n];
            int r = rank + dy[n];
            while (f>=0 && r>=0 && f<bitboard_t<kind>::board_files && r < bitboard_t<kind>::board_ranks) {
               int to = bitboard_t<kind>::pack_rank_file(r, f);
               moves.set(to);
               f += dx[n];
               r += dy[n];
               if (occ.test(to)) break;
            }
         }
      }

      return moves;
   }


   bitboard_t<kind> generate_slider_move_bitboard(move_flag_t flags, side_t /* side */, int square, bitboard_t<kind> occ) const {
      assert(is_slider(flags));
      bitboard_t<kind> moves;
      int file = unpack_file(square);
      int rank = unpack_rank(square);
      int diag = occ.diagonal_nr[square];
      int anti = occ.anti_diagonal_nr[square];
      int index;

      if (flags & MF_SLIDER_H) {
         index = occ.get_rank(rank);
         moves |= horizontal_slider_move[file][index] & bitboard_t<kind>::board_rank[rank];
      }

      if (flags & MF_SLIDER_V) {
         index = occ.get_file(file);
         moves |= vertical_slider_move[rank][index] & bitboard_t<kind>::board_file[file];
      }

      if (flags & MF_SLIDER_D) {
         bitboard_t<kind> mask = bitboard_t<kind>::board_diagonal[diag];
         index = (occ & mask).fill_south().get_rank(0);

         moves |= horizontal_slider_move[file][index] & mask;
      }

      if (flags & MF_SLIDER_A) {
         bitboard_t<kind> mask = bitboard_t<kind>::board_antidiagonal[anti];
         index = (occ & mask).fill_south().get_rank(0);

         moves |= horizontal_slider_move[file][index] & mask;
      }

      return moves;
   }

   bitboard_t<kind> generate_hopper_move_bitboard(move_flag_t flags, side_t /* side */, int square, bitboard_t<kind> occ) const {
      assert(is_hopper(flags));
      bitboard_t<kind> moves;
      int file = unpack_file(square);
      int rank = unpack_rank(square);
      int diag = occ.diagonal_nr[square];
      int anti = occ.anti_diagonal_nr[square];
      int index;

      //moves = generate_slider_move_bitboard(flags>>4, side, square, occ);
      //return generate_slider_move_bitboard(flags>>4, side, square, occ&~moves) ^ moves;

      if (flags & MF_HOPPER_H) {
         index = occ.get_rank(rank);
         moves |= horizontal_hopper_move[file][index] & bitboard_t<kind>::board_rank[rank];
      }

      if (flags & MF_HOPPER_V) {
         index = occ.get_file(file);
         moves |= vertical_hopper_move[rank][index] & bitboard_t<kind>::board_file[file];
      }

      if (flags & MF_HOPPER_D) {
         bitboard_t<kind> mask = bitboard_t<kind>::board_diagonal[diag];
         index = (occ & mask).fill_south().get_rank(0);

         moves |= horizontal_hopper_move[file][index] & mask;
      }

      if (flags & MF_HOPPER_A) {
         bitboard_t<kind> mask = bitboard_t<kind>::board_antidiagonal[anti];
         index = (occ & mask).fill_south().get_rank(0);

         moves |= horizontal_hopper_move[file][index] & mask;
      }

      return moves;
   }

   bitboard_t<kind> generate_stepper_move_bitboard(move_flag_t flags, side_t side, bitboard_t<kind> occ, bitboard_t<kind> steppers) const
   {
      bitboard_t<kind> moves;

      /* Check for single stepper moves, which are generated in parallel */
      int si = get_stepper_index(flags);
      int d;
      for (d=0; d<8; d++) {
         int c = (stepper_description[si][side] >> (d*4)) & 15;
         bitboard_t<kind> dmoves = steppers;

         if (c == 0) continue;

         /* We have a repetition count, so we do a number of steps one after the other.
          * This can effectively duplicate a slider.
          */
         for ( ; c>0; c--) {
            dmoves &= step_mask[d];
            dmoves = dmoves.sshift(step_shift[d]);
            moves |= dmoves;
            dmoves &= ~occ;
         }
      }

      return moves;
   }


   bitboard_t<kind> generate_super_attacks_for_squares(bitboard_t<kind> squares, const bitboard_t<kind> super[sizeof(kind)*8]) const
   {
      bitboard_t<kind> attacks;

      while (!squares.is_empty()) {
         int square = squares.bitscan();
         squares.reset(square);
         attacks |= super[square];
      }

      return attacks;
   }

   /* Generate an attack bitboard for all attackers within a specified mask */
   inline bitboard_t<kind> generate_attack_bitboard_mask(const board_t<kind> *board, const bitboard_t<kind> test_squares, const bitboard_t<kind> source_mask, const bitboard_t<kind> occ_mask, side_t side_to_move) const
   {
      piece_description_t<kind> *piece_types;
      move_flag_t *piece_capture_flags;
      bitboard_t<kind> own, enemy, own_movers;
      bitboard_t<kind> occupied;
      bitboard_t<kind> attacked;
      int n;

      piece_types = board->piece_types;
      piece_capture_flags = piece_types->piece_capture_flags;

      /* Bookkeeping: we keep a pointer to the next move in the move list, and
       * update the number of moves in the list at the end of this function
       */
      own = board->bbc[side_to_move] & occ_mask;
      enemy = board->bbc[next_side[side_to_move]] & occ_mask;

      occupied = own | enemy | test_squares;

      own_movers = own & source_mask;

      bitboard_t<kind> possible_attackers = own_movers;

      for (n=0; n<piece_types->num_piece_types && !possible_attackers.is_empty(); n++) {
         if ((possible_attackers & board->bbp[n]).is_empty()) continue;
         possible_attackers &= ~board->bbp[n];

         bitboard_t<kind> bb = own_movers & board->bbp[n];
         bitboard_t<kind> occ = occupied | piece_types->block[side_to_move][n];

         /* Steppers */
         if (is_stepper(piece_capture_flags[n])) {
            int si = get_stepper_index(piece_capture_flags[n]);
            int d;
            for (d=0; d<8; d++) {
               int c = (stepper_description[si][side_to_move] >> (d*4)) & 15;
               bitboard_t<kind> captures = bb;
               /* We have a repetition count, so we do a number of steps one after the other.
                * This can effectively duplicate a slider.
                */
               for ( ; c>0; c--) {
                  captures &= step_mask[d];
                  captures = captures.sshift(step_shift[d]);
                  captures &= piece_types->prison[side_to_move][n];
                  attacked |= captures;
                  captures &= ~occ;
               }
            }
         }

         /* Sliders and leapers */
         if (piece_capture_flags[n] & MF_HOPSLIDELEAP)
         while (!bb.is_empty()) {
            move_flag_t capture_flags = piece_capture_flags[n];
            int from = bb.bitscan();
            bb.reset(from);

            bitboard_t<kind> from_bb = bitboard_t<kind>::square_bitboards[from];

            bitboard_t<kind> attack;

            if (is_leaper(capture_flags)) attack |= generate_leaper_move_bitboard(capture_flags, side_to_move, from, occ) &~ from_bb;
            if (is_slider(capture_flags)) attack |= generate_slider_move_bitboard(capture_flags, side_to_move, from, occ);
            if (is_hopper(capture_flags)) attack |= generate_hopper_move_bitboard(capture_flags, side_to_move, from, occ);
            if (is_rider (capture_flags)) attack |= generate_rider_move_bitboard (capture_flags, side_to_move, from, occ);
            attacked |= attack & piece_types->prison[side_to_move][n];
         }
      }

      return attacked;
   }

   inline bitboard_t<kind> generate_attack_bitboard(const board_t<kind> *board, const bitboard_t<kind> test_squares, const bitboard_t<kind> source_mask, side_t side_to_move) const
   {
      return generate_attack_bitboard_mask(board, test_squares, source_mask, bitboard_t<kind>::board_all, side_to_move);
   }

   inline bitboard_t<kind> generate_move_bitboard_for_flags(move_flag_t flags, int square, const bitboard_t<kind> occupied, side_t side_to_move) const
   {
      bitboard_t<kind> attacked;

      /* Steppers */
      if (is_stepper(flags)) {
         bitboard_t<kind> bb = bitboard_t<kind>::square_bitboards[square];
         int si = get_stepper_index(flags);
         for (int d=0; d<8; d++) {
            int c = (stepper_description[si][side_to_move] >> (d*4)) & 15;
            bitboard_t<kind> captures = bb;
            /* We have a repetition count, so we do a number of steps one after the other.
             * This can effectively duplicate a slider.
             */
            for ( ; c>0; c--) {
               captures &= step_mask[d];
               captures = captures.sshift(step_shift[d]);
               attacked |= captures;
               captures &= ~occupied;
            }
         }
      }

      /* Sliders and leapers */
      if (flags & MF_HOPSLIDELEAP) {
         bitboard_t<kind> from_bb = bitboard_t<kind>::square_bitboards[square];
         if (is_leaper(flags)) attacked |= generate_leaper_move_bitboard(flags, side_to_move, square, occupied) &~ from_bb;
         if (is_slider(flags)) attacked |= generate_slider_move_bitboard(flags, side_to_move, square, occupied);
         if (is_hopper(flags)) attacked |= generate_hopper_move_bitboard(flags, side_to_move, square, occupied);
         if (is_rider (flags)) attacked |= generate_rider_move_bitboard (flags, side_to_move, square, occupied);
      }

      return attacked;
   }

   inline bitboard_t<kind>
   generate_move_bitboard_from_squares_for_flags(move_flag_t flags, bitboard_t<kind> squares, const bitboard_t<kind> occupied, side_t side_to_move) const
   {
      bitboard_t<kind> attacked;
      while (!squares.is_empty()) {
         int square = squares.bitscan();
         squares.reset(square);
         attacked |= generate_move_bitboard_for_flags(flags, square, occupied, side_to_move);
      }
      return attacked;
   }

   /* Generate an attack bitboard for all attackers within a specified mask */
   inline bitboard_t<kind> generate_moves_bitboard(const board_t<kind> *board, bitboard_t<kind> test_squares, bitboard_t<kind> source_mask, side_t side_to_move) const
   {
      piece_description_t<kind> *piece_types;
      move_flag_t *piece_move_flags;
      bitboard_t<kind> own, enemy, own_movers;
      bitboard_t<kind> occupied;
      bitboard_t<kind> attacked;

      piece_types = board->piece_types;

      /* Bookkeeping: we keep a pointer to the next move in the move list, and
       * update the number of moves in the list at the end of this function
       */
      own = board->bbc[side_to_move];
      enemy = board->bbc[next_side[side_to_move]];

      occupied = own | enemy | test_squares;

      own_movers = own & source_mask;

      for (int k = 0; k<2; k++) {
      piece_move_flags = (k == 0) ? piece_types->piece_move_flags
                                  : piece_types->piece_special_move_flags;
      for (int n=0; n<piece_types->num_piece_types; n++) {
         bitboard_t<kind> possible_attackers = own_movers & board->bbp[n];
         if (piece_move_flags[n] == 0) continue;
         if (k == 1) possible_attackers &= piece_types->special_zone[side_to_move][n];
         if (possible_attackers.is_empty()) continue;

         bitboard_t<kind> bb = possible_attackers;

         /* Steppers */
         if (is_stepper(piece_move_flags[n])) {
            int si = get_stepper_index(piece_move_flags[n]);
            int d;
            for (d=0; d<8; d++) {
               int c = (stepper_description[si][side_to_move] >> (d*4)) & 15;
               bitboard_t<kind> captures = bb;
               /* We have a repetition count, so we do a number of steps one after the other.
                * This can effectively duplicate a slider.
                */
               for ( ; c>0; c--) {
                  captures &= step_mask[d];
                  captures = captures.sshift(step_shift[d]);
                  captures &= piece_types->prison[side_to_move][n];
                  attacked |= captures;
                  captures &= ~occupied;
               }
            }
         }

         /* Sliders and leapers */
         if (piece_move_flags[n] & MF_HOPSLIDELEAP)
         while (!bb.is_empty()) {
            move_flag_t capture_flags = piece_move_flags[n];
            int from = bb.bitscan();
            bb.reset(from);

            if (is_leaper(capture_flags)) attacked |= generate_leaper_move_bitboard(capture_flags, side_to_move, from, occupied);
            if (is_slider(capture_flags)) attacked |= generate_slider_move_bitboard(capture_flags, side_to_move, from, occupied);
            if (is_hopper(capture_flags)) attacked |= generate_hopper_move_bitboard(capture_flags, side_to_move, from, occupied);
            if (is_rider (capture_flags)) attacked |= generate_rider_move_bitboard (capture_flags, side_to_move, from, occupied);
         }

         attacked &= piece_types->prison[side_to_move][n];
      }
      }

      return attacked;
   }


   move_flag_t define_slider(const char *movestr) {
      const char *s = movestr;
      move_flag_t flags = 0;

      while (*s && isspace(*s)) s++;

      if (*s == '\0')
         return 0;

      int shift = 0;

      if (strstr(s, "slide") == s)
         shift = 0;

      if (strstr(s, "hop") == s)
         shift = 4;

      while (*s) {
         switch (*s) {
            case 'H':
               flags |= MF_SLIDER_H << shift;
               break;
            case 'V':
               flags |= MF_SLIDER_V << shift;
               break;
            case 'D':
               flags |= MF_SLIDER_D << shift;
               break;
            case 'A':
               flags |= MF_SLIDER_A << shift;
               break;
            case ')':
               break;
            default:
               break;
         }
         s++;
      }

      if (shift == 0)
         super_slider_flags |= flags;
      else
         super_hopper_flags |= flags;

      return flags;
   }


   move_flag_t define_asymmetric_leaper(const char *movestr) {
      int board_files = bitboard_t<kind>::board_files;
      int board_ranks = bitboard_t<kind>::board_ranks;
      int size = board_files * board_ranks;
      bitboard_t<kind> moves[3][NUM_SIDES][sizeof(kind)*8];

      const char *s = movestr;
      char op = ' ';

      int ii = 0;
      int index_flags = 1;
      uint8_t description = 0;

      while (*s && *s != ' ') s++;
      s++;
      while (*s) {
         int n, m;

         while (*s && s[-1] != '(') s++;
         while (*s && *s == '(') s++;
         if(!*s) break;
         sscanf(s, "%d", &n);

         while (*s && s[-1] != ',') s++;
         if(!*s) break;
         sscanf(s, "%d", &m); s++;

         switch (op) {
            case '|':   /* Define a leaper with more than one type of move */
            case ' ':
               break;

            case '+':   /* A compound leaper, with two steps one after the other */
               ii = 1;
               index_flags |= 2;
               printf("Error (Compound asymmetric leapers are not implemented)\n");
               break;

            case '&':   /* A compound leaper, with a mask (used to implement "lame leapers") */
               /* Define a new type of leaper for the mask.
                * FIXME: check if this type was already defined and re-use.
                */
               ii = 2;
               index_flags |= 4;
               printf("Error (lame asymmetric leapers are not implemented)\n");
               break;
         }

         for (int sqr=0; sqr<size; sqr++) {
            moves[ii][WHITE][sqr] |= make_aleaper_bitboard(sqr, n,  m);
            moves[ii][BLACK][sqr] |= make_aleaper_bitboard(sqr, n, -m);
         }

         s++;
         while (*s && *s == ')') s++;
         op = s[0];
      }

      int index[3];
      index[0] = index[1] = index[2] = 0;

      /* Test if this leaper is equivalent to another one */
      for (int ii = 0; ii < 3; ii++) {
         index[ii] = 0;
         if ((index_flags & (1<<ii)) == 0) continue;

         bool exists = false;
         for (int n = 0; n<number_of_aleapers; n++) {
            bool same = true;
            for (int sqr=0; sqr<size && same; sqr++)
               same = same && (aleaper[WHITE][n][sqr] == moves[ii][WHITE][sqr]) && (aleaper[BLACK][n][sqr] == moves[ii][BLACK][sqr]);
            if (same) {
               index[ii] = n;
               exists = true;
               break;
            }
         }

         if (!exists) {
            if (number_of_aleapers >= MAX_LEAPER_TYPES) {
               printf("Error (too many aleapers)\n");
               return 0;
            }
            for (int sqr = 0; sqr < size; sqr++) {
               aleaper[WHITE][number_of_aleapers][sqr] = moves[ii][WHITE][sqr];
               aleaper[BLACK][number_of_aleapers][sqr] = moves[ii][BLACK][sqr];
            }

            index[ii] = number_of_aleapers;
            number_of_aleapers++;
         }
      }

      return (index[0] | (index[1] << 4) | (index[2] << 8) | (index_flags << 12))<<16 | MF_LEAPER_ASYMM | MF_IS_LEAPER;
   }

   move_flag_t define_symmetric_leaper(const char *movestr) {
      int board_files = bitboard_t<kind>::board_files;
      int board_ranks = bitboard_t<kind>::board_ranks;
      int size = board_files * board_ranks;
      bitboard_t<kind> moves[3][sizeof(kind)*8];

      const char *s = movestr;
      char op = ' ';

      int ii = 0;
      int index_flags = 1;
      uint8_t description = 0;

      while (*s && *s != ' ') s++;
      s++;
      while (*s) {
         int n, m;

         while (*s && s[-1] != '(') s++;
         while (*s && *s == '(') s++;
         if(!*s) break;
         sscanf(s, "%d", &n);

         while (*s && s[-1] != ',') s++;
         if(!*s) break;
         sscanf(s, "%d", &m); s++;

         switch (op) {
            case '|':   /* Define a leaper with more than one type of move */
            case ' ':
               break;

            case '+':   /* A compound leaper, with two steps one after the other */
               ii = 1;
               index_flags |= 2;
               break;

            case '&':   /* A compound leaper, with a mask (used to implement "lame leapers") */
               /* Define a new type of leaper for the mask.
                * FIXME: check if this type was already defined and re-use.
                */
               ii = 2;
               index_flags |= 4;
               break;
         }

         for (int sqr=0; sqr<size; sqr++)
            moves[ii][sqr] |= make_leaper_bitboard(sqr, n, m);

         s++;
         while (*s && *s == ')') s++;
         op = s[0];
      }

      int index[3];
      index[0] = index[1] = index[2] = 0;

      /* Test if this leaper is equivalent to another one */
      for (int ii = 0; ii < 3; ii++) {
         index[ii] = 0;
         if ((index_flags & (1<<ii)) == 0) continue;

         bool exists = false;
         for (int n = 0; n<number_of_leapers; n++) {
            bool same = true;
            for (int sqr=0; sqr<size && same; sqr++)
               same = same && (leaper[n][sqr] == moves[ii][sqr]);
            if (same) {
               index[ii] = n;
               exists = true;
               break;
            }
         }

         if (!exists) {
            if (number_of_leapers >= MAX_LEAPER_TYPES) {
               printf("Error (too many leapers)\n");
               return 0;
            }
            for (int sqr = 0; sqr < size; sqr++)
               leaper[number_of_leapers][sqr] = moves[ii][sqr];

            index[ii] = number_of_leapers;
            number_of_leapers++;
         }
      }

      move_flag_t move_flags = (index[0] | (index[1] << 4) | (index[2] << 8) | (index_flags << 12))<<16 | MF_IS_LEAPER;

      /* Define double-step leapers, for super leaper */
      if (index_flags == 3) {
         for (int sqr = 0; sqr < size; sqr++) {
            bitboard_t<kind> from_bb = bitboard_t<kind>::square_bitboards[sqr];

            leaper[number_of_leapers][sqr] = generate_leaper_move_bitboard(move_flags, WHITE, sqr, from_bb) &~ from_bb;
         }
         number_of_leapers++;
      }

      return move_flags;
   }


#define update_leaper_bb(bb,n,m)                                  \
   if ( (x+n) >= 0 && (y+m) >= 0 &&                               \
        (x+n) < board_files && (y+m) < board_ranks) {             \
      int dest_sqr = bitboard_t<kind>::pack_rank_file(y+m, x+n);  \
      bb.set(dest_sqr);                                           \
   }
   inline bitboard_t<kind> make_aleaper_bitboard(int sqr, int n, int m) const
   {
      int board_files = bitboard_t<kind>::board_files;
      int board_ranks = bitboard_t<kind>::board_ranks;

      int x = unpack_file(sqr);
      int y = unpack_rank(sqr);

      bitboard_t<kind> leaper;

      update_leaper_bb(leaper, n, m);

      return leaper;
   }


   inline bitboard_t<kind> make_leaper_bitboard(int sqr, int n, int m) const
   {
      int board_files = bitboard_t<kind>::board_files;
      int board_ranks = bitboard_t<kind>::board_ranks;

      int x = unpack_file(sqr);
      int y = unpack_rank(sqr);

      bitboard_t<kind> leaper;

      update_leaper_bb(leaper, n, m);
      update_leaper_bb(leaper, n,-m);
      update_leaper_bb(leaper,-n, m);
      update_leaper_bb(leaper,-n,-m);

      update_leaper_bb(leaper, m, n);
      update_leaper_bb(leaper,-m, n);
      update_leaper_bb(leaper, m,-n);
      update_leaper_bb(leaper,-m,-n);

      return leaper;
   }
#undef update_leaper_bb

   move_flag_t define_rider(const char *movestr) {
      const char *s = movestr;
      uint32_t wstep = 0;
      uint32_t bstep = 0;

      /* Dimensions of the board */
      int w = bitboard_t<kind>::board_files;
      int h = bitboard_t<kind>::board_ranks;
      int board_size = w*h;

      if (!movestr)
         return 0;

      while (*s && isspace(*s)) s++;

      if (*s == '\0')
         return 0;

      if (strstr(s, "ride ") != s)
         return 0;
      s+=5;

      int index = number_of_riders;

      for (int n=0; n<4; n++)
         rider_step[index][n].dx = rider_step[index][n].dy = 0;

      for (int from = 0; from<board_size; from++)
         for (int to = 0; to<board_size; to++)
            rider_ray[index][from][to].clear();

      move_flag_t flags = 0;

      for (int k=0; s && *s && k<4; k++) {
         int dx = -128;
         int dy = -128;
         if (sscanf(s, "(%d, %d)", &dx, &dy) != 2)
            break;

         s = strstr(s, "|");
         if (s) s++;
         dx = abs(dx);
         dy = abs(dy);

         /* Minor optimisation: W and F riders are sliders */
         if ( (dx+dy)==1 ) {
            flags |= define_slider("slide (H,V)");
            k--;
            continue;
         }

         if ( (dx*dy)==1 ) {
            flags |= define_slider("slide (A,D)");
            k--;
            continue;
         }

         if (k == 0) {
            flags |= make_rider_index(index);
            number_of_riders++;
         }

         rider_step[index][k].dx = dx;
         rider_step[index][k].dy = dy;

         for (int from = 0; from<board_size; from++) {
            int file = unpack_file(from);
            int rank = unpack_rank(from);
            int sx = rider_step[index][k].dx;
            int sy = rider_step[index][k].dy;

            int dx[8] = {  sx,  sx, -sx, -sx,  sy,  sy, -sy, -sy };
            int dy[8] = {  sy, -sy,  sy, -sy,  sx, -sx,  sx, -sx };

            for (int n = 0; n<8; n++) {
               int old_to = bitboard_t<kind>::pack_rank_file(rank, file);
               int f = file + dx[n];
               int r = rank + dy[n];
               int to = bitboard_t<kind>::pack_rank_file(r, f);
               while (f>=0 && r>=0 && f<bitboard_t<kind>::board_files && r < bitboard_t<kind>::board_ranks) {
                  rider_ray[index][from][to] = rider_ray[index][from][old_to];
                  if (old_to != from) rider_ray[index][from][to].set(old_to);
                  f += dx[n];
                  r += dy[n];
                  old_to = to;
                  to = bitboard_t<kind>::pack_rank_file(r, f);
               }
            }
         }
      }

      return flags;
   }

   move_flag_t define_stepper(const char *movestr) {
      const char *s = movestr;
      uint32_t wstep = 0;
      uint32_t bstep = 0;

      /* Dimensions of the board */
      int w = bitboard_t<kind>::board_files;
      int h = bitboard_t<kind>::board_ranks;

      if (!movestr)
         return 0;

      while (*s && isspace(*s)) s++;

      if (*s == '\0')
         return 0;

      if (strstr(s, "step ") != s)
         return 0;
      s+=5;

      while (*s) {
         int count = 1;
         int shift = 0;

         if (isdigit(*s)) {
            sscanf(s, "%d", &count);
            assert(count < 16);
            assert(count >= 0);
            s++;
         }

         if (strstr(s, "NE") == s) shift = 1;
         else if (strstr(s, "NW") == s) shift = 7;
         else if (strstr(s, "SE") == s) shift = 3;
         else if (strstr(s, "SW") == s) shift = 5;
         else if (strstr(s, "N") == s)  shift = 0;
         else if (strstr(s, "E") == s)  shift = 2;
         else if (strstr(s, "S") == s)  shift = 4;
         else if (strstr(s, "W") == s)  shift = 6;

         wstep |= count << (4*shift);
         bstep |= count << (4*inverse_step[shift]);
         
         while(*s && *s != ',')
            s++;
         if (*s) s++;
         while (*s && isspace(*s)) s++;
      }

      for (int n = 1; n<number_of_steppers; n++) {
         if (stepper_description[n][WHITE] == wstep) {
            return make_stepper_index(n);
         }
      }
      if (number_of_steppers >= MAX_STEPPER_TYPES) {
         return 0;
      }
      int index = number_of_steppers;
      number_of_steppers++;
      stepper_description[index][WHITE] = wstep;
      stepper_description[index][BLACK] = bstep;

      return make_stepper_index(index);
   }

   /* TODO: Betza/Mueller notation for piece movement describes moves and
    * captures in one go, not individually.
    */
   move_flag_t define_betza(const char *movestr) {
      const char *s = movestr;
      move_flag_t flags = 0;

      while (*s) {
         const char *atom = s;
         /* Find first upper-case character */
         while (*atom && (islower(*atom) || isspace(*atom))) atom++;

         if (atom != s) {
            /* TODO: modifiers */
         }
         /* Repeated atoms = riders.
          * The only ones we implement are WW and FF
          */
         char a = atom[0];
         if (atom[1] == atom[0] || atom[1] == '0') {
            switch (*atom) {
               case 'W':
                  a = 'R';
                  break;

               case 'F':
                  a = 'B';
                  break;

               default:
                  return 0;
            }
            atom++;
         }
         switch (a) {
            case 'K':   /* King = FW */
               flags |= define_piece_move("leap (1,0)|(1,1)");
               break;

            case 'Q':   /* Queen = RB */
               flags |= define_piece_move("slide (H,V,A,D)");
               break;

            case 'R':   /* Rook = WW */
               flags |= define_piece_move("slide (H,V)");
               break;

            case 'B':   /* Bishop = FF */
               flags |= define_piece_move("slide (A,D)");
               break;

            case ' ':
            case 'O':   /* No move, or castling */
               break;

            case 'W':   /* Wazir = (1,0) */
               flags |= define_piece_move("leap (1,0)");
               break;

            case 'F':   /* Ferz = (1,1) */
               flags |= define_piece_move("leap (1,1)");
               break;

            case 'D':   /* Dabbabah = (2,0) */
               flags |= define_piece_move("leap (2,0)");
               break;

            case 'N':   /* Knight = (2,1) */
               flags |= define_piece_move("leap (2,1)");
               break;

            case 'A':   /* Alfil = (2, 2) */
               flags |= define_piece_move("leap (2,2)");
               break;

            case 'H':   /* Threeleaper = (3, 0) */
               flags |= define_piece_move("leap (3,0)");
               break;

            case 'C':   /* Camel = (3, 1) */
            case 'L':
               flags |= define_piece_move("leap (3,1)");
               break;

            case 'Z':   /* Zebra = (3, 2) */
            case 'J':
               flags |= define_piece_move("leap (3,2)");
               break;

            case 'G':   /* (3, 3) leaper */
               flags |= define_piece_move("leap (3,3)");
               break;

            default:
               return 0;
         }
         s = atom + 1;
      }

      return flags;
   }

   move_flag_t define_piece_move(const char *movestr) {
      if (!movestr) return 0;
      const char *s = movestr;
      while (isspace(*s)) s++;
      if (s[0] == '\0') return 0;

      /* What type of mover is this? */
      if (strstr(s, "none ") == s)
         return 0;
      if (strstr(s, "slide ") == s)
         return define_slider(s);
      if (strstr(s, "hop ") == s)
         return define_slider(s);
      if (strstr(s, "step ") == s)
         return define_stepper(s);
      if (strstr(s, "aleap ") == s)
         return define_asymmetric_leaper(s);
      if (strstr(s, "leap ") == s)
         return define_symmetric_leaper(s);
      if (strstr(s, "ride ") == s)
         return define_rider(s);

      /* TODO: try to interpret a Betza-like move description and translate
       * it to what is used internally.
       */
      return 0;
   }

   void clear_castle_rule(int idx, side_t side)
   {
      castle_mask[idx][side].clear();
      castle_free[idx][side].clear();
      castle_safe[idx][side].clear();
      castle_king_from[idx][side] = -1;
      castle_king_dest[idx][side].clear();
      castle_rook_dest[idx][side].clear();
   }

   /* Deduce castle flags from king positions and destinations and rook locations. */
   int deduce_castle_flags(side_t side, int king_from, int king_to, int rook_from)
   {
      /* King-side or queen side? */
      bool king_side = (unpack_file(king_to) >= bitboard_t<kind>::board_files/2);
      bitboard_t<kind> mask, free, safe;
      int c, c_first, c_last;
      int delta = 0;

      if (unpack_rank(king_from) == unpack_rank(rook_from)) delta = 1;
      if (unpack_file(king_from) == unpack_file(rook_from)) delta = bitboard_t<kind>::board_files;
      if (bitboard_t<kind>::diagonal_nr[king_from] == bitboard_t<kind>::diagonal_nr[rook_from]) delta = bitboard_t<kind>::board_files+1;
      if (bitboard_t<kind>::anti_diagonal_nr[king_from] == bitboard_t<kind>::anti_diagonal_nr[rook_from]) delta = bitboard_t<kind>::board_files-1;
      if (delta == 0) return -1;

      int rook_to = king_side ? (king_to - delta) : (king_to + delta);

      /* It is not enough that the king and rook have a clear path
       * between them: the path to the destination squares needs to be cleared
       * as well.
       * This is implied in normal chess, but not in FRC.
       */
      mask = bitboard_t<kind>::square_bitboards[king_from] | bitboard_t<kind>::square_bitboards[rook_from];
      free.clear();

      /* The path of the King */
      c_first = std::min(king_from, king_to);
      c_last  = std::max(king_from, king_to);
      for (c = c_first; c <= c_last; c+=delta)
         free.set(c);
      safe = free;

      /* The path of the Rook */
      c_first = std::min(rook_to, rook_from);
      c_last  = std::max(rook_to, rook_from);
      for (c = c_first; c <= c_last; c+=delta)
         free.set(c);

      /* Make sure the king and rook are not marked on the "free" bitboard.
       * Makes no difference for normal chess, but does affect FRC.
       */
      free &= ~mask;

      mask &= bitboard_t<kind>::board_all;
      free &= bitboard_t<kind>::board_all;
      safe &= bitboard_t<kind>::board_all;

      int idx = king_side ? SHORT : LONG;

      castle_mask[idx][side] |= mask;
      castle_free[idx][side] |= free;
      castle_safe[idx][side] |= safe;
      castle_king_from[idx][side] = king_from;
      castle_king_dest[idx][side].set(king_to);
      castle_rook_dest[idx][side].set(rook_to);
      return idx;
   }

   bitboard_t<kind> get_all_attackers(const board_t<kind> *board, bitboard_t<kind> mask, int square) const
   {
      bitboard_t<kind> occupied           = board->get_occupied() & mask;
      bitboard_t<kind> possible_attackers = occupied & super[square];
      occupied.set(square);

      bitboard_t<kind> attacked;
      bitboard_t<kind> attacker;

      for (int n=0; n<board->piece_types->num_piece_types && !possible_attackers.is_empty(); n++) {
         move_flag_t capture_flags = board->piece_types->piece_capture_flags[n];
         for (side_t side = WHITE; side<=BLACK; side++) {
            bitboard_t<kind> bb = possible_attackers & board->bbp[n] & board->bbc[side];

            if (bb.is_empty()) continue;
            possible_attackers ^= bb;

            /* Steppers */
            if (is_stepper(capture_flags)) {
               int si = get_stepper_index(board->piece_types->piece_capture_flags[n]);
               int d;
               for (d=0; d<8; d++) {
                  int max_c = (stepper_description[si][side] >> (d*4)) & 15;
                  bitboard_t<kind> captures = bb & super_stepper[square];
                  /* We have a repetition count, so we do a number of steps one after the other.
                   * This can effectively duplicate a slider.
                   */
                  for (int c = 1; c<=max_c && !captures.is_empty(); c++) {
                     captures &= step_mask[d];
                     captures = captures.sshift(step_shift[d]);
                     captures &= board->piece_types->prison[side][n];
                     if (captures.test(square)) {
                        attacker.set(square - c*step_shift[d]);
                        break;
                     }
                     captures &= ~occupied;
                  }
               }
            }

            /* Sliders and leapers */
            if (is_leaper(capture_flags)) {
               bitboard_t<kind> bp = bb;
               while (!(bp & super_leaper[square]).is_empty()) {
                  int s = (bp & super_leaper[square]).bitscan();
                  bp.reset(s);
                  attacked = generate_leaper_move_bitboard(capture_flags, side, s, occupied);
                  attacked &= board->piece_types->prison[side][n];

                  if (attacked.test(square)) {
                     attacker.set(s);
                     bb.reset(s);
                  }
               }
            }

            if (is_rider(capture_flags)) {
               bitboard_t<kind> bp = bb;
               while (!(bp & super_rider[square]).is_empty()) {
                  int s = (bp & super_rider[square]).bitscan();
                  bp.reset(s);
                  attacked = generate_rider_move_bitboard(capture_flags, side, s, occupied);
                  attacked &= board->piece_types->prison[side][n];

                  if (attacked.test(square)) {
                     attacker.set(s);
                     bb.reset(s);
                  }
               }
            }
#if 0
            if (is_slider(capture_flags)) {
               bitboard_t<kind> bp = bb;
               while (!(bp & super_slider[square]).is_empty()) {
                  int s = (bp & super_slider[square]).bitscan();
                  bp.reset(s);
                  attacked = generate_slider_move_bitboard(capture_flags, side, s, occupied);
                  attacked &= board->piece_types->prison[side][n];

                  if (attacked.test(square)) {
                     attacker.set(s);
                     bb.reset(s);
                  }
               }
            }
#endif
            if (is_hopper(capture_flags)) {
               bitboard_t<kind> bp = bb;
               while (!(bp & super_hopper[square]).is_empty()) {
                  int s = (bp & super_hopper[square]).bitscan();
                  bp.reset(s);
                  attacked = generate_hopper_move_bitboard(capture_flags, side, s, occupied);
                  attacked &= board->piece_types->prison[side][n];

                  if (attacked.test(square)) {
                     attacker.set(s);
                     bb.reset(s);
                  }
               }
            }
         }
      }

      /* Find sliders */
      move_flag_t cf[] = { MF_SLIDER_V, MF_SLIDER_H, MF_SLIDER_D, MF_SLIDER_A };
      int imax = sizeof cf / sizeof *cf;
      for (int i=0; i<imax; i++) {
         bitboard_t<kind> sliders;
         for (int piece = 0; piece<board->piece_types->num_piece_types; piece++) {
            if (board->piece_types->piece_capture_flags[piece] & cf[i])
               sliders |= board->bbp[piece];
         }

         sliders &= super_slider[square] & mask;
         if (sliders.is_empty()) continue;

         attacker |= sliders & generate_slider_move_bitboard(cf[i], WHITE, square, occupied);
      }

      return attacker;
   }

   bool player_in_check(const board_t<kind> *board, side_t side) const
   {
      bitboard_t<kind> royal = board->royal & board->bbc[side];
      bitboard_t<kind> empty;

      /* If there are no royal pieces, then there is no check */
      if (royal.is_empty()) return false;

      /* If there is more than one king, we can never be in check - unless
       * the rules say we are when all kings are under attack.
       */
      if (!royal.onebit() && !(board->rule_flags & (RF_KING_DUPLECHECK|RF_CHECK_ANY_KING))) return false;

      move_flag_t *capture_flags = board->piece_types->piece_capture_flags;
      bitboard_t<kind> sup[4];
      bitboard_t<kind> mask[4];
      bitboard_t<kind> kmask;

      bitboard_t<kind> bb = royal;
      while (!bb.is_empty()) {
         int square = bb.bitscan();
         bb.reset(square);
         sup[0] |= super_slider[square];
         sup[1] |= super_leaper[square] | super_rider[square];
         sup[2] |= super_stepper[square];
         sup[3] |= super_hopper[square];
         kmask   = super[square] & board->bbc[next_side[side]];
      }
      if (royal.onebit() && kmask.is_empty()) return false;
      for (int n=0; n<4; n++)
         sup[n] &= board->bbc[next_side[side]];
      for (int n = 0; n<board->piece_types->num_piece_types; n++) {
         if (is_slider(capture_flags[n]))  mask[0] |= sup[0] & board->bbp[n];
         if (is_leaper(capture_flags[n]) || is_rider(capture_flags[n]))  mask[1] |= sup[1] & board->bbp[n];
         if (is_stepper(capture_flags[n])) mask[2] |= sup[2] & board->bbp[n];
         if (is_hopper(capture_flags[n]))  mask[3] |= sup[3] & board->bbp[n];
      }

      /* Mask out pieces that occur in more than one mask: we only need to
       * test them once, afterall.
       */
      mask[3] &= ~(mask[0] | mask[1] | mask[2]);
      mask[2] &= ~(mask[0] | mask[1]);
      mask[1] &= ~(mask[0]);

      bitboard_t<kind> attacked_squares;
      
      /* TODO: we can do one better, at least for sliders and normal
       * leapers (lame leapers are more tricky): when generating the attack
       * bitboard, first generate appropriate attacks from the target
       * square and intersect with the piece type. This allows us to test
       * against all pieces of the particular type in one go, and we avoid
       * some possible false positives.
       */
      for (int n=0; n<4; n++) {
         if (mask[n].is_empty()) continue;
         attacked_squares |= generate_attack_bitboard(board, empty, mask[n], next_side[side]);
         if ((attacked_squares & royal) == royal) return true;
      }

      if (expect(board->rule_flags & RF_KING_TABOO, false)) {
         bitboard_t<kind> other_king = board->royal & kmask;
         if (!other_king.is_empty()) {
            int square = other_king.bitscan();
            bitboard_t<kind> occ = board->get_occupied();

            attacked_squares |= generate_slider_move_bitboard(MF_SLIDER_V, next_side[side], square, occ);

         }
         return (attacked_squares & royal) == royal;
      }
      if (board->rule_flags & RF_CHECK_ANY_KING)
         return !(attacked_squares & royal).is_empty();
      return false;
   }

   bool was_checking_move(board_t<kind> *board, side_t side, move_t lastmove) const
   {
      side_t oside = next_side[side];
      bitboard_t<kind> royal = board->royal & board->bbc[side];
      bitboard_t<kind> empty;

      if (royal.is_empty()) return false;

      if (royal.onebit()) {
         bitboard_t<kind> move_bb;
         int king = royal.bitscan();
         int to   = get_move_to(lastmove);
         assert(to   < 8*sizeof(kind));
         move_bb.set(to);

         if (!is_drop_move(lastmove)) {
            int from = get_move_from(lastmove);
            assert(from < 8*sizeof(kind));
            move_bb.set(from);
         }

         if (is_castle_move(lastmove)) {
            int from = get_castle_move_from2(lastmove);
            int to   = get_castle_move_to2(lastmove);
            assert(from < 8*sizeof(kind));
            assert(to   < 8*sizeof(kind));

            move_bb.set(from);
            move_bb.set(to);
         }

         if (is_capture_move(lastmove)) {
            int square = get_move_capture_square(lastmove);
            assert(square < 8*sizeof(kind));
            move_bb.set(square);
         }

         if ((super[king] & move_bb).is_empty()) {
            assert(!player_in_check(board, side));
            return false;
         } else {
            bitboard_t<kind> mask;
            // FIXME: the following test can be excluded for games without
            // hoppers, where moving to the same ray as the king either
            // blocks the ray (harmless, or it would already have been
            // check) or is a direct attack. However, if hoppers are in the
            // game, then it's possible to add an attacker.
            //move_bb.reset(to);
            if (is_castle_move(lastmove)) {
               mask.set(get_castle_move_to2(lastmove));
               move_bb.reset(get_castle_move_to2(lastmove));
            }

            int king_file = unpack_file(king);
            int king_rank = unpack_rank(king);
            int king_diag = bitboard_t<kind>::diagonal_nr[king];
            int king_anti = bitboard_t<kind>::anti_diagonal_nr[king];
            while (!move_bb.is_empty()) {
               int square = move_bb.bitscan();
               int file = unpack_file(square);
               int rank = unpack_rank(square);
               int diag = bitboard_t<kind>::diagonal_nr[square];
               int anti = bitboard_t<kind>::anti_diagonal_nr[square];
               move_bb.reset(square);

               if (file == king_file) mask |= bitboard_t<kind>::board_file[file];
               if (rank == king_rank) mask |= bitboard_t<kind>::board_rank[rank];
               if (diag == king_diag) mask |= bitboard_t<kind>::board_diagonal[diag];
               if (anti == king_anti) mask |= bitboard_t<kind>::board_antidiagonal[anti];
            }

            //bitboard_t<kind> sliders;
            //for (int n = 0; n<board->piece_types->num_piece_types; n++) {
            //   if (board->piece_types->piece_capture_flags[n] & (MF_HOPSLIDE|MF_STEPPER))
            //      sliders |= board->bbp[n];
            //}
            //mask &= sliders;
            mask.set(to);

            /* Possible lame leapers */
            mask |= super_leaper[king] & board->bbc[oside];

            bitboard_t<kind> attacked_squares = generate_attack_bitboard(board, empty, mask, oside);
            if (attacked_squares.test(king)) return true;

            assert(!player_in_check(board, side));
            return false;
         }
      }

      return player_in_check(board, side);
   }

   bitboard_t<kind> get_pinned_pieces(const board_t<kind> *board, side_t side) const
   {
      bitboard_t<kind> royal = board->royal & board->bbc[side];
      bitboard_t<kind> pinned;
      bitboard_t<kind> potential_pins;

      /* If there is more than one king, or no king at all - ignore pins.
       */
      if (!royal.onebit()) return pinned;

      int king = royal.bitscan();
      potential_pins = board->bbc[side] & super[king];

      for (int n = 0; n<board->piece_types->num_piece_types; n++) {
         bitboard_t<kind> atk = board->bbp[n] & board->bbc[next_side[side]] & super[king];
         move_flag_t atk_flags = board->piece_types->piece_capture_flags[n];

         while(!atk.is_empty()) {
            int attacker = atk.bitscan();
            atk.reset(attacker);

            /* Sliders */
            if (is_slider(atk_flags)) {
               bitboard_t<kind> occ = board->get_occupied();
               bitboard_t<kind> bb = occ & bitboard_t<kind>::board_between[king][attacker];
               bb &= generate_slider_move_bitboard(atk_flags, next_side[side], attacker, occ & ~bb);

               if (bb.onebit()) pinned |= bb&potential_pins;
            }

            /* Hoppers */
            if (is_hopper(atk_flags)) {
               bitboard_t<kind> occ = board->get_occupied();
               bitboard_t<kind> bb = occ & bitboard_t<kind>::board_between[king][attacker];
               bb &= generate_slider_move_bitboard(atk_flags>>4, next_side[side], attacker, occ & ~bb);

               if (bb.twobit()) pinned |= bb&potential_pins;
            }

            /* Riders */
            if (is_rider(atk_flags)) {
               int index = get_rider_index(atk_flags);
               bitboard_t<kind> occ = board->get_occupied();
               bitboard_t<kind> bb = occ & rider_ray[index][king][attacker];
               bb &= generate_rider_move_bitboard(atk_flags>>4, next_side[side], attacker, occ & ~bb);

               if (bb.twobit()) pinned |= bb&potential_pins;
            }

            /* TODO: multi-steppers */

            /* Lame leapers */
            if (is_leaper(atk_flags) && is_masked_leaper(atk_flags) && is_double_leaper(atk_flags)) {
               bitboard_t<kind> occ = board->get_occupied();
               bitboard_t<kind> atk = generate_leaper_move_bitboard(atk_flags, next_side[side], attacker, occ);

               if (!atk.test(king)) {
                  int index = get_leaper_index(atk_flags);
                  potential_pins &= leaper[index][attacker];
                  while (!potential_pins.is_empty()) {
                     int square = potential_pins.bitscan();
                     potential_pins.reset(square);

                     occ.reset(square);
                     atk = generate_leaper_move_bitboard(atk_flags, next_side[side], attacker, occ);
                     if (atk.test(king)) {
                        pinned.set(square);
                        break;
                     }
                  }
               }
            }
         }
      }

      return pinned;
   }


   template<bool special>
   void generate_stepper_moves_mask_for_piece(movelist_t *movelist, const board_t<kind> *board,
      int piece, move_flag_t move_flags, piece_flag_t piece_flags, piece_description_t<kind> *piece_types,
      bitboard_t<kind> from_bb, bitboard_t<kind> destination_mask, bitboard_t<kind> occupied,
      promotion_zone_t<kind> *promotion, bitboard_t<kind> promotion_zone, bitboard_t<kind> optional_promotion_zone,
      side_t side, piece_bit_t allowed_promotion_pieces) const
   {
      move_t move;

      /* Check for stepper moves, which are generated in parallel */
      if (!is_stepper(move_flags) || from_bb.is_empty()) return;

      int si = get_stepper_index(move_flags);
      for (int d=0; d<8; d++) {   /* Loop over all directions */
         int max_c = (stepper_description[si][side] >> (d*4)) & 15;
         bitboard_t<kind> moves = from_bb;
         /* We have a repetition count, so we do a number of steps one after the other.
          * This can effectively duplicate a slider.
          */
         for (int c = 1; c<=max_c; c++) {
            moves &= step_mask[d];
            moves = moves.sshift(step_shift[d]);
            moves &= ~occupied;
            moves &= piece_types->prison[side][neutral_piece(piece)];

            /* Scan all bits */
            bitboard_t<kind> bb = moves & destination_mask;
            while (!bb.is_empty()) {
               int to = bb.bitscan();
               int from = to - c*step_shift[d];
               bb.reset(to);

               /* Check for promotions
                * When the piece moves into the promotion zone, it will get promoted to one of the allowed
                * promotion pieces, which can be different for each piece type (and further restricted, for
                * instance during Q-search).
                * Promotion to a royal piece is only allowed if the number of royal pieces a player has is
                * smaller than the maximum number of royal pieces.
                */
               bool norm = true;
               bool opt  = false;
               bool comp = false;
               bitboard_t<kind> mask, wild;

               if ((piece_flags & PF_PROMOTEWILD) && (board->bbc[side]&board->bbp[neutral_piece(piece)]).onebit())
                  wild = bitboard_t<kind>::board_all;

               mask.clear();
               if (!(board->rule_flags & RF_PROMOTE_BY_MOVE)) {
                  mask.set(to);
                  mask.set(from);
                  mask &= piece_types->entry_promotion_zone[side][neutral_piece(piece)];
                  mask |= wild;
               }

               piece_bit_t ptried = 0;
               if (!(mask & promotion_zone).is_empty())
               for (int k=0; k<MAX_PZ && promotion[k].choice; k++) {
                  bitboard_t<kind> pz = (promotion[k].zone[side] & mask) | wild;
                  if (pz.test(to) || (pz.test(from) && !(board->rule_flags & RF_PROMOTE_IN_PLACE))) {
                     piece_bit_t c = promotion[k].choice & allowed_promotion_pieces & ~ptried;
                     ptried |= promotion[k].choice;
                     while (c) {
                        int tpiece = bitscan32(c);
                        c ^= 1<<tpiece;
                        if (piece_types->piece_maximum[tpiece][side] == 128 ||
                            board->piece_count(tpiece, side) < piece_types->piece_maximum[tpiece][side]) {
                           tpiece = piece_for_side(tpiece, side);
                           move = encode_normal_promotion(piece, from, to, tpiece);
                           movelist->push(move);
                        }
                     }

                     /* If promotions are optional, we also encode a normal move */
                     //if (!(pz & optional_promotion_zone).is_empty())
                     if (optional_promotion_zone.test(to) || (optional_promotion_zone.test(from) && !promotion_zone.test(to) ) || wild.test(from))
                        opt = true;
                     else if (promotion[k].zone[side].test(to))
                        comp = true;

                     norm = false;
                  }
               }
               opt = opt && !comp;
               if (norm || opt) {
                  move = encode_normal_move(piece, from, to);
                  if (special && c>1 && piece_flags & PF_SET_EP)
                     move |= MOVE_SET_ENPASSANT;
                  if (piece_flags & PF_NORET)
                     move |= MOVE_RESET50;
                  movelist->push(move);
               }
            }
         }
      }
   }

   template<bool capture_to_holdings, bool capture_victim_sideeffect >
   void generate_stepper_captures_mask_for_piece(movelist_t *movelist, const board_t<kind> *board,
      int piece, move_flag_t move_flags, piece_flag_t piece_flags, piece_description_t<kind> *piece_types,
      bitboard_t<kind> from_bb, bitboard_t<kind> destination_mask, bitboard_t<kind> occupied,
      bitboard_t<kind> enemy, bitboard_t<kind> ep_dest,
      promotion_zone_t<kind> *promotion, bitboard_t<kind> promotion_zone, bitboard_t<kind> optional_promotion_zone,
      side_t side, piece_bit_t allowed_promotion_pieces) const
   {
      move_t move;

      /* Check for stepper moves, which are generated in parallel */
      if (is_stepper(move_flags)) {
         int si = get_stepper_index(move_flags);
         for (int d=0; d<8; d++) {   /* Loop over all directions */
            int max_c = (stepper_description[si][side] >> (d*4)) & 15;
            bitboard_t<kind> captures = from_bb;
            /* We have a repetition count, so we do a number of steps one after the other.
             * This can effectively duplicate a slider.
             */
            for (int c = 1; c<=max_c; c++) {
               captures &= step_mask[d];
               captures = captures.sshift(step_shift[d]);
               captures &= piece_types->prison[side][neutral_piece(piece)];

               /* Scan all bits */
               bitboard_t<kind> bb = captures & (enemy | ep_dest) & destination_mask;
               while (!bb.is_empty()) {
                  int to = bb.bitscan();
                  int from = to - c*step_shift[d];
                  int idtaken = board->get_piece(to);
                  bb.reset(to);
                  if (ep_dest.test(to)) idtaken = board->get_piece(board->ep_victim);

                  if ( (piece_types->piece_allowed_victims[neutral_piece(piece)] & (1 << idtaken)) == 0 )
                     continue;

                  /* Check for promotions
                   * When the piece moves into the promotion zone, it will get promoted to one of the allowed
                   * promotion pieces, which can be different for each piece type (and further restricted, for
                   * instance during Q-search).
                   * Promotion to a royal piece is only allowed if the number of royal pieces a player has is
                   * smaller than the maximum number of royal pieces.
                   */
                  bool norm = true;
                  bool opt  = false;
                  bool comp = false;
                  bitboard_t<kind> mask, wild;

                  if ((piece_flags & PF_PROMOTEWILD) && (board->bbc[side]&board->bbp[neutral_piece(piece)]).onebit())
                     wild = bitboard_t<kind>::board_all;

                  mask.clear();
                  if (!(board->rule_flags & RF_PROMOTE_BY_MOVE)) {
                     mask.set(to);
                     mask.set(from);
                     mask &= piece_types->entry_promotion_zone[side][neutral_piece(piece)];
                  }

                  piece_bit_t ptried = 0;
                  if (!(mask & promotion_zone).is_empty() && !(board->rule_flags & RF_QUIET_PROMOTION))
                  for (int k=0; k<MAX_PZ && promotion[k].choice; k++) {
                     bitboard_t<kind> pz = (promotion[k].zone[side] & mask) | wild;
                     if (pz.test(to) || (pz.test(from) && !(board->rule_flags & RF_PROMOTE_IN_PLACE))) {
                        piece_bit_t c = promotion[k].choice & allowed_promotion_pieces & ~ptried;
                        ptried |= promotion[k].choice;
                        if (piece_types->piece_flags[idtaken] & PF_ASSIMILATE) {
                           c = (1<<idtaken) & ~ptried;
                           ptried |= c;
                        }
                        while (c) {
                           int tpiece = bitscan32(c);
                           c ^= 1<<tpiece;
                           if (piece_types->piece_maximum[tpiece][side] == 128 ||
                               board->piece_count(tpiece, side) < piece_types->piece_maximum[tpiece][side]) {
                              tpiece = piece_for_side(tpiece, side);
                              move = encode_capture_promotion(piece, from, to, tpiece);
                              if (capture_to_holdings) {
                                 int victim = to;
                                 if (ep_dest.test(to)) victim = board->ep_victim;
                                 int pstore = piece_types->demotion[board->get_piece(victim)];
                                 side_t store_side = NONE;
                                 if (board->rule_flags & RF_KEEP_CAPTURE)   store_side = side;
                                 if (board->rule_flags & RF_RETURN_CAPTURE) store_side = next_side[side];
                                 assert(store_side != NONE);
                                 move = add_move_store(move, piece_for_side(pstore, store_side), 1);
                              }
                              movelist->push(move);
                           }
                        }

                        /* If promotions are optional, we also encode a normal move */
                        //if (!(pz & optional_promotion_zone).is_empty())
                        if (optional_promotion_zone.test(to) || (optional_promotion_zone.test(from) && !promotion_zone.test(to) ) || wild.test(from))
                           opt = true;
                        else if (promotion[k].zone[side].test(to))
                           comp = true;

                        norm = false;
                     }
                  }
                  opt = opt && !comp;
                  if (norm || opt) {
                     if (ep_dest.test(to)) {
                        move = encode_en_passant_capture(piece, from, to, board->ep_victim);
                     } else {
                        if (piece_types->piece_flags[idtaken] & PF_ASSIMILATE) {
                           int tpiece = piece_for_side(idtaken, side);
                           move = encode_capture_promotion(piece, from, to, tpiece);
                        } else {
                           move = encode_normal_capture(piece, from, to);
                        }
                     }
                     if (capture_to_holdings) {
                        int pstore = piece_types->demotion[idtaken];
                        side_t store_side = NONE;
                        if (board->rule_flags & RF_KEEP_CAPTURE)   store_side = side;
                        if (board->rule_flags & RF_RETURN_CAPTURE) store_side = next_side[side];
                        assert(store_side != NONE);
                        move = add_move_store(move, piece_for_side(pstore, store_side), 1);
                     }
                     movelist->push(move);
                  }
               }
               captures &= ~occupied;
            }
         }
      }
   }

   void generate_double_moves(movelist_t *movelist, move_flag_t move_flags, bitboard_t<kind> destination_mask, side_t side_to_move, bitboard_t<kind> occupied, int piece, int from) const
   {
      move_flag_t cf1 = move_flags & ( MF_LEAPER | MF_IS_LEAPER | MF_LEAPER_ASYMM );
      move_flag_t cf2 = move_flags & (             MF_IS_LEAPER | MF_LEAPER_ASYMM );
      cf2 |= get_leaper_index2(move_flags) << 16;

      bitboard_t<kind> moves1 = generate_leaper_move_bitboard(cf1, side_to_move, from, occupied);
      bitboard_t<kind> moves2, bb;
      bb = moves1;
      while (!bb.is_empty()) {
         int to = bb.bitscan();
         bb.reset(to);
         moves2 |= generate_leaper_move_bitboard(cf2, side_to_move, to, occupied);
      }
      if (!(moves1 & ~occupied).is_empty()) bb = moves2 & bitboard_t<kind>::square_bitboards[from];
      moves2 &= destination_mask & ~occupied;
      moves2 |= bb;

      /* Serialise */
      while (!moves2.is_empty()) {
         int to = moves2.bitscan();
         moves2.reset(to);

         /* Push this move directly */
         move_t move = encode_normal_move(piece, from, to);
         movelist->push(move);
      }
   }

   void generate_double_captures(movelist_t *movelist, move_flag_t capture_flags, bitboard_t<kind> destination_mask, side_t side_to_move, bitboard_t<kind> occupied, bitboard_t<kind> enemy, int piece, int from) const
   {
      move_flag_t cf1 = capture_flags & ( MF_LEAPER | MF_IS_LEAPER | MF_LEAPER_ASYMM );
      move_flag_t cf2 = capture_flags & (             MF_IS_LEAPER | MF_LEAPER_ASYMM );
      cf2 |= get_leaper_index2(capture_flags) << 16;

      bitboard_t<kind> captures = generate_leaper_move_bitboard(cf1, side_to_move, from, occupied);
      bitboard_t<kind> moves    = captures;// & ~occupied;
      captures &= destination_mask & enemy;

      /* Serialise primary captures */
      bitboard_t<kind> done;
      while (!captures.is_empty()) {
         int to = captures.bitscan();
         captures.reset(to);

         /* Push this move directly */
         move_t move = encode_normal_capture(piece, from, to);
         movelist->push(move);
         done.set(to);

         /* Second step */
         bitboard_t<kind> c2 = generate_leaper_move_bitboard(cf2, side_to_move, to, occupied) & destination_mask;
         bitboard_t<kind> m2 = c2 & ~occupied;
         c2 &= enemy;
         m2.set(from);

         /* Serialise second capture */
         while(!c2.is_empty()) {
            int to2 = c2.bitscan();
            c2.reset(to2);

            move_t move = encode_double_capture(piece, from, to2, to);
            movelist->push(move);
         }

         /* Serialise second move (non-capture) */
         while(!m2.is_empty()) {
            int to2 = m2.bitscan();
            m2.reset(to2);

            move_t move = encode_en_passant_capture(piece, from, to2, to);
            movelist->push(move);
         }
      }

      /* Serialise primary moves */
      while (!moves.is_empty()) {
         int to = moves.bitscan();
         moves.reset(to);

         bitboard_t<kind> c2 = generate_leaper_move_bitboard(cf2, side_to_move, to, occupied);
         c2 &= destination_mask & enemy;
         c2 &= ~done;

         while(!c2.is_empty()) {
            int to2 = c2.bitscan();
            c2.reset(to2);

            if (!done.test(to2)) {
               move_t move = encode_normal_capture(piece, from, to2);
               movelist->push(move);
            }
            done.set(to2);
         }
      }
   }


   template<bool generate_drops,
            bool capture_to_holdings,
            bool generate_pickup,
            bool promote_in_place,
            bool capture_victim_sideeffect>
   void do_generate_moves_mask(movelist_t *movelist, const board_t<kind> *board, bitboard_t<kind> source_mask, bitboard_t<kind> destination_mask, side_t side_to_move, piece_bit_t allowed_promotion_pieces, piece_bit_t allowed_drop_pieces, piece_bit_t allowed_piece_deferrals) const
   {
      piece_description_t<kind> *piece_types;
      move_flag_t *piece_capture_flags;
      move_flag_t *piece_move_flags;
      move_flag_t *special_move_flags;
      move_flag_t *initial_move_flags;
      bitboard_t<kind> own, enemy, own_movers;
      bitboard_t<kind> attacked;
      move_t move;
      int n;

      assert(!player_in_check(board, next_side[side_to_move]));

      piece_types = board->piece_types;
      piece_capture_flags = piece_types->piece_capture_flags;
      piece_move_flags = piece_types->piece_move_flags;
      special_move_flags = piece_types->piece_special_move_flags;
      initial_move_flags = piece_types->piece_initial_move_flags;

      /* Bookkeeping: we keep a pointer to the next move in the move list, and
       * update the number of moves in the list at the end of this function
       */
      own = board->bbc[side_to_move];
      enemy = board->bbc[next_side[side_to_move]];

      own_movers = own & source_mask;
      bitboard_t<kind> movers = own_movers;

      /* Generate drops */
      if (generate_drops && allowed_drop_pieces) {
         bool dropped = false;
         for (n=0; n<piece_types->num_piece_types; n++) {
            bitboard_t<kind> occupied = board->get_occupied();

            if (board->holdings[n][side_to_move] && (allowed_drop_pieces & (1 << n))) {
               dropped = true;

               int piece = piece_for_side(n, side_to_move);
               bitboard_t<kind> drops = destination_mask & ~occupied & piece_types->drop_zone[side_to_move][n];

               if (piece_types->piece_flags[n] & PF_DROPONEFILE) {
                  for (int f = 0; f<bitboard_t<kind>::board_files; f++) {
                     bitboard_t<kind> bb = own & board->bbp[n] & bitboard_t<kind>::board_file[f];
                     if (!bb.is_empty())
                        if (piece_types->piece_drop_file_maximum[n] < 2 || bb.popcount() >= piece_types->piece_drop_file_maximum[n])
                           drops &= ~bitboard_t<kind>::board_file[f];
                  }
               }

               while (!drops.is_empty()) {
                  int to = drops.bitscan();
                  drops.reset(to);
                  move = encode_drop_move(piece, to);
                  move = add_move_retrieve(move, piece, 1);
                  movelist->push(move);

                  if (board->rule_flags & RF_PROMOTE_ON_DROP) {
                     piece_bit_t c = piece_types->piece_promotion_choice[n] & allowed_promotion_pieces;
                     while (c) {
                        int tpiece = bitscan32(c);
                        c ^= 1<<tpiece;
                        if (piece_types->piece_maximum[tpiece][side_to_move] == 128 ||
                            board->piece_count(tpiece, side_to_move) < piece_types->piece_maximum[tpiece][side_to_move]) {
                           tpiece = piece_for_side(tpiece, side_to_move);
                           move = encode_drop_move(tpiece, to);
                           move = add_move_retrieve(move, piece, 1);
                           movelist->push(move);
                        }
                     }
                  }
               }
            }
         }

         /* Break out early if drops are possible and are forced if possible; no other moves are legal. */
         if (dropped && (board->rule_flags & RF_FORCE_DROPS))
            goto done;
      }

      /* Generate lifts */
      if (generate_pickup && allowed_drop_pieces && !board->check()) {
         for (n=0; n<piece_types->num_piece_types; n++) {
            if (piece_types->piece_flags[n] & PF_ROYAL) continue;
            bitboard_t<kind> lift = own_movers & board->bbp[n];

            if (!lift.is_empty() && (allowed_drop_pieces & (1 << n))) {
               int piece = piece_for_side(n, side_to_move);

               while (!lift.is_empty()) {
                  int from = lift.bitscan();
                  lift.reset(from);
                  move = encode_pickup_move(piece, from);
                  move = add_move_store(move, piece, 1);
                  movelist->push(move);
               }
            }
         }
      }


      /* Now generate moves for all pieces; only scan our own pieces. This mainly helps variants with different
       * armies.
       * We generate all moves for a particular piece-type first.
       */
      for (n=0; n<piece_types->num_piece_types && !movers.is_empty(); n++) {
         if ((movers & board->bbp[n]).is_empty()) continue;
         bitboard_t<kind> occupied = board->get_occupied() | piece_types->block[side_to_move][n];
         movers &= ~board->bbp[n];

         bitboard_t<kind> special_zone = piece_types->special_zone[side_to_move][n];
         bitboard_t<kind> initial_zone = initial_move_flags[n] ? board->init : bitboard_t<kind>::board_empty;
         if (board->rule_flags & RF_SPECIAL_IS_INIT) special_zone &= board->init;
         if ((piece_types->piece_flags[n] & PF_ROYAL) && board->check())
            special_zone.clear();

         bitboard_t<kind> ep_dest;

         if (piece_types->piece_flags[n] & PF_TAKE_EP)
            ep_dest = board->ep;

         promotion_zone_t<kind> *promotion = piece_types->promotion[n];
         bitboard_t<kind> promotion_zone = piece_types->promotion_zone[side_to_move][n];
         bitboard_t<kind> optional_promotion_zone = piece_types->optional_promotion_zone[side_to_move][n];

         bitboard_t<kind> bb = own_movers & board->bbp[n];
         int piece = piece_for_side(n, side_to_move);

         /* In-place promotions */
         if (promote_in_place) {
            promotion_zone_t<kind> *promotion = piece_types->promotion[n];
            for (int k=0; k<MAX_PZ && promotion[k].choice; k++) {
               bitboard_t<kind> pz = promotion[k].zone[side_to_move];
               if ((piece_types->piece_flags[n] & PF_PROMOTEWILD) && (board->bbc[side_to_move]&board->bbp[n]).onebit())
                  pz = bitboard_t<kind>::board_all;
               bitboard_t<kind> bp = bb & pz;
               while (!bp.is_empty()) {
                  int square = bp.bitscan();
                  bp.reset(square);

                  piece_bit_t c = piece_types->piece_promotion_choice[n] & promotion[k].choice & allowed_promotion_pieces;
                  while (c) {
                     int tpiece = bitscan32(c);
                     c ^= 1<<tpiece;
                     if (piece_types->piece_maximum[tpiece][side_to_move] == 128 ||
                           board->piece_count(tpiece, side_to_move) < piece_types->piece_maximum[tpiece][side_to_move]) {
                        tpiece = piece_for_side(tpiece, side_to_move);
                        move = encode_normal_promotion(piece, square, square, tpiece);
                        movelist->push(move);
                     }
                  }
               }
            }
         }

         /* Generate stepper moves, in parallel */
         generate_stepper_moves_mask_for_piece<false>(movelist, board,
                  piece, piece_move_flags[n], piece_types->piece_flags[n], piece_types,
                  bb & ~(special_zone|initial_zone), destination_mask, occupied,
                  promotion, promotion_zone, optional_promotion_zone,
                  side_to_move, allowed_promotion_pieces);
         generate_stepper_moves_mask_for_piece<true>(movelist, board,
                  piece, special_move_flags[n], piece_types->piece_flags[n], piece_types,
                  bb & special_zone, destination_mask, occupied,
                  promotion, promotion_zone, optional_promotion_zone,
                  side_to_move, allowed_promotion_pieces);
         generate_stepper_moves_mask_for_piece<true>(movelist, board,
                  piece, initial_move_flags[n], piece_types->piece_flags[n], piece_types,
                  bb & initial_zone, destination_mask, occupied,
                  promotion, promotion_zone, optional_promotion_zone,
                  side_to_move, allowed_promotion_pieces);
         generate_stepper_captures_mask_for_piece<capture_to_holdings, capture_victim_sideeffect>(movelist, board,
                  piece, piece_capture_flags[n], piece_types->piece_flags[n], piece_types,
                  bb, destination_mask, occupied, enemy, ep_dest,
                  promotion, promotion_zone, optional_promotion_zone,
                  side_to_move, allowed_promotion_pieces);

         /* Castling
          * Because of the hassle when doing legality testing, we explicitly test whether castling is allowed in
          * the current position by testing for attacks on any of the critical squares. This is a hassle and
          * potentially slow, but only if castling may be possible in the current position.
          */
         if (expect(piece_types->piece_flags[n] & PF_CASTLE, false) && !(board->init&bb).is_empty()) {
            if (!((piece_types->piece_flags[n] & PF_ROYAL) && board->check()))
            for (int c = SHORT; c<NUM_CASTLE_MOVES; c++) {
               if (!(bb & castle_mask[c][side_to_move]).is_empty() &&
                   (board->init & castle_mask[c][side_to_move]) == castle_mask[c][side_to_move]) {
                  if (castle_free[c][side_to_move].is_empty() ||
                      ((occupied & castle_free[c][side_to_move]).is_empty() &&
                       !(destination_mask & castle_free[c][side_to_move]).is_empty())) {
                     bitboard_t<kind> test             = castle_safe[c][side_to_move];
                     bitboard_t<kind> mask             = generate_super_attacks_for_squares(test, super);
                     bitboard_t<kind> attacked_squares = generate_attack_bitboard(board, test, mask, next_side[side_to_move]);
                     if ((attacked_squares & castle_safe[c][side_to_move]).is_empty()) {
                        int from1 = (castle_mask[c][side_to_move] &  bb).bitscan();
                        int from2 = (castle_mask[c][side_to_move] & ~bb).bitscan();
                        int piece2 = piece_for_side(board->get_piece(from2), side_to_move);
                        bitboard_t<kind> king_dest = castle_king_dest[c][side_to_move];
                        bitboard_t<kind> rook_dest = castle_rook_dest[c][side_to_move];
                        while (!king_dest.is_empty() && !rook_dest.is_empty()) {
                           int to1 = king_dest.bitscan();
                           int to2 = rook_dest.bitscan();
                           king_dest.reset(to1);
                           rook_dest.reset(to2);
                           move = encode_castle_move(piece, from1, to1, piece2, from2, to2);
                           movelist->push(move);
                        }
                     }
                  }
               }
            }
         }

         /* Now determine slider and leaper moves for this piece type - if it has any */
         bool test_other_moves = (piece_move_flags[n] | piece_capture_flags[n] | special_move_flags[n]) & (MF_SLIDER|MF_HOPPER|MF_IS_LEAPER|MF_RIDER);
         if ( test_other_moves || (board->rule_flags & RF_PROMOTE_BY_MOVE)) {
            while (!bb.is_empty()) {
               bitboard_t<kind> moves;
               bitboard_t<kind> captures;
               int from = bb.bitscan();
               bb.reset(from);

               move_flag_t move_flags = piece_move_flags[n];
               move_flag_t capture_flags = piece_capture_flags[n];
               if (special_zone.test(from))
                  move_flags = special_move_flags[n];

               if (initial_move_flags[n] && initial_zone.test(from))
                  move_flags = initial_move_flags[n];

               move_flags    &= (MF_SLIDER | MF_HOPPER | MF_LEAPER_FLAGS | MF_RIDER);
               capture_flags &= (MF_SLIDER | MF_HOPPER | MF_LEAPER_FLAGS | MF_RIDER);

               /* Multi-step leapers (Lion) */
               if (is_double_leaper(move_flags) && !is_masked_leaper(move_flags)) {
                  generate_double_moves(movelist, move_flags, destination_mask, side_to_move, occupied, piece, from);
                  move_flags &= (MF_SLIDER | MF_HOPPER | MF_RIDER);
               }
               if (is_double_leaper(capture_flags) && !is_masked_leaper(capture_flags)) {
                  generate_double_captures(movelist, capture_flags, destination_mask, side_to_move, occupied, enemy, piece, from);
                  capture_flags &= (MF_SLIDER | MF_HOPPER | MF_RIDER);
               }

               if (is_leaper(move_flags)) moves |= generate_leaper_move_bitboard(move_flags, side_to_move, from, occupied);
               if (is_slider(move_flags)) moves |= generate_slider_move_bitboard(move_flags, side_to_move, from, occupied);
               if (is_hopper(move_flags)) moves |= generate_hopper_move_bitboard(move_flags, side_to_move, from, occupied);
               if (is_rider (move_flags)) moves |= generate_rider_move_bitboard (move_flags, side_to_move, from, occupied);
               moves &= piece_types->prison[side_to_move][n];

               /* Filter royal moves that pass through check */
               if ( (board->rule_flags & RF_NO_MOVE_PAST_CHECK) && (piece_types->piece_flags[n] & PF_ROYAL) ) {
                  bitboard_t<kind> mask             = generate_super_attacks_for_squares(moves, super);
                  bitboard_t<kind> attacked_squares = generate_attack_bitboard(board, bitboard_t<kind>::board_empty, mask, next_side[side_to_move]);

                  if (!(moves & attacked_squares).is_empty()) {
                     bitboard_t<kind> ok_moves;
                     while (!moves.is_empty()) {
                        int to = moves.bitscan();
                        moves.reset(to);

                        if ( (bitboard_t<kind>::board_between[from][to] & attacked_squares).is_empty())
                           ok_moves.set(to);

                     }
                     moves = ok_moves;
                  }
               }

               /* Optimise the common case where pieces move the same way
                * they capture.
                */
               if (capture_flags == move_flags) {
                  captures = moves;
               } else {
                  if (is_leaper(capture_flags)) captures |= generate_leaper_move_bitboard(capture_flags, side_to_move, from, occupied);
                  if (is_slider(capture_flags)) captures |= generate_slider_move_bitboard(capture_flags, side_to_move, from, occupied);
                  if (is_hopper(capture_flags)) captures |= generate_hopper_move_bitboard(capture_flags, side_to_move, from, occupied);
                  if (is_rider (capture_flags)) captures |= generate_rider_move_bitboard (capture_flags, side_to_move, from, occupied);
                  captures &= piece_types->prison[side_to_move][n];
               }

               /* Pass */
               if (moves.test(from)) {
                  move_t move = encode_normal_move(piece, from, from);
                  movelist->push(move);
               }

               /* Mask out occupied squares from normal moves, only capture enemy pieces */
               moves &= ~occupied;
               captures &= enemy;

               moves &= destination_mask;
               captures &= destination_mask;

               /* Serialise moves
                * We separate out the promotion moves and options and
                * serialise those in a separate loop.
                */
               bitboard_t<kind> pmoves    = moves    & (promotion_zone & ~optional_promotion_zone);
               bitboard_t<kind> pcaptures = captures & (promotion_zone & ~optional_promotion_zone);
               moves     ^= pmoves;
               captures  ^= pcaptures;
               pmoves    |= moves & optional_promotion_zone;
               pcaptures |= captures & optional_promotion_zone;

               pmoves &= piece_types->entry_promotion_zone[side_to_move][n];
               pcaptures &= piece_types->entry_promotion_zone[side_to_move][n];

               /* Also include moves that originate in the promotion zone. */
               if (promotion_zone.test(from)) {
                  pmoves    |= moves;
                  pcaptures |= captures;

                  if (!optional_promotion_zone.test(from)) {
                     moves.clear();
                     captures.clear();
                  }
               }

               if (!(allowed_piece_deferrals & (1<<n))) {
                  moves    &= ~pmoves;
                  captures &= ~pcaptures;
               }

               while (!moves.is_empty()) {
                  int to = moves.bitscan();
                  moves.reset(to);
                  move = encode_normal_move(piece, from, to);
                  movelist->push(move);
               }
               while (!captures.is_empty()) {
                  int to = captures.bitscan();
                  int ptaken = board->get_piece(to);

                  captures.reset(to);

                  if ( capture_victim_sideeffect && (piece_types->piece_allowed_victims[n] & (1 << ptaken)) == 0 )
                     continue;

                  if (capture_victim_sideeffect && (piece_types->piece_flags[ptaken] & PF_ASSIMILATE) && !(piece_types->piece_flags[n] & PF_ROYAL)) {
                     int tpiece = piece_for_side(ptaken, side_to_move);
                     move = encode_capture_promotion(piece, from, to, tpiece);
                  } else {
                     move = encode_normal_capture(piece, from, to);
                  }
                  if (capture_to_holdings) {
                     int pstore = piece_types->demotion[board->get_piece(to)];
                     side_t store_side = NONE;
                     if (board->rule_flags & RF_KEEP_CAPTURE)   store_side = side_to_move;
                     if (board->rule_flags & RF_RETURN_CAPTURE) store_side = next_side[side_to_move];
                     assert(store_side != NONE);
                     move = add_move_store(move, piece_for_side(pstore, store_side), 1);
                  }
                  movelist->push(move);
               }

               /* Special promotion moves */
               if (board->rule_flags & RF_PROMOTE_BY_MOVE) {
                  bitboard_t<kind> pm;
                  bitboard_t<kind> pc;

                  piece_bit_t ptried = 0;
                  for (int k=0; k<MAX_PZ && promotion[k].choice; k++) {
                     bitboard_t<kind> pz = promotion[k].zone[side_to_move];
                     if ((piece_types->piece_flags[n] & PF_PROMOTEWILD) && (board->bbc[side_to_move]&board->bbp[n]).onebit())
                        pz = bitboard_t<kind>::board_all;
                     if (!pz.test(from)) continue;
                     piece_bit_t c = promotion[k].choice & allowed_promotion_pieces & ~ptried;
                     ptried |= promotion[k].choice;
                     while (c) {
                        int tp = bitscan32(c);
                        c ^= 1<<tp;

                        move_flag_t move_flags = piece_move_flags[tp];
                        move_flag_t capture_flags = piece_capture_flags[tp];

                        if (is_leaper(move_flags)) pm |= generate_leaper_move_bitboard(move_flags, side_to_move, from, occupied);
                        if (is_slider(move_flags)) pm |= generate_slider_move_bitboard(move_flags, side_to_move, from, occupied);
                        if (is_hopper(move_flags)) pm |= generate_hopper_move_bitboard(move_flags, side_to_move, from, occupied);
                        if (is_rider (move_flags)) pm |= generate_rider_move_bitboard (move_flags, side_to_move, from, occupied);

                        if (move_flags == capture_flags)
                           pc |= pm;
                        else {
                           if (is_leaper(capture_flags)) captures |= generate_leaper_move_bitboard(capture_flags, side_to_move, from, occupied);
                           if (is_slider(capture_flags)) captures |= generate_slider_move_bitboard(capture_flags, side_to_move, from, occupied);
                           if (is_hopper(capture_flags)) captures |= generate_hopper_move_bitboard(capture_flags, side_to_move, from, occupied);
                           if (is_rider (capture_flags)) captures |= generate_rider_move_bitboard (capture_flags, side_to_move, from, occupied);
                        }
                     }
                  }
                  pm &= ~occupied;
                  pc &= enemy;

                  pm &= destination_mask;
                  pc &= destination_mask;
                  
                  pmoves = pm;
                  pcaptures = pc;
               }

               if (board->rule_flags & RF_QUIET_PROMOTION) pcaptures.clear();

               /* Promotions */
               while (!pmoves.is_empty()) {
                  int to = pmoves.bitscan();
                  piece_bit_t ptried = 0;
                  pmoves.reset(to);
                  for (int k=0; k<MAX_PZ && promotion[k].choice; k++) {
                     bitboard_t<kind> pz = promotion[k].zone[side_to_move];
                     if ((piece_types->piece_flags[n] & PF_PROMOTEWILD) && (board->bbc[side_to_move]&board->bbp[n]).onebit())
                        pz = bitboard_t<kind>::board_all;
                     if (!pz.test(to) && !pz.test(from)) continue;
                     piece_bit_t c = promotion[k].choice & allowed_promotion_pieces & ~ptried;
                     ptried |= promotion[k].choice;
                     while (c) {
                        int tpiece = bitscan32(c);
                        c ^= 1<<tpiece;
                        if (piece_types->piece_maximum[tpiece][side_to_move] == 128 ||
                              board->piece_count(tpiece, side_to_move) < piece_types->piece_maximum[tpiece][side_to_move]) {
                           tpiece = piece_for_side(tpiece, side_to_move);
                           move = encode_normal_promotion(piece, from, to, tpiece);
                           movelist->push(move);
                        }
                     }
                  }
               }
               while (!pcaptures.is_empty()) {
                  int to = pcaptures.bitscan();
                  int ptaken = board->get_piece(to);
                  pcaptures.reset(to);

                  if ( capture_victim_sideeffect && (piece_types->piece_allowed_victims[n] & (1 << ptaken)) == 0 )
                     continue;

                  piece_bit_t ptried = 0;
                  for (int k=0; k<MAX_PZ && promotion[k].choice; k++) {
                     bitboard_t<kind> pz = promotion[k].zone[side_to_move];
                     if ((piece_types->piece_flags[n] & PF_PROMOTEWILD) && (board->bbc[side_to_move]&board->bbp[n]).onebit())
                        pz = bitboard_t<kind>::board_all;
                     if (!pz.test(to) && !pz.test(from)) continue;
                     piece_bit_t c = promotion[k].choice & allowed_promotion_pieces & ~ptried;
                     ptried |= promotion[k].choice;
                     if (capture_victim_sideeffect && (piece_types->piece_flags[ptaken] & PF_ASSIMILATE)) {
                        c = (1<<ptaken) & ~ptried;
                        ptried |= c;
                     }
                     while (c) {
                        int tpiece = bitscan32(c);
                        c ^= 1<<tpiece;
                        if (piece_types->piece_maximum[tpiece][side_to_move] == 128 ||
                              board->piece_count(tpiece, side_to_move) < piece_types->piece_maximum[tpiece][side_to_move]) {
                           tpiece = piece_for_side(tpiece, side_to_move);
                           move = encode_capture_promotion(piece, from, to, tpiece);
                           if (capture_to_holdings) {
                              int victim = to;
                              if (ep_dest.test(to)) victim = board->ep_victim;
                              int pstore = piece_types->demotion[board->get_piece(victim)];
                              side_t store_side = NONE;
                              if (board->rule_flags & RF_KEEP_CAPTURE)   store_side = side_to_move;
                              if (board->rule_flags & RF_RETURN_CAPTURE) store_side = next_side[side_to_move];
                              assert(store_side != NONE);
                              move = add_move_store(move, piece_for_side(pstore, store_side), 1);
                           }
                           movelist->push(move);
                        }
                     }
                  }
               }
            }
         }
      }

   done:
      return;
   }

   template<bool drop_rules,
            bool hold_rules,
            bool pickup_rules,
            bool promote_rules>
   void do_generate_moves_mask_victim(movelist_t *ml, const board_t<kind> *board, bitboard_t<kind> from, bitboard_t<kind> to, side_t stm, uint32_t allowed_prom, uint32_t allowed_drop, uint32_t allowed_defer) const
   {
      bool victim_effect = (board->rule_flags & RF_VICTIM_SIDEEFFECT) != 0;
      bool immune = false;

      if (victim_effect) {
         bitboard_t<kind> iron, danger;
         if (!board->retaliate_ok()) {
            for (int n = 0; n<board->piece_types->num_piece_types; n++) {
               if (board->piece_types->piece_flags[n] & PF_NO_RETALIATE)
                  iron |= board->bbp[n];
            }
         }
         for (int n = 0; n<board->piece_types->num_piece_types; n++) {
            if (board->piece_types->piece_flags[n] & PF_ENDANGERED)
               danger |= board->bbp[n];
            if (board->piece_types->piece_flags[n] & PF_IRON)
               iron |= board->bbp[n];
         }
         iron &= to;
         to &= ~iron;
         //if ((from & danger).is_empty() && iron.is_empty() && !immune) victim_effect = false;

         if (!(from & danger).is_empty()) {
            bitboard_t<kind> to_danger = to & ~danger;
            while (!(danger & to & board->bbc[next_side[stm]]).is_empty()) {
               int sqr = (danger & to & board->bbc[next_side[stm]]).bitscan();
               danger.reset(sqr);
               bitboard_t<kind> test             = bitboard_t<kind>::square_bitboards[sqr];
               bitboard_t<kind> mask             = super[sqr] & board->bbc[next_side[stm]];
               bitboard_t<kind> attacked_squares = generate_attack_bitboard(board, test, mask, next_side[stm]);
               if ((attacked_squares & test).is_empty())
                  to_danger.set(sqr);
            }
            do_generate_moves_mask<drop_rules, hold_rules, pickup_rules, promote_rules, true>(ml, board, from&danger, to_danger, stm, allowed_prom, allowed_drop, allowed_defer);

            from &= ~danger;
         }

      }

      if (victim_effect)
         do_generate_moves_mask<drop_rules, hold_rules, pickup_rules, promote_rules, true>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
      else
         do_generate_moves_mask<drop_rules, hold_rules, pickup_rules, promote_rules, false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
   }


   template<bool drop_rules, bool hold_rules, bool pickup_rules>
   void do_generate_moves_mask_inplace(movelist_t *ml, const board_t<kind> *board, bitboard_t<kind> from, bitboard_t<kind> to, side_t stm, uint32_t allowed_prom, uint32_t allowed_drop, uint32_t allowed_defer) const
   {
      if (board->rule_flags & RF_PROMOTE_IN_PLACE)
         do_generate_moves_mask_victim<drop_rules, hold_rules, pickup_rules, true>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
      else
         do_generate_moves_mask_victim<drop_rules, hold_rules, pickup_rules, false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
   }

   template<bool drop_rules, bool hold_rules>
   void do_generate_moves_mask_pickup(movelist_t *ml, const board_t<kind> *board, bitboard_t<kind> from, bitboard_t<kind> to, side_t stm, uint32_t allowed_prom, uint32_t allowed_drop, uint32_t allowed_defer) const
   {
      if ((board->rule_flags & RF_ALLOW_PICKUP))
         do_generate_moves_mask_inplace<drop_rules, hold_rules, true>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
      else
         do_generate_moves_mask_inplace<drop_rules, hold_rules, false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
   }

   template<bool drop_rules, bool quiesc_only>
   void do_generate_moves_mask_hold(movelist_t *ml, const board_t<kind> *board, bitboard_t<kind> from, bitboard_t<kind> to, side_t stm, uint32_t allowed_prom, uint32_t allowed_drop, uint32_t allowed_defer) const
   {
      /* If we don't use drop rules, we don't update holdings. */
      if (!drop_rules) {
         do_generate_moves_mask_pickup<drop_rules, false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
         return;
      }

      if (!quiesc_only) {
         if (board->rule_flags & RF_USE_CAPTURE)
            do_generate_moves_mask_pickup<drop_rules, true>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
         else
            do_generate_moves_mask_pickup<drop_rules, false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
      } else {
         bitboard_t<kind> oking = board->royal & board->bbc[next_side[stm]];
         bitboard_t<kind> king_zone;
         if (oking.onebit())
            king_zone = bitboard_t<kind>::neighbour_board[oking.bitscan()];

         if (board->rule_flags & RF_USE_CAPTURE) {
            do_generate_moves_mask_pickup<false, true>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
            //do_generate_moves_mask_pickup<true, true>(ml, board, from, king_zone, stm, allowed_prom, allowed_drop, allowed_defer);
         } else {
            do_generate_moves_mask_pickup<false, false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
            //do_generate_moves_mask_pickup<true, false>(ml, board, from, king_zone, stm, allowed_prom, allowed_drop, allowed_defer);
         }
      }
   }

   template<bool quiesc_only>
   void do_generate_moves_mask_quiesc(movelist_t *ml, const board_t<kind> *board, bitboard_t<kind> from, bitboard_t<kind> to, side_t stm, uint32_t allowed_prom, uint32_t allowed_drop, uint32_t allowed_defer) const
   {
      if (board->rule_flags & (RF_ALLOW_DROPS | RF_FORCE_DROPS)) {
         do_generate_moves_mask_hold<true, quiesc_only>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
      } else
         do_generate_moves_mask_hold<false, quiesc_only>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
   }

   inline void generate_moves_mask(movelist_t *ml, const board_t<kind> *board, bitboard_t<kind> from, bitboard_t<kind> to, side_t stm, uint32_t allowed_prom, uint32_t allowed_drop, uint32_t allowed_defer, bool quiesc_only = false) const
   {
      if (quiesc_only)
         do_generate_moves_mask_quiesc<true>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
      else
         do_generate_moves_mask_quiesc<false>(ml, board, from, to, stm, allowed_prom, allowed_drop, allowed_defer);
   }


   /* Generate pseudo-legal check evasions. This is mainly useful in that
    * it removes most illegal moves. It may not catch all of them though,
    * depending on rules and piece moves in a particular variant.
    */
   bool generate_evasions(movelist_t *movelist, const board_t<kind> *board, side_t side_to_move) const
   {
      assert(board->check());

      bitboard_t<kind> destination = bitboard_t<kind>::board_all;
      bitboard_t<kind> origin = bitboard_t<kind>::board_all;
      bitboard_t<kind> attacker, bb, kings, pinned, occ, destest, safe;
      kings = board->royal & board->bbc[side_to_move];
      occ   = board->get_occupied();

      /* FIXME: if there are multiple kings we get duplicate moves in the
       * movelist.
       */
      if (!kings.onebit()) return false;

      movelist->num_moves = 0;

      /* Identify attacking pieces */
      bb = kings;
      while (!bb.is_empty()) {
         int king = bb.bitscan();
         bb.reset(king);
         attacker |= get_all_attackers(board, occ, king);
      }
      assert(!attacker.is_empty());
      attacker &= ~board->bbc[side_to_move];

      /* Evasions */
      if (kings.onebit()) {
         safe = destination;
         safe &= ~generate_attack_bitboard_mask(board, bitboard_t<kind>::board_empty, attacker, ~kings, next_side[side_to_move]);
         generate_moves_mask(movelist, board, kings, safe, side_to_move, ~0, 0, ~0);
      } else {
         bb = kings;
         while (!bb.is_empty()) {
            bitboard_t<kind> king_bb;
            int king = bb.bitscan();
            bb.reset(king);
            king_bb.set(king);

            safe = destination;
            safe &= ~generate_attack_bitboard_mask(board, king_bb, attacker, ~king_bb, side_to_move);
            generate_moves_mask(movelist, board, king_bb, safe, side_to_move, ~0, 0, ~0);
         }
      }

      /* Evacuate interposing pieces acting as a cannon mount.
       * Collect double leapers.
       */
      bitboard_t<kind> multi;
      for (int n=0; n<board->piece_types->num_piece_types; n++) {
         move_flag_t atk_flags = board->piece_types->piece_capture_flags[n];
         if (is_double_leaper(atk_flags) && !is_masked_leaper(atk_flags)) multi |= board->bbp[n];
         if (!is_hopper(atk_flags)) continue;
         bb = attacker & board->bbp[n];
         while (!bb.is_empty()) {
            int square = bb.bitscan();
            bb.reset(square);

            bitboard_t<kind> king_bb = kings;
            while (!king_bb.is_empty()) {
               int king = king_bb.bitscan();
               king_bb.reset(king);

               bitboard_t<kind> from_bb = bitboard_t<kind>::board_between[king][square];
               generate_moves_mask(movelist, board, from_bb, ~attacker, side_to_move, ~0, ~0, ~0);
            }
         }
      }

      /* Captures of attacking pieces. Take sepecial care of multi-step
       * pieces, which can move again after capturing the attacker, so we
       * cannot mask out everything but the attacker.
       */
      bb = multi & board->bbc[side_to_move];
      if (!bb.is_empty())
         generate_moves_mask(movelist, board, multi, destination, side_to_move, ~0, 0, ~0);
      generate_moves_mask(movelist, board, origin^kings^multi, attacker, side_to_move, ~0, 0, ~0);
      destest |= attacker;

      /* En-passant captures */
      if (board->ep_victim && attacker.test(board->ep_victim)) {
         bitboard_t<kind> bb = board->ep;
         generate_moves_mask(movelist, board, origin^kings, bb, side_to_move, ~0, 0, ~0);
      }

      /* Interpose */
      bb = kings;
      pinned = get_pinned_pieces(board, side_to_move);
      while (!bb.is_empty()) {
         int king = bb.bitscan();
         bb.reset(king);

         bitboard_t<kind> bp = attacker;
         while (!bp.is_empty()) {
            int square = bp.bitscan();
            int piece  = board->get_piece(square);
            bp.reset(square);

            bitboard_t<kind> destination = bitboard_t<kind>::board_between[king][square] & ~occ;
            if (!destination.is_empty())
               generate_moves_mask(movelist, board, origin^(kings | pinned), destination, side_to_move, ~0, ~0, ~0);
            else if (is_masked_leaper(board->piece_types->piece_capture_flags[piece])) {
               /* Block a lame leaper */
               int index = get_leaper_index(board->piece_types->piece_capture_flags[piece]);

               destination = leaper[index][square] & ~occ;
               generate_moves_mask(movelist, board, origin^(kings | pinned), destination, side_to_move, ~0, ~0, ~0);
            }
            destest |= destination;
         }
      }

      /* Promotion to king, if promotion to king is allowed */
      for (int n = 0; n<board->piece_types->num_piece_types; n++) {
         if (!(board->piece_types->piece_promotion_choice[n] & board->piece_types->royal_pieces))
            continue;

         if ((board->bbp[n] & board->bbc[side_to_move]).is_empty())
            continue;

         bitboard_t<kind> from_mask = board->bbp[n]&(~pinned);
         bitboard_t<kind> to_mask   = destination & (~destest | board->piece_types->promotion_zone[side_to_move][n]);

         if (!to_mask.is_empty() && !from_mask.is_empty())
            generate_moves_mask(movelist, board, from_mask, to_mask, side_to_move, (~0) & board->piece_types->royal_pieces, 0, ~0);
      }

      return true;
   }

   /* Add gating moves to the move list.
    * Intended for Seirawan chess.
    */
   void generate_gate_moves(movelist_t *movelist, const board_t<kind> *board, side_t side_to_move) const
   {
      bitboard_t<kind> king = board->royal & board->bbc[side_to_move];
      bitboard_t<kind> rank = bitboard_t<kind>::board_north_edge;
      int n_last, n;

      if (side_to_move == WHITE)
         rank = bitboard_t<kind>::board_south_edge;

      /* We only care about pieces that have not yet moved */
      if ((rank & board->init).is_empty())
         return;

      /* If the holdings are empty, there is nothing to do here */
      uint32_t gate_mask = 0;
      for (int n=0; n<board->piece_types->num_piece_types; n++)
         if (board->holdings[n][side_to_move]) gate_mask |= 1<<n;
      if (gate_mask == 0) return;

      bitboard_t<kind> pinned = get_pinned_pieces(board, side_to_move);
      pinned &= rank & board->init;

      if (!pinned.is_empty()) {
         /* Filter out moves of pinned pieces that are not along the same rank:
          * they are illegal because they expose the king to check.
          */
         n_last = movelist->num_moves-1;
         n = 0;
         while (n<movelist->num_moves) {
            move_t move = movelist->move[n];
            bitboard_t<kind> from = bitboard_t<kind>::square_bitboards[get_move_from(move)];
            bitboard_t<kind> to   = bitboard_t<kind>::square_bitboards[get_move_to(move)];
            if (!(pinned & from).is_empty() && (rank & to).is_empty()) {
               movelist->move[n] = movelist->move[n_last];
               movelist->move[n_last] = move;
               movelist->num_moves--;
               n_last--;
            }
            n++;
         }
      }

      /* Go through the move list and add appropriate gating moves */
      n_last = movelist->num_moves;
      for (n=0; n<n_last; n++) {
         move_t base = movelist->move[n];
         int from = get_move_from(base);
         bitboard_t<kind> bb_from = bitboard_t<kind>::square_bitboards[from];

         if (!(bb_from & rank & board->init).is_empty()) {
            for (int n=0; n<board->piece_types->num_piece_types; n++) {
               if (board->holdings[n][side_to_move] == 0) continue;
               int tpiece = piece_for_side(n, side_to_move);
               movelist->push(add_move_gate(base, tpiece, from) | MOVE_RESET50);
            }
         }

         if (is_castle_move(base)) {
            int from = get_castle_move_from2(base);
            bitboard_t<kind> bb_from = bitboard_t<kind>::square_bitboards[from];

            for (int n=0; n<board->piece_types->num_piece_types; n++) {
               if (board->holdings[n][side_to_move] == 0) continue;
               int tpiece = piece_for_side(n, side_to_move);
               movelist->push(add_move_gate(base, tpiece, from) | MOVE_RESET50);
            }
         }
      }
   }


   void generate_moves(movelist_t *movelist, const board_t<kind> *board, side_t side_to_move, bool quiesc_only = false, uint32_t allowed_piece_deferrals = ~0) const
   {
      bitboard_t<kind> destination = bitboard_t<kind>::board_all;
      bitboard_t<kind> origin = bitboard_t<kind>::board_all;

      /* If we are in check, then only generate moves in/to the area that can be reached by a superpiece standing
       * in the location of the king(s). These will be the only candidates for resolving the check, all other
       * moves will be pruned anyway.
       */
      if (board->check() && !(board->rule_flags & RF_FORCE_CAPTURE)) {
         if (generate_evasions(movelist, board, side_to_move))
            goto finalise;
         else {
            bitboard_t<kind> royal = board->royal & board->bbc[side_to_move];
            assert(!royal.is_empty());

            destination = generate_super_attacks_for_squares(royal, super);
            quiesc_only = false;
         }
      } else if (quiesc_only || (board->rule_flags & RF_FORCE_CAPTURE)) {
         destination = board->bbc[next_side[side_to_move]];
      }

      movelist->num_moves = 0;
      generate_moves_mask(movelist, board, origin, destination, side_to_move, ~0, ~0, allowed_piece_deferrals, quiesc_only);

      if ((board->rule_flags & RF_FORCE_CAPTURE) && !board->ep.is_empty()) {
         bitboard_t<kind> origin;
         for (int n=0; n<board->piece_types->num_piece_types; n++)
            if (board->piece_types->piece_flags[n] & PF_TAKE_EP)
               origin |= board->bbp[n];
         origin &= board->bbc[side_to_move];
         generate_moves_mask(movelist, board, origin, board->ep, side_to_move, ~0, ~0, allowed_piece_deferrals, quiesc_only);
      }

      if (movelist->num_moves == 0 && (board->rule_flags & RF_FORCE_CAPTURE) && !quiesc_only) {
         destination = bitboard_t<kind>::board_all ^ board->bbc[next_side[side_to_move]];
         generate_moves_mask(movelist, board, origin, destination, side_to_move, ~0, ~0, allowed_piece_deferrals, quiesc_only);
      }

      if (quiesc_only) {
         for (int n=0; n<board->piece_types->num_piece_types; n++) {
            destination = board->piece_types->promotion_zone[side_to_move][n] & ~board->bbc[next_side[side_to_move]];
            origin      = board->bbp[n] & board->bbc[side_to_move];
            if (destination.is_empty()) continue;
            if (origin.is_empty()) continue;

            generate_moves_mask(movelist, board, origin, destination, side_to_move, ~0, 0, allowed_piece_deferrals, quiesc_only);
         }
      }

finalise:
      if ( (board->rule_flags & RF_GATE_DROPS) &&
            !(board->init & board->bbc[side_to_move] & (bitboard_t<kind>::board_south_edge | bitboard_t<kind>::board_north_edge)).is_empty()) {
         generate_gate_moves(movelist, board, side_to_move);
      }
      return;
   }

   stage_t generate_staged_moves(stage_t stage, movelist_t *movelist, const board_t<kind> *board, side_t side_to_move) const
   {
      movelist->clear();

      if (stage == STAGE_DONE)
         return stage;

      side_t oside = next_side[side_to_move];
      bitboard_t<kind> oking = board->royal & board->bbc[oside];
      bitboard_t<kind> occ = board->get_occupied();
      uint32_t defer = board->piece_types->deferral_allowed;

      switch (stage) {
         /* Normal move generation (TODO) */
         case STAGE_DROP:
            break;

         case STAGE_NORMAL:
            break;

         /* Mate/Tsume search */
         case STAGE_CHECKING_DROP:
            if (oking.onebit())
            for (int n = 0; n<board->piece_types->num_piece_types; n++) {
               if (board->holdings[n][side_to_move]) {
                  move_flag_t mf = board->piece_types->piece_capture_flags[n];
                  bitboard_t<kind> check_mask = generate_move_bitboard_for_flags(mf, oking.bitscan(), occ, oside);

                  generate_moves_mask(movelist, board, bitboard_t<kind>::board_empty, check_mask, side_to_move, 0, 1<<n, defer);
               }
            }
            break;

         case STAGE_CHECKING_MOVE:
            if (oking.onebit())
            for (int n = 0; n<board->piece_types->num_piece_types; n++) {
               if ( !(board->bbp[n] & board->bbc[side_to_move]).is_empty() ) {
                  move_flag_t mf = board->piece_types->piece_capture_flags[n];
                  bitboard_t<kind> check_mask = generate_move_bitboard_for_flags(mf, oking.bitscan(), occ, oside);

                  generate_moves_mask(movelist, board, board->bbp[n], check_mask, side_to_move, 0, 0, defer);
               }
            }
            break;

         /* Evasion generation */
         case STAGE_CHECK_EVADE:
            generate_moves(movelist, board, side_to_move, defer != 0);
            break;


         case STAGE_DONE:
            break;
      }

      return next_stage[stage];
   }

   void generate_chase_candidates(movelist_t *movelist, const board_t<kind> *board, side_t side_to_move) const
   {
      assert(board->rule_flags & RF_USE_CHASERULE);
      bitboard_t<kind> destination = board->bbc[next_side[side_to_move]];
      bitboard_t<kind> origin = bitboard_t<kind>::board_all;
      bitboard_t<kind> self = bitboard_t<kind>::board_north;
      bitboard_t<kind> other = bitboard_t<kind>::board_north;

      if (side_to_move == BLACK) {
         self = bitboard_t<kind>::board_north;
         other = bitboard_t<kind>::board_south;
      }

      for (int n = 0; n<board->piece_types->num_piece_types; n++) {
         if (board->piece_types->royal_pieces & (1 << n))      origin &= ~board->bbp[n];
         if (board->piece_types->defensive_pieces & (1 << n))  origin &= ~board->bbp[n];
         if (board->piece_types->pawn_pieces & (1 << n))       origin &= ~(board->bbp[n] & self);
         if (board->piece_types->pawn_pieces & (1 << n))       destination &= ~(board->bbp[n] & other);
      }

      movelist->num_moves = 0;
      generate_moves_mask(movelist, board, origin, destination, side_to_move, ~0, ~0, ~0);
   }

};

#endif
