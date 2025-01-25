/* Store a "counter move"; this is a depth-independent extension of the
 * killer principle.
 */
void inline store_counter_move(int /* depth */, move_t prev_move, move_t move)
{
   if (board.check() || is_promotion_move(move) || is_capture_move(move) || is_pickup_move(move) || !prev_move)
      return;

   int side = board.side_to_move;
   int to   = get_move_to(prev_move);
   int from = is_drop_move(prev_move) ? to : get_move_from(prev_move);
   counter[from][to][side] = move;
}

bool inline is_counter(int /* depth */, move_t prev_move, move_t move) const
{
   int side = board.side_to_move;
   int to   = get_move_to(prev_move);
   int from = is_drop_move(prev_move) ? to : get_move_from(prev_move);
   return counter[from][to][side] == move;
}

bool is_killer(int depth, move_t move) const
{
   if (killer[depth][0] == move || killer[depth][1] == move)
      return true;
   return false;
}

/* Store moves that kill a branch in the killer slots, but only if:
 *  - we were not in check at the beginning of this move
 *  - the move is not a promotion (already high in the tree)
 *  - the move was not a capture (already high in the tree)
 */
inline void store_killer(move_t move, int depth)
{
   if (board.check() || is_promotion_move(move) || is_capture_move(move))
      return;

   if (move == killer[depth][0]) {
      /* The move was the first killer - do nothing */
   } else {
      /* This was either the last killer (out of 2 or 3), or it's a new
       * move. Either way, Degrade first killer to second killer (etc) and
       * store the new first killer.
       */
      killer[depth][1]=killer[depth][0];
      killer[depth][0]=move;
   }
}


inline void store_mate_killer(move_t move, int depth, int best_score)
{
   if (board.check() || is_promotion_move(move) || is_capture_move(move))
      return;

   if (best_score >= (LEGALWIN-1000))
      mate_killer[depth] = move;
}

void store_null_killer(int depth, move_t move)
{
   if (!is_capture_move(move) && !is_promotion_move(move))
      null_killer[depth] = move;
}

