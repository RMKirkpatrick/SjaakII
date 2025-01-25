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
#ifndef BOARD_H
#define BOARD_H

#include "compilerdef.h"
#include "bitboard.h"
#include "pieces.h"
#include "piece_types.h"
#include "move.h"
#include "hashkey.h"
#include "squares.h"
#include "ansi.h"

/* Possible rule flags */
#define RF_FORCE_CAPTURE      0x00000001
#define RF_MULTI_CAPTURE      0x00000002

#define RF_KEEP_CAPTURE       0x00000004  /* Captured pieces go to your hand */
#define RF_RETURN_CAPTURE     0x00000008  /* Captured pieces go back to their owner's hand */
#define RF_USE_CAPTURE        (RF_KEEP_CAPTURE | RF_RETURN_CAPTURE)

#define RF_KING_TABOO         0x00000010  /* Kings cannot face eachother along a ray */
#define RF_KING_TRAPPED       0x00000020  /* Kings are trapped in a "palace" */
#define RF_CHECK_ANY_KING     0x00000040  /* If there are multiple kings, attacking any one of them counts as check. */
#define RF_KING_DUPLECHECK    0x00000080  /* It's check if all kings are under attack, if there is more than one. */

#define RF_ALLOW_DROPS        0x00000100  /* The game allows drop moves */
#define RF_FORCE_DROPS        0x00000200  /* Drops are forced if possible */
#define RF_GATE_DROPS         0x00000400  /* Drops work as S-chess gates */
#define RF_USE_DROPS          0x00000700  /* Game uses drops */

#define RF_ALLOW_PICKUP       0x00000800  /* The player is allowed to take pieces in-hand */

#define RF_PROMOTE_IN_PLACE   0x00001000  /* Promotions can be done in-place, without moving a piece */
#define RF_PROMOTE_ON_DROP    0x00002000  /* Pieces can promote when they are dropped */

#define RF_SPECIAL_IS_INIT    0x00004000  /* Special moves are only initial moves */

#define RF_VICTIM_SIDEEFFECT  0x00008000  /* Capture victims may have side effects */

#define RF_USE_HOLDINGS       (RF_USE_DROPS | RF_USE_CAPTURE | RF_ALLOW_PICKUP)  /* Game uses holdings in some way */

#define RF_USE_SHAKMATE       0x00010000  /* The checking sequence prior to mate needs particular pieces */
#define RF_USE_BARERULE       0x00020000  /* Shatranj-style baring rule */
#define RF_USE_CHASERULE      0x00040000  /* Xiangqi-style chase rule is in effect */
#define RF_QUIET_PROMOTION    0x00080000  /* Promotion moves must be quiet moves (Sittuyin style) */

#define RF_CAPTURE_ANY_FLAG   0x00100000  /* There is a "capture the flag" victory condition. */
#define RF_CAPTURE_ALL_FLAG   0x00200000  /* There is a "capture the flag" victory condition. */
#define RF_CAPTURE_THE_FLAG   (RF_CAPTURE_ANY_FLAG | RF_CAPTURE_ALL_FLAG)  /* There is a "capture the flag" victory condition. */

#define RF_NO_MOVE_PAST_CHECK 0x00400000  /* Sliding royals may not slide through check */
#define RF_PROMOTE_BY_MOVE    0x00800000  /* Promote by moving as target piece */

/* Board state */
#define BF_CHECK              0x0001      /* Whether the side to move is in-check or not */
#define BF_WSHAK              0x0002      /* Whether a "shak" was given or not */
#define BF_BSHAK              0x0004      /* Whether a "shak" was given or not */
#define BF_NO_RETALIATE       0x0008      /* Whether retaliation is allowed or not */

template <typename kind>
struct unmake_info_t {
   bitboard_t<kind> init;
   bitboard_t<kind> ep;
   uint64_t board_hash;
   uint64_t hash;
   int8_t fifty_counter;
   int8_t check_count[2];

   int8_t ep_victim;

   uint8_t board_flags;

   uint8_t pickup_piece[4];
#ifdef DEBUGMODE
   move_t move;
#endif
};

template <typename kind>
struct board_t {
   bitboard_t<kind> bbc[NUM_SIDES];
   bitboard_t<kind> bbp[MAX_PIECE_TYPES];
   bitboard_t<kind> flag[NUM_SIDES];  /* Flag bitboard, for "capture the flag" */
   bitboard_t<kind> royal;
   bitboard_t<kind> init;
   bitboard_t<kind> ep;
   int8_t piece[8 * sizeof(kind)];

   /* Piece holdings.
    * These are indexed by [piece type][side to move]
    * We actually have a rule flag to specify whether we're interested in these or not, so we can skip a chunk
    * of code for variants where we're not.
    */
   int8_t holdings[MAX_PIECE_TYPES][NUM_SIDES];

   /* Hash key */
   uint64_t hash;
   uint64_t board_hash;

   /* Rule flags, to change the behaviour of the move generator or the evaluation function */
   uint32_t rule_flags;

   /* Record the board state, in check, castle status */
   uint8_t board_flags;

   /* En-passant target square and capture location. */
   int8_t ep_victim;

   /* Half-move clock (50-move counter) */
   int8_t fifty_counter;

   /* Check count */
   int8_t check_count[2];

   /* Side to move */
   side_t side_to_move;

   /* Description of all piece types */
   piece_description_t<kind> *piece_types;

   int virtual_files;
   int virtual_ranks;
   int bit_to_square[128];

   bool check() const { return (board_flags & BF_CHECK); }
   void check(bool chk) {
      board_flags &= ~BF_CHECK;
      board_flags |= uint8_t(chk);
      if (!chk) board_flags &= ~(BF_WSHAK << side_to_move);
      if (chk)  check_count[side_to_move]++;
   }

   void shak() {
      board_flags |= (BF_WSHAK << side_to_move);
   }
   bool have_shak() {
      return (board_flags & (BF_WSHAK << side_to_move)) != 0;
   }

   bool retaliate_ok() const { return !(board_flags & BF_NO_RETALIATE); }

   void clear()
   {
      for (side_t side = WHITE; side<NUM_SIDES; side++) bbc[side].clear();
      for (int p = 0; p<MAX_PIECE_TYPES; p++) bbp[p].clear();
      royal.clear();
      init.clear();
      ep.clear();
      memset(piece, 0, sizeof piece);

      memset(holdings, 0, sizeof holdings);

      check_count[WHITE] = 0;
      check_count[BLACK] = 0;

      hash = 0;
      board_hash = 0;

      board_flags = 0;

      ep_victim = 0;

      fifty_counter = 0;

      side_to_move = WHITE;
   }

   void put_piece(int type, side_t side, int square)
   {
      assert(!bbc[side].test(square));
      assert(!bbp[type].test(square));
      bbc[side].set(square);
      bbp[type].set(square);
      piece[square] = type;
      if (piece_types->piece_flags[type] & PF_ROYAL)
         royal.set(square);
      hash ^= piece_key[type][side][square];
      board_hash ^= piece_key[type][side][square];
   }

   void clear_piece(int type, side_t side, int square)
   {
      assert(bbc[side].test(square));
      assert(bbp[type].test(square));
      assert(piece[square] == type);
      bbc[side].reset(square);
      bbp[type].reset(square);
      royal.reset(square);
      init.reset(square);
      hash ^= piece_key[type][side][square];
      board_hash ^= piece_key[type][side][square];
   }

   void put_new_piece(int type, side_t side, int square)
   {
      put_piece(type, side, square);
      init.set(square);
   }

   int8_t get_piece(int square) const {
      assert(square >= 0);
      assert(square < 8*sizeof(kind));
      return piece[square];
   }
   side_t get_side(int square) const {
      if (bbc[WHITE].test(square)) return WHITE;
      if (bbc[BLACK].test(square)) return BLACK;
      return NONE;
   }

   bitboard_t<kind> get_occupied() const { return bbc[WHITE] | bbc[BLACK]; }

   int piece_count(int piece, side_t side) const { return (bbc[side] & bbp[piece]).popcount(); }

   int locate_least_valued_piece(bitboard_t<kind> mask) const
   {
      int *perm = piece_types->val_perm;

      for (int n=0; n<piece_types->num_piece_types; n++) {
         if (!(bbp[perm[n]] & mask).is_empty())
            return (bbp[perm[n]] & mask).bitscan();
      }

      return -1;
   }

   void print_move(move_t move) const
   {
      side_t swap_side[3];
      int    swap_piece[3];
      int    swap_to[3];
      int    n;

      /* Second: resolve all pickups */
      n = get_move_pickups(move);
      printf("\n%d pickups\n", n);
      for (int c=0; c<n; c++) {
         uint16_t p  = get_move_pickup(move, c);
         int square  = decode_pickup_square(p);
         int piece   = get_piece(square);
         side_t side = get_side(square);

         printf("%d %s %s %d\n", c, square_names[square], piece_types->piece_abbreviation[piece][side], side);
      }

      /* Third: resolve all swaps */
      n = get_move_swaps(move);
      printf("%d swaps\n", n);
      for (int c=0; c<n; c++) {
         uint16_t p    = get_move_swap(move, c);
         int from      = decode_swap_from(p);
         swap_to[c]    = decode_swap_to(p);
         swap_piece[c] = get_piece(from);
         swap_side[c]  = get_side(from);

         printf("%d %s %s %s\n", c, square_names[from], square_names[swap_to[c]], piece_types->piece_abbreviation[swap_piece[c]][swap_side[c]]);
      }

      /* Fourth: resolve all drops */
      n = get_move_drops(move);
      printf("%d drops\n", n);
      for (int c=0; c<n; c++) {
         uint16_t p  = get_move_drop(move, c);
         int square  = decode_drop_square(p);
         int piece   = decode_drop_piece(p);
         side_t side = decode_drop_side(p);

         printf("%d %s %s %d\n", c, square_names[square], piece_types->piece_abbreviation[piece][side], side);
      }

      /* Fifth: update holdings */
      if ((rule_flags & RF_USE_HOLDINGS) && get_move_holdings(move)) {
         uint16_t p  = get_move_holding(move);
         int count   = decode_holding_count(p);
         int piece   = decode_holding_piece(p);
         side_t side = decode_holding_side(p);
         printf("Holdings %s %d\n", piece_types->piece_abbreviation[piece][side], count);
      }
   }


   void makemove(move_t move, unmake_info_t<kind> *ui)
   {
      side_t swap_side[3];
      int    swap_piece[3];
      int    swap_to[3];
      int    n;

      /* First: backup information for unmake */
      ui->init = init;
      ui->hash = hash;
      ui->board_hash = board_hash;
      ui->fifty_counter = fifty_counter;
      ui->ep = ep;
      ui->ep_victim = ep_victim;
      ui->board_flags = board_flags;
      ui->check_count[WHITE] = check_count[WHITE];
      ui->check_count[BLACK] = check_count[BLACK];
#ifdef DEBUGMODE
      ui->move = move;
#endif
      board_flags &= ~BF_NO_RETALIATE;

      /* Second: resolve all pickups */
      n = get_move_pickups(move);
      for (int c=0; c<n; c++) {
         uint16_t p  = get_move_pickup(move, c);
         int square  = decode_pickup_square(p);
         int piece   = get_piece(square);
         side_t side = get_side(square);

         ui->pickup_piece[c] = piece_for_side(piece, side);
         clear_piece(piece, side, square);
         if ((piece_types->piece_flags[piece] & PF_NO_RETALIATE) && side != side_to_move)
            board_flags |= BF_NO_RETALIATE;
      }

      /* Third: resolve all swaps */
      n = get_move_swaps(move);
      for (int c=0; c<n; c++) {
         uint16_t p    = get_move_swap(move, c);
         int from      = decode_swap_from(p);
         swap_to[c]    = decode_swap_to(p);
         swap_piece[c] = get_piece(from);
         swap_side[c]  = get_side(from);

         if ((piece_types->piece_flags[swap_piece[c]] & PF_NO_RETALIATE) && swap_side[c] == side_to_move)
            board_flags &= ~BF_NO_RETALIATE;
         clear_piece(swap_piece[c], swap_side[c], from);
      }
      for (int c=0; c<n; c++)
         put_piece(swap_piece[c], swap_side[c], swap_to[c]);

      /* Fourth: resolve all drops */
      n = get_move_drops(move);
      for (int c=0; c<n; c++) {
         uint16_t p  = get_move_drop(move, c);
         int square  = decode_drop_square(p);
         int piece   = decode_drop_piece(p);
         side_t side = decode_drop_side(p);

         put_piece(piece, side, square);
      }

      /* Fifth: update holdings */
      if (expect((rule_flags & RF_USE_HOLDINGS) && get_move_holdings(move), false)) {
         uint16_t p  = get_move_holding(move);
         int count   = decode_holding_count(p);
         int piece   = decode_holding_piece(p);
         side_t side = decode_holding_side(p);
         if (count < 0)
            hash ^= hold_key[piece][side][holdings[piece][side]];
         holdings[piece][side] += count;
         if (count > 0)
            hash ^= hold_key[piece][side][holdings[piece][side]];
      }

      /* Sixth: update status bits */
      ep_victim = 0;
      ep.clear();
      if (move & MOVE_SET_ENPASSANT) {
         ep = bitboard_t<kind>::board_between[get_move_from(move)][get_move_to(move)];
         ep_victim = get_move_to(move);
      }

      /* Seventh: flip side to move */
      if (expect((move & MOVE_KEEP_TURN) == 0, true)) {
         side_to_move = next_side[side_to_move];
         hash ^= side_to_move_key;
         board_hash ^= side_to_move_key;
      }

      /* Finally: update 50-move clock */
      fifty_counter++;
      if (move & MOVE_RESET50) fifty_counter = 0;


      /* Assume we're not in check */
      check(false);
   }

   void unmakemove(move_t move, unmake_info_t<kind> *ui)
   {
      side_t swap_side[3];
      int    swap_piece[3];
      int    swap_to[3];
      int    n;

      /* First: flip side to move */
      if (expect((move & MOVE_KEEP_TURN) == 0, true))
         side_to_move = next_side[side_to_move];

      /* Second: reverse all drops */
      n = get_move_drops(move);
      for (int c=0; c<n; c++) {
         uint16_t p  = get_move_drop(move, c);
         int square  = decode_drop_square(p);
         int piece   = decode_drop_piece(p);
         side_t side = decode_drop_side(p);

         clear_piece(piece, side, square);
      }

      /* Third: reverse all swaps */
      n = get_move_swaps(move);
      for (int c=0; c<n; c++) {
         uint16_t p    = get_move_swap(move, c);
         int from      = decode_swap_to(p);
         swap_to[c]    = decode_swap_from(p);
         swap_piece[c] = get_piece(from);
         swap_side[c]  = get_side(from);

         clear_piece(swap_piece[c], swap_side[c], from);
      }
      for (int c=0; c<n; c++)
         put_piece(swap_piece[c], swap_side[c], swap_to[c]);

      /* Fourth: reverse all pickups */
      n = get_move_pickups(move);
      for (int c=0; c<n; c++) {
         uint16_t p  = get_move_pickup(move, c);
         int square  = decode_pickup_square(p);
         int piece   = neutral_piece(ui->pickup_piece[c]);
         side_t side = side_for_piece(ui->pickup_piece[c]);

         put_piece(piece, side, square);
      }

      /* Fifth: update holdings */
      if (expect((rule_flags & RF_USE_HOLDINGS) && get_move_holdings(move), false)) {
         uint16_t p  = get_move_holding(move);
         int count   = decode_holding_count(p);
         int piece   = decode_holding_piece(p);
         side_t side = decode_holding_side(p);
         holdings[piece][side] -= count;
         assert(holdings[piece][side] >= 0);
      }

      /* Finally: restore backedup information */
      init = ui->init;
      hash = ui->hash;
      board_hash = ui->board_hash;
      fifty_counter = ui->fifty_counter;
      ep = ui->ep;
      ep_victim = ui->ep_victim;
      board_flags = ui->board_flags;
      check_count[WHITE] = ui->check_count[WHITE];
      check_count[BLACK] = ui->check_count[BLACK];
   }

   void print(FILE* file = stdout, bitboard_t<kind> xmark = bitboard_t<kind>::board_empty, bitboard_t<kind> omark = bitboard_t<kind>::board_empty, bool ansi = true) const
   {
      const char *bg_colour_string[] = { "\033[45m", "\033[46m", "\033[44m" };
      char mark[256];
      int colour[256];
      int pieces[256];
      bool occupied[256];
      side_t side[256];
      bitboard_t<kind> occ = get_occupied();
      int c, n;

      for (int r=0; r<virtual_ranks; r++) {
         for (int f=0; f<virtual_files; f++) {
            int square = f + r*virtual_files;

            occupied[square] = false;
            colour[square]   = 2;
            pieces[square]   = NONE;
            mark[square]     = ' ';
         }
      }

      for (int r=0; r<bitboard_t<kind>::board_ranks; r++) {
         for (int f=0; f<bitboard_t<kind>::board_files; f++) {
            int bit    = bitboard_t<kind>::pack_rank_file(r, f);
            int square = bit_to_square[bit];

            if (square < 0) continue;

            colour[square] = ((square / virtual_files) ^ (square % virtual_files)) & 1;
            if (!bitboard_t<kind>::board_all.test(bit)) colour[square] = 2;

            if (omark.test(bit)) mark[square] ='*';
            if (xmark.test(bit)) mark[square] ='+';

            if (occ.test(bit)) {
               occupied[square] = true;
               side[square] = BLACK;
               if (bbc[WHITE].test(bit)) side[square] = WHITE;
               pieces[square] = get_piece(bit);
            } else {
               occupied[square] = false;
            }
         }
      }

      if (file != stdout) ansi = false;

      for (c=virtual_ranks-1; c>=0; c--) {
         fprintf(file, "%2s", rank_names[c]);
         if (ansi) ansi_code("\033[1m");
         for (n=0; n<virtual_files; n++) {
            int square = n + c*virtual_files;
            int piece  = pieces[square];
            // 46/45 works for purple/cyan
            // 43/41 works for red/yellow
            if (ansi) ansi_code(bg_colour_string[colour[square]]);
            if (occupied[square]) {
               bool white = (side[square] == WHITE);
               if (ansi) ansi_code(white ? "\033[37m" : "\033[30m");
               fprintf(file, "%-2s", piece_types->piece_abbreviation[piece][1-white]);
            } else {
               if (ansi)
                  fprintf(file, " %c", mark[square]);
               else
                  fprintf(file, "%c%c", colour[square] ? '+' : '.', mark[square]);
            }
         }
         if (ansi) ansi_code("\033[0m");
         if (side_to_move == BLACK && c == virtual_ranks-1) fprintf(file, "*");
         if (side_to_move == WHITE && c == 0) fprintf(file, "*");
         fprintf(file, "\n");
      }
      fprintf(file, "  ");
      for (n=0; n<virtual_files; n++)
         fprintf(file, "%s ", file_names[n]);
      fprintf(file, "\n");

      if (rule_flags & RF_USE_HOLDINGS) {
         for (c=0; c<NUM_SIDES; c++) {
            fprintf(file, "%s holdings [ ", c ? "Black" : "White");
            for (n=0; n<piece_types->num_piece_types; n++)
               if (holdings[n][c])
                  fprintf(file, "%s: %02d ", piece_types->piece_abbreviation[n][c], holdings[n][c]);
            fprintf(file, "]\n");
         }
      }
   }

   void print_bitboards() const
   {
      int c, n;

      if (side_to_move == WHITE)
         printf("White to move\n");
      else
         printf("Black to move\n");

      printf("White pieces\tBlack pieces\tUnmoved pieces\tRoyal\t\tep\t\tepc\n");
      bitboard_t<kind> epbb, epcbb;
      epbb = ep;
      if (ep_victim) epcbb.set(ep_victim);
      for (c=bitboard_t<kind>::board_ranks-1; c>=0; c--) {
         printf("%s", bbc[0].rank_string(c));
         printf("\t");
         printf("%s", bbc[1].rank_string(c));
         printf("\t");
         printf("%s", init.rank_string(c));
         printf("\t");
         printf("%s", royal.rank_string(c));
         printf("\t");
         printf("%s", epbb.rank_string(c));
         printf("\t");
         printf("%s", epcbb.rank_string(c));
         printf("\n");
      }

      if (!piece_types)
         return;

      if (!flag[WHITE].is_empty() || !flag[BLACK].is_empty()) {
         printf("\nWhite flags\tBlack flags\n");
         for (c=bitboard_t<kind>::board_ranks-1; c>=0; c--) {
            printf("%s", flag[WHITE].rank_string(c));
            printf("\t");
            printf("%s", flag[BLACK].rank_string(c));
            printf("\n");
         }
      }

      for (n=0; n<piece_types->num_piece_types; n+=7) {
         printf("\n");
         int k;
         for (k=0; k<7; k++) {
            if (n+k >= piece_types->num_piece_types) break;
            char *s = piece_types->piece_name[n+k];
            printf("%*s", -bitboard_t<kind>::board_files-2, s);
         }
         printf("\n");
         for (c=bitboard_t<kind>::board_ranks-1; c>=0; c--) {
            for (k=0; k<7; k++) {
               if (n+k >= piece_types->num_piece_types) break;
               printf("%s", bbp[n+k].rank_string(c));
               printf("  ");
            }
            printf("\n");
         }
      }

      if (rule_flags & RF_USE_HOLDINGS) {
         for (c=0; c<NUM_SIDES; c++) {
            printf("%s holdings [ ", c ? "Black" : "White");
            for (n=0; n<piece_types->num_piece_types; n++)
               if (holdings[n][c])
                  printf("%s: %02d ", piece_types->piece_abbreviation[n][c], holdings[n][c]);
            printf("]\n");
         }
      }
   }
};


#endif
