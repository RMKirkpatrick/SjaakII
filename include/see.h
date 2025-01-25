int move_mvvlva(move_t move) const
{
   int value = 0;

   if (is_capture_move(move)) {
      value += pt.see_piece_value[board.get_piece(get_move_capture_square(move))] * 8;
      value -= pt.see_piece_value[board.get_piece(get_move_from(move))] / 8;
   }

   return value;
}

int move_value(move_t move, side_t /* side_to_move */) const
{
   /* Capture a piece */
   int value = 0;
   if (is_drop_move(move))
      return 0;
   if (is_capture_move(move))
      value += pt.see_piece_value[board.get_piece(get_move_capture_square(move))];
   if (is_promotion_move(move)) {
      //value += pt.see_piece_value[get_move_promotion_piece(move)];
      //value -= pt.see_piece_value[board.get_piece(get_move_from(move))];
   }
   /* TODO: gate moves have an extra drop */
   return value;
}

struct {
   move_t move;
   uint32_t lock;
   int score;
} see_cache[0xFFFF + 1 + 8];

bool probe_see_cache(move_t move, int *score)
{
   int index = board.hash & 0xFFFF;
   uint32_t key = board.hash >> 32;

   for (int n = 0; n<8; n++) {
      if (see_cache[index + n].lock == key && see_cache[index + n].move == move) {
         *score = see_cache[index + n].score;
         return true;
      }
   }

   return false;
}

void store_see_cache(move_t move, int score)
{
   int index = board.hash & 0xFFFF;
   uint32_t key = board.hash >> 32;

   uint32_t okey = key;
   move_t omove  = move;
   int oscore    = score;
   for (int n = 0; n<8; n++) {
      uint32_t new_key = see_cache[index + n].lock;
      move_t new_move  = see_cache[index + n].move;
      int new_score    = see_cache[index + n].score;

      see_cache[index+n].lock  = okey;
      see_cache[index+n].move  = omove;
      see_cache[index+n].score = oscore;

      if (new_key == key && new_move == move) 
         return;

      okey   = new_key;
      omove  = new_move;
      oscore = new_score;
   }
}

/* Perform static exchange evaluation for a given capture move.
 * Uses the "swap" algorithm
 * (http://chessprogramming.wikispaces.com/SEE+-+The+Swap+Algorithm).
 * returns the evaluation in centipawns.
 * The move doesn't need to be a capture; if it isn't then this function
 * can be used to find out if it puts a piece "en-prise"
 */
int see(move_t move)
{
   int score[128];
   int depth;
   side_t side    = board.side_to_move;
   int square     = get_move_to(move);
   int piece      = get_move_piece(move);
   int last_piece = piece;
   bitboard_t<kind> attackers, hidden_attackers, pinned;
   bitboard_t<kind> xray_update;
   bitboard_t<kind> own;
   bitboard_t<kind> mask = bitboard_t<kind>::board_all;

   /* Test if this position/move was calculated before and re-use */
   if (probe_see_cache(move, &depth))
      return depth;

   /* Initial gain: the piece on the target square */
   depth = 0;
   score[depth] = move_value(move, board.side_to_move);
   if (is_capture_move(move)) {
      /* Easy: if the piece we use to capture the enemy piece is worth less
       * than the piece we captured, it's always a winning capture, so return a
       * rough estimate for the SEE score.
       */
      if (score[depth] > pt.piece_value[piece])
         return score[depth] - pt.piece_value[piece]/2;

      /* Perform the capture on the board.
       * In this case we need to explicitly clear the capture square because
       * the first move might be an en-passant capture.
       */
      mask.reset(get_move_capture_square(move));
   }

   /* We need to update the list of attackers if the moving piece is a
    * slider or moves along a ray. This includes some types of leaper, but
    * that is currently NOT taken into account.
    * *TODO*
    */
   for (int n = 0; n<pt.num_piece_types; n++) {
      if (is_slider(pt.piece_capture_flags[n]) ||
          is_hopper(pt.piece_capture_flags[n]) ||
          is_stepper(pt.piece_capture_flags[n]))
         xray_update |= board.bbp[n];
   }
   xray_update &= movegen.super_slider[square] | movegen.super_stepper[square];

   /* Get a new list of all attackers/defenders */
   if (!is_drop_move(move))
      mask.reset(get_move_from(move));
   attackers = movegen.get_all_attackers(&board, mask, square);
   hidden_attackers = xray_update;
   hidden_attackers &= ~attackers;

   //pinned = movegen.get_pinned_pieces(&board, next_side[side]) | movegen.get_pinned_pieces(&board, side);
   //attackers &= ~pinned;
   //hidden_attackers &= ~pinned;
   //mask &= ~pinned;

   do {
      assert(depth < 128);
      /* Consider next move - one ply deeper, flip side */
      depth++;
      side = next_side[side];
      /* Update score (speculative) */
      score[depth] = pt.piece_value[last_piece] - score[depth-1];
      /* Perform LVA capture */
      own = attackers & board.bbc[side];
      if (!own.is_empty()) {
         /* Find the least valued attacker */
         int from  = board.locate_least_valued_piece(own);
         int piece = board.get_piece(from);
         last_piece = piece;

         /* Update the list of attackers/defenders */
         attackers.reset(from);
         mask.reset(from);

         if (xray_update.test(from) && !(hidden_attackers & mask).is_empty()) {
            attackers = movegen.get_all_attackers(&board, mask, square);
            hidden_attackers &= ~attackers;
         }
      }
   } while(!own.is_empty());

   /* Compute final score */
   while (--depth)
      score[depth-1] = -std::max(-score[depth-1], score[depth]);

   /* Cache result */
   store_see_cache(move, score[0]);

   return score[0];
}

