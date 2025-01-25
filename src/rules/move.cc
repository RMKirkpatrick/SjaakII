/*  Sjaak, a program for playing chess variants
 *  Copyright (C) 2011  Evert Glebbeek
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "compilerdef.h"
#include "move.h"
#include "squares.h"

char piece_symbol_string[MAX_PIECE_TYPES+1];
char piece_psymbol_string[MAX_PIECE_TYPES+1];
char piece_drop_string[MAX_PIECE_TYPES+1];

const char *move_to_lan_string(move_t move, bool castle_san, bool castle_kxr, char *buffer)
{
   static char static_buffer[64];
   char *s = static_buffer;
   char dash = '-';
   if (buffer)
      s = buffer;
   s[0] = '\0';

   const char *p = "";
   if (is_promotion_move(move) || is_gate_move(move)) {
      p = piece_symbol_string + get_move_promotion_piece(move);
      if (piece_symbol_string[get_move_piece(move)] == '+')
         p = "+";
   }

   if (!is_drop_move(move) && get_move_from(move) == get_move_to(move) && !is_capture_move(move) && !is_castle_move(move) && !is_promotion_move(move) && !is_drop_move(move)) {
      snprintf(s, sizeof static_buffer, "@@@@");
      return s;
   }

   if (is_double_capture_move(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      int first = 1;
      int c1, c2;
      uint16_t p = 0;
   
      if (get_move_swaps(move))
         first = 0;

      p = get_move_pickup(move, first);
      c1 = decode_pickup_square(p);
      p = get_move_pickup(move, first+1);
      c2 = decode_pickup_square(p);
      if (c2 == to) {
         snprintf(s, 256, "%s%s,%s%s", square_names[from], square_names[c1], square_names[c1], square_names[to]);
      } else {
         snprintf(s, 256, "%s%s,%s%s,%s%s", square_names[from], square_names[c1], square_names[c1], square_names[c2], square_names[c2], square_names[to]);
      }
      return s;
   }

   if (is_capture_move(move) && get_move_capture_square(move) != get_move_to(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      int cap = get_move_capture_square(move);
      char fp = piece_symbol_string[get_move_piece(move)];
      if (fp != ' ') {
         snprintf(s, 256, "%s%s,%s%s", square_names[from], square_names[cap], square_names[cap], square_names[to]);
         return s;
      }
   }

   if (is_castle_move(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      if (castle_san) {
         if (unpack_file(to) >= div_file)
            snprintf(s, sizeof static_buffer, "%s", kingside_castle);
         else
            snprintf(s, sizeof static_buffer, "%s", queenside_castle);
      } else if (castle_kxr) {
         snprintf(s, sizeof static_buffer, "%s%s%c", square_names[from], square_names[get_castle_move_from2(move)], tolower(*p));
      } else {
         snprintf(s, sizeof static_buffer, "%s%s%c", square_names[from], square_names[to], tolower(*p));
         if (abs(from-to) == 1) {
            int from2 = get_castle_move_from2(move);
            int to2 = get_castle_move_to2(move);
            snprintf(s, sizeof static_buffer, "%s%s,%s%s", square_names[from], square_names[to], square_names[from2], square_names[to2]);
         }
      }
      if (is_gate_move(move)) {
         if (get_move_drop_square(move) != from) {
            snprintf(s, sizeof static_buffer, "%s%s%c", square_names[get_castle_move_from2(move)], square_names[from], tolower(*p));
         } else {
            snprintf(s, sizeof static_buffer, "%s%s%c", square_names[from], square_names[to], tolower(*p));
         }
      }
      return s;
   } else if (is_drop_move(move)) {
      snprintf(s, sizeof static_buffer, "%c@%s", piece_drop_string[get_move_piece(move)], square_names[get_move_to(move)]);
      /* Promotion drop? */
      uint16_t h  = get_move_holding(move);
      int piece   = decode_holding_piece(h);
      if (piece != get_move_piece(move)) {
         p = piece_symbol_string + get_move_piece(move);
         snprintf(s, sizeof static_buffer, "%c@%s%c", piece_drop_string[piece], square_names[get_move_to(move)], tolower(*p));
      }
      return s;
   } else if (is_pickup_move(move))
      //snprintf(s, sizeof static_buffer, "%c^%s", piece_drop_string[get_move_piece(move)], square_names[get_move_from(move)]);
      snprintf(s, sizeof static_buffer, "@@%s", square_names[get_move_from(move)]);
   else
      snprintf(s, sizeof static_buffer, "%s%s%c", square_names[get_move_from(move)], square_names[get_move_to(move)], tolower(*p));

   //if (is_gate_move(move))
   //   snprintf(s + strlen(s), sizeof static_buffer-strlen(s), "%c", tolower(*p));

   return s;
}

const char *move_to_string(move_t move, char *buffer)
{
   static char static_buffer[256];
   char *s = static_buffer;
   char dash = '-';
   if (buffer)
      s = buffer;
   s[0] = '\0';
   int n = 0;

   /* In normal chess (and Capablanca), "O-O" is king-side castling, "O-O-O" is queen-side.
    * This holds true in FRC and CRC games, but it is not true in Janus chess.
    * This is a problem, because simply testing whether the king starts out on the left-side of the board
    * would break FRC/CRC variants.
    */
   if (is_castle_move(move)) {
      int to = unpack_file(get_move_to(move));
      if (to >= div_file)
         snprintf(s, 256, "%s", kingside_castle);
      else
         snprintf(s, 256, "%s", queenside_castle);

      if (is_gate_move(move)) {
         snprintf(s + strlen(s), 256-strlen(s), "/%c%s", piece_symbol_string[get_move_promotion_piece(move)], square_names[get_move_drop_square(move)]);
      }
      return s;
   }

   int p = get_move_piece(move);
   if (is_drop_move(move)) {
      int to = get_move_to(move);
      char fp = piece_symbol_string[p];
      snprintf(s, 256, "%c@%s", fp, square_names[to]);
      return s;
   }

   if (is_pickup_move(move)) {
      int from = get_move_from(move);
      char fp = piece_symbol_string[p];
      snprintf(s, 256, "%c^%s", fp, square_names[from]);
      return s;
   }

   if (is_double_capture_move(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      char fp = piece_symbol_string[get_move_piece(move)];
      int first = 1;
      int c1, c2;
      uint16_t p = 0;
   
      if (get_move_swaps(move))
         first = 0;

      p = get_move_pickup(move, first);
      c1 = decode_pickup_square(p);
      p = get_move_pickup(move, first+1);
      c2 = decode_pickup_square(p);
      if (c2 == to) {
         snprintf(s, 256, "%c%sx%sx%s", fp, square_names[from], square_names[c1], square_names[to]);
      } else {
         snprintf(s, 256, "%c%sx%sx%s-%s", fp, square_names[from], square_names[c1], square_names[c2], square_names[to]);
      }
      return s;
   }

   if (is_capture_move(move) && get_move_capture_square(move) != get_move_to(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      int cap = get_move_capture_square(move);
      char fp = piece_symbol_string[get_move_piece(move)];
      if (fp != ' ') {
         snprintf(s, 256, "%c%sx%s-%s", fp, square_names[from], square_names[cap], square_names[to]);
         return s;
      }
   }

   /* Normal move or capture */
   if (is_capture_move(move))
      dash = 'x';

   int from, to;
   char fp = '\0', tp = '\0';

   if (is_promotion_move(move))
      tp = piece_symbol_string[get_move_promotion_piece(move)];

   from = get_move_from(move);
   to = get_move_to(move);
   fp = piece_symbol_string[p];

   snprintf(s, 256, "%c%s%c%s%c", fp, square_names[from], dash, square_names[to], tp);
   if (fp == '+') {
      if (tp) tp = '+';
      snprintf(s, 256, "%c%c%s%c%s%c", fp, piece_psymbol_string[p], square_names[from], dash, square_names[to], tp);
   }

   if (is_gate_move(move))
      snprintf(s + strlen(s), 256-strlen(s), "/%c", piece_symbol_string[get_move_promotion_piece(move)]);
   return s;
}

