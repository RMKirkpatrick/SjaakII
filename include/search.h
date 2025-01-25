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
#include "chase.h"

#define iter if (output_iteration) output_iteration
#define uci  if (uci_output)       uci_output
#define xb   if (xboard_output)    xboard_output

#define check_clock() \
{\
   if (clock.max_nodes && (clock.nodes_searched >= clock.max_nodes)) abort_search |= 1;\
   if (check_keyboard && check_keyboard(this)) abort_search |= 1;\
   if (clock.check_clock && ((clock.nodes_searched & clock_nodes)==0))\
      abort_search |= clock.check_clock(&clock);\
}(void)0

#include "mate.h"

void dump_moves_since_root(void)
{
   for (int n=clock.root_moves_played; n<moves_played; n++)
      printf("%s ", move_to_short_string(move_list[n]));
   printf("\n");
}

/* Test if the checking piece delivers a "shak" mate */
void test_shak(void)
{
   assert(board.check());

   bitboard_t<kind> king = board.bbc[board.side_to_move] & board.royal;
   assert(!king.is_empty());

   bitboard_t<kind> atk = movegen.get_all_attackers(&board, bitboard_t<kind>::board_all, king.bitscan());

   for (int n=0; n<pt.num_piece_types; n++) {
      if ((atk & board.bbc[next_side[board.side_to_move]] & board.bbp[n]).is_empty()) continue;

      if (pt.piece_flags[n] & PF_SHAK) {
         board.shak();
         break;
      }
   }
}


/* Test if the checking piece is allowed to deliver mate */
bool is_valid_mate(void)
{
   assert(board.check());

   bitboard_t<kind> king = board.bbc[board.side_to_move] & board.royal;
   assert(!king.is_empty());

   bitboard_t<kind> atk = movegen.get_all_attackers(&board, bitboard_t<kind>::board_all, king.bitscan());

   for (int n=0; n<pt.num_piece_types; n++) {
      if ((atk & board.bbc[next_side[board.side_to_move]] & board.bbp[n]).is_empty()) continue;

      if (pt.piece_flags[n] & PF_NOMATE)
         return false;
   }

   return true;
}

bool side_captured_flag(side_t side)
{
   if (!(board.rule_flags & RF_CAPTURE_THE_FLAG)) return false;

   bitboard_t<kind> flag = board.flag[side];
   bitboard_t<kind> bb = flag & board.bbc[side];

   if (bb.is_empty()) return false;;

   /* Filter out pieces that are not allowed to capture the flag */
   for (int n=0; n<pt.num_piece_types; n++)
      if (!(pt.piece_flags[n] & PF_CAPTUREFLAG))
         bb &= ~board.bbp[n];

   if (bb.is_empty()) return false;;

   if (board.rule_flags & RF_CAPTURE_ANY_FLAG) return true;
   if (bb == flag) return true;

   return false;
}


bool flags_are_captured()
{
   if (!(board.rule_flags & RF_CAPTURE_THE_FLAG)) return false;

   for (side_t side = WHITE; side<=BLACK; side++) {
      bitboard_t<kind> flag = board.flag[side];
      bitboard_t<kind> bb = flag & board.bbc[side];

      if (bb.is_empty()) continue;

      /* Filter out pieces that are not allowed to capture the flag */
      for (int n=0; n<pt.num_piece_types; n++)
         if (!(pt.piece_flags[n] & PF_CAPTUREFLAG))
            bb &= ~board.bbp[n];

      if (bb.is_empty()) continue;

      if (board.rule_flags & RF_CAPTURE_ANY_FLAG) return true;
      if (bb == flag) return true;
   }

   return false;
}

/* Return the number of repetitions of the current position in the game
 * tree.
 */
int count_repetition()
{
   int n;
   int count;
   count = 0;

   for (n=(int)moves_played-2; n>=0; n-=2) {
      if ((board.rule_flags & RF_USE_CAPTURE) == 0 && is_irreversible_move(move_list[n+1]))
         return count;

      if ((board.rule_flags & RF_USE_CAPTURE) == 0 && is_irreversible_move(move_list[n]))
         return count;

      count += (ui[n].hash == board.hash);
   }

   return count;
}

inline bool position_repeated()
{
   int rep_in_game = 0;
   int count = 0;
   if (repetition_hash_table[board.hash&0xFFFF]>=2) {
      int n;
      for (n=(int)moves_played-2; n>=0; n-=2) {
         if ((board.rule_flags & RF_USE_CAPTURE) == 0 && is_irreversible_move(move_list[n])) break;
         if ((board.rule_flags & RF_USE_CAPTURE) == 0 && is_irreversible_move(move_list[n+1])) break;

         if (ui[n].hash == board.hash) {
            count++;

            rep_in_game += (n <= clock.root_moves_played);
         }
      }
   }

   return count && count >= rep_in_game;
}

inline bool board_position_repeated_loss()
{
   if (board_repetition_hash_table[board.board_hash&0xFFFF]>=2) {
      const side_t me = board.side_to_move;
      bool pure_loss = true;
      bool pure_gain = true;
      int8_t holdings[MAX_PIECE_TYPES][NUM_SIDES];
      memset(holdings, 0, sizeof holdings);

      for (int n=(int)moves_played-1; n>=0; n--) {
         move_t move = move_list[n];

         if ((board.rule_flags & RF_USE_CAPTURE) == 0 && is_irreversible_move(move)) break;

         if (get_move_holdings(move)) {
            uint16_t p  = get_move_holding(move);
            int count   = decode_holding_count(p);
            int piece   = decode_holding_piece(p);
            side_t side = decode_holding_side(p);

            holdings[piece][side] += count;
         }

         if (ui[n].board_hash == board.board_hash && ui[n].hash != board.hash) {
            for (int piece = 0; piece<pt.num_piece_types; piece++) {
               if (holdings[piece][me] < 0) pure_gain = false;
               if (holdings[piece][me] > 0) pure_loss = false;
            }

            if (pure_gain) return false;
            return pure_loss;
         }
      }
   }

   return false;
}

inline bool material_draw()
{
   if (board.rule_flags & RF_CAPTURE_THE_FLAG) return false;

   if (lone_king(WHITE) && lone_king(BLACK)) return true;

   /* TODO */

   return false;
}

inline bool lone_king(side_t side)
{
   return  (board.royal & board.bbc[side]) == board.bbc[side] && board.bbc[side].onebit();
}

inline bool zugzwang_threat()
{
   for (side_t side = WHITE; side < NUM_SIDES; side++) {
      /* If one side has a lone king, there is a zugzwang threat */
      if (lone_king(side)) return true;
   }

   int tempo_movers = 0;
   /* If the side to move only has pieces that cannot make a tempo move,
    * ditto.
    * FIXME: currently, we only detect the presence of pieces that cannot
    * return. The correct zugzwang threat detection needs to know if a
    * piece can return to its original square in an odd number of steps.
    */
   for (int n=0; n<board.piece_types->num_piece_types; n++) {
      if (!(board.bbc[board.side_to_move] & board.bbp[n]).is_empty()) {
         if (!(board.piece_types->piece_flags[n] & PF_NORET))
            tempo_movers++;
      }
   }

   return tempo_movers <= 1;
}

bool is_pawn_push(move_t move, side_t side)
{
   if (pt.pawn_pieces & (1 << board.get_piece(get_move_from(move)))) {
      if (is_drop_move(move)) return false;
      if (bitboard_t<kind>::board_homeland[next_side[side]].test(get_move_from(move)))
         return true;
   }
   return false;
}

bool is_recapture_move(move_t move, move_t prev_move)
{
   return is_capture_move(move) &&
          is_capture_move(prev_move) &&
          get_move_to(move) == get_move_to(prev_move);
}

void score_moves(int depth, move_t hash_move, move_t prev_move, move_t threat_move)
{
   const side_t me = board.side_to_move;

   for (int n = 0; n<movelist[depth].num_moves; n++) {
      move_t move = movelist[depth].move[n];
      movelist[depth].score[n] = 0;
      if (move == hash_move) {
         movelist[depth].score[n] = LEGALWIN;
         if (best_move[depth] == 0)
            best_move[depth] = hash_move;
      } else if (is_promotion_move(move)) {
         movelist[depth].score[n] += 2200 + pt.piece_value[get_move_promotion_piece(move)] / 10;
      } else if (mate_killer[depth] == move) {
         movelist[depth].score[n] += 2100;
      } else if (mate_killer[depth+2] == move) {
         movelist[depth].score[n] += 2050;
      } else if (is_killer(depth, move)) {
         movelist[depth].score[n] += 2000;
         if (move == killer[depth][0])
            movelist[depth].score[n] += 50;
      } else if (depth > 2 && is_killer(depth-2, move)) {
         movelist[depth].score[n] += 900;
         if (move == killer[depth-2][0])
            movelist[depth].score[n] += 50;
      } else if (is_killer(depth+2, move)) {
         movelist[depth].score[n] += 800;
         if (move == killer[depth+2][0])
            movelist[depth].score[n] += 50;
      } else if (is_counter(depth, prev_move, move)) {
         movelist[depth].score[n] += 1500;
      } else if (is_castle_move(move)) {
         movelist[depth].score[n] += 600;
      } else if (is_capture_move(move)) {
         int s = see(move);
         if (s > 0)
            movelist[depth].score[n] += 2400 + s / 10;
         else
            movelist[depth].score[n] += s;

         if (threat_move && !is_drop_move(threat_move) && get_move_to(move) == get_move_from(threat_move) && s < 0)
            movelist[depth].score[n] += 1000;
      } else if (is_pickup_move(move)) {
         if (threat_move && get_move_to(threat_move) == get_move_from(move)) {
            movelist[depth].score[n] += 1500;
         } else {
            movelist[depth].score[n] += 0;
         }
      } else if (is_drop_move(move)) {
         int history_score = get_move_history_score(move);
         int history_scale = get_move_history_scale(move);
         int s = see(move);
         if (s > 0) {
            int h = history_scale ? 100 * history_score / history_scale : 100;
            if (!(board.bbc[me] & board.royal & bitboard_t<kind>::neighbour_board[get_move_to(move)]).is_empty())
               movelist[depth].score[n] += 1200 + h;
            else if (bitboard_t<kind>::board_homeland[next_side[me]].test(get_move_to(move)))
               movelist[depth].score[n] += 1100 + h;
            movelist[depth].score[n] += s / 10;
         } else {
            int h = history_scale ? 500 * history_score / history_scale : 500;
            movelist[depth].score[n] += s + h;
         }
      } else if (max_history && !board.check() && !is_promotion_move(move) && !is_capture_move(move)) {
         int history_score = get_move_history_score(move);
         int history_scale = get_move_history_scale(move);
         if (history_scale) {
            int s = 500 * history_score / history_scale;
            if (s) {
               movelist[depth].score[n] = 0 + s;
            } else {
               s = see(move);
               movelist[depth].score[n] += s;
            }
         }

         if (threat_move && get_move_from(move) == get_move_to(threat_move))
            movelist[depth].score[n] += 500;
      } else {
         int s = see(move);
         movelist[depth].score[n] += s;

         if (threat_move && get_move_from(move) == get_move_to(threat_move))
            movelist[depth].score[n] += 500;
      }

      if (is_gate_move(move))
         movelist[depth].score[n] += 200;

      if (depth < 3 && clock.root_moves_played < 10 && random_ok) {
         movelist[depth].score[n] += genrandui() & 0xff;
      }
   }
}

int qsearch(int alpha, int beta, int draft, int depth)
{
   int score = -LEGALWIN;
   side_t me = board.side_to_move;

   truncate_principle_variation(depth);

   /* Check whether our search time for this move has expired */
   check_clock();
   if (abort_search) return 0;

   if (!board.check() && (board.rule_flags & RF_FORCE_CAPTURE) && (draft < -2)) {
      return static_qsearch(beta, depth+1);
   }

   if ((board.rule_flags & RF_ALLOW_PICKUP) && (draft < -2))
      return static_qsearch(beta, depth+1);

   if (depth >= MAX_TOTAL_DEPTH)
      return static_qsearch(beta, depth+1);

   int static_score = 0;
   if (!board.check()) {
      static_score = static_evaluation<false>(me, alpha, beta);

      /* Stand-pat cut-off */
      if (static_score >= beta)
         return static_score;

      if (static_score > alpha)
         alpha = static_score;
   }

   /* Generate moves */
   movelist[depth].clear();
   movegen.generate_moves(&movelist[depth], &board, me, true, pt.deferral_allowed);

   if (movelist[depth].num_moves == 0)
      return static_score;

   /* Store the previous move: this is used to change move ordering on this
    * move. If there is no previous move, then this move will be null.
    */
   assert(moves_played > 0);
   move_t prev_move = move_list[moves_played-1];

   /* Score and sort the moves */
   for (int n = 0; n<movelist[depth].num_moves; n++) {
      move_t move = movelist[depth].move[n];
      /* TODO: LVA/MVV or SEE ordering */
      //movelist[depth].score[n] = see(move);
      movelist[depth].score[n] = move_mvvlva(move);
      if (is_promotion_move(move))
         movelist[depth].score[n] += pt.piece_value[get_move_promotion_piece(move)];
   }

   /* Search all other moves, until we find a cut-off
    * Explicitly cut quiet moves when not in check; the move generator
    * still produces some of these if promotions are optional.
    */
   int legal_moves = movelist[depth].num_moves;
   int best_score = score;
   int best_score2 = best_score;
   move_t move;
   while (alpha < beta && (move = movelist[depth].next_move())) {
      if (!board.check()) {
         int ms = see(move);//movelist[depth].get_move_score();
         if (ms < 0) continue;
         if (!is_promotion_move(move) && !is_capture_move(move)) continue;
         if (is_drop_move(move)) continue;
      }
      if (is_pickup_move(move)) continue;
      playmove(move);
      if (player_in_check(me)) {
         legal_moves--;
         takeback();
         continue;
      }
      board.check(movegen.was_checking_move(&board, board.side_to_move, move));
      clock.nodes_searched++;
      score = -qsearch(-beta, -alpha, draft-1, depth+1);
      takeback();

      if (score > best_score) {
         best_score2 = best_score;
         best_score = score;
      }

      if (score > alpha) { /* New best line */
         alpha = score;
         backup_principle_variation(depth, move);
      }
   }

   if (board.rule_flags & RF_ALLOW_PICKUP) best_score = best_score2;
   if (best_score == -LEGALWIN) best_score = static_score;

   if (board.check() && legal_moves == 0) {
      best_score = (mate_score + depth);

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

      /* Verify that all moves during QS were check moves, so we know that
       * all evasions were examined.
       */
      int c = 2;
      for (int n=draft+2; n<=0; n+=2) {
         if ((ui[moves_played-c].board_flags & BF_CHECK) == 0)
            best_score = static_score;
         c+=2;
      }
   }
      
   return best_score;
}

/* Idea taken from Senpai */
int static_qsearch(int beta, int depth)
{
   int static_score = static_evaluation<false>(board.side_to_move);
   int best_score = static_score;

   if (static_score >= beta)
      return static_score;

   movelist[depth].clear();
   movegen.generate_moves(&movelist[depth], &board, board.side_to_move, true, pt.deferral_allowed);

   bitboard_t<kind> cdone;
   for (int n=0; n<movelist[depth].num_moves; n++) {
      move_t move = movelist[depth].move[n];
      if (!is_capture_move(move) && !is_promotion_move(move)) continue;

      /* Skip captures that are part of a sequence we already investigated */
      if (cdone.test(get_move_to(move))) continue;
      cdone.set(get_move_to(move));

      int value = see(move);
      //if (is_promotion_move(move) && value > 0)
      //   value += pt.piece_value[get_move_promotion_piece(move)] - pt.piece_value[board.get_piece(get_move_to(prev_move))];
      int score = static_score + value;

      if (score > best_score) {
         best_score = score;

         if (best_score >= beta)
            break;
      }
   }

   return best_score;
}

int get_extension(move_t /* move */, int move_score)
{
   /* Check extension: only for safe checks.
    * TODO: exempt dicovered checks
    */
   return board.check() && move_score >= 0;//see(move) >= 0;
}

int get_reduction(move_t move, int move_score, int /* move_count */, int draft)
{
   int r = 0;

   if (move_score > 1500) return 0;

   r = 1;
   if (is_drop_move(move) && move_score < 1000) r++;
   if (is_pickup_move(move) && move_score < 1000) r++;
   if (r && draft > 4) r += draft / 4;

   return r;
}

int search(int alpha, int beta, int draft, int depth)
{
   assert(beta > alpha);
   assert(depth >= 0);
   int best_score = -LEGALWIN;
   int score = -LEGALWIN;
   int moves_searched = 0;
   const side_t me = board.side_to_move;
   move_t prev_move = 0;
   move_t move = 0;

   truncate_principle_variation(depth);

   /* Check whether our search time for this move has expired */
   check_clock();
   if (abort_search) return 0;

   /* Check whether we're looking for a checkmate.
    * If so, prune branches that are worse than the best mate found so far
    * (mate distance pruning).
    */
   alpha = std::max(alpha, -LEGALWIN + depth);
   beta  = std::min(beta,   LEGALWIN - depth - 1);
   if (alpha >= beta)
      return alpha;

   assert(alpha >= -LEGALWIN);
   assert(beta  <=  LEGALWIN);

   if (flags_are_captured())
      return flag_score + depth;

   /* We still need to return a move at the root, even if the
    * heuristic says the current position is a repetition draw!
    * If it were, we would not get here.
    */
   if (depth > 0) {
      if (fifty_limit && board.fifty_counter >= fifty_limit) {
         if (!board.check())      /* defer the 50-move claim when in check, in case it's mate */
            return LEGALDRAW;
      }

      if (position_repeated()) {
         if (board.rule_flags & RF_USE_CHASERULE) {
            if (depth > 1)
            switch (test_chase()) {
               case NO_CHASE:
                  break;

               case DRAW_CHASE:
                  return LEGALDRAW;

               case WIN_CHASE:
                  if (beta == alpha+1)
                     return ILLEGAL;
                  break;

               case LOSE_CHASE:
                  return -ILLEGAL;
                  break;
            }
         } else {
            if (board.check()) {
               if (perpetual == LEGALDRAW)
                  return -perpetual;

               if (perpetual == -ILLEGAL)
                  return -perpetual;

               return -(perpetual + depth);
            }

            if (rep_score == LEGALDRAW)
               return rep_score;

            /* Allow check evasions to repeat */
            if (rep_score == -ILLEGAL) {
               if ((ui[moves_played-1].board_flags & BF_CHECK) == 0) /* HACK: allow check evasion */
                  return -rep_score;
            } else {
               return -(rep_score + depth);
            }
         }
      } else if ((board.rule_flags & RF_USE_CAPTURE) && board_position_repeated_loss() && depth) {
         return alpha-1;
      }

      if (material_draw())
         return LEGALDRAW;

      if (board.rule_flags & RF_USE_BARERULE) {
         if (lone_king(next_side[me])) {
            /* We know that not both sides have a bare king (otherwise it
             * would already have been declared a legal draw) and it is our
             * turn, so if the enemy could have captured our last piece he
             * clearly failed to do so - in other words, we have won.
             */
            return -bare_king_score;
         }
      }

      if (check_limit && board.check_count[board.side_to_move] >= check_limit)
         return (check_score == LEGALDRAW) ? check_score
                                           : check_score + depth;
   }

   /* Test if the last move was an illegal promotion */
   if ((board.rule_flags & RF_QUIET_PROMOTION) && moves_played > 0 && is_promotion_move(move_list[moves_played-1]) ) {
      if (board.check()) return ILLEGAL;
      int square = get_move_to(move_list[moves_played-1]);
      int p = board.get_piece(square);
      bitboard_t<kind> atk = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[p], square, board.get_occupied(), next_side[board.side_to_move]);
      if (!(atk & board.bbc[board.side_to_move]).is_empty()) return ILLEGAL;
   }

   //if ( (depth == 0 || depth == 1 || draft > 8 || is_mate_score(beta)) && (board.rule_flags & RF_USE_DROPS)) {
   bool mate_search = (depth == 1) ||
                      (depth == 0 && best_move[0] != 0) ||
                      (draft > 8);  
   if ( mate_search && ((option_ms >= MATE_SEARCH_ENABLED) || ((board.rule_flags & RF_USE_DROPS) && option_ms >= MATE_SEARCH_ENABLE_DROP)) ) {
      int xply  = (depth < 2) ? draft / 4 : 0;
      int mply  = (depth == 0 && best_move[0] == 0) ? 1 : (3 + 2*xply);
      int score = msearch(alpha, beta, mply, depth);

      if (is_mate_score(score))
         return score;
   }

   if (draft <= 0 || depth > MAX_SEARCH_DEPTH)
      return qsearch(alpha, beta, 0, depth);

   /* Test for transposition table cut-offs */
   int hash_depth = 0, hash_score = 0;
   unsigned int hash_flag = 0;
   move_t hash_move = 0;
   bool have_hash = retrieve_table(transposition_table, board.hash, &hash_depth, &hash_score, &hash_flag, &hash_move);
   bool hash_ok   = hash_depth >= draft || is_mate_score(hash_score);
   if (have_hash && hash_ok && (fifty_limit == 0 || board.fifty_counter < fifty_scale_limit) && depth > 0) {
      hash_score = score_from_hashtable(hash_score, depth);
      //positions_in_hashtable++;
      //bool exact_ok = (depth > 1) || (beta == alpha+1);
      bool exact_ok = (beta == alpha+1) || (hash_score <= alpha) || hash_score >= beta;     // FIXME: use crafty-like PV table
      if ((hash_flag & HASH_TYPE_EXACT) && exact_ok) {
         if (hash_score > alpha) backup_principle_variation(depth, hash_move);

         return hash_score;
      } else if ((hash_flag & (HASH_TYPE_UPPER|HASH_TYPE_EXACT)) && (hash_score<=alpha)) {
         return hash_score;
      } else if ((hash_flag & (HASH_TYPE_LOWER|HASH_TYPE_EXACT)) && (hash_score>=beta)) {
         return hash_score;
      }
   }

   /* Store the previous move: this is used to change move ordering on this
    * move. If there is no previous move, then this move will be null.
    */
   if (moves_played > 0)
      prev_move = move_list[moves_played-1];

   /* Pre-conditions for particular pruning options. */
   bool razor = draft <= 3 &&
                !is_mate_score(beta) &&
                !board.check();

   bool avoid_null = beta > alpha+1 ||
                     board.check() ||
                     prev_move == 0 ||
                     zugzwang_threat() ||
                     is_mate_score(beta);

   /* Calculate static evaluation if needed */
   bool need_static = razor || !avoid_null;
   int static_score = 0;
   if (need_static) {
      static_score = static_evaluation<false>(me, alpha, beta);
      avoid_null = avoid_null || static_score < beta;
   }

   /* Razoring */
   if (razor) {
      assert(need_static);
      int score = static_score - draft * draft * 50;

      if (score >= beta)
         return score;
   }

   /* Null-move */
   move_t threat_move = 0;
   if (!avoid_null) {
      assert(need_static);
      assert(!player_in_check(me));
      int r = 2 + draft / 4;
      int score;
      playmove(0);
      clock.nodes_searched++;
      if (draft - r < 1)
         score = -static_qsearch(-(beta-1), depth+1);
      else
         score = -search(-beta, -(beta-1), draft - 1 -r, depth+1);
      takeback();

      if (score >= beta && !is_mate_score(score) && !abort_search) {// && draft >= 3) {
         if (abs(score) < LEGALWIN)
         store_table_entry(transposition_table, board.hash, draft - 1 - r, score_to_hashtable(score, depth), HASH_TYPE_LOWER, 0);
         return score;
      }
      threat_move = best_move[depth+1];
      if (!is_capture_move(threat_move) && !is_mate_score(score))
         threat_move = 0;
   }

   /* Internal iterative deepening */
   if (hash_move == 0 && beta>alpha+1 && draft > 3) {
      int score = search(alpha, beta, draft-2, depth);
      if (score > alpha)
         hash_move = best_move[depth];
   }

   /* Generate all moves */
   movelist[depth].clear();
   movegen.generate_moves(&movelist[depth], &board, me, false, pt.deferral_allowed);
   int legal_moves = movelist[depth].num_moves;

   /* No moves generated at all? */
   if (legal_moves == 0)
      goto no_moves;

   /* Score and sort the moves */
   score_moves(depth, hash_move, prev_move, threat_move);

   /* Search the first move */
   hash_flag = HASH_TYPE_UPPER;
   while ((move = movelist[depth].next_move())) {
      /* Multi-pv mode */
      if (multipv > 1 && depth == 0 && exclude.contains(move)) continue;
      int move_score = movelist[depth].get_move_score();
      playmove(move);
      if (player_in_check(me)) {   /* Illegal move */
         legal_moves--;
         takeback();
         continue;
      }
      clock.nodes_searched++;
      board.check(movegen.was_checking_move(&board, board.side_to_move, move));
      if ((board.rule_flags & RF_USE_SHAKMATE) && board.check()) test_shak();
      int e = get_extension(move, move_score);
      score = -search(-beta, -alpha, draft-1 + e, depth+1);

      if (score < LEGALLOSS) {   /* Illegal move, but requires search to identify */
         legal_moves--;
         takeback();
         continue;
      }

      if (score > alpha && !abort_search) {                      /* New best line found */
         hash_flag = HASH_TYPE_EXACT;
         alpha = score;

         backup_principle_variation(depth, move);
      }
      takeback();
      moves_searched++;
      break;
   }

   /* Search all other moves, until we find a cut-off */
   best_score = score;
   hash_move = move;
   while (alpha < beta && (move = movelist[depth].next_move())) {
      if (multipv > 1 && depth == 0 && exclude.contains(move)) continue;
      bool in_check = board.check();

      int move_score = movelist[depth].get_move_score();

      /* Prune unsafe drops near the tips */
      if (is_drop_move(move)) {
         if (draft < 3 && move_score < 500 && !in_check) continue;
         //if (draft < 2 && !in_check && !is_mate_score(beta)) continue;
         if (depth == 0 && draft == 1 && moves_searched) continue;
      }
      if (draft < 3 && is_pickup_move(move)) continue;

      playmove(move);
      if (player_in_check(me)) {
         legal_moves--;
         takeback();
         continue;
      }
      board.check(movegen.was_checking_move(&board, board.side_to_move, move));
      if ((board.rule_flags & RF_USE_SHAKMATE) && board.check()) test_shak();
      clock.nodes_searched++;

      /* Futility pruning of drop moves in frontier nodes */
      if (draft == 1 && is_drop_move(move) && !in_check && !board.check()) {
         takeback();
         continue;
      }

      bool avoid_reduction = in_check || board.check() || is_capture_move(move) || is_promotion_move(move) || is_castle_move(move) || is_pawn_push(move, me);

      int e = get_extension(move, move_score);
      int r = (e || avoid_reduction) ? 0 : get_reduction(move, move_score, moves_searched, draft);

      if (depth < 2) {
         score = -search(-beta, -alpha, draft-1 + e-r, depth+1);
         if (score > alpha && score < beta && r)
            score = -search(-beta, -alpha, draft-1 + e, depth+1);
      } else {
         score = -search(-(alpha+1), -alpha, draft-1 + e-r, depth+1);
         if (score < LEGALLOSS) {   /* Illegal move, but requires search to identify */
            legal_moves--;
            takeback();
            continue;
         }
         if (score > alpha && ((beta > alpha+1) || r))
            score = -search(-beta, -alpha, draft-1 + e, depth+1);
      }
      takeback();
      moves_searched++;

      if (abort_search) return 0;

      if (score > best_score) {
         best_score = score;

         if (score > alpha) { /* New best line */
            hash_flag = HASH_TYPE_EXACT;
            alpha = score;
            hash_move  = move;

            backup_principle_variation(depth, move);
         }
      }
   }

   if (alpha >= beta) {        /* Beta cutoff */
      /* Store good moves in the killer slots, but only if:
       *  - we were not in check at the beginning of this move
       *  - the move is not a promotion (already high in the list)
       *  - the move was not a capture (already high in the list)
       */
      store_killer(hash_move, depth);
      store_mate_killer(hash_move, depth, best_score);
      store_counter_move(depth, prev_move, hash_move);
      update_history(move, draft * draft);
      if (prev_move == 0)
         store_null_killer(depth, hash_move);

      hash_flag = HASH_TYPE_LOWER;
      branches_pruned++;

      /* Update history for all other moves that have failed to produce a
       * cut-off, rather than doing this for all moves that do not improve
       * alpha.
       * Doing this here avoids polluting the history tables in ALL-nodes.
       */
      for (int n=0; n<movelist[depth].cur_move-1; n++)
         update_history(movelist[depth].move[n], -draft * draft);
   }

no_moves:
   /* If there are no legal moves, the game is over */
   if (legal_moves == 0) {
      assert(depth > 0);
      hash_flag = HASH_TYPE_EXACT;
      hash_move = 0;
      best_score = board.check() ? (mate_score + depth)
                                 : (stale_score + depth);

      /* Make sure mate wasn't caused by an illegal drop */
      if (board.check() && is_drop_move(prev_move)) {
         int p = board.get_piece(get_move_to(prev_move));
         if (pt.piece_flags[p] & PF_DROPNOMATE)
            return ILLEGAL;
      }

      if (board.bbc[board.side_to_move].is_empty())
         best_score = (no_piece_score < 0) ? no_piece_score + depth
                                           : no_piece_score - depth;

      if (best_score == depth) hash_flag = HASH_TYPE_UPPER;

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

   assert(best_score > -LEGALWIN || legal_moves == 0);
   assert(hash_move || legal_moves == 0);

   best_move[depth] = hash_move;

   /* store evaluation in the transposition table */
   //if (hash_flag & HASH_TYPE_UPPER)
   //   hash_move = max_nodes_move;
   if (legal_moves)
   store_table_entry(transposition_table, board.hash, draft, score_to_hashtable(best_score, depth), hash_flag, hash_move);

   return best_score;
}

void print_principle_variation(int d = 0)
{
   int c;
   int root_move_number = start_move_count;

   if (board.side_to_move) {
      iter(" %d. ...", (int)(root_move_number+moves_played)/2+1);
      root_move_number += 1;
   }
   for (c=0; c<length_of_variation[d]; c++) {
      movelist_t movelist;
      generate_legal_moves(&movelist);
      if (!movelist.contains(principle_variation[c][0])) {
         length_of_variation[d] = c;
         break;
      }
      if (board.side_to_move == WHITE)
         iter(" %d.", (int)(root_move_number+moves_played)/2+1);
      iter(" %-6s", move_to_short_string(principle_variation[c][d], &movelist, NULL, castle_san_ok));
      playmove(principle_variation[c][d]);
   }
   for (c=0; c<length_of_variation[d]; c++)
      takeback();
}

void print_principle_variation_xb(void)
{
   int c;
   int root_move_number = start_move_count;

   if (board.side_to_move == BLACK) {
      xb(" %d. ...", (int)(root_move_number+moves_played)/2+1);
      root_move_number += 1;
   }
   for (c=0; c<length_of_variation[0]; c++) {
      movelist_t movelist;
      generate_legal_moves(&movelist);
      if (!movelist.contains(principle_variation[c][0])) {
         length_of_variation[0] = c;
         break;
      }
      if (board.side_to_move == WHITE)
         xb(" %d.", (int)(root_move_number+moves_played)/2+1);
      xb(" %s", move_to_short_string(principle_variation[c][0], &movelist, NULL, castle_san_ok));
      playmove(principle_variation[c][0]);
   }
   for (c=0; c<length_of_variation[0]; c++)
      takeback();
}


void print_principle_variation_uci()
{
   int c;

   for (c=0; c<length_of_variation[0]; c++) {
      uci(" %s", move_to_lan_string(principle_variation[c][0], false));
   }
}

void store_principle_variation(int score, int depth)
{
   if ((score - depth) < -LEGALWIN) depth = LEGALWIN + score;
   if ((score + depth) >  LEGALWIN) depth = LEGALWIN - score;
   for (int c=0; c<length_of_variation[0]; c++) {
      store_table_entry(transposition_table, board.hash, depth-c, score_to_hashtable(score, c), HASH_TYPE_EXACT, principle_variation[c][0]);
      playmove(principle_variation[c][0]);

      score = -score;
   }
   for (int c=0; c<length_of_variation[0]; c++)
      takeback();
}

play_state_t get_game_end_state(movelist_t *movelist = NULL)
{
   movelist_t static_movelist;
   if (movelist == NULL) movelist = &static_movelist;

   test_move_game_check();
   if ((board.rule_flags & RF_USE_SHAKMATE) && board.check()) test_shak();

   /* Count a number of obvious things: repetition draw, 50 move rule */
   if (fifty_limit && board.fifty_counter >= fifty_limit) {
      /* Take care: we are allowed to mate on the 50th move */
      if (board.check()) {
         generate_legal_moves(movelist);
         if (movelist->num_moves)
            return SEARCH_GAME_ENDED_50_MOVE;
      } else
         return SEARCH_GAME_ENDED_50_MOVE;
   }

   if (count_repetition() >= repeat_claim && !(board.rule_flags & RF_USE_CHASERULE))
      return SEARCH_GAME_ENDED_REPEAT;

   if (flags_are_captured())
      return SEARCH_GAME_ENDED_FLAG_CAPTURED;

   if (material_draw())
      return SEARCH_GAME_ENDED_INSUFFICIENT;

   generate_legal_moves(movelist);

   /* No legal moves? */
   if (movelist->num_moves == 0) {
      if (board.bbc[board.side_to_move].is_empty())
         return SEARCH_GAME_ENDED_NOPIECES;

      if (!board.check())
         return SEARCH_GAME_ENDED_STALEMATE;

      /* Make sure mate wasn't caused by an illegal drop or a piece that is
       * not allowed to deliver mate.
       */
      if (moves_played) {
         move_t prev_move = move_list[moves_played-1];
         int p = board.get_piece(get_move_to(prev_move));
         if (is_drop_move(prev_move)) {
            if (pt.piece_flags[p] & PF_DROPNOMATE)
               return SEARCH_GAME_ENDED_FORFEIT;
         }

         if (!is_valid_mate())
            return SEARCH_GAME_ENDED_FORFEIT;
      }

      /* Test if the mate is legal, that is to say:
       *  - The checking piece is allowed to deliver mate
       *  - The mate follows a sequence of checks at least one of which
       *    makes the mate OK (is classified as "shak").
       * The first is illegal, the second is only a draw.
       */
      if (board.rule_flags & RF_USE_SHAKMATE)
         if (!board.have_shak()) return SEARCH_GAME_ENDED_INADEQUATEMATE;

      return SEARCH_GAME_ENDED_MATE;
   }

   if (board.rule_flags & RF_USE_BARERULE) {
      side_t other = next_side[board.side_to_move];

      if (lone_king(board.side_to_move)) {
         /* Can we capture the last piece? */
         if ((board.bbc[other] & ~board.royal).onebit()) {
            for (int n = 0; n<movelist->num_moves; n++) {
               if (is_capture_move(movelist->move[n]))
                  return SEARCH_OK;
            }

            /* Apparently not - we've lost! */
            return SEARCH_GAME_ENDED_LOSEBARE;
         }
      }

      if (lone_king(other))
         return SEARCH_GAME_ENDED_WINBARE;
   }

   if (check_limit && board.check_count[board.side_to_move] >= check_limit)
      return SEARCH_GAME_ENDED_CHECK_COUNT;

   return SEARCH_OK;
}

void score_static_moves(movelist_t *movelist)
{
   const side_t me = board.side_to_move;
   movelist_t omoves;
   bitboard_t<kind> threat;
   int threat_see = 0;

   playmove(0);
   generate_legal_moves(&omoves);
   for (int n = 0; n<movelist->num_moves; n++) {
      move_t move = movelist->move[n];
      if (is_capture_move(move)) {
         int s = see(move);
         if (s > threat_see) {
            threat.clear();
            threat.set(get_move_to(move));
            threat_see = s;
         }
      }
   }
   takeback();

   int phase = 0;
   for (side_t side = WHITE; side<NUM_SIDES; side++) {
      for (int n=0; n<pt.num_piece_types; n++) {
         int *perm = pt.val_perm;
         int piece = perm[n];
         phase += pt.phase_weight[piece] * board.holdings[piece][side];
         phase += pt.phase_weight[piece] * (board.bbp[piece] & board.bbc[side]).popcount();
      }
   }

   for (int n = 0; n<movelist->num_moves; n++) {
      move_t move = movelist->move[n];
      movelist->score[n] = 0;
      if (is_promotion_move(move)) {
         int s = see(move);
         if (s >= 0)
            movelist->score[n] += 2200 + s + pt.piece_value[get_move_promotion_piece(move)];
         else
            movelist->score[n] += s;
      } else if (is_castle_move(move)) {
         movelist->score[n] += 600;
      } else if (is_capture_move(move)) {
         int s = see(move);
         if (s > 0)
            movelist->score[n] += 2400 + s / 10;
         else
            movelist->score[n] += s;
      } else if (is_drop_move(move)) {
         int s = see(move);
         if (s >= 0) {
            if (!(board.bbc[me] & board.royal & bitboard_t<kind>::neighbour_board[get_move_to(move)]).is_empty())
               movelist->score[n] += 1200;
            else if (bitboard_t<kind>::board_homeland[next_side[me]].test(get_move_to(move)))
               movelist->score[n] += 1100;
            movelist->score[n] += s / 10;
         } else {
            movelist->score[n] += s;
         }
      } else {
         int s = see(move);
         movelist->score[n] += s;
      }

      int piece = get_move_piece(move);
      int pst = centre_table[get_move_to(move)] - (is_drop_move(move) ? 0 : centre_table[get_move_from(move)]);
      if (pt.royal_pieces & (1<<piece)) {
         movelist->score[n] += (-pst*4*phase + pst*(pt.phase_scale - phase)) / pt.phase_scale;
      } else {
         movelist->score[n] += pst;
      }

      /* Penalise moves that just unmake the last move */
      if (moves_played >= 2) {
         move_t prev_move = move_list[moves_played-2];

         if (!is_drop_move(move) && !is_drop_move(prev_move) && get_move_from(move) == get_move_to(prev_move)) {
            movelist->score[n] -= 10;
            if (get_move_to(move) == get_move_from(prev_move))
               movelist->score[n] -= 100;
         }
      }

      /* Push passed pawns */
      if (pt.pawn_pieces & (1<<piece)) {
         pawn_structure_t<kind> ps;
         calculate_pawn_structure(&ps);

         if (!is_drop_move(move) && ps.passed.test(get_move_from(move)))
            movelist->score[n] += 50;
      }

      /* TODO:
       *  - Close with defending king in end game.
       *  - Some sense of game phase (for pushing pawns)
       *  - Some sort of threat detection: double attacks, attacking pinned
       *    pieces, and the like.
       */

      if (threat_see && !is_drop_move(move) && !threat.test(get_move_from(move)))
         movelist->score[n] -= threat_see;


      if (is_gate_move(move))
         movelist->score[n] += 200;

      playmove(move);
      generate_legal_moves(&omoves);
      if (player_in_check(next_side[me])) {
         movelist->score[n] += 100;
         if (omoves.num_moves == 0)
            movelist->score[n] += 2000;
      } else {
         if (omoves.num_moves == 0)
            movelist->score[n] = -50;
      }
      takeback();

      movelist->score[n] *= 8;
      movelist->score[n] += genrandui() & 0xf;

   }
   //movelist->show();
}

play_state_t think(int max_depth)
{
   play_state_t state;
   uint64_t start_time;
   bool mate_search = false;
   movelist_t movelist;
   int depth;

   clock.extra_time = 0;
   clock.root_moves_played = (int)moves_played;
   clock.nodes_searched = 0;

   branches_pruned = 0;
   abort_search = false;

   /* Start the clock */
   start_time = get_timer();
   start_clock(&clock);

   /* Test if the game has ended */
   state = get_game_end_state(&movelist);
   if (!repetition_claim && state == SEARCH_GAME_ENDED_REPEAT)
      state = SEARCH_OK;
   if (state != SEARCH_OK)
      return state;

   for (side_t side = WHITE; side<=BLACK; side++)
      if ((board.bbc[side] & ~board.royal).is_empty()) mate_search = true;

   /* Level of play */
   switch (level) {
      /* Pure random mover */
      case LEVEL_RANDOM:
         playmove(movelist.move[int(genrandf()*movelist.num_moves)]);
         return SEARCH_OK;
         break;

      case LEVEL_STATIC:
         score_static_moves(&movelist);
         //movelist.show();
         movelist.rewind();
         playmove(movelist.next_move());
         return SEARCH_OK;
         break;

      /* Normal alpha/beta search */
      default:
         break;
   }

   /* Prepare the transpostion table for a new search iteration:
    * Resets the write count and increases the generation counter.
    */
   if (!analysing)
      prepare_hashtable_search(transposition_table);

   int hash_depth, hash_score;
   uint32_t hash_flag;
   retrieve_table(transposition_table, board.hash, &hash_depth, &hash_score, &hash_flag, &best_move[0]);

   xb("# Begin iterative deepening loop for position \"%s\"\n", make_fen_string());

   /* Iterative deepening loop */
   int e = board.check();
   clock.extra_time = clock.time_left;
   best_move[0] = 0;
   int score = search(-LEGALWIN, LEGALWIN, 1, 0);
   clock.extra_time = 0;
   if (abort_search) {
      xb("# Aborted ply 1 search - no move!\n");
      if (best_move[0] == 0) {
         xb("# Pick random move\n");
         best_move[0] = movelist.move[0];
      }

      length_of_variation[0] = 1;
      principle_variation[0][0] = best_move[0];
      xb("% 3d % 5d %6d %9d ", 1, score, (int)(get_timer()-start_time)/10000, (int)clock.nodes_searched);
      print_principle_variation_xb();
      xb("\n");
   }
   move_t move = best_move[0];
   if (!abort_search)
   for (depth=2; depth<=max_depth; depth++) {
      scale_history();
      movelist.rewind();
      exclude.clear();

      for (int pv=0; pv<multipv; pv++) {
         if (pv > movelist.num_moves || exclude.num_moves >= movelist.num_moves) break;
         for (int window = 10; window < 2*LEGALWIN; window *= 2) {
            if (depth <= 2) window = (LEGALWIN+1000) / (depth - 1);
            if (mate_search && !is_mate_score(score)) window = LEGALWIN;
            int alpha = std::max(score - window, -LEGALWIN);
            int beta  = std::min(score + window,  LEGALWIN);

            int new_score = search(alpha, beta, depth + e, 0);
            clock.extra_time = 0;

            if (abort_search) break;
            score = new_score;

            if (score > alpha && score < beta) {
               move = best_move[0];
               break;
            }

            /* Report a fail-low */
            if (score < alpha && move) {
               move_t move = best_move[0];
               if (show_fail_low) {
                  iter("% 3d.  %6.2f   %9d  %+2.2f  ", depth, (get_timer()-start_time)/1000000.0, (int)clock.nodes_searched, score/100.0);
                  iter(" %d.", (int)(start_move_count + moves_played)/2+1);
                  if (board.side_to_move) iter(" ...");
                  iter(" %s?", move_to_short_string(move, &movelist, NULL, castle_san_ok));
                  iter("\n");

                  xb("% 3d % 5d %6d %9d ", depth, score, (int)(get_timer()-start_time)/10000, (int)clock.nodes_searched);
                  xb(" %d.", (int)(start_move_count + moves_played)/2+1);
                  if (board.side_to_move) xb(" ...");
                  xb(" %s?", move_to_short_string(move, &movelist, NULL, castle_san_ok));
                  xb("\n");
               }
               clock.extra_time = (9*clock.time_left)/10;
            }


            /* Report a fail-high */
            if (score >= beta) {
               move = best_move[0];
               if (show_fail_high) {
                  iter("% 3d.  %6.2f   %9d  %+2.2f  ", depth, (get_timer()-start_time)/1000000.0, (int)clock.nodes_searched, score/100.0);
                  iter(" %d.", (int)(start_move_count + moves_played)/2+1);
                  if (board.side_to_move) iter(" ...");
                  iter(" %s!", move_to_short_string(move, &movelist, NULL, castle_san_ok));
                  iter("\n");

                  xb("% 3d % 5d %6d %9d ", depth, score, (int)(get_timer()-start_time)/10000, (int)clock.nodes_searched);
                  xb(" %d.", (int)(start_move_count + moves_played)/2+1);
                  if (board.side_to_move) xb(" ...");
                  xb(" %s!", move_to_short_string(move, &movelist, NULL, castle_san_ok));
                  xb("\n");
               }
               clock.extra_time = (5*clock.time_left)/10;
            }
         }
         iter("% 3d.  %6.2f   %9d  %+2.2f  ", depth, (get_timer() - start_time)/1000000.0, (int)clock.nodes_searched, score/100.0);
         print_principle_variation();
         iter("\n");

         xb("% 3d % 5d %6d %9d ", depth, score, (int)(get_timer()-start_time)/10000, (int)clock.nodes_searched);
         print_principle_variation_xb();
         xb("\n");

         uci("info depth %d score cp %d nodes %d time %d hashfull %d pv",
               depth, score, (int)clock.nodes_searched, (int)peek_timer(&clock), (int)std::min(1000, (int)(transposition_table->write_count*1000/transposition_table->number_of_elements)));
         print_principle_variation_uci();
         if (multipv>1) { uci(" multipv %d", pv+1); }
         uci("\n");

         store_principle_variation(score, depth);
         exclude.push(principle_variation[0][0]);
      }

      if (is_mate_score(score) && (LEGALWIN - abs(score)) == length_of_variation[0] && move && !analysing) break;
      if (abort_search) break;
      if (movelist.num_moves == 1 && move && !pondering && !analysing) break;

      /* Check if we have enough time for the next iteration, assuming an
       * effective branching ratio of 2.
       */
      if ( clock.check_clock && peek_timer(&clock) > get_chess_clock_time_for_move(&clock)/2 && move)
         break;
   }

   while (analysing && !abort_search)
      check_clock();

   while (pondering && !abort_search)
      check_clock();

   if (score < resign_threshold)
      resign_count++;
   else
      resign_count = 0;

   if (abs(score) < draw_threshold)
      draw_count++;
   else
      draw_count = 0;

   if (moves_played > 400)
      draw_count += 50;

   //printf("# %d [draw %d / %d] [resign %d / %d]\n", score, draw_threshold, draw_count, resign_threshold, resign_count);

   assert(move);
   playmove(move);

   ponder_move = 0;
   if (length_of_variation[0] > 2)
      ponder_move = principle_variation[1][0];

   return SEARCH_OK;
}

bool ponder()
{
   move_t ponder_move = this->ponder_move;

   /* Verify that the ponder move is legal.
    * There is a corner-case where the ponder move is illegal if the game
    * was terminated by 50-move or three-fold repetition.
    * There may be a more elegant way to resolve this, but this works.
    */
   movelist_t movelist;
   generate_legal_moves(&movelist);
   pondering = false;
   for (int n = 0; n<movelist.num_moves; n++) {
      if (movelist.move[n] == ponder_move) {
         pondering = true;
         break;
      }
   }
   if (!pondering)
      return false;

   xb("Hint: %s\n", move_to_lan_string(ponder_move));
   playmove(ponder_move);

   /* Search the current position */
   if (think(MAX_SEARCH_DEPTH) == SEARCH_OK)
      takeback();

   while (pondering && !abort_search)
      check_clock();

   takeback();
   pondering = false;

   this->ponder_move = ponder_move;

   return true;
}

bool analyse()
{
   move_t ponder_move = this->ponder_move;

   if (analyse_move) return false;

   analyse_move = 0;
   generate_legal_moves(&analyse_movelist);

   /* Search the current position */
   abort_search = false;
   while (analysing && !abort_search) {
      if (think(MAX_SEARCH_DEPTH) == SEARCH_OK)
         takeback();
      else {
         while (analysing && !abort_search)
            check_clock();
      }
   }
   if (analyse_move) playmove(analyse_move);
   analyse_move = 0;

   this->ponder_move = ponder_move;

   return true;
}

#undef iter
#undef uci
#undef xb
#undef check_clock
