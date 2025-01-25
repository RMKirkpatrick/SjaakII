#define HISTORY_MAX 0x4000
#define USE_HISTORY_HEURISTIC

void update_drop_history(move_t move, int score)
{
#ifdef USE_HISTORY_HEURISTIC
   int history_score;

   int piece = get_move_piece(move);
   int side  = board.side_to_move;
   int to    = get_move_to(move);

   drop_history[side][piece][to] += score;
   history_score = abs(drop_history[side][piece][to]);
   if (history_score > max_drop_history[side])
      max_drop_history[side] = history_score;

   /* Rescale as needed */
   if (max_drop_history[side] > HISTORY_MAX) {
      max_drop_history[side] /= 2;
      for (side = 0; side <= 1; side++)
         for (piece=0; piece<pt.num_piece_types; piece++)
            for (to=0; to<bitboard_t<kind>::board_ranks*bitboard_t<kind>::board_files; to++)
               drop_history[side][piece][to] /= 2;
   }
#endif
}

void update_history(move_t move, int score)
{
#ifdef USE_HISTORY_HEURISTIC
   int history_score;

   if (board.check() || is_promotion_move(move) || is_capture_move(move) || is_pickup_move(move) || is_castle_move(move))
      return;

   if (is_drop_move(move)) {
      update_drop_history(move, score);
      return;
   }

   int piece = board.get_piece(get_move_from(move));
   int side  = board.side_to_move;
   int to    = get_move_to(move);

   history[side][piece][to] += score;
   history_score = abs(history[side][piece][to]);
   if (history_score > max_history[side])
      max_history[side] = history_score;

   /* Rescale as needed */
   if (max_history[side] > HISTORY_MAX) {
      max_history[side] /= 2;
      for (side = 0; side <= 1; side++)
         for (piece=0; piece<pt.num_piece_types; piece++)
            for (to=0; to<bitboard_t<kind>::board_ranks*bitboard_t<kind>::board_files; to++)
               history[side][piece][to] /= 2;
   }
#endif
}

int get_move_history_score(const move_t move) const
{
#ifdef USE_HISTORY_HEURISTIC
   int piece = get_move_piece(move);
   int side  = board.side_to_move;
   int to    = get_move_to(move);
   if (is_drop_move(move))
      return drop_history[side][piece][to];
   else
      return history[side][piece][to];
#else
   return 0;
#endif
}

int get_move_history_scale(move_t move) const
{
#ifdef USE_HISTORY_HEURISTIC
   int side = board.side_to_move;
   if (is_drop_move(move))
      return max_drop_history[side];
   else
      return max_history[side];
#else
   return 1;
#endif
}

void clear_history()
{
#ifdef USE_HISTORY_HEURISTIC
   memset(history, 0, sizeof history);
   max_history[0] = max_history[1] = 0;

   memset(drop_history, 0, sizeof drop_history);
   max_drop_history[0] = max_drop_history[1] = 0;
#endif
}

void scale_history()
{
#ifdef USE_HISTORY_HEURISTIC
   /* Scale down history scores */
   for (int side = 0; side <= 1; side++) {
      max_history[side] /= 2;
      max_drop_history[side] /= 2;
      for (int piece=0; piece<pt.num_piece_types; piece++)
         for (int to=0;
         to<bitboard_t<kind>::board_ranks*bitboard_t<kind>::board_files; to++) {
            history[side][piece][to] /= 2;
            drop_history[side][piece][to] /= 2;
         }
   }
#endif
}
