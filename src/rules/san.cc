#include <cstring>
#include "san.h"
#include "squares.h"
#include "compilerdef.h"

const char *move_to_short_string(move_t move, const movelist_t *movelist, char *buffer, bool san_castle)
{
   static char static_buffer[256];
   char *s = buffer;
   const char *gate_token = "";
   const char *token = "";
   const char *origin = "";
   char piece = ' ';
   char tp = '\0';

   if (!s) s = static_buffer;

   if (move == 0) {
      snprintf(s, sizeof static_buffer, "(pass)");
      return s;
   }

   int to   = get_move_to(move);
   int from = is_drop_move(move) ? to : get_move_from(move);
   int p    = get_move_piece(move);
   piece = piece_symbol_string[p];

   if (san_castle && is_castle_move(move)) {
      int f = unpack_file(to);
      if (f >= div_file)
         snprintf(s, sizeof static_buffer, "%s", kingside_castle);
      else
         snprintf(s, sizeof static_buffer, "%s", queenside_castle);

      if (is_gate_move(move)) {
         snprintf(s + strlen(s), 256-strlen(s), "/%c%s", piece_symbol_string[get_move_promotion_piece(move)], square_names[get_move_drop_square(move)]);
      }
      return s;
   }
   if (abs(from-to) == 1 && is_castle_move(move)) {
      int from2 = get_castle_move_from2(move);
      int to2 = get_castle_move_to2(move);
      snprintf(s, sizeof static_buffer, "%cx%s-%s", piece, square_names[from2], square_names[to]);
      return s;
   }

   if (is_capture_move(move)) token = "x";
   if (is_drop_move(move)) token = "@";
   if (is_promotion_move(move) || is_gate_move(move)) tp = piece_symbol_string[get_move_promotion_piece(move)];
   if (is_gate_move(move)) gate_token = "/";

   if (is_double_capture_move(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      char fp = piece;
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


   if (piece != ' ' && is_capture_move(move) && !is_double_capture_move(move) && get_move_capture_square(move)!=get_move_to(move)) {
      int from = get_move_from(move);
      int to = get_move_to(move);
      int cap = get_move_capture_square(move);
      char fp = piece_symbol_string[get_move_piece(move)];
      /* TODO: disambiguate moves */
      snprintf(s, 256, "%c%sx%s-%s", fp, "", square_names[cap], square_names[to]);
      return s;
   }


   if (is_drop_move(move)) piece = piece_drop_string[p];

   if (is_pickup_move(move)) {
      piece = piece_drop_string[p];
      token = "^"; 
      to = from;
      goto disambiguous;
   }

   if (is_drop_move(move)) goto disambiguous;

   /* Slightly special case: pawn capture */
   if (piece == ' ' && is_capture_move(move)) {
      origin = file_names[unpack_file(from)];
   } else if (movelist) {
      /* The information we have now might be ambiguous - check */
      int count = 0;
      int n;
      for (n=0; n<movelist->num_moves; n++) {
         if (is_drop_move(movelist->move[n])) continue;
         if (is_capture_move(movelist->move[n]) && !is_capture_move(move)) continue;
         if (!is_capture_move(movelist->move[n]) && is_capture_move(move)) continue;
         if (is_double_capture_move(movelist->move[n]) && !is_double_capture_move(move)) continue;
         if (!is_double_capture_move(movelist->move[n]) && is_double_capture_move(move)) continue;
         if (get_move_piece(move) == get_move_piece(movelist->move[n]) && to == get_move_to(movelist->move[n])) {
            if (is_promotion_move(move) && is_promotion_move(movelist->move[n]))
               count += tp == piece_symbol_string[get_move_promotion_piece(movelist->move[n])];
            else if (!is_promotion_move(move) && !is_promotion_move(movelist->move[n])) {
               if (is_gate_move(move) && is_gate_move(movelist->move[n]))
                  count += tp == piece_symbol_string[get_move_promotion_piece(movelist->move[n])];
               else if (!is_gate_move(move) && !is_gate_move(movelist->move[n]))
                  count++;
            }
         }
      }
      if (count <= 1) goto disambiguous;

      /* Try to disambiguate by file */
      count = 0;
      for (n=0; n<movelist->num_moves; n++) {
         if (is_drop_move(movelist->move[n])) continue;
         if (get_move_piece(move) == get_move_piece(movelist->move[n]) &&
             to == get_move_to(movelist->move[n]) &&
             unpack_file(from) == unpack_file(get_move_from(movelist->move[n])))
            count++;
      }
      if (count == 1) {
         origin = file_names[unpack_file(from)];
         goto disambiguous;
      } 

      /* Try to disambiguate by rank */
      count = 0;
      for (n=0; n<movelist->num_moves; n++) {
         if (is_drop_move(movelist->move[n])) continue;
         if (get_move_piece(move) == get_move_piece(movelist->move[n]) &&
             to == get_move_to(movelist->move[n]) &&
             unpack_rank(from) == unpack_rank(get_move_from(movelist->move[n])))
            count++;
      }
      if (count == 1) {
         origin = rank_names[unpack_rank(from)];
         goto disambiguous;
      }

      /* Give up, list whole square */
      origin = square_names[from];
   }
disambiguous:

   snprintf(s, 15, "%c%s%s%s%s%c", piece, origin, token, square_names[to], gate_token, tp);
   if (piece == '+') {
      if (tp) tp = '+';
      snprintf(s, 15, "%c%c%s%s%s%s%c", piece, piece_psymbol_string[p], origin, token, square_names[to], gate_token, tp);
   }

   return s;
}



