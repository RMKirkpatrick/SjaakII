bitboard_t<kind> get_chased_pieces(int backtrack)
{
   side_t chaser = next_side[board.side_to_move];
   move_t last_move = 0;

   for (int n = 0; n<2*backtrack; n++)
      takeback();

   if (moves_played) last_move = move_list[moves_played-1];

   /* 1. Identify threats:
    *    A "winning capture" is an up-capture, or an equal capture with SEE > 0
    *    A "threat move" is a winning capture, except for captures
    *    - with the king
    *    - with a pawn
    *    - with a defensive piece
    *    - of a pawn on the enemy side of the board
    *    Discard pieces that are pinned - they cannot chase
    *
    *    A threat move is a candidate chase.
    */
   movelist_t chase_candidates, old_threats;
   movegen.generate_chase_candidates(&chase_candidates, &board, chaser);

   //printf("Threats:\n");
   //chase_candidates.print();

   /* Filter out old threats */
   takeback();
   movegen.generate_chase_candidates(&old_threats, &board, chaser);
   for (int n=0; n<chase_candidates.num_moves; n++) {
      move_t chase = chase_candidates.move[n];
      for (int k = 0; k<old_threats.num_moves; k++) {
         if (chase == old_threats.move[k]) {
            chase_candidates.move[n--] = chase_candidates.move[--chase_candidates.num_moves];
            break;
         }
         /* Special case: a piece moved along a ray, preserving a threat */
         if (last_move && get_move_from(old_threats.move[k]) == get_move_from(last_move)) {
            if (get_move_to(old_threats.move[k]) == get_move_to(chase)) {
               chase_candidates.move[n--] = chase_candidates.move[--chase_candidates.num_moves];
               break;
            }
         }
      }
   }
   replaymove();

   /* Equal captures are not chase moves if the reverse capture is also
    * possible. It might not be for lame leapers.
    */
   for (int n=0; n<chase_candidates.num_moves; n++) {
      move_t move = chase_candidates.move[n];
      int from = get_move_from(move);
      int cap  = get_move_capture_square(move);

      if (board.get_piece(from) == board.get_piece(cap)) {
         move_flag_t flags = board.piece_types->piece_capture_flags[n];

         if (is_leaper(flags) && is_masked_leaper(flags)) {
            bitboard_t<kind> occ = board.get_occupied();
            bitboard_t<kind> rev;

            rev = movegen.generate_leaper_move_bitboard(flags, board.side_to_move, cap, occ);

            if (rev.test(from))
               chase_candidates.move[n--] = chase_candidates.move[--chase_candidates.num_moves];   /* No chase */
         } else {
            chase_candidates.move[n--] = chase_candidates.move[--chase_candidates.num_moves];      /* No chase */
         }
      } else {
         int to = get_move_to(move);
         bitboard_t<kind> revatk = movegen.get_all_attackers(&board, board.bbc[board.side_to_move], to);
         revatk &= board.royal;

         if (!revatk.is_empty())
            chase_candidates.move[n--] = chase_candidates.move[--chase_candidates.num_moves];      /* No chase */
      }
   }

   /* Identify chased pieces */
   bitboard_t<kind> chased;
   for (int n=0; n<chase_candidates.num_moves; n++) {
      chased.set(get_move_to(chase_candidates.move[n]));
   }

   /* Follow chased pieces around as they move */
   for (int n = 0; n<2*backtrack; n++) {
      move_t move = move_list[moves_played-1];
      if ( (n & 1) && chased.test(get_move_from(move)) ) {
         chased.reset(get_move_from(move));
         chased.set(get_move_to(move));
      }
      replaymove();
   }

   //printf("Chases:\n");
   //chase_candidates.print();

   return chased;
}

/* The result is one of:
 *  NO_CHASE      - not a chase situation, allowed
 *  DRAW_CHASE    - both sides chase, game is drawn
 *  LOSE_CHASE    - illegal chase, side to move wins
 */
inline chase_state_t test_chase()
{
   assert(moves_played > 0);
   /* TODO */
   int count = count_repetition();

   //printf("Position repeated %d times\n", count);

   int backup = 0;
   for (int n=(int)moves_played-2; n>=0; n-=2) {
      //printf("%3d %016llx %016llx\n", n, ui[n].hash, board.hash);
      if (is_irreversible_move(move_list[n+1])) break;
      if (is_irreversible_move(move_list[n])) break;
      backup+=2;
      if (ui[n].hash == board.hash) break;
   }

   side_t chaser = next_side[board.side_to_move];
   side_t chasee = board.side_to_move;
   bitboard_t<kind> chased_pieces[NUM_SIDES];// = board.bbc[board.side_to_move];
   bool chasing[NUM_SIDES];

   chased_pieces[WHITE] = board.bbc[WHITE];
   chased_pieces[BLACK] = board.bbc[BLACK];
   
   /* Get chased pieces on the current ply */
   chased_pieces[chasee] = board.bbc[chasee];
   for (int n = 0; n<backup/2; n++) {
      bitboard_t<kind> chased = get_chased_pieces(n);
      chased_pieces[chasee] &= chased;
   }
   chasing[chaser] = !chased_pieces[chasee].is_empty();


   /* Other side */
   takeback();
   chased_pieces[chaser] = board.bbc[chaser];
   for (int n = 0; n<backup/2; n++) {
      bitboard_t<kind> chased = get_chased_pieces(n);
      chased_pieces[chaser] &= chased;
   }
   replaymove();
   move_t move = move_list[moves_played-1];
   if ( chased_pieces[chaser].test(get_move_from(move)) ) {
      chased_pieces[chaser].reset(get_move_from(move));
      chased_pieces[chaser].set(get_move_to(move));
   }
   chasing[chasee] = !chased_pieces[chaser].is_empty();

   /* If one side is evading check, then it cannot chase */
   if (!(chased_pieces[WHITE] & board.royal).is_empty()) chasing[WHITE] = false;
   if (!(chased_pieces[BLACK] & board.royal).is_empty()) chasing[BLACK] = false;

   //printf("White chasing (%d)\n", chasing[WHITE]);
   //chased_pieces[BLACK].print();

   //printf("Black chasing (%d)\n", chasing[BLACK]);
   //chased_pieces[WHITE].print();

   //if (!chasing[WHITE] && !chasing[BLACK])
   //   return NO_CHASE;

   if (chasing[WHITE] == chasing[BLACK])
      return DRAW_CHASE;

   if (!chasing[board.side_to_move])
      return WIN_CHASE;

   return LOSE_CHASE;
}

