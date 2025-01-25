move_t move_string_to_move(const char *move_str, const movelist_t *external_movelist) const
{
   const int board_size = bitboard_t<kind>::board_files*bitboard_t<kind>::board_ranks;
   movelist_t movelist;
   int src = 0, dest = 0, ppiece = 0;
   char *s;

   /* First, generate the list of moves for this position */
   if (external_movelist) {
      movelist.clear();
      for(int n = 0; n<external_movelist->num_moves; n++)
         movelist.push(external_movelist->move[n]);
   } else {
      generate_legal_moves(&movelist);
   }

   while (isspace(*move_str) && *move_str) move_str++;
   char ms[16];
   snprintf(ms, sizeof(ms), "%s", move_str);
   s = strstr(ms, "=");
   if (s) {
      while (*s) {
         *s = *(s+1);
         s++;
      }
   }

   for (int n = 0; n<movelist.num_moves; n++) {
      move_t move = movelist.move[n];
      char buffer[128];

      move_to_short_string(move, &movelist, buffer);
      trim(buffer);
      if (strcmp(buffer, ms) == 0) return move;

      move_to_lan_string(move, true, false, buffer);
      trim(buffer);
      if (strcmp(buffer, ms) == 0) return move;

      move_to_lan_string(move, false, true, buffer);
      trim(buffer);
      if (strcmp(buffer, ms) == 0) return move;

      move_to_lan_string(move, false, false, buffer);
      trim(buffer);
      if (strcmp(buffer, ms) == 0) return move;

      move_to_string(move, buffer);
      trim(buffer);
      if (strcmp(buffer, ms) == 0) return move;

      /* Other alternates? *TODO* */
   }

   /* No match, but perhaps the input includes a "+" for a checking move? */
   int l = strlen(ms);
   if (ms[l-1] == '+') {
      ms[l-1] = '\0';
      return move_string_to_move(ms, &movelist);
   }

   return 0;
}

