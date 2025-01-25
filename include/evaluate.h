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
#if 0
struct pawn_structure_t {
   bitboard_t<kind> open;
   bitboard_t<kind> passed;
   bitboard_t<kind> stop;
   bitboard_t<kind> weak;
   bitboard_t<kind> attacked[NUM_SIDES];
};
#endif

template <typename kind> 
void game_template_t<kind>::calculate_pawn_structure(pawn_structure_t<kind> *ps)
{
   bitboard_t<kind> passed;
   bitboard_t<kind> stop;
   bitboard_t<kind> open;
   bitboard_t<kind> weak;
   bitboard_t<kind> pawns;

   open = bitboard_t<kind>::board_all;

   memset(ps, 0, sizeof *ps);

   for (side_t side=WHITE; side<NUM_SIDES; side++) {
      if (pt.pawn_index[side] < 0) continue;
      pawns |= board.bbp[pt.pawn_index[side]] & board.bbc[side];
   }

   for (side_t side=WHITE; side<NUM_SIDES; side++) {
      if (pt.pawn_index[side] < 0) continue;

      bitboard_t<kind> bb = pawns &  board.bbc[side];
      bitboard_t<kind> ob = pawns & ~board.bbc[side];

      /* Shelter score (0-8) */
      for (int f = 0; f<bitboard_t<kind>::board_files; f++) {
         int lf = (f == 0)       ? f+1 : f-1;
         int rf = (f == files-1) ? f-1 : f+1;
         int r = 0;
         if (side == BLACK) r = bitboard_t<kind>::board_ranks-1;
         int square = pack_rank_file(r, f);

         bitboard_t<kind> mask1 = bitboard_t<kind>::neighbour_board[square];
         bitboard_t<kind> mask2 = bitboard_t<kind>::king_zone[side][square] ^ mask1;
         bitboard_t<kind> mask3 = bitboard_t<kind>::board_file[f];
         bitboard_t<kind> mask4 = bitboard_t<kind>::board_file[lf];
         bitboard_t<kind> mask5 = bitboard_t<kind>::board_file[rf];

         if      (!(bb & mask1 & mask3).is_empty()) ps->shelter_score[side][f] += 4;
         else if (!(bb & mask2 & mask3).is_empty()) ps->shelter_score[side][f] += 2;
         if      (!(bb & mask1 & mask4).is_empty()) ps->shelter_score[side][f] += 2;
         else if (!(bb & mask2 & mask4).is_empty()) ps->shelter_score[side][f] += 1;
         if      (!(bb & mask1 & mask5).is_empty()) ps->shelter_score[side][f] += 2;
         else if (!(bb & mask2 & mask5).is_empty()) ps->shelter_score[side][f] += 1;
      }

      /* Identify passers, open files, candidate weak pawns */
      bitboard_t<kind> bp = bb;
      while(!bp.is_empty()) {
         int square = bp.bitscan();
         bp.reset(square);

         if (pt.piece_promotion_choice[pt.pawn_index[side]] && (pt.passer_mask[side][square] & ob).is_empty())
            passed.set(square);

         open &= ~bitboard_t<kind>::board_file[unpack_file(square)];

         if (!pt.weak_mask[side][square].is_empty() && (pt.weak_mask[side][square] & bb).is_empty())
            weak.set(square);
      }
   }

   ps->passed = passed;
   ps->open   = open;
   ps->stop   = stop;
   ps->weak   = weak;
}

template <typename kind>
template <bool print>
eval_t game_template_t<kind>::static_evaluation(side_t side_to_move, int /* alpha */, int /* beta */)
{
   bitboard_t<kind> moves[8*sizeof(kind)];
   bitboard_t<kind> attack[8*sizeof(kind)];
   bitboard_t<kind> less_attacks[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind> attacks[NUM_SIDES][MAX_PIECE_TYPES];
   bitboard_t<kind> all_attacks[NUM_SIDES];
   bitboard_t<kind> multi_attacks[NUM_SIDES];
   bitboard_t<kind> pawn_attacks[NUM_SIDES];
   bitboard_t<kind> pawns[NUM_SIDES];
   bitboard_t<kind> minors[NUM_SIDES];
   bitboard_t<kind> occ = board.get_occupied();
   bitboard_t<kind> defence, castle;
   bitboard_t<kind> defatk;
   pawn_structure_t<kind> ps;
   bool can_win[NUM_SIDES] = { false, false };
   int mate_potential[NUM_SIDES] = { 0, 0 };
   int num_pieces[NUM_SIDES]     = { 0, 0 };
   int num_royals[NUM_SIDES]     = { 0, 0 };
   int num_pawns[NUM_SIDES]      = { 0, 0 };
   int num_supers[NUM_SIDES]      = { 0, 0 };
   int num_def[NUM_SIDES]        = { 0, 0 };
   int num_light_bound[NUM_SIDES]= { 0, 0 };
   int num_dark_bound[NUM_SIDES] = { 0, 0 };
   uint32_t piece_ids[NUM_SIDES] = { 0, 0 };
   int square_list[8*sizeof(kind)] = {0}, square_count = 0;
   int king[NUM_SIDES] = { -1, -1 };
   int shelter[2] = {0, 0};
   eval_pair_t def[2] =  { 0, 0 };  // Defensive pieces
   eval_pair_t mat = 0;     // Material balance
   eval_pair_t psq = 0;     // Piece square tables
   eval_pair_t mob = 0;     // Mobility
   eval_pair_t pss = 0;     // Pawn structure score
   eval_pair_t kss = 0;     // King safety score
   eval_pair_t tp = 0;      // Tempo bonus  
   eval_t ev = 0;
   eval_t hash_ev = 0;
   eval_t tempo = 0;
   int phase = 0;
   bool symmetric = true;

   bool have_eval_hash = query_eval_table_entry(eval_table, board.hash, &hash_ev);
#ifndef DEBUG_EVHASH
   if (have_eval_hash && !print) {
      ev = hash_ev;
      goto exit;
   }
#endif

   /* Randomised evaluation */
   if (level == LEVEL_BEAL) {
      ev = genrandui() & 0x7ff;
      if (side_to_move == BLACK) ev = -ev;
      goto exit;
   }

   calculate_pawn_structure(&ps);

   for (side_t side = WHITE; side<NUM_SIDES; side++) {
      int *perm = pt.val_perm;
      bitboard_t<kind> less_attack;    /* Accumulate attack bitmask of pieces less valuable than the current piece. */

      if (board.rule_flags & RF_CAPTURE_ALL_FLAG) {
         int flag_count  = board.flag[side].popcount();
         int cflag_count = (board.flag[side] & board.bbc[side]).popcount();
         if (flag_count) {
            psq += 5 * cflag_count*cflag_count / flag_count;
         }
      }

      /* Evaluate pieces and collect mobility and attack information.
       * Do the pieces in order of increasing value, so we can do safe
       * mobility.
       */
      int gate_space = 0;
      if (board.rule_flags & RF_GATE_DROPS) {
         for (int n=0; n<pt.num_piece_types; n++)
            gate_space += board.holdings[n][side];
      }

      /* Space advantage, * http://www.talkchess.com/forum/viewtopic.php?p=609260 */
      psq.mg += PST_SPACE_MG*(bitboard_t<kind>::board_homeland[side] & ~occ).popcount();

      for (int n=0; n<pt.num_piece_types; n++) {
         int piece = perm[n];
         bitboard_t<kind> bb = board.bbc[side] & board.bbp[piece];

         if (board.rule_flags & RF_GATE_DROPS) {
            if (gate_space) {
               int gate_scale = files*(files-1);
               int gate_score = (board.init & ~board.royal & board.bbc[side] & (bitboard_t<kind>::board_south_edge | bitboard_t<kind>::board_north_edge)).popcount();
               int pst = PST_HOLDINGS;

               float scale = (float)gate_score*(gate_score-1) / gate_scale;
               if (gate_score > gate_space+2) {
                  scale = 1.0;
                  phase += pt.phase_weight[piece] * board.holdings[piece][side];
               } else if (gate_score >= gate_space) {
                  gate_scale = gate_score*(gate_score-1) + 1;
                  scale = (float)gate_score*(gate_score-1) / gate_scale;
                  phase += pt.phase_weight[piece] * board.holdings[piece][side];
                  pst = 0;
               } else {
                  phase += (int)(pt.phase_weight[piece] * board.holdings[piece][side] * scale);
                  pst = 0;
               }

               mat += pt.eval_value[piece] * board.holdings[piece][side] * scale;
               psq += (int)(pst * board.holdings[piece][side] * scale);
            }
         } else {
            mat += pt.eval_value[piece] * board.holdings[piece][side];
            psq += PST_HOLDINGS * board.holdings[piece][side];
         }

         if (bb.is_empty()) continue;
         if (!(pt.piece_flags[piece] & PF_ROYAL)) piece_ids[side] |= 1<<piece;

         /* Pair bonus */
         if (pt.piece_flags[piece] & PF_PAIRBONUS) {
            if ( !(bb & bitboard_t<kind>::board_light).is_empty() && !(bb & bitboard_t<kind>::board_dark).is_empty())
               mat += pt.eval_pair_bonus[piece];
            else if ( (pt.defensive_pieces & (1 << piece)) && !bb.onebit())
               mat += pt.eval_pair_bonus[piece];
         }

         /* Piece square tables/material evaluation */
         while(!bb.is_empty()) {
            int square = bb.bitscan();
            bb.reset(square);
            square_list[square_count++] = square;

            if (pt.defensive_pieces & (1<<piece)) {
               def[side] += pt.eval_value[piece];
               defence.set(square);
               num_def[side]++;
            }
            mat += pt.eval_value[piece];

            psq   += pt.eval_pst[piece][psq_map[side][square]];
            phase += pt.phase_weight[piece];

            num_pieces[side]++;
            if (pt.pawn_index[side] == piece) num_pawns[side]++;

            if (pt.piece_flags[piece] & PF_ROYAL) {
               king[side] = square;
               num_royals[side]++;
               castle |= bitboard_t<kind>::board_homeland[side] & pt.prison[side][piece];
            }

            if (pt.piece_flags[piece] & PF_COLOURBOUND) {
               if (bitboard_t<kind>::board_light.test(square))
                  num_light_bound[side]++;
               else
                  num_dark_bound[side]++;
            }

            if (!(pt.piece_flags[piece] & PF_CANTMATE))
               mate_potential[side]++;

            moves[square] = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[piece], square, occ, side);

            /* Collect attack bitmasks
             * TODO: for pawns it is more efficient to do this for all pawns
             * at once (in bulk) after the loop is done.
             */
            bitboard_t<kind> atk = moves[square];
            if (pt.piece_move_flags[piece] != pt.piece_capture_flags[piece])
               atk = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[piece], square, occ, side);
            multi_attacks[side]  |= all_attacks[side] & atk;
            attacks[side][piece] |= atk;
            all_attacks[side]    |= atk;
            attack[square] = atk;
            moves[square] &= ~occ;

            if (pt.defensive_pieces & (1<<piece))
               defatk |= attacks[side][piece];

            if (pt.minor_pieces & (1<<piece))
               minors[side].set(square);

            if (pt.super_pieces & (1<<piece))
               num_supers[side]++;

            if (pt.pawn_pieces & (1<<piece)) {
               pawn_attacks[side] |= atk;
               pawns[side].set(square);
            }

            /* Bonus for attacking flag squares */
            if (!(atk & (board.flag[WHITE]|board.flag[BLACK])).is_empty()) {
               psq += 2 * (atk & (board.flag[WHITE]|board.flag[BLACK])).popcount();
            }

            /* Open files */
            if ((pt.piece_move_flags[piece] & MF_SLIDER_V) && ps.open.test(square)) {
               psq.mg += SLIDER_OPENFILE_MG;
               psq.eg += SLIDER_OPENFILE_EG;
            }

         }
         //printf("%s\n", pt.piece_name[piece]);
         //less_attack.print();
         less_attacks[side][piece] |= less_attack;
         less_attack |= attacks[side][piece];
      }

      /* Flip evaluation values for side to move.
       * Idea from Senpai.
       */
      mat = -mat;
      psq = -psq;
   }

   /* Take into account defensive material */
   // FIXME: doing it this way allows the program to drop a piece, which it
   // thinks is "fine" because it still has its defensive pieces.
   //if (mat.mg < 0) mat.mg = std::min(0, mat.mg + def[WHITE].mg/2);
   //if (mat.eg < 0) mat.eg = std::min(0, mat.eg + def[WHITE].eg/2);
   //if (mat.mg > 0) mat.mg = std::max(0, mat.mg - def[BLACK].mg/2);
   //if (mat.eg > 0) mat.eg = std::max(0, mat.eg - def[BLACK].eg/2);

   /* Gather data for shelter and assess winning chances. */
   for (side_t side = WHITE; side<NUM_SIDES; side++) {
      side_t oside = next_side[side];

      /* Pawn shelter/castle position */
      if (num_royals[side] == 1) {
         assert(king[side] >= 0);
         int piece = board.get_piece(king[side]);

         eval_t score = 0;
         eval_t cscore = 0;
         int f = unpack_file(king[side]);

         score = ps.shelter_score[side][f];  /* 0-8 */
         shelter[side] = 4*score;            /* 0-32 */

         for (int c = SHORT; c<NUM_CASTLE_MOVES; c++)
         if ((board.init & movegen.castle_mask[c][side] & board.bbc[side]) == movegen.castle_mask[c][side]) {
            bitboard_t<kind> bb = movegen.castle_king_dest[c][side];
            while (!bb.is_empty()) {
               int square = bb.bitscan();
               int f = unpack_file(square);
               bb.reset(square);
               cscore = std::max(cscore, ps.shelter_score[side][f]);
            }
         }

         //printf("Shelter: %d %d\n", score, cscore);
         if (cscore > score) score = (score + cscore) / 2;

         kss.mg += 4*score;

         /* Defensive pieces */
         if ( !(defence & castle).is_empty() ) {
            eval_t score = 0;
            
            score += 2*(bitboard_t<kind>::board_file[f] & defence & castle &  bitboard_t<kind>::board_homeland[side]).popcount();
            score += ((bitboard_t<kind>::board_file[f-1]|bitboard_t<kind>::board_file[f+1]) & defence & castle &  bitboard_t<kind>::board_homeland[side]).popcount();
            score += (defatk & castle).popcount();
            score += (defatk & defence & castle).popcount();
            shelter[side] = std::min(2*score, KS_SHELTER_WEIGHT);

            kss.mg += shelter[side];
         }

         /* Shelter score for drop-games */
         if ( pt.defensive_pieces == 0 && (board.rule_flags & RF_USE_CAPTURE) ) {
            bitboard_t<kind> king_zone = bitboard_t<kind>::neighbour_board[king[side]];
            eval_t score = 0;

            score = ps.shelter_score[side][f];
            score += 2*(king_zone & board.bbc[side]).popcount();
            score += (king_zone & less_attacks[side][piece]).popcount();
            shelter[side] = std::min(1*score, KS_SHELTER_WEIGHT);

            if ((king_zone & (multi_attacks[oside]|(all_attacks[oside]&~less_attacks[side][piece]))).is_empty())
               shelter[side] /= 2;

            kss.mg += shelter[side] / 2;
         }
      }

      /* King safety */
      /* TODO */

      /* Winning chances
       * TODO: test if pawns can promote to something with mate potential.
       */
      if (mate_potential[side] >= 1)
         can_win[side] = true;
      else {
         int non_pawn_non_royal = num_pieces[side]-num_pawns[side]-num_royals[side] - num_def[side];
         if (num_pawns[side] >= 1 || non_pawn_non_royal > 2)
            can_win[side] = true;

         if (non_pawn_non_royal == 2 && !can_win[side]) {
            uint32_t p = piece_ids[side];
            int n1, n2;

            n1 = n2 = bitscan32(p);
            p ^= 1<<n2;
            if (pt.pieces_can_win[n1][n2])
               can_win[side] = true;
            while (p && !can_win[side]) {
               n1 = n2;
               n2 = bitscan32(p);
               p ^= 1<<n2;
               if (pt.pieces_can_win[n1][n2])
                  can_win[side] = true;
            }
         }

         /* If we only have colour-bound pieces on the same colour, we
          * cannot win.
          */
         if (non_pawn_non_royal == num_light_bound[side] && num_pawns[side] == 0) can_win[side] = false;
         if (non_pawn_non_royal == num_dark_bound[side]  && num_pawns[side] == 0) can_win[side] = false;
      }

      /* No pawns and no pieces that can give mate - can't win */
      if ((piece_ids[side] & (pt.shak_pieces | pt.pawn_pieces)) == 0)
         can_win[side] = false;

      /* Mop-up evaluation:
       *  - drive the lone king to a corner
       *  - keep the king and the attacking pieces close
       *  - keep the attacking king closer to the centre
       * This is mainly for the benefit of colour-bound pieces, which may
       * have special requirements with respect to the mating corner.
       */
      if (num_royals[side] == 1 && num_pieces[side] == 1 && can_win[oside]) {
         int piece = board.get_piece(king[side]);
         int ocb = num_light_bound[oside] + num_dark_bound[oside];
         int opc = num_pieces[oside] - num_pawns[oside] - num_royals[oside] - num_def[oside];

         if (ocb && opc - ocb <= 1) {
            int df = abs(unpack_file(king[side]) - unpack_file(king[oside]));
            int dr = abs(unpack_rank(king[side]) - unpack_rank(king[oside]));
            psq = 4*(std::max(dr*dr, df*df) - 4);
            bitboard_t<kind> bb = board.bbc[oside];
            while (!bb.is_empty()) {
               int square = bb.bitscan();
               bb.reset(square);

               int score = (ranks + files)/2 - pt.tropism[piece][king[side]][square];

               psq += 4*score*abs(score);
            }

            psq += 8*(all_attacks[side] & ~all_attacks[oside]).popcount();

            int weight = num_light_bound[oside] - num_dark_bound[oside];
            for (int n = 0; n<2; n++) {
               if (weight > 0) {
                  bitboard_t<kind> avoid = (n == 0 ? bitboard_t<kind>::board_light
                                                   : bitboard_t<kind>::board_dark ) & bitboard_t<kind>::board_corner;
                  while (!avoid.is_empty()) {
                     int square = avoid.bitscan();
                     avoid.reset(square);

                     int score = (ranks + files)/2 - pt.tropism[piece][king[side]][square];

                     psq -= 2*weight * score * abs(score);

                     psq += 1*abs(centre_table[king[side]]) * centre_table[king[side]] -
                            3*abs(centre_table[king[oside]])* centre_table[king[oside]];

                     psq -= pt.tropism[piece][king[oside]][square]/2;
                  }
               }
               weight = -weight;
            }
         }
      }

      /* Flip evaluation values for side to move.
       * Idea from Senpai.
       */
      psq = -psq;
      kss = -kss;
      pss = -pss;
   }

   /* Draw-ish material combinations */
   if (num_pawns[WHITE] == 0 && num_pawns[BLACK] == 0) {
      if (mate_potential[WHITE] == mate_potential[BLACK] && mate_potential[WHITE] == 1 && abs(num_pieces[WHITE]-num_pieces[BLACK]) <= 1)
         mat /= 4;

      if (num_pieces[WHITE] == num_pieces[BLACK] && num_pieces[WHITE] == 1 && mate_potential[WHITE]+mate_potential[BLACK] == 1)
         mat /= 8;
   }

   /* Mobility, piece safety */
   {
      int wa_weight[NUM_SIDES][8*sizeof(kind)] = { { 0 } };
      int wa_count[NUM_SIDES][8*sizeof(kind)] = { { 0 } };
      int si = 0;
      for (side_t side = WHITE; side<NUM_SIDES; side++) {
         int ka_scale  = (KS_SHELTER_WEIGHT*KS_ATTACK_WEIGHT*pt.phase_scale);
         int ka_weight = 0;
         int ka_count  = 0;
         bitboard_t<kind> king_zone;
         side_t oside = next_side[side];

         if (num_royals[oside] == 1) {
            king_zone = bitboard_t<kind>::king_zone[oside][king[oside]];

            if ( board.rule_flags & RF_USE_HOLDINGS ) {
               for (int piece = 0; piece<pt.num_piece_types; piece++) {
                  if (board.holdings[piece][side]) {
                     ka_weight += std::max(1, pt.king_safety_weight[piece]);
                     ka_count++;
                  }
               }
            }
         }

         while (board.bbc[side].test(square_list[si]) && si < square_count) {
            int square = square_list[si];
            int piece = board.get_piece(square);

            /* King attack pattern */
            if ((!(attack[square] & ~less_attacks[oside][piece] & king_zone).is_empty())) {
               ka_weight += std::max(1, pt.king_safety_weight[piece]);
               ka_count++;
            }

            /* Attacks on weak pawns */
            bitboard_t<kind> wpa = attack[square] & ps.weak;
            while (!wpa.is_empty()) {
               int square = wpa.bitscan();
               wpa.reset(square);

               wa_weight[side][square] += std::max(1, pt.phase_weight[piece]);
               wa_count[side][square]++;
            }

            /* Attack on base of pawn structure */
            if ((pt.major_pieces & (1<<piece)) && !pawns[oside].is_empty() && (pt.piece_move_flags[piece] & MF_SLIDER_V)) {
               int br = (side == WHITE) ? pawns[BLACK].msb()
                                        : pawns[WHITE].lsb();

               br = unpack_rank(br);

               if (br == unpack_rank(square)) {
                  psq.mg += ROOK_BASE_PAWN_MG;
                  psq.eg += ROOK_BASE_PAWN_EG;
               }
            }

            /* Pawn terms */
            if (pt.pawn_pieces & (1<<piece)) {

               /* Passed pawns */
               if (ps.passed.test(square)) {
                  /* TODO: distance to promotion square. Properly. */
                  int rank = unpack_rank(square);
                  if (side == BLACK) rank = (bitboard_t<kind>::board_ranks - 1) - rank;

                  eval_t scale = PASSER_RANK_BASE;

                  bitboard_t<kind> fs = pt.front_span[side][square];
                  if ((fs & occ).is_empty()) scale++;
                  fs.set(square);
                  if (all_attacks[side].test(square)) scale++;
                  if ((fs & all_attacks[side]) == fs) scale++;
                  if ((fs & all_attacks[oside]).is_empty()) scale++;
                  scale *= PASSER_RANK_SCALE;
                  scale /= 128;

                  pss.eg += scale*rank*rank;
               }
            }


            /* Kings should stay put until the end game, so don't score king
             * mobility in the middle game.
             */
            if (pt.piece_flags[piece] & PF_ROYAL) {
               bitboard_t<kind> safe   = moves[square] & ~all_attacks[oside];
               bitboard_t<kind> unsafe = moves[square] ^ safe;

               mob.eg += pt.eval_mobility[piece][safe.popcount()].eg   * SAFE_MOB_WEIGHT / 128;
            } else {
               bitboard_t<kind> up_attacks = all_attacks[oside] ^ less_attacks[oside][piece];
               bitboard_t<kind> safe       = (moves[square] & ~less_attacks[oside][piece]) |
                                             (moves[square] & up_attacks & all_attacks[side]);
               bitboard_t<kind> unsafe     = moves[square] ^ safe;

               mob    += pt.eval_mobility[piece][safe.popcount()]      * SAFE_MOB_WEIGHT / 128;

               bitboard_t<kind> forward = (side == WHITE) ?  bitboard_t<kind>::board_northward[unpack_rank(square)]
                                                          :  bitboard_t<kind>::board_southward[unpack_rank(square)];
               if ((moves[square] & forward & ~pawns[side]).is_empty())
                  psq.mg -= MOB_FORWARD_BLOCKED;
            }

            /* Board control
             * Important squares to control in the middle game are in the
             * centre and the opponent side of the board.
             * In the end game focus changes to pawns and blocking passers.
             */
            bitboard_t<kind> control = attack[square] & ~pawn_attacks[oside];
            int cw = 0;
            if ( pt.minor_pieces & (1 << piece) ) cw = 4;
            if ( pt.major_pieces & (1 << piece) ) cw = 2;
            if ( pt.super_pieces & (1 << piece) ) cw = 1;
            eval_t score = 3*(control & bitboard_t<kind>::board_centre).popcount()
                         + 2*(control & bitboard_t<kind>::board_xcentre).popcount()
                         + 1*(control & bitboard_t<kind>::board_xxcentre).popcount()
                         + 1*(control & bitboard_t<kind>::board_homeland[oside]).popcount();

            mob.mg += MOB_SCALE * cw * (score - 4);
            if ( pt.royal_pieces & (1 << piece) ) cw = 4;
            mob.eg += MOB_SCALE * cw * 4 * (control & (ps.weak | ps.passed) & board.bbc[side]).popcount();


            /* Piece placement */
            if (pt.minor_pieces & (1<<piece)) {
               /* Unprotected minor */

               if (!pawn_attacks[side].test(square)) psq.mg -= ((1+num_supers[oside])*LOOSE_MINOR_PENALTY)/2;
               //if (less_attacks[side][piece].test(square)) psq.mg -= 10;
               //if (less_attacks[oside][piece].test(square)) psq.mg -= pt.piece_value[piece]/16;
               //else if (!all_attacks[oside].test(square)) psq.mg -= pt.piece_value[piece]/32;

               /* Outpost */
               //if ( (pawn_attacks[side] & ~bitboard_t<kind>::board_edge & bitboard_t<kind>::board_homeland[oside]).test(square) ) {
               //   psq.mg += 5;
               //}
            }

            /* In the end game the king should move towards enemy pawns */
            /* TODO */

            /* Defensive pieces should defend eachother
             * By definition, defensive pieces cannot attack squares on the
             * opponent's side of the board, so if they are ever attacked,
             * it's by one of their own pieces.
             */
            if (pt.defensive_pieces & (1<<piece)) {
               if (defatk.test(square))
                  psq += DEF_PROTECT;

               int rank = unpack_rank(square);
               if (!(board.royal & board.bbc[side] & bitboard_t<kind>::board_rank[rank]).is_empty())
                  psq += DEF_SHIELD_FILE;
            }

            /* Hoppers on same file as king (unblocked) */
            if (is_hopper(pt.piece_capture_flags[piece]) &&
               unpack_file(square) == unpack_file(king[oside]) && 
               bitboard_t<kind>::board_between[square][king[oside]].is_empty()) {
               kss.mg += HOPPER_KINGFILE_MG;
               kss.eg += HOPPER_KINGFILE_EG;
            }

            si++;
         }

         /* King attack pattern: the (safe) attack count causes the score
          * to increase exponentially, while a good shelter causes it to
          * decrease linearly. The exact form of the expression is
          * based on Senpai's implementation of the idea.
          * TODO: tune the overall constant scale factor.
          */
         assert(KS_SHELTER_WEIGHT - shelter[oside] >= 0);
         ka_weight *= (KS_ATTACK_WEIGHT - (KS_ATTACK_WEIGHT >> ka_count)) * (KS_SHELTER_WEIGHT - shelter[oside]);
         if (ka_scale) kss.mg    += KING_SAFETY_WEIGHT * ka_weight / ka_scale;

         mob = -mob;
         psq = -psq;
         kss = -kss;
         pss = -pss;
      }

      /* Pressure against weak pawns */
      for (side_t side = WHITE; side<NUM_SIDES; side++) {
         int wa_scale  = pt.phase_scale;
         side_t oside = next_side[side];
         eval_pair_t base;
         base.mg = WEAK_PAWN_BASE_MG;
         base.eg = WEAK_PAWN_BASE_EG;

         bitboard_t<kind> wp = ps.weak & board.bbc[side];
         while (!wp.is_empty()) {
            int square = wp.bitscan();
            wp.reset(square);

            int weight = (8 - (8 >> wa_count[oside][square])) * std::max(0, 8 - wa_count[side][square]);

            pss -= base;
            pss -= base * weight / 64;
         }

         pss = -pss;
      }

      mob /= MOB_SCALE;
      assert(si == square_count);
   }

   /* Disable phase-scaling in variants where captured pieces are returned:
    * there material does not represent game phase.
    */
   if (board.rule_flags & RF_USE_CAPTURE) phase = pt.phase_scale;

   if ( board.rule_flags & (RF_USE_CAPTURE | RF_ALLOW_PICKUP) ) {
      int t = 0;
      for (int piece = 0; piece<pt.num_piece_types; piece++) {
         if (board.holdings[piece][side_to_move]) {
            t += TEMPO_DROP_WEIGHT*std::max(1, pt.king_safety_weight[piece]);
         }
      }
      tp.mg = t;
      tp.eg = 0;

      /* Calculate tempo bonus */
      tempo = std::min((eval_t)TEMPO_DROP_MAX, tp.interpolate(phase, pt.phase_scale));
      if (tempo) symmetric = false;
   }

   if (print) {
      printf("Component:       MG:    EG:\n");
      printf("Material:        % 4d   % 4d\n", mat.mg, mat.eg);
      printf("Piece square:    % 4d   % 4d\n", psq.mg, psq.eg);
      printf("Mobility:        % 4d   % 4d\n", mob.mg, mob.eg);
      printf("Pawn structure:  % 4d   % 4d\n", pss.mg, pss.eg);
      printf("King safety:     % 4d   % 4d\n", kss.mg, kss.eg);
      printf("Tempo:           % 4d   % 4d\n",  tp.mg,  tp.eg);
   }

   /* Interpolate evaluation score. */
   ev = (mat + psq + mob + pss + kss).interpolate(phase, pt.phase_scale);

   if (check_limit)
      ev += -(board.check_count[WHITE] - board.check_count[BLACK])*check_score / (20*check_limit);

   /* Adjust the score: if the side that is nominally ahead can't win, drop the score to 0(ish) */
   phase = std::min(phase, pt.phase_scale);
   if (!can_win[WHITE] && !can_win[BLACK]) ev = 0;
   if (ev > 0 && !can_win[WHITE]) { symmetric = false; ev = psq.interpolate(phase, pt.phase_scale); }
   if (ev < 0 && !can_win[BLACK]) { symmetric = false; ev = psq.interpolate(phase, pt.phase_scale); }

#ifdef DEBUG_EVHASH
   /* Sanity check: the hashed score should equal the current score.
    * We only ever get here if we're debugging the evaluation hash.
    */
   if (symmetric) {
      if (!(!have_eval_hash || (hash_ev == ev))) {
         printf("%d %d %d %d\n", have_eval_hash, hash_ev, ev, symmetric);
         printf("0x%016llx\n", board.hash);
         board.print_bitboards();
         //printf("%s\n", make_fen_string(game, NULL));
      }
      assert(!have_eval_hash || (hash_ev == ev));
   }
#endif

   /* Store the results of the evaluation in the evaluation hash table.
    * If the evaluation is symmetric, we can store it for the other side
    * to move as well, which will safe an evaluation after null-move.
    */
   store_eval_hash_entry(eval_table, board.hash, ev);
   if (symmetric)
      store_eval_hash_entry(eval_table, side_to_move_key^board.hash, ev);

exit:

   /* Tapered evaluation when we're about to hit the 50 move counter */
   if (fifty_scale_limit && board.fifty_counter > fifty_scale_limit)
      ev = ev * (fifty_limit - board.fifty_counter) / (fifty_limit - fifty_scale_limit);

   /* Add a pseudo-random contribution to the opening moves */
   if (random_ok && random_amplitude && (start_move_count + moves_played) < random_ply_count) {
      unsigned int rand = (unsigned int)(board.hash ^ random_key);
      int amp = int(random_amplitude * (random_ply_count - start_move_count - moves_played) / random_ply_count);
      int r = ((int)(rand & 0xff) - 0x7f) * amp / 0x7f;
      if (print)
         printf("Random factor:   % 4d\n", r);
      ev += r;
   }

   return ((side_to_move == WHITE) ? ev : -ev) + tempo;
}
