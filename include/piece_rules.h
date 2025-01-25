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
int add_piece_type(move_flag_t move_flags, move_flag_t capture_flags, piece_flag_t piece_flags,
      const bitboard_t<kind> promotion_zone[2], const char *promotion_string,
      const char *name, const char *symbol, const char *notation, int value = 0)
{
   if (pt.num_piece_types>=MAX_PIECE_TYPES)
      return -1;

   if (!promotion_string) promotion_string = "";
   if (!name) name = "";
   if (!symbol) symbol = "";
   if (!notation) notation = "";

   int n = pt.num_piece_types;

   free(pt.piece_name[n]);
   free(pt.piece_abbreviation[n][WHITE]);
   free(pt.piece_notation[n]);
   free(pt.demotion_string[n]);
   memset(pt.promotion[n], 0, sizeof pt.promotion[n]);

   pt.piece_move_flags[n]     = move_flags;
   pt.piece_capture_flags[n]  = capture_flags;
   pt.piece_flags[n]          = piece_flags;
   pt.special_zone[WHITE][n]  = bitboard_t<kind>::board_empty;
   pt.special_zone[BLACK][n]  = bitboard_t<kind>::board_empty;
   pt.promotion[n][0].zone[WHITE] = promotion_zone[WHITE];
   pt.promotion[n][0].zone[BLACK] = promotion_zone[BLACK];
   pt.promotion[n][0].string = strdup(promotion_string);
   pt.promotion[n][0].choice = 0;
   pt.promotion_zone[WHITE][n].clear();
   pt.promotion_zone[BLACK][n].clear();
   pt.optional_promotion_zone[WHITE][n].clear();
   pt.optional_promotion_zone[BLACK][n].clear();
   pt.entry_promotion_zone[WHITE][n] = bitboard_t<kind>::board_all;
   pt.entry_promotion_zone[BLACK][n] = bitboard_t<kind>::board_all;
   pt.prison[WHITE][n]        = bitboard_t<kind>::board_all;
   pt.prison[BLACK][n]        = bitboard_t<kind>::board_all;
   pt.block[WHITE][n]         = bitboard_t<kind>::board_empty;
   pt.block[BLACK][n]         = bitboard_t<kind>::board_empty;
   pt.drop_zone[WHITE][n]     = bitboard_t<kind>::board_all;
   pt.drop_zone[BLACK][n]     = bitboard_t<kind>::board_all;
   pt.piece_name[n]           = strdup(name);
   pt.piece_notation[n]       = strdup(notation);
   pt.demotion_string[n]      = NULL;
   pt.allowed_victim[n]       = NULL;
   pt.piece_value[n]          = value;
   pt.demotion[n]             = n;
   pt.piece_promotion_choice[n] = 0;
   pt.piece_allowed_victims[n] = ~0;
   pt.pzset[n]                = false;
   pt.piece_drop_file_maximum[n] = 1;
   if (piece_flags & PF_ROYAL) {
      pt.piece_maximum[n][WHITE] = 1;
      pt.piece_maximum[n][BLACK] = 1;
   } else {
      pt.piece_maximum[n][WHITE] = 128;
      pt.piece_maximum[n][BLACK] = 128;
   }

   pt.piece_abbreviation[n][WHITE] = strdup(symbol);
   char *p = strchr(pt.piece_abbreviation[n][WHITE], ',');
   if (p) {
      p[0] = '\0';
      p++;
   } else {
      p = pt.piece_abbreviation[n][WHITE] + strlen(pt.piece_abbreviation[n][WHITE]);
   }
   pt.piece_abbreviation[n][BLACK] = p;

   pt.num_piece_types++;

   piece_symbol_string[n  ] = notation[0];
   piece_symbol_string[n+1] = '\0';

   piece_psymbol_string[n  ] = notation[1];
   piece_psymbol_string[n+1] = '\0';

   piece_drop_string[n  ] = toupper(pt.piece_abbreviation[n][WHITE][0]);
   piece_drop_string[n+1] = '\0';
   return n;
}

bool add_special_move(const char *symbol, const bitboard_t<kind> *zone, move_flag_t move_flags)
{
   int n;

   if (!symbol)
      return false;

   for (n=0; n<pt.num_piece_types; n++) {
      if (strcmp(pt.piece_abbreviation[n][WHITE], symbol) == 0 || strcmp(pt.piece_abbreviation[n][BLACK], symbol) == 0) {
         pt.special_zone[WHITE][n] = zone[WHITE];
         pt.special_zone[BLACK][n] = zone[BLACK];
         pt.piece_special_move_flags[n] = move_flags;
         return true;
      }
   }

   return false;
}

bool add_special_move(const char *symbol, bitboard_t<kind> zone, move_flag_t move_flags)
{
   bitboard_t<kind> bz[2] = { zone, zone };
   return add_special_move(symbol, bz, move_flags);
}

void set_maximum_number_of_kings(side_t side, int count)
{
   for (int n=0; n<pt.num_piece_types; n++)
      if (pt.piece_flags[n] & PF_ROYAL)
         pt.piece_maximum[n][side] = count;
}

bool add_initial_move(const char *symbol, move_flag_t move_flags)
{
   int n;

   if (!symbol)
      return false;

   for (n=0; n<pt.num_piece_types; n++) {
      if (strcmp(pt.piece_abbreviation[n][WHITE], symbol) == 0 || strcmp(pt.piece_abbreviation[n][BLACK], symbol) == 0) {
         pt.piece_initial_move_flags[n] = move_flags;
         return true;
      }
   }

   return false;
}


void set_maximum_number_of_pieces(const char *symbol, side_t side, int count)
{
   for (int n=0; n<pt.num_piece_types; n++) {
      if (strcmp(pt.piece_abbreviation[n][WHITE], symbol) == 0 || strcmp(pt.piece_abbreviation[n][BLACK], symbol) == 0) {
         pt.piece_maximum[n][side] = count;
      }
   }
}

void identify_castle_partner()
{
   /* Identify partner in castling moves (for decoding FENs that don't
    * include the file of the partner in random setups).
    */
   for (int n = 0; n<NUM_CASTLE_MOVES; n++) {
      pt.castle_piece[WHITE][n] = -1;
      pt.castle_piece[BLACK][n] = -1;
   }

   if (!start_fen)
      return;

   /* Setup the initial position */
   setup_fen_position(start_fen, true);

   for (int n = 0; n<NUM_CASTLE_MOVES; n++)
   for (side_t side = WHITE; side<=BLACK; side++) {
      int square = movegen.castle_king_from[n][side];
      if (square < 0) continue;

      if (board.bbc[side].test(square)) {
         int piece = board.get_piece(square);
         pt.piece_flags[piece] |= PF_CASTLE;
      }

      bitboard_t<kind> bb = movegen.castle_mask[n][side];
      bb.reset(square);
      assert(bb.onebit());

      pt.castle_piece[side][n] = board.get_piece(bb.bitscan());
   }

   board.clear();
}

void deduce_castle_flags(side_t side, int king_from, int king_to, int rook_from)
{
   movegen.deduce_castle_flags(side, king_from, king_to, rook_from);
}

void identify_promotion_options()
{
   char buffer[3];
   for (int n = 0; n<pt.num_piece_types; n++) {
      pt.piece_promotion_choice[n] = 0;

      /* Store allowed pieces for promotion in a bit field */
      for (int k=0; k<MAX_PZ; k++) {
      piece_bit_t promotion_choice = 0;
      if (pt.promotion[n][k].string && pt.promotion[n][k].string[0]) {
         const char *s = pt.promotion[n][k].string;
         while (*s) {
            if (*s == '+') {
               buffer[0] = '+';
               buffer[1] = pt.piece_abbreviation[n][WHITE][0];
               buffer[2] = 0;
               if (buffer[1] == '+') {
                  buffer[0] = pt.piece_abbreviation[n][WHITE][1];
                  buffer[1] = 0;
               }
            } else {
               buffer[0] = *s;
               buffer[1] = 0;
               if (s[1] == '~') {
                  buffer[1] = '~';
                  buffer[2] = 0;
               }
            }
            int c;
            for (c=0; c<pt.num_piece_types; c++) {
               if (strcmp(pt.piece_abbreviation[c][WHITE], buffer) == 0 ||
                   strcmp(pt.piece_abbreviation[c][BLACK], buffer) == 0) {
                  promotion_choice |= 1<<c;

                  if (buffer[0] == '+') pt.demotion[c] = n;
                  if (buffer[1] == '~') pt.demotion[c] = n;
                  if (pt.piece_abbreviation[n][WHITE][0] == '+') pt.demotion[c] = n;
               }
            }
            s++;
            if (*s == '~') s++;
         }
      }

      pt.piece_promotion_choice[n] |= promotion_choice;
      pt.promotion[n][k].choice = promotion_choice;
      }
   }

   for (side_t side=WHITE; side<=BLACK; side++)
   for (int n = 0; n<pt.num_piece_types; n++) {
      pt.promotion_zone[side][n].clear();
      for (int k=0; k<MAX_PZ; k++)
         pt.promotion_zone[side][n] |= pt.promotion[n][k].zone[side];
   }

   for (int n = 0; n<pt.num_piece_types; n++) {
      const char *s = pt.demotion_string[n];
      if (pt.demotion_string[n] == NULL) continue;

      for (int c=0; c<pt.num_piece_types; c++) {
         if (strcmp(pt.piece_abbreviation[c][WHITE], s) == 0 || strcmp(pt.piece_abbreviation[c][BLACK], s) == 0) {
            pt.demotion[n] = c;
            break;
         }
      }
   }

   for (int n = 0; n<pt.num_piece_types; n++) {
      const char *s = pt.allowed_victim[n];
      if (s == NULL) continue;

      pt.piece_allowed_victims[n] = 0;
      while (*s) {
         const char *p = s;
         while (*p && *p != ',') p++;
         while (*p == ' ') p++;
         for (int c=0; c<pt.num_piece_types; c++) {
            if (strcmp(pt.piece_abbreviation[c][WHITE], s) == 0 ||
                strcmp(pt.piece_abbreviation[c][BLACK], s) == 0) {
               pt.piece_allowed_victims[n] |= 1<<c;
            }
         }
         s = p+1;
         if (!*p) break;
      }
   }
}
