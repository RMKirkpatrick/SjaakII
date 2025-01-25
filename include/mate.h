//stage_t generate_staged_moves(stage_t stage, movelist_t *movelist, const board_t<kind> *board, side_t side_to_move) const
// STAGE_DROP=STAGE_START, STAGE_NORMAL,      /* Normal move generation */
// STAGE_CHECKING_DROP, STAGE_CHECKING_MOVE,  /* Mate/Tsume search */
// STAGE_CHECK_EVADE,                         /* Check evasion */
// STAGE_DONE } stage_t;

struct {
   uint32_t lock;
   int16_t score;
   int16_t ply;
} mate_cache[0xFFFF + 1 + 8];

bool probe_mate_cache(int ply, int *score)
{
   int index = board.hash & 0xFFFF;
   uint32_t key = board.hash >> 32;

   for (int n = 0; n<8; n++) {
      if (mate_cache[index + n].lock == key && mate_cache[index + n].ply <= ply) {
         *score = mate_cache[index + n].score;
         return true;
      }
   }

   return false;
}

void store_mate_cache(int ply, int score)
{
   int index = board.hash & 0xFFFF;
   uint32_t key = board.hash >> 32;

   uint32_t okey = key;
   int oply      = ply;
   int oscore    = score;
   for (int n = 0; n<8; n++) {
      uint32_t new_key = mate_cache[index + n].lock;
      int new_ply      = mate_cache[index + n].ply;
      int new_score    = mate_cache[index + n].score;

      mate_cache[index+n].lock  = okey;
      mate_cache[index+n].ply   = oply;
      mate_cache[index+n].score = oscore;

      if (new_key == key) 
         return;

      okey   = new_key;
      oply   = new_ply;
      oscore = new_score;
   }
}


/* Test if the current position is a +mate-in-mply-or-better, using
 * drop-checks.
 * Returns either a mate-score (if mate is found) or not. Matescores can be
 * trusted, non-mate scores indicate a call to reqular qsearch is in order.
 */
int msearch(int alpha, int beta, int mply, int depth, int ply = 0)
{
   /* Check whether our search time for this move has expired */
   check_clock();
   if (abort_search) return 0;

   truncate_principle_variation(depth);

   move_t move = 0;
   int best_score = -ILLEGAL;
   if (ply == 0 && !(mply & 1)) mply--;
   if (mply < 0 || depth >= MAX_SEARCH_DEPTH) return 0;

   /* If the position is repeated, it cannot be part of a line of optimal
    * play.
    */
   if (position_repeated()) return 0;

   /* Test for transposition table cut-offs */
   int hash_depth, hash_score;
   unsigned int hash_flag;
   move_t hash_move = 0;
   bool have_hash = retrieve_table(transposition_table, board.hash, &hash_depth, &hash_score, &hash_flag, &hash_move);
   have_hash = false;
   if (have_hash) {
      hash_score = score_from_hashtable(hash_score, depth);
      bool exact_ok = (depth > 0);
      if (is_mate_score(hash_score)) {
         if (hash_score > alpha) backup_principle_variation(depth, hash_move);

         return hash_score;
      }
      if (hash_depth >= mply)
         return 0;
   }

   if (probe_mate_cache(ply, &best_score)) {
      if (!is_mate_score(best_score))
         return best_score;

      /* Follow the mating path, to reconstruct the like */
   }
   best_score = -ILLEGAL;

   stage_t stage = STAGE_START;
   /* Generate moves */
   if (ply & 1) {     /* Evade */
      assert(board.check());
      stage = STAGE_CHECK_EVADE;
   } else {             /* Begin with a drop */
      if (board.check()) return 0;
      stage = STAGE_CHECKING_DROP;
   }

   move_t best = 0;
   side_t me = board.side_to_move;
   int legal_moves = 0;
   while (stage != STAGE_DONE && (alpha < beta)) {
      stage = movegen.generate_staged_moves(stage, movelist+depth, &board, me);
      legal_moves += movelist[depth].num_moves;

      for (int n = 0; n<movelist[depth].num_moves; n++) {
         move_t move = movelist[depth].move[n];
         movelist[depth].score[n] = 0;
         if (mate_killer[depth] == move) {
            movelist[depth].score[n] += 2100;
         } else if (mate_killer[depth+2] == move) {
            movelist[depth].score[n] += 2050;
         } else if (is_killer(depth, move)) {
            movelist[depth].score[n] += 2000;
            if (move == killer[depth][0])
               movelist[depth].score[n] += 50;
         } else if (is_killer(depth+2, move)) {
            movelist[depth].score[n] += 800;
            if (move == killer[depth+2][0])
               movelist[depth].score[n] += 50;
         } else {
            int s = see(move);
            movelist[depth].score[n] += s;
         }
      }

      move_t move;
      while ((move = movelist[depth].next_move())) {
         playmove(move);
         if (player_in_check(me)) {   /* Illegal move */
            legal_moves--;
            takeback();
            continue;
         }
         board.check(movegen.was_checking_move(&board, board.side_to_move, move));
         if ( !(ply & 1) && !board.check()) {
            takeback();
            continue;
         }
         if ((board.rule_flags & RF_USE_SHAKMATE) && board.check()) test_shak();

         clock.nodes_searched++;
         int score = -msearch(-beta, -alpha, mply-1, depth+1, ply+1);

         if (score < LEGALLOSS)     /* Illegal move, but requires search to identify */
            legal_moves--;
         takeback();

         if (abort_search) return 0;

         if (score > best_score) {
            best_score = score;
            hash_move = move;

            if (score > alpha && score) {
               hash_flag = HASH_TYPE_EXACT;
               best_move[depth] = move;
               backup_principle_variation(depth, move);
               alpha = score;
               if (ply == 0 && alpha >= LEGALWIN - 1000) alpha = beta; /* Cut-off if we found a mate - not necessarily the fastest one*/
               if (alpha >= beta) {
                  store_killer(move, depth);
                  store_mate_killer(move, depth, score);
                  hash_flag = HASH_TYPE_LOWER;
                  break;
               }
            }
         }

         /* Take a cut-off: if we've reached the limit and found at least
          * one evasion, it's not mate.
          */
         if (mply == 0) return 0;
      }

      /* Only consider check-drops in the first leg */
      //if (ply == 0) break;
   }

   /* We only look for mates on odd plies */
   if (legal_moves == 0 && (ply & 1)) {
      best_score = board.check() ? (mate_score + depth)
                                 : (stale_score + depth);

      move_t prev_move = move_list[moves_played-1];

      /* Make sure mate wasn't caused by an illegal drop */
      if (board.check() && is_drop_move(prev_move)) {
         int p = board.get_piece(get_move_to(prev_move));
         if (pt.piece_flags[p] & PF_DROPNOMATE)
            return ILLEGAL;
      }

      /* Test if the mate is legal, that is to say:
       *  - The checking piece is allowed to deliver mate
       *  - The mate follows a sequence of checks at least one of which
       *    makes the mate OK (is classified as "shak").
       * The first is illegal, the second is only a draw.
       */
      if (board.check()) {
         if (!is_valid_mate())
            return ILLEGAL;

         if (board.rule_flags & RF_USE_SHAKMATE) {
            if (!board.have_shak()) best_score = 0;
         }
      }
   }

   if (best_score == -ILLEGAL) best_score = 0;

   store_mate_cache(ply, best_score);

   if (is_mate_score(best_score))   /* Only mate scores are reliable */
      store_table_entry(transposition_table, board.hash, mply, score_to_hashtable(best_score, depth), hash_flag, hash_move);

   return best_score;
}
