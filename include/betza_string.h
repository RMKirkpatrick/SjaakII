int slider_hopper_move_flags_to_betza(move_flag_t flags, char *buffer, size_t size) const
{
   static struct {
      move_flag_t flags; const char *txt;
   } hopper_move_string[] = {
      { MF_SLIDER_H | MF_SLIDER_V | MF_SLIDER_D | MF_SLIDER_A, "Q" },
      { MF_SLIDER_H | MF_SLIDER_V                            , "R" },
      {                             MF_SLIDER_D | MF_SLIDER_A, "B" },
      { MF_SLIDER_H                                          , "sR" },
      {               MF_SLIDER_V                            , "vR" },
      {                             MF_SLIDER_D              , "rflbB" },
      {                                           MF_SLIDER_A, "lfrbB" },
      { 0, NULL }
   };

   int n = 0;
   const char *hop = "";
   while (is_slider(flags) || is_hopper(flags)) {
      if (is_slider(flags)) {

         for (int k=0; hopper_move_string[k].flags; k++) {
            if ((flags & hopper_move_string[k].flags) == hopper_move_string[k].flags) {
               n += snprintf(buffer+n, size-n, "%s%s", hop, hopper_move_string[k].txt);
               flags &= ~hopper_move_string[k].flags;
            }
         }

         flags &= ~MF_SLIDER;
      }

      if (is_hopper(flags)) {
         flags |= (flags >> 4) & MF_SLIDER;
         hop = "p";
         flags &= ~MF_HOPPER;
      }
   }

   return n;
}

int leaper_move_flags_to_betza(move_flag_t flags, char *buffer, size_t size) const
{
   int n = 0;

   enum sym4_t { V, S, F, B, L, R, RF4, LF4, RB4, LB4, NUM_SYM4 };
   enum sym8_t { FL, LF, FR, RF, BL, LB, BR, RB, FF, BB, LL, RR, FH, BH, LH, RH, NUM_SYM8 };

   static struct {
      int dx, dy;
      const char *txt;
      int symmetry;
   } leaper_description[] = {
      { 1, 0, "W", 4 },
      { 1, 1, "F", 4 },
      { 2, 0, "D", 4 },
      { 2, 1, "N", 8 },
      { 2, 2, "A", 4 },
      { 3, 0, "H", 4 },
      { 3, 1, "C", 8 },
      { 3, 2, "Z", 8 },
      { 3, 3, "G", 4 },
      { 0, 0, NULL, 0 }
   };
   bitboard_t<kind> sym4[NUM_SYM4];
   const char *sym4txt[NUM_SYM4];
   bitboard_t<kind> sym8[NUM_SYM8];
   const char *sym8txt[NUM_SYM8];
   sym4txt[V] = "v";
   sym4txt[S] = "s";
   sym4txt[F] = "f";
   sym4txt[B] = "b";
   sym4txt[L] = "l";
   sym4txt[R] = "r";
   sym4txt[RF4] = "rf";
   sym4txt[LF4] = "lf";
   sym4txt[RB4] = "rb";
   sym4txt[LB4] = "lb";

   sym8txt[FH] = "fh";
   sym8txt[BH] = "bh";
   sym8txt[LH] = "lh";
   sym8txt[RH] = "rh";
   sym8txt[FF] = "ff";
   sym8txt[BB] = "bb";
   sym8txt[LL] = "ll";
   sym8txt[RR] = "rr";
   sym8txt[FL] = "fl";
   sym8txt[LF] = "lf";
   sym8txt[FR] = "fr";
   sym8txt[RF] = "rf";
   sym8txt[BL] = "bl";
   sym8txt[LB] = "lb";
   sym8txt[BR] = "br";
   sym8txt[RB] = "rb";

   /* Lame leapers */
   const char *block = "";
   if (is_masked_leaper(flags)) {
      move_flag_t f = flags & MF_LEAPER_MASK;
      flags &= ~MF_LEAPER_FLAGS;
      flags |= (f >> 8) | MF_IS_LEAPER;
      block = "n";
   }

   /* Simple leapers */
   if (is_simple_leaper(flags) || is_double_leaper(flags)) {
      int index  = get_leaper_index(flags);
      int index2 = get_leaper_index2(flags);

      /* Some combination of simple leapers, this is a bit complicated.
       * We use the following algorithm:
       *  - Find the square where the leaper has maximum mobility.
       *  - Find the simple leaper that has the largest overlap (test the King first)
       *  - If the overlap is complete, we are done. If not, we need to extract directions.
       *  - Repeat until all compound leapers have been identified.
       */

      /* Find the square where the leaper has best mobility.
       * We need this because the movement tables could have been hacked to exclude certain regions of the board.
       */
      bitboard_t<kind> leaper;
      int square = 0;
      int max_moves = 0;
      for (int nn=0; nn<files*ranks; nn++) {
         bitboard_t<kind> bb = is_aleaper(flags) ? movegen.aleaper[WHITE][index][nn] : movegen.leaper[index][nn];
         int moves = bb.popcount();
         if (moves > max_moves || (moves == max_moves && nn < 4*files)) {  /* Don't get stuck on back rank */
            max_moves = moves;
            square = nn;
            leaper = bb;
         }
      }

      /* Construct symmetry masks */
      if (is_aleaper(flags)) {
         bitboard_t<kind> fd, bd, fa, ba;
         int rank = unpack_rank(square);
         int file = unpack_file(square);
         int diag = bitboard_t<kind>::diagonal_nr[square];
         int anti = bitboard_t<kind>::anti_diagonal_nr[square];

         sym4[F] = bitboard_t<kind>::board_northward[rank];
         sym4[B] = bitboard_t<kind>::board_southward[rank];
         sym4[R] = bitboard_t<kind>::board_eastward[file];
         sym4[L] = bitboard_t<kind>::board_westward[file];
         sym4[V] = sym4[F] | sym4[B];
         sym4[S] = sym4[R] | sym4[L];

         sym4[RF4] = sym4[R] & sym4[L];
         sym4[LF4] = sym4[L] & sym4[L];
         sym4[RB4] = sym4[R] & sym4[B];
         sym4[LB4] = sym4[L] & sym4[B];

         sym8[FH] = sym4[F];
         sym8[BH] = sym4[B];
         sym8[LH] = sym4[L];
         sym8[RH] = sym4[R];

         fd.clear();
         for (int n = diag+1; n<16; n++)
            fd |= bitboard_t<kind>::board_diagonal[n];
         fa.clear();
         for (int n = anti+1; n<16; n++)
            fa |= bitboard_t<kind>::board_antidiagonal[n];
         bd.clear();
         for (int n = 0; n < diag; n++)
            bd |= bitboard_t<kind>::board_diagonal[n];
         ba.clear();
         for (int n = 0; n < anti; n++)
            ba |= bitboard_t<kind>::board_antidiagonal[n];

         sym8[FF] = fd & fa;
         sym8[BB] = bd & ba;
         sym8[LL] = fd & ba;
         sym8[RR] = bd & fa;

         sym8[FL] = sym8[FH] & sym8[LL];
         sym8[LF] = sym8[LH] & sym8[FF];
         sym8[FR] = sym8[FH] & sym8[RR];
         sym8[RF] = sym8[RH] & sym8[FF];
         sym8[BL] = sym8[BH] & sym8[LL];
         sym8[LB] = sym8[LH] & sym8[BB];
         sym8[BR] = sym8[BH] & sym8[RR];
         sym8[RB] = sym8[RH] & sym8[BB];
      }

      /* Find which combination of simple leapers matches best.
       * We try the non-atom king first.
       */
      bitboard_t<kind> kb = movegen.make_leaper_bitboard(square, 1, 1)|movegen.make_leaper_bitboard(square, 1, 0);
      if (leaper == kb) {
         if (index2 == index && is_double_leaper(flags))
            n += snprintf(buffer+n, size-n, "a");
         n += snprintf(buffer+n, size-n, "K");
         leaper.clear();
      }

      /* Loop over all fundamental leapers.
       * FIXME: the loop can become infinite if the leaper is long (> 4) in any direction.
       */
      while (!leaper.is_empty()) {
         bitboard_t<kind> best_leaper;
         int best = -1;
         int best_count = 0;
         for (int nn = 0; leaper_description[nn].txt; nn++) {
            int dx = leaper_description[nn].dx;
            int dy = leaper_description[nn].dy;
            bitboard_t<kind> bb = movegen.make_leaper_bitboard(square, dx, dy);

            int count = (bb & leaper).popcount();
            if (count > best_count) {
               best_count = count;
               best = nn;
               best_leaper = bb;

               if (leaper == bb) break;
            }
         }

         if (best == -1) break;  /* Break out if nothing matches */

         if ((leaper & best_leaper) == best_leaper) {
            n += snprintf(buffer+n, size-n, "%s%s", block, leaper_description[best].txt);
         } else {
            assert(is_aleaper(flags));

            /* Only some of the directions match, find out which */
            if (leaper_description[best].symmetry == 4) { /* 4-point symmetry */
               while (!(leaper & best_leaper).is_empty()) {
                  int best = -1;
                  int best_count = 0;
                  for (int k = 0; k<NUM_SYM4; k++) {
                     /* Make sure the direction does not imply unwanted extra bits */
                     if (!((sym4[k]&best_leaper) & ~(best_leaper&leaper)).is_empty()) continue;

                     bitboard_t<kind> bb = sym4[k] & best_leaper & leaper;

                     int count = bb.popcount();
                     if (count > best_count) {
                        best_count = count;
                        best = k;
                     }
                  }

                  if (best == -1) break;  /* Break out if nothing matches */

                  n += snprintf(buffer+n, size-n, "%s", sym4txt[best]);

                  leaper &= ~(best_leaper & sym4[best]);
               }
            } else if (leaper_description[best].symmetry == 8) { /* 8-point symmetry */
               while (!(leaper & best_leaper).is_empty()) {
                  int best = -1;
                  int best_count = 0;
                  for (int k = 0; k<NUM_SYM8; k++) {
                     /* Make sure the direction does not imply unwanted extra bits */
                     if (!((sym8[k]&best_leaper) & ~(best_leaper&leaper)).is_empty()) continue;

                     bitboard_t<kind> bb = sym8[k] & best_leaper & leaper;

                     int count = bb.popcount();
                     if (count > best_count) {
                        best_count = count;
                        best = k;
                     }
                  }

                  if (best == -1) break;  /* Break out if nothing matches */
                  n += snprintf(buffer+n, size-n, "%s", sym8txt[best]);

                  leaper &= ~(best_leaper & sym8[best]);
               }
            }
            if (is_double_leaper(flags)) {
               if (index2 == index)
                  n += snprintf(buffer+n, size-n, "%sa%s%s", block, block, leaper_description[best].txt);
            } else {
               n += snprintf(buffer+n, size-n, "%s%s", block, leaper_description[best].txt);
            }
         }

         leaper &= ~best_leaper;
      }
   }


   return n;
}

int stepper_move_flags_to_betza(move_flag_t flags, char *buffer, size_t size) const
{
   if (!is_stepper(flags)) return 0;

   int n = 0;

   enum sym4_t { A, V, S, F, B, L, R, RF4, LF4, RB4, LB4, NUM_SYM4 };

   bitboard_t<kind> sym4[NUM_SYM4];
   const char *sym4txt[NUM_SYM4];
   sym4txt[A] = "";
   sym4txt[V] = "v";
   sym4txt[S] = "s";
   sym4txt[F] = "f";
   sym4txt[B] = "b";
   sym4txt[L] = "l";
   sym4txt[R] = "r";
   sym4txt[RF4] = "rf";
   sym4txt[LF4] = "lf";
   sym4txt[RB4] = "rb";
   sym4txt[LB4] = "lb";

   int square = bitboard_t<kind>::pack_rank_file(ranks/2, files/2);
   int rank = unpack_rank(square);
   int file = unpack_file(square);

   sym4[A] = bitboard_t<kind>::board_all;
   sym4[F] = bitboard_t<kind>::board_northward[rank];
   sym4[B] = bitboard_t<kind>::board_southward[rank];
   sym4[R] = bitboard_t<kind>::board_eastward[file];
   sym4[L] = bitboard_t<kind>::board_westward[file];
   sym4[V] = sym4[F] | sym4[B];
   sym4[S] = sym4[R] | sym4[L];

   sym4[RF4] = sym4[R] & sym4[F];
   sym4[LF4] = sym4[L] & sym4[F];
   sym4[RB4] = sym4[R] & sym4[B];
   sym4[LB4] = sym4[L] & sym4[B];

   bitboard_t<kind> atombb[2];
   atombb[0] = movegen.make_leaper_bitboard(square, 1, 0);
   atombb[1] = movegen.make_leaper_bitboard(square, 1, 1);

   int si = get_stepper_index(flags);
   int dim = std::max(files, ranks)-1;

   /* Decompose the stepper.
    * The array index runs over the number of steps, bit indicate which directions are possible.
    */
#if defined _MSC_VER
   std::vector<uint8_t> direction_mask(dim, 0);
#else
   uint8_t direction_mask[dim];
   memset(direction_mask, 0, dim);
#endif

   /* Check for single stepper moves, which are generated in parallel */
   for (int d=0; d<8; d++) {
      int max_c = (movegen.stepper_description[si][WHITE] >> (d*4)) & 15;

      if (max_c > dim) max_c = dim;

      for (int c = 0; c<max_c; c++) {
         direction_mask[c] |= 1<<d;
      }
   }

   /* Now decompose the movement directions */
   for (int c = dim-1; c >= 0; c--) {
      if (direction_mask[c] == 0) continue;

      /* Atoms that describe the motion, decompose in F and W */
      const char *atom[2] = { "W", "F" };
      char countstr[10];
      countstr[0] = 0;
      if (c == dim-1) {
         atom[0] = "R";
         atom[1] = "B";
      } else if (c > 0) {
         snprintf(countstr, 10, "%d", c+1);
      }

      /* Construct move atom */
      bitboard_t<kind> moves;
      for (int d=0; d<8; d++) {
         if ((direction_mask[c] & (1<<d)) == 0) continue;

         moves.set(square + movegen.step_shift[d]);
      }

      for (int ai = 0; ai < 2; ai++) {
         if ((moves & atombb[ai]).is_empty()) continue;

         /* Construct direction modifier prefix */
         char s[64];
         s[0] = ' ';
         s[1] = 0;
         int k = 0;
         while (!(moves & atombb[ai]).is_empty()) {
            /* Find the directional modifier that best describes the move */
            int best = -1;
            int best_count = 0;
            for (int kk=0; kk<NUM_SYM4; kk++) {
               /* Make sure the direction does not imply unwanted extra bits */
               if (!((sym4[kk]&atombb[ai]) & ~(atombb[ai]&moves)).is_empty()) continue;

               bitboard_t<kind> bb = sym4[kk] & atombb[ai] & moves;

               int count = bb.popcount();
               if (count > best_count) {
                  best_count = count;
                  best = kk;
               }
            }

            if (best == -1) break;  /* Break out if nothing matches */

            /* Make sure directions are unambiguous for diagonal atoms by repeating the last direction if needed.
             * Due to the way the matching works, the first iteration will always try to match the SVFBRL directions.
             * This maps (NW,NE,SE) -> fF + SE -> ffrbF.
             */
            if (ai == 1 && strlen(sym4txt[best]) == 2 && k == 1)
               k += snprintf(s+k, sizeof(s)-k, "%c", s[k-1]);
            k += snprintf(s+k, sizeof(s)-k, "%s", sym4txt[best]);

            moves &= ~(sym4[best]&atombb[ai]);
         }

         n += snprintf(buffer+n, size-n, "%s%s%s", s, atom[ai], countstr);
      }

      /* Mask these directions for all lower-number directions */
      for (int k = c-1; k>=0; k--)
         direction_mask[k] &= ~direction_mask[c];
   }

   return n;
}

int rider_move_flags_to_betza(move_flag_t flags, char *buffer, size_t size) const
{
   int n = 0;

   static struct {
      int dx, dy;
      const char *txt;
      int symmetry;
   } leaper_description[] = {
      { 1, 0, "W", 4 },
      { 1, 1, "F", 4 },
      { 2, 0, "D", 4 },
      { 2, 1, "N", 8 },
      { 2, 2, "A", 4 },
      { 3, 0, "H", 4 },
      { 3, 1, "C", 8 },
      { 3, 2, "Z", 8 },
      { 3, 3, "G", 4 },
      { 0, 0, NULL, 0 }
   };

   int index = get_rider_index(flags);
   for (int k=0; k<4; k++)
   for (int nn = 0; leaper_description[nn].txt; nn++) {
      int dx = leaper_description[nn].dx;
      int dy = leaper_description[nn].dy;
      if ((dx == movegen.rider_step[index][k].dx && dy == movegen.rider_step[index][k].dy) ||
          (dy == movegen.rider_step[index][k].dx && dx == movegen.rider_step[index][k].dy)) {
         n += snprintf(buffer+n, size-n, "%s0", leaper_description[nn].txt);
         break;
      }
   }

   return n;
}

const char *move_flags_to_betza(move_flag_t flags, char *buffer = NULL, size_t size = 256) const
{
   static char static_buffer[256] = { 0 };
   int n = 0;
   if (buffer == NULL) {
      buffer = static_buffer;
      size = sizeof static_buffer;
   }
   buffer[0] = 0;
   if (flags == 0) goto done;

   buffer[0] = '?';
   buffer[1] = 0;

   /* Describe slider moves */
   n += slider_hopper_move_flags_to_betza(flags, buffer + n, size - n);
   flags &= ~MF_SLIDER;
   flags &= ~MF_HOPPER;
   if (flags == 0) goto done;

   /* Describe leaper moves */
   n += leaper_move_flags_to_betza(flags, buffer + n, size - n);
   flags &= ~MF_LEAPER_FLAGS;
   if (flags == 0) goto done;

   /* Describe rider moves */
   n += rider_move_flags_to_betza(flags, buffer + n, size - n);
   flags &= ~MF_RIDER;
   if (flags == 0) goto done;

   /* Describe stepper moves */
   n += stepper_move_flags_to_betza(flags, buffer + n, size - n);
   flags &= ~MF_STEPPER;
   if (flags == 0) goto done;

done:
   return buffer;
}


const char *piece_moves_to_betza(int piece, char *buffer = NULL, size_t size = 256) const
{
   const char *post_movetype = "pn";
   char s_string[256] = { 0 };
   char m_string[256] = { 0 };
   char c_string[256] = { 0 };
   char i_string[256] = { 0 };
   char b_string[256] = { 0 };
   static char static_buffer[768] = { 0 };
   char *s;
   int n = 0;
   if (buffer == NULL) {
      buffer = static_buffer;
      size = sizeof static_buffer;
   }
   buffer[0] = 0;

   move_flag_t move    = pt.piece_move_flags[piece];
   move_flag_t capture = pt.piece_capture_flags[piece];
   move_flag_t special = pt.piece_special_move_flags[piece];
   move_flag_t initial = pt.piece_initial_move_flags[piece];

   move_flags_to_betza(move,    m_string, sizeof(m_string));
   move_flags_to_betza(capture, c_string, sizeof(c_string));
   move_flags_to_betza(special, i_string, sizeof(i_string));
   move_flags_to_betza(initial, b_string, sizeof(b_string));

   /* Eliminate duplicates between the normal move and the initial move */
   if ((s = strstr(i_string, m_string))) {
      size_t n1 = strlen(m_string);
      size_t n2 = strlen(i_string);
      char *p = s + n1;
      if (!isdigit(p[0]))
      for (size_t k = 0; k <= (n2-n1); k++) {
         s[k] = p[k];
         s[k+1] = 0;
         if (s[k] == 0) break;
      }
   }
   if ((s = strstr(b_string, m_string))) {
      size_t n1 = strlen(m_string);
      size_t n2 = strlen(b_string);
      char *p = s + n1;
      if (!isdigit(p[0]))
      for (size_t k = 0; k <= (n2-n1); k++) {
         s[k] = p[k];
         s[k+1] = 0;
         if (s[k] == 0) break;
      }
   }

   if (move == capture) {
      strncpy(s_string, m_string, sizeof s_string);
      m_string[0] = c_string[0] = 0;
   }

   /* Move options common to move and capture to a shared string */
   if (c_string[0] && (s = strstr(c_string, m_string))) {
      if (s == c_string || isupper(s[-1])) {
         strncpy(s_string, m_string, sizeof s_string);
         m_string[0] = 0;

         size_t n1 = strlen(s_string);
         size_t n2 = strlen(c_string);
         char *p = s + n1;
         if (!isdigit(p[0]))
         for (size_t k = 0; k <= (n2-n1); k++) {
            s[k] = p[k];
            s[k+1] = 0;
            if (s[k] == 0) break;
         }
      }
   }

   if (m_string[0] && (s = strstr(m_string, c_string))) {
      if (s == m_string || isupper(s[-1])) {
         strncpy(s_string, c_string, sizeof s_string);
         c_string[0] = 0;

         size_t n1 = strlen(s_string);
         size_t n2 = strlen(m_string);
         char *p = s + n1;
         if (!isdigit(p[0]))
         for (size_t k = 0; k <= (n2-n1); k++) {
            s[k] = p[k];
            s[k+1] = 0;
            if (s[k] == 0) break;
         }
      }
   }

   if (strstr(s_string, "a")) {
      char *s = strstr(s_string, "a")+1;
      s[-1] = 0;
      n += snprintf(buffer+n, size-n, "mcpa%s", s);
      n += snprintf(buffer+n, size-n, "mcab%s", s);
   }

   /* Common between move and capture */
   n += snprintf(buffer+n, size-n, "%s", s_string);

   /* Pure moves */
   size_t l = strlen(m_string);
   for (size_t k=0; k<l; k++) {
      if (isupper(m_string[k]) || strchr(post_movetype, m_string[k])) {
         buffer[n++] = 'm';
         buffer[n] = 0;
      }
      buffer[n++] = m_string[k];
      if (strchr(post_movetype, m_string[k]))
         buffer[n++] = m_string[++k];
      buffer[n] = 0;
   }

   /* Pure captures */
   l = strlen(c_string);
   for (size_t k=0; k<l; k++) {
      if (isupper(c_string[k]) || strchr(post_movetype, c_string[k])) {
         buffer[n++] = 'c';
         if (pt.piece_flags[piece] & PF_TAKE_EP) buffer[n++] = 'e';
         buffer[n] = 0;
      }
      buffer[n++] = c_string[k];
      if (strchr(post_movetype, c_string[k]))
         buffer[n++] = c_string[++k];
      buffer[n] = 0;
   }

   /* Initial and special moves */
   if (special) {
      n += snprintf(buffer+n, size-n, "i");
      if (initial)
         n += snprintf(buffer+n, size-n, "i");

      size_t l = strlen(i_string);
      for (size_t k=0; k<l; k++) {
         if (isupper(i_string[k]) || strchr(post_movetype, i_string[k])) {
            buffer[n++] = 'm';
            buffer[n] = 0;
         }
         buffer[n++] = i_string[k];
         if (strchr(post_movetype, i_string[k]))
            buffer[n++] = i_string[++k];
         buffer[n] = 0;
      }
   }
   if (initial) {
      n += snprintf(buffer+n, size-n, "i");

      size_t l = strlen(b_string);
      for (size_t k=0; k<l; k++) {
         if (isupper(b_string[k]) || strchr(post_movetype, b_string[k])) {
            buffer[n++] = 'm';
            buffer[n] = 0;
         }
         buffer[n++] = b_string[k];
         if (strchr(post_movetype, b_string[k]))
            buffer[n++] = b_string[++k];
         buffer[n] = 0;
      }
   }


   if (pt.piece_flags[piece] & PF_CASTLE) {
      uint32_t castle_range[NUM_CASTLE_MOVES+1] = { 0 };

      for (int c = SHORT; c<NUM_CASTLE_MOVES; c++) {
         bitboard_t<kind> kd = movegen.castle_king_dest[c][WHITE];
         while (!kd.is_empty()) {
            bitboard_t<kind> bb = movegen.castle_mask[SHORT][WHITE] & movegen.castle_mask[LONG][WHITE];
            int s1 = bb.bitscan();
            int s2 = kd.bitscan();
            int r  = abs(s2 - s1);

            castle_range[c] |= 1<<r;
            kd.reset(s2);
         }
      }

      castle_range[NUM_CASTLE_MOVES] = castle_range[SHORT] & castle_range[LONG];
      castle_range[SHORT] &= ~castle_range[NUM_CASTLE_MOVES];
      castle_range[LONG]  &= ~castle_range[NUM_CASTLE_MOVES];

      for (int c = SHORT; c<NUM_CASTLE_MOVES; c++) {
         bitboard_t<kind> side = bitboard_t<kind>::board_east_edge | bitboard_t<kind>::board_west_edge;
         bitboard_t<kind> kd = movegen.castle_king_dest[c][WHITE];
         while (!kd.is_empty()) {
            bitboard_t<kind> bb = movegen.castle_mask[SHORT][WHITE] & movegen.castle_mask[LONG][WHITE];
            int s1 = bb.bitscan();
            int s2 = kd.bitscan();
            int r  = abs(s2 - s1);

            kd.reset(s2);

            char ch = 's';
            if (castle_range[c] & (1 << r)) ch = "rl"[c];
            if (ch == 's' && c > SHORT) continue;

            n += snprintf(buffer+n, size-n, "i%c", ch);

            /* We need the "j" modifier if the castle partner is not a
             * "corner piece".
             */
            bb = movegen.castle_mask[c][WHITE];
            bb.reset(s1);
            int s3 = bb.bitscan();
            if ((side & bb).is_empty() && bitboard_t<kind>::board_all.test(s3+1))
               n += snprintf(buffer+n, size-n, "j");

            n += snprintf(buffer+n, size-n, "O%d", r);
         }
      }
   }

   if (strlen(buffer) == 0) {
      /* Hack: try to convince XBoard that this piece can do anything */
      snprintf(buffer, size, "QNCZt");
   }

   /* Drop extension,
    * http://www.talkchess.com/forum/viewtopic.php?p=612101#612101
    */
   if (board.rule_flags & RF_USE_DROPS) {
      bool print = false;
      /* No drops on first file */
      if ((pt.drop_zone[WHITE][piece] & bitboard_t<kind>::board_rank[0]).is_empty()) {
         n += snprintf(buffer+n, size-n, "j");
         print = true;
      }
      /* Restriction on number of pieces of this type, board */
      if (pt.piece_maximum[piece][WHITE]<128 && !(pt.piece_maximum[piece][WHITE]==1 && (pt.piece_flags[piece]&PF_ROYAL))) {
         print = true;
         for (int k = 0; k<pt.piece_maximum[piece][WHITE]; k++)
            n += snprintf(buffer+n, size-n, "b");
      }
      /* Restriction on number of pieces of this type, file */
      if (pt.piece_flags[piece] & PF_DROPONEFILE) {
         print = true;
         for (int k = 0; k<pt.piece_drop_file_maximum[piece]; k++)
            n += snprintf(buffer+n, size-n, "f");
      }
      /* Shorten promotion zone */
      int last_rank = ranks;
      while (last_rank > 0 && (pt.drop_zone[WHITE][piece] & bitboard_t<kind>::board_rank[last_rank-1]).is_empty())
         last_rank--;
      if (last_rank != ranks)
         print = true;
      if (print) {
         n += snprintf(buffer+n, size-n, "@");
         if (last_rank != ranks)
            n += snprintf(buffer+n, size-n, "%d", last_rank);
      }
   }
   return buffer;
}

