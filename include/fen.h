void record_castle_state(char state, bitboard_t<kind> *castle_init)
{
   /* Deal with different type of castle flags */
   int board_files = bitboard_t<kind>::board_files;
   int board_ranks = bitboard_t<kind>::board_ranks;

   int rook_file = -1;
   int rook_from = -1;

   if (state == '-') return;
   if (!isalpha(state)) return;

   side_t side = BLACK;
   if (isupper(state)) side = WHITE;

   /* Fischer-random style castling. This identifies the file of the piece with which
    * we can castle. We can find the file of the king by looking at the royal bitboard, and we
    * know the king destination after castling from the rule description. From this, we can work
    * out the required bitmasks.
    */
   int king_from = -1;
   for (int n = 0; n<pt.num_piece_types; n++) {
      if (pt.piece_flags[n] & PF_CASTLE) {
         king_from = (board.bbc[side] & board.bbp[n]).bitscan();
         break;
      }
   }
   if (king_from < 0) {
      /* No castling, but we should still mark virgin pieces */
      bitboard_t<kind> side_mask = bitboard_t<kind>::board_rank[(side == WHITE) ? 0 : board_ranks-1];
      char file_char = (side == WHITE) ? 'A' : 'a';
      int file = state - file_char;
      if (file >= board_files || file < 0) return;

      *castle_init |= bitboard_t<kind>::board_file[file] & side_mask;

      return;
   }
   int king_rank = unpack_rank(king_from);

   char file_char = (side == WHITE) ? 'A' : 'a';

   if (state-file_char < board_files) {
      rook_file = state - file_char;
      rook_from = bitboard_t<kind>::pack_rank_file(king_rank, rook_file);

      castle_init->set(rook_from);

      if (board.rule_flags & RF_GATE_DROPS) return;
   }

   switch(state) {
      case 'K':
         /* Starting at the right edge of the board, find the first castle
          * piece.
          */
         state = '-';
         for (int n = board_files - 1; n>=0; n--) {
            int sqr = bitboard_t<kind>::pack_rank_file(king_rank, n);
            if (board.get_piece(sqr) == pt.castle_piece[side][SHORT]) {
               rook_from = sqr;
               state = file_char + n;
               break;
            }

         }
         if (state != 'K')
         record_castle_state(state, castle_init);
         break;

      case 'Q':
         /* Starting at the left edge of the board, find the first castle
          * piece.
          */
         state = '-';
         for (int n = 0; n<board_files; n++) {
            int sqr = bitboard_t<kind>::pack_rank_file(king_rank, n);
            if (board.get_piece(sqr) == pt.castle_piece[side][LONG]) {
               rook_from = sqr;
               state = file_char + n;
               break;
            }

         }
         if (state != 'Q')
         record_castle_state(state, castle_init);
         break;

      case 'k':
         /* Starting at the right edge of the board, find the first castle
          * piece.
          */
         state = '-';
         for (int n = board_files - 1; n>=0; n--) {
            int sqr = bitboard_t<kind>::pack_rank_file(king_rank, n);
            if (board.get_piece(sqr) == pt.castle_piece[side][SHORT]) {
               rook_from = sqr;
               state = file_char + n;
               break;
            }

         }
         if (state != 'k')
         record_castle_state(state, castle_init);
         break;

      case 'q':
         /* Starting at the left edge of the board, find the first castle
          * piece.
          */
         state = '-';
         for (int n = 0; n<board_files; n++) {
            int sqr = bitboard_t<kind>::pack_rank_file(king_rank, n);
            if (board.get_piece(sqr) == pt.castle_piece[side][LONG]) {
               rook_from = sqr;
               state = file_char + n;
               break;
            }

         }
         if (state != 'q')
         record_castle_state(state, castle_init);
         break;
   }

   if (rook_from < 0) return;

   int castle_side = LONG;
   if (unpack_file(rook_from) >= unpack_file(king_from)) castle_side = SHORT;

   bitboard_t<kind> king_dest = movegen.castle_king_dest[castle_side][side];
   movegen.clear_castle_rule(castle_side, side);
   while (!king_dest.is_empty()) {
      int king_to = king_dest.bitscan();
      king_dest.reset(king_to);

      //printf("%d (%s %s) (%s)\n", side, square_names[king_from], square_names[king_to], square_names[rook_from]);
      movegen.deduce_castle_flags(side, king_from, king_to, rook_from);

      castle_init->set(king_from);
   }

   return;
}

void setup_fen_position(const char *str, bool skip_castle = false)
{
   const char *s = str;
   int prev_rank = 2*bitboard_t<kind>::board_files;
   int square = top_left;
   int n;

   if (!s) return;
   moves_played = 0;
   board.clear();
   memset(repetition_hash_table, 0, sizeof repetition_hash_table);
   memset(board_repetition_hash_table, 0, sizeof board_repetition_hash_table);

   /* Parse first record: piece positions */
   while(*s && (*s != ' ') && (*s != '[') && square>=0) {
      switch (*s) {
         case '/':
            square -= prev_rank;
            break;
         case '*':
            square++;
            break;
         case '1':
            if (isdigit(s[1])) {
               square += 10;
               break;
            }
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
         case '0':
            square += (*s - '1')+1;
            break;

         default:
            side_t side = NONE;
            int piece = -1;
            int best_len = 0;
            for (n=0; n<pt.num_piece_types; n++) {
               /* Determine piece colour */
               side_t match_side = NONE;
               if (strstr(s, pt.piece_abbreviation[n][WHITE]) == s) match_side = WHITE;
               if (strstr(s, pt.piece_abbreviation[n][BLACK]) == s) match_side = BLACK;

               if (match_side != NONE) {
                  int len = (int)strlen(pt.piece_abbreviation[n][match_side]);
                  if (len > best_len) {
                     best_len = len;
                     side = match_side;
                     piece = n;
                  }
               }
            }
            if (piece == -1) {
               if (error_output) error_output("Error: unknown piece type '%c' (bad FEN %s)\n", *s, str);
               if (xboard_output) xboard_output("tellusererror unknown piece type '%c' (bad FEN %s)\n\n", *s, str);
               return;
            }
            assert(side != NONE);
            assert(square_to_bit[square] >= 0);
            board.put_new_piece(piece, side, square_to_bit[square]);
            s += strlen(pt.piece_abbreviation[piece][side])-1;
            square++;
            break;
      }
      s++;
   }

   /* Optional: check for holdings */
   while(*s && (*s == ' ')) s++;
   if (*s == '[') {
      s++;
      while (*s != ']') {
         for (n=0; n<pt.num_piece_types; n++) {
            /* Determine piece colour */
            side_t side = NONE;
            if (strchr(pt.piece_abbreviation[n][WHITE], *s)) side = WHITE;
            if (strchr(pt.piece_abbreviation[n][BLACK], *s)) side = BLACK;
            if (side != NONE) {
               board.holdings[n][side]++;
               break;
            }
         }
         s++;
      }
      s++;
   }

   /* Second record: side to move */
   while(*s && (*s == ' ')) s++;
   board.side_to_move = WHITE;
   if (*s == 'b') {
      board.side_to_move = BLACK;
      //board.hash ^= side_to_move_key;
   }
   if (*s) s++;

   /* Initialise attack map */
   //initialise_attack_tables(&board);

   /* Third record: castling rights/init state.
    * Skip if the next entry is a number, in which case the game doesn't have castling.
    */
   while(*s && (*s == ' ')) s++;
   if (!isdigit(*s)) {

      /* Clear castle flags by default */
      for (int n = 0; n<pt.num_piece_types; n++) {
         if (!(pt.pawn_pieces & (1<<n)))
            board.init &= ~board.bbp[n];
      }

      while(*s && (*s != ' ')) {
         bitboard_t<kind> castle_init;
         if (!skip_castle)
            record_castle_state(*s, &castle_init);
         s++;
         board.init |= castle_init;
      }
      if (*s) s++;
   }

   /* Make sure the initial flags are at least somewhat sane by making sure only occupied squares have their
    * init bits set.
    */
   board.init &= board.get_occupied();

   /* Fourth record: En-passant square
    * If this record is a number, then the game doesn't have en-passant capture and we skip it.
    */
   while(*s && (*s == ' ')) s++;
   if (!isdigit(*s)) {
      if (*s && (*s != '-')) {
         int file = *s - 'a';
         int rank = s[1] - '1';
         s+=2;
         /* En-passant move-to square */
         board.ep.set(bitboard_t<kind>::pack_rank_file(rank, file));
         /* En-passant capture square, this may be encoded in the FEN */
         if (*s && !isspace(*s)) {
            int file = *s - 'a';
            int rank = s[1] - '1';
            s+=2;
            board.ep_victim = bitboard_t<kind>::pack_rank_file(rank, file);
         } else {
            /* Assume we have normal pawns, in which case we can simply derive it from the move-to square */
            if (board.side_to_move == WHITE)
               board.ep_victim = board.ep.bitscan() - bitboard_t<kind>::board_files;
            else
               board.ep_victim = board.ep.bitscan() + bitboard_t<kind>::board_files;
         }
      }
      while(*s && (*s != ' ')) s++;
   }

   /* Fifth record: half-move counter (50 move counter) */ 
   while(*s && (*s == ' ')) s++;
   n = 0;
   sscanf(s, "%d\n", &n);
   board.fifty_counter = n;
   while(*s && (*s != ' ')) s++;

   /* Sixth record: full-move counter */
   while(*s && (*s == ' ')) s++;
   n = 0;
   sscanf(s, "%d\n", &n);
   start_move_count = 2*std::max(0, n-1);
   while(*s && (*s != ' ')) s++;

   repetition_hash_table[board.hash&0xFFFF] = 1;
   board_repetition_hash_table[board.board_hash&0xFFFF] = 1;

   /* Record check state */
   board.check(player_in_check(board.side_to_move));
}

const char *make_fen_string(char *buffer = NULL) const
{
   static char static_buffer[4096];
   bitboard_t<kind> occ;
   char *fen = buffer;
   int n = 0;
   int r, f;

   if (!fen) fen = static_buffer;
   fen[0] = '\0';

   occ = board.bbc[WHITE] | board.bbc[BLACK];

   /* First record: board position */
   /* Scan all ranks */
   for (r = board.virtual_ranks-1; r>=0; r--) {
      int count = 0;
      for (f = 0; f < board.virtual_files; f++) {
         int square = f + r*board.virtual_files;
         int bit    = square_to_bit[square];

         if (bit < 0 || bit_to_square[bit] < 0 || !bitboard_t<kind>::board_all.test(bit)) {
            if (count) n += snprintf(fen+n, 4096 - n, "%d", count);
            count = 0;
            n += snprintf(fen+n, 4096-n, "*");
            continue;
         }

         /* Empty? */
         if (!occ.test(bit)) {
            count++;
            continue;
         }

         /* Not empty, do we have a count? */
         if (count) n += snprintf(fen+n, 4096 - n, "%d", count);
         count = 0;

         /* Print piece */
         side_t side = board.get_side(bit);
         int piece = board.get_piece(bit);

         n += snprintf(fen+n, 4096-n, "%s", pt.piece_abbreviation[piece][side]);
      }
      if (count) n += snprintf(fen+n, 4096 - n, "%d", count);
      if (r) n += snprintf(fen+n, 4096 - n, "/");
   }

   /* Holdings */
   if (board.rule_flags & RF_USE_HOLDINGS) {
      n += snprintf(fen+n, 4096 - n, "[");
      bool empty = true;
      for (side_t side = WHITE; side <= BLACK; side++) {
         for (int piece = 0; piece < pt.num_piece_types; piece++) {
            for (int count = 0; count < board.holdings[piece][side]; count++) {
               empty = false;
               n += snprintf(fen+n, 4096 - n, "%s", pt.piece_abbreviation[piece][side]);
            }
         }
      }
      if (empty) n += snprintf(fen+n, 4096 - n, "-");
      n += snprintf(fen+n, 4096 - n, "]");
   }

   /* Second record: side to move */
   if (board.side_to_move == WHITE)
      n += snprintf(fen+n, 4096 - n, " w ");
   else
      n += snprintf(fen+n, 4096 - n, " b ");

   /* Third record: castling rights
    * TODO: FRC-style strings.
    */
   bool have_castle = false;
   for (int n = 0; n<pt.num_piece_types; n++)
      have_castle = have_castle || (pt.piece_flags[n] & PF_CASTLE);
   if (have_castle) {
      bitboard_t<kind> short_mask, long_mask;
      bool wrote_castle = false;

      for (side_t side = WHITE; side <= BLACK; side++) {
         short_mask = movegen.castle_mask[SHORT][side];
         long_mask  = movegen.castle_mask[LONG][side];

         if (!short_mask.is_empty() && (short_mask & board.init) == short_mask) {
            char cc = 'k';

            if (side == WHITE) cc = toupper(cc);
            n += snprintf(fen+n, 4096 - n, "%c", cc);
            wrote_castle = true;
         }

         if (!long_mask.is_empty() && (long_mask & board.init) == long_mask) {
            char cc = 'q';

            if (side == WHITE) cc = toupper(cc);
            n += snprintf(fen+n, 4096 - n, "%c", cc);
            wrote_castle = true;
         }
      }

      if (!wrote_castle) n += snprintf(fen+n, 4096 - n, "-");
      n += snprintf(fen+n, 4096 - n, " ");
   }

   /* Fourth record: en-passant square */
   bool have_ep = false;
   for (int n = 0; n<pt.num_piece_types; n++)
      have_ep = have_ep || (pt.piece_flags[n] & PF_SET_EP);
   if (have_ep) {
      if (!board.ep.is_empty())
         n += snprintf(fen+n, 4096 - n, "%s ", square_names[board.ep.bitscan()]);
      else
         n += snprintf(fen+n, 4096 - n, "- ");
   }

   /* Fifth and sixth record: half-move counter and full-move counter */
   int par = (moves_played&1) && next_side[board.side_to_move];
   n += snprintf(fen+n, 4096 - n, "%d ", board.fifty_counter);
   n += snprintf(fen+n, 4096 - n, "%d", (int)(start_move_count + moves_played + par)/2 + 1);

   return fen;
}
