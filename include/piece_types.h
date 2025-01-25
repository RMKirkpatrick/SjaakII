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
#ifndef PIECESTYPES_H
#define PIECESTYPES_H

#include <stdint.h>
#include <algorithm>
#include "bitboard.h"
#include "moveflag.h"
#include "pieces.h"
#include "eval_types.h"

/* Piece flags */
#define PF_ROYAL        0x00000001  /* Piece is royal */
#define PF_CASTLE       0x00000002  /* Piece may castle */
#define PF_SET_EP       0x00000004  /* Piece sets "en-passant" square when doing a special move */
#define PF_TAKE_EP      0x00000008  /* Piece may capture en-passant */

#define PF_DROPNOCHECK  0x00000010  /* Piece may not be dropped if it gives check */
#define PF_DROPNOMATE   0x00000020  /* Piece may not be dropped if it gives mate */
#define PF_DROPONEFILE  0x00000040  /* Piece may not be dropped if another piece of this type is on the same file */
#define PF_DROPDEAD     0x00000080  /* Piece may be dropped where it has no moves */

#define PF_CANTMATE     0x00000100  /* This piece, by itself, cannot mate */
#define PF_COLOURBOUND  0x00000200  /* This piece is colour-bound */
#define PF_PAIRBONUS    0x00000400  /* Piece gets a pair bonus for similar pieces on opposite colour */
#define PF_NORET        0x00000800  /* Piece cannot return to its original position */

#define PF_NOMATE       0x00001000  /* Piece may not give mate (it may give check) */
#define PF_SHAK         0x00002000
#define PF_CAPTUREFLAG  0x00004000  /* This piece is allowed to capture the flag */
#define PF_ASSIMILATE   0x00008000  /* This piece assimilates a cpaturing piece */

#define PF_NO_RETALIATE 0x00010000  /* A piece with this property may not be recaptured. */
#define PF_ENDANGERED   0x00020000  /* A piece with this property may only capture unprotected pieces with this property */
#define PF_IRON         0x00040000  /* A piece with this property may not be captured */
#define PF_PROMOTEWILD  0x00080000  /* A piece with this property can promote anywhere if it is the last of its kind */

typedef uint32_t piece_flag_t;

struct value_comparator_t {
   int sort_value[256];
   bool operator()(int a, int b) const { return sort_value[a] < sort_value[b]; }
};

#define MAX_PZ    16
template<typename kind>
struct promotion_zone_t {
   bitboard_t<kind>  zone[NUM_SIDES];
   const char *      string;
   piece_bit_t       choice;
};

template<typename kind>
struct piece_description_t {
   /* Define the properties of all piece types.
    * We don't use a struct for a piece type so that we can keep properties of different pieces more closely
    * packed, which is better for cache performance.
    * The index in the arrays here corresponds to the index in the board piece list.
    *
    * Basic entries:
    *    move_flags           (32 bits)
    *    capture_flags        (32 bits)
    *    flags                (32 bits)
    *    name                 (string)
    *    abbreviation         (string)
    *    notation             (string)
    * For evaluation:
    *    piece_value          (16 bit)
    *    centre_weight        (8 bit)
    *    advance_weight       (8 bit)
    *    shield_weight        (8 bit)
    *    mobility_score       (8 bit)
    */
   promotion_zone_t<kind> promotion[MAX_PIECE_TYPES][MAX_PZ];
   bitboard_t<kind>  promotion_zone[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind>  optional_promotion_zone[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind>  special_zone[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind>  prison[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind>  block[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind>  drop_zone[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind>  entry_promotion_zone[NUM_SIDES][MAX_PIECE_TYPES];

   int          num_piece_types;
   int          pawn_steps[NUM_SIDES];  /* The number of moves a pawn has, a factor in pawn mobility and passer eval */
   int8_t       pawn_index[NUM_SIDES];  /* The piece type that will be evaluated as a pawn */
   int8_t       castle_piece[NUM_SIDES][NUM_CASTLE_MOVES];/* The piece type that will castle with the king, nominally "rook" */

   move_flag_t  piece_move_flags[MAX_PIECE_TYPES];
   move_flag_t  piece_capture_flags[MAX_PIECE_TYPES];
   move_flag_t  piece_special_move_flags[MAX_PIECE_TYPES];
   move_flag_t  piece_initial_move_flags[MAX_PIECE_TYPES];
   piece_bit_t  piece_promotion_choice[MAX_PIECE_TYPES];
   piece_bit_t  piece_allowed_victims[MAX_PIECE_TYPES];
   piece_flag_t piece_flags[MAX_PIECE_TYPES];
   int8_t       demotion[MAX_PIECE_TYPES];
   uint8_t      piece_maximum[MAX_PIECE_TYPES][NUM_SIDES];
   uint8_t      piece_drop_file_maximum[MAX_PIECE_TYPES];
   bool         pieces_can_win[MAX_PIECE_TYPES][MAX_PIECE_TYPES];

   int          val_perm[MAX_PIECE_TYPES];
   int16_t      piece_value[MAX_PIECE_TYPES];
   int16_t      see_piece_value[MAX_PIECE_TYPES];
   int16_t      piece_promotion_value[MAX_PIECE_TYPES];

   bitboard_t<kind>  passer_mask[NUM_SIDES][8*sizeof(kind)];
   bitboard_t<kind>  weak_mask[NUM_SIDES][8*sizeof(kind)];
   bitboard_t<kind>  front_span[NUM_SIDES][8*sizeof(kind)];

   /* Piece classifications */
   piece_bit_t royal_pieces;
   piece_bit_t defensive_pieces;
   piece_bit_t pawn_pieces;
   piece_bit_t minor_pieces;
   piece_bit_t major_pieces;
   piece_bit_t super_pieces;
   piece_bit_t shak_pieces;
   piece_bit_t deferral_allowed;

   eval_pair_t eval_value[MAX_PIECE_TYPES];
   eval_pair_t eval_mobility[MAX_PIECE_TYPES][8*sizeof(kind)];
   eval_pair_t eval_pair_bonus[MAX_PIECE_TYPES];
   eval_pair_t eval_pst[MAX_PIECE_TYPES][8*sizeof(kind)];

   int         king_safety_weight[MAX_PIECE_TYPES];
   int         phase_weight[MAX_PIECE_TYPES];
   int         phase_scale;
   int         avg_moves[MAX_PIECE_TYPES];
   int         max_moves[MAX_PIECE_TYPES];
   int         min_moves[MAX_PIECE_TYPES];

   int8_t      tropism[MAX_PIECE_TYPES][8*sizeof(kind)][8*sizeof(kind)];
   int8_t      mobility_score[MAX_PIECE_TYPES][8*sizeof(kind)];

   char *      piece_name[MAX_PIECE_TYPES];
   char *      piece_abbreviation[MAX_PIECE_TYPES][NUM_SIDES];
   char *      piece_notation[MAX_PIECE_TYPES];
   char *      demotion_string[MAX_PIECE_TYPES];
   char *      allowed_victim[MAX_PIECE_TYPES];

   bool        pzset[MAX_PIECE_TYPES];

   void sort_piece_values() {
      value_comparator_t compare;

      for (int n=0; n<num_piece_types; n++) {
         compare.sort_value[n] = piece_value[n];
         if (piece_flags[n] & PF_ROYAL)
            compare.sort_value[n] += 16000;

         eval_value[n] = piece_value[n];
         see_piece_value[n] = compare.sort_value[n];
         val_perm[n] = n;
      }
      std::sort(val_perm, val_perm+num_piece_types, compare);

#if 0
      for (int n = 0; n<num_piece_types; n++)
         printf("%4d ", piece_value[n]);
      printf("\n");
      for (int n = 0; n<num_piece_types; n++)
         printf("%4d ", val_perm[n]);
      printf("\n");
      for (int n = 0; n<num_piece_types; n++)
         printf("%4d ", piece_value[val_perm[n]]);
      printf("\n");
#endif
   }


   int piece_id_from_string(const char *symbol) const
   {
      for (int n = 0; n<num_piece_types; n++) {
         if (strstr(symbol, piece_abbreviation[n][WHITE]) == symbol) return n;
         if (strstr(symbol, piece_abbreviation[n][BLACK]) == symbol) return n;
         if (strcmp(symbol, piece_notation[n]) == 0) return n;
      }
      return -1;
   }

   int promotion_piece_id_from_string(const char *symbol) const
   {
      char ss[32];
      snprintf(ss, sizeof ss, "%s~", symbol);
      for (int n = 0; n<num_piece_types; n++) {
         if (strcmp(ss, piece_abbreviation[n][WHITE]) == 0) return n;
         if (strcmp(ss, piece_abbreviation[n][BLACK]) == 0) return n;
      }
      return piece_id_from_string(symbol);
   }

   void print() const
   {
      for (int n=0; n<num_piece_types; n++) {
         int mate_helpers = 0;
         if (piece_flags[n] & PF_CANTMATE) {
            int n2;
            for (n2=0; n2<num_piece_types; n2++) {
               if (!(piece_flags[n2] & PF_CANTMATE)) continue;
               if (pieces_can_win[n][n2]) mate_helpers |= (1<<n2);
            }
         }
         printf("% 2d %s %s (%s, %s)\n", n, piece_name[n], piece_notation[n], piece_abbreviation[n][WHITE], piece_abbreviation[n][BLACK]);
         if (piece_move_flags[n] & MF_SLIDER)
            printf("   Slider  %08x (move)\n", piece_move_flags[n] & MF_SLIDER);
         if (piece_move_flags[n] & MF_HOPPER)
            printf("   Hopper  %08x (move)\n", piece_move_flags[n] & MF_HOPPER);
         if (piece_move_flags[n] & MF_IS_LEAPER)
            printf("   Leaper  %08x (move)\n", piece_move_flags[n] & 0xffff0000);
         if (piece_move_flags[n] & MF_STEPPER)
            printf("   Stepper %08x (move)\n", piece_move_flags[n] & MF_STEPPER);

         if (piece_capture_flags[n] & MF_SLIDER)
            printf("   Slider  %08x (capture)\n", piece_capture_flags[n] & MF_SLIDER);
         if (piece_capture_flags[n] & MF_HOPPER)
            printf("   Hopper  %08x (capture)\n", piece_capture_flags[n] & MF_HOPPER);
         if (piece_capture_flags[n] & MF_IS_LEAPER)
            printf("   Leaper  %08x (capture)\n", piece_capture_flags[n] & 0xffff0000);
         if (piece_capture_flags[n] & MF_STEPPER)
            printf("   Stepper %08x (capture)\n", piece_capture_flags[n] & MF_STEPPER);

         if (piece_special_move_flags[n] & MF_SLIDER)
            printf("   Slider  %08x (special)\n", piece_special_move_flags[n] & MF_SLIDER);
         if (piece_special_move_flags[n] & MF_HOPPER)
            printf("   Hopper  %08x (special)\n", piece_special_move_flags[n] & MF_HOPPER);
         if (piece_special_move_flags[n] & MF_IS_LEAPER)
            printf("   Leaper  %08x (special)\n", piece_special_move_flags[n] & 0xffff0000);
         if (piece_special_move_flags[n] & MF_STEPPER)
            printf("   Stepper %08x (special)\n", piece_special_move_flags[n] & MF_STEPPER);

         if (piece_initial_move_flags[n] & MF_SLIDER)
            printf("   Slider  %08x (initial)\n", piece_initial_move_flags[n] & MF_SLIDER);
         if (piece_initial_move_flags[n] & MF_HOPPER)
            printf("   Hopper  %08x (initial)\n", piece_initial_move_flags[n] & MF_HOPPER);
         if (piece_initial_move_flags[n] & MF_IS_LEAPER)
            printf("   Leaper  %08x (initial)\n", piece_initial_move_flags[n] & 0xffff0000);
         if (piece_initial_move_flags[n] & MF_STEPPER)
            printf("   Stepper %08x (initial)\n", piece_initial_move_flags[n] & MF_STEPPER);

         printf("   Move statistics: %d--%d <%d>\n", min_moves[n], max_moves[n], avg_moves[n]);

         if (piece_flags[n] & PF_ROYAL)
            printf("   Royal\n");
         if (piece_flags[n] & PF_PAIRBONUS)
            printf("   Pair bonus\n");
         if (piece_flags[n] & PF_NORET)
            printf("   No return\n");
         if (piece_flags[n] & PF_CANTMATE)
            printf("   Cannot deliver mate alone\n");
         if (mate_helpers) {
            printf("   Can deliver mate with help of ");
            while (mate_helpers) {
               int n = bitscan32(mate_helpers);
               mate_helpers ^= (1<<n);
               printf("%s ", piece_notation[n]);
            };
            printf("\n");
         }
         if (piece_flags[n] & PF_CAPTUREFLAG)
            printf("   May capture the flag\n");
         if (piece_flags[n] & PF_IRON)
            printf("   May not be captured\n");

         if (royal_pieces & (1<<n)) printf("   Royal piece\n");
         if (pawn_pieces & (1<<n))  printf("   Pawn-class piece\n");
         if (minor_pieces & (1<<n)) printf("   Minor-class piece\n");
         if (major_pieces & (1<<n)) printf("   Major-class piece\n");
         if (super_pieces & (1<<n)) printf("   Super-class piece\n");

         if (pawn_index[WHITE] == n)
            printf("   White pawn\n");
         if (pawn_index[BLACK] == n)
            printf("   Black pawn\n");

         printf("   King attack weight %d\n", king_safety_weight[n]);
         printf("   Game-phase weight  %d / %d\n", phase_weight[n], phase_scale);
         printf("   Value             [%d %d]\n", eval_value[n].mg, eval_value[n].eg);
      }
      //if (game->board.rule_flags & RF_KING_TRAPPED)
      //   printf("Trapped kings\n");
   }

};

#endif
