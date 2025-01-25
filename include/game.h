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
#ifndef GAME_H
#define GAME_H

#include <cstdio>
#include <cmath>
#include <algorithm>
#include "xstring.h"
#include "genrand.h"
#include "bitboard.h"
#include "pieces.h"
#include "movegen.h"
#include "move.h"
#include "score.h"
#include "hashtable.h"
#include "evalhash.h"
#include "eval_types.h"
#include "eval_param.h"
#include "timer.h"
#include "pst.h"
#include "san.h"

#define MAX_SEARCH_DEPTH 60       /* maximum depth of search tree */

#define HARD_HORIZON -20

#define MAX_TOTAL_DEPTH (MAX_SEARCH_DEPTH - HARD_HORIZON)

#define HASH_TABLE_SIZE (16*1024*1024)

#undef USE_HISTORY_HEURISTIC

static bool abort_search;

enum play_state_t { SEARCH_OK=0, SEARCH_GAME_ENDED, SEARCH_GAME_ENDED_REPEAT, SEARCH_GAME_ENDED_50_MOVE, SEARCH_GAME_ENDED_MATE, SEARCH_GAME_ENDED_STALEMATE, SEARCH_GAME_ENDED_INSUFFICIENT, SEARCH_GAME_ENDED_LOSEBARE, SEARCH_GAME_ENDED_WINBARE, SEARCH_GAME_ENDED_FORFEIT, SEARCH_GAME_ENDED_INADEQUATEMATE, SEARCH_GAME_ENDED_FLAG_CAPTURED, SEARCH_GAME_ENDED_NOPIECES, SEARCH_GAME_ENDED_CHECK_COUNT };
enum chase_state_t { NO_CHASE=0, DRAW_CHASE, LOSE_CHASE, WIN_CHASE };

/* Settings */
enum { MATE_SEARCH_DISABLED=0, MATE_SEARCH_ENABLE_DROP, MATE_SEARCH_ENABLED };

/* Level of play */
enum level_t { LEVEL_RANDOM, LEVEL_NORMAL, LEVEL_BEAL, LEVEL_STATIC, LEVEL_NUM_LEVELS };

extern void (*default_iteration_output)(const char *, ...);
extern void (*default_uci_output)(const char *, ...);
extern void (*default_xboard_output)(const char *, ...);
extern void (*default_error_output)(const char *, ...);
extern size_t default_hash_size;

struct game_t {
   /* Functions */
   virtual ~game_t() {}
   virtual void setup_fen_position(const char * /* str */, bool skip_castle = false) { (void)skip_castle; }
   virtual const char *make_fen_string(char *buffer = NULL) const { return buffer; }
   virtual void start_new_game(void) {}
   virtual void set_transposition_table_size(size_t /* size */) {}
   virtual void print_board(FILE * file = stdout) const {(void)file;}
   virtual void print_bitboards() const {}
   virtual void generate_moves(movelist_t * /* movelist */) const {}
   virtual void generate_legal_moves(movelist_t * /* movelist */) const {}
   virtual void test_move_game_check() {}
   virtual side_t get_side_to_move() { return NUM_SIDES; }
   virtual bool player_in_check(side_t /* side*/ ) { return false; }
   virtual side_t side_piece_on_square(int /* square */) { return NONE; }
   virtual void playmove(move_t /* move*/) {}
   virtual void takeback() {}
   virtual int  eval() { return 0; }
   virtual int  static_qsearch(int /* beta */, int depth = 0) { (void)depth; return 0; }
   virtual int  see(move_t /* move */) { return 0; }
   virtual size_t get_moves_played() { return 0 ; }
   virtual move_t move_string_to_move(const char * move_str, const
   movelist_t * external_movelist = NULL) const { (void)move_str,external_movelist; return 0; }
   virtual play_state_t think(int /* max_depth */) { return SEARCH_OK; }
   virtual bool ponder() { return false; }
   virtual bool analyse() { return false; }
   virtual void write_piece_descriptions(bool xb = false) const { (void)xb; }
   virtual void print_wiki_rules(void) {}
   virtual void print_rules(void) {}
   virtual void print_pieces(void) const {}
   virtual void print_eval_parameters(FILE * file = stdout) {(void)file;}
   virtual void load_eval_parameters(FILE *f) {(void)f;}
   virtual void print_attacker_bitboard(int /* square */) {}
   virtual void print_attack_bitboard(int /* square */) {}
   virtual int  pack_rank_file(int /* rank */, int /* file */) { return 0; }

   virtual play_state_t get_game_end_state(movelist_t * movelist = NULL) { (void)movelist; return SEARCH_OK; }

   virtual void deduce_castle_flags(side_t /* side */, int /* king_from */, int /* king_to */, int /* rook_from */) {};
   virtual void add_rule(uint32_t /* rule */) {}
   virtual void remove_rule(uint32_t /* rule */) {}
   virtual uint32_t get_rules(void) { return 0; }
   virtual void set_maximum_number_of_pieces(const char * /* symbol */, side_t /* side */, int /* count */) {}
   virtual void set_maximum_number_of_kings(side_t /* side */, int /* count */) {}
   virtual chase_state_t test_chase() {return NO_CHASE;}

   virtual bool side_captured_flag(side_t /* side */) { return false; }
   virtual void print_pst(void) {}


   const char *get_name() const { return name; }
   virtual const char *get_piece_notation(int id) const { (void)id; return ""; }
   virtual const char *get_piece_abbreviation(side_t side, int id) const { (void)side; (void)id; return ""; }
   virtual int get_most_valuable_piece_id(uint32_t mask) const { (void)mask; return -1; }

   void set_default_output_function(void (*func)(const char *, ...))
   {
      output_iteration = func;
      default_iteration_output = func;
   }

   void set_uci_output_function(void (*func)(const char *, ...))
   {
      uci_output = func;
      default_uci_output = func;
   }

   void set_xboard_output_function(void (*func)(const char *, ...))
   {
      xboard_output = func;
      default_xboard_output = func;
   }

   void set_error_output_function(void (*func)(const char *, ...))
   {
      error_output = func;
      default_error_output = func;
   }

   move_t get_last_move() {
      if (moves_played) {
         return move_list[moves_played-1];
      }
      return 0;
   }

   int files, ranks, holdsize;
   int virtual_files, virtual_ranks;


   /* Variables */
   eval_t mate_score;         /* The score returned for mate, normally -LEGALWIN */
   eval_t stale_score;        /* The score returned for stale mate, normally -LEGALDRAW */
   eval_t rep_score;          /* The score returned for a repetition, normally -LEGALDRAW */
   eval_t bare_king_score;    /* The score penalty for a lone king.  Normally 0, but -LEGALWIN for shatranj */
   eval_t no_piece_score;     /* The score returned for having no pieces, normally -LEGALWIN */
   eval_t flag_score;         /* The score returned when flags are captured, normally -LEGALWIN */
   eval_t perpetual;          /* The score returned when an in-check position is repeated */
   eval_t check_score;        /* The score returned when the check-count exceeds the limit */

   int check_limit;           /* Terminate the game when the check count exceeds this limit */

   int repeat_claim;          /* Make a claim if the root position has been repeated this many times */

   eval_t resign_threshold;   /* Increment resign count if the evaluation is below this limit */
   eval_t draw_threshold;     /* Increment draw count if the absolute evaluation is below this limit */
   int resign_count;
   int draw_count;

   chess_clock_t clock;

   char *start_fen;           /* FEN string encoding the starting position of this game */
   char *xb_setup;            /* setup string that has to be sent to XBoard (startup position will be attached) */
   char *xb_parent;           /* parent variant for XBoard */
   
   int clock_nodes;

   int start_move_count;      /* Full-move counter at the beginning of the game. */
   size_t moves_played;       /* Number of moves played to current position */
   size_t last_move;          /* Number of the last move played in the game; useful when we take back a move */

   size_t max_moves;          /* Maximum number of moves that can be stored */
   move_t *move_list;         /* list of moves played in the game */
   int    *move_clock;        /* Time-control for engine clock */

   uint64_t branches_pruned;

   movelist_t *movelist;

   /* Data structure for retrieving the principle variation. At each depth,
    * there are two branches for the tree: the principle variation and the
    * "current" branch. If the current branch turns out to be better than
    * the PV, it becomes the new PV.
    * A binary tree would be enough to store this information (and be easy
    * to manipulate), but for now we use a plain NxN array (of which half
    * the space is wasted, obviously)
    */
   move_t principle_variation[MAX_TOTAL_DEPTH][MAX_TOTAL_DEPTH];
   int length_of_variation[MAX_TOTAL_DEPTH];
   move_t best_move[MAX_TOTAL_DEPTH];

   /* Transposition table */
   size_t hash_size;
   hash_table_t *transposition_table;

   /* Evaluation table */
   eval_hash_table_t *eval_table;

   /* Hash table for repetition detection */
   int8_t repetition_hash_table[0xFFFF+1];
   int8_t board_repetition_hash_table[0xFFFF+1];
   int8_t fifty_limit;
   int8_t fifty_scale_limit;

   /* Whether the engine is currently pondering or not, if it is, disable
    * time control.
    */
   bool pondering;
   move_t ponder_move;

   int multipv;
   movelist_t exclude;
   bool analysing;
   move_t analyse_move;
   movelist_t analyse_movelist;
   const char *analyse_fen;
   bool analyse_new;
   int analyse_undo;
   int analyse_moves_played;
   int option_ms;

   unsigned int random_key;
   eval_t random_amplitude;
   size_t random_ply_count;
   bool random_ok;
   bool castle_san_ok;

   bool trace;
   bool show_fail_high;
   bool show_fail_low;
   bool repetition_claim;

   level_t level;

   int square_to_bit[256];
   int bit_to_square[256];
   int top_left;

   /* Various function pointers, so we can easily customise things and
    * adjust to different UIs.
    */
   void (*output_iteration)(const char *, ...) ATTRIBUTE_FORMAT_PRINTF;
   void (*uci_output)(const char *, ...) ATTRIBUTE_FORMAT_PRINTF;
   void (*xboard_output)(const char *, ...) ATTRIBUTE_FORMAT_PRINTF;
   void (*error_output)(const char *, ...) ATTRIBUTE_FORMAT_PRINTF;

   bool (*check_keyboard)(struct game_t *game);

   /* Meta-data */
   char *name;

   void truncate_principle_variation(int depth) {
      length_of_variation[depth] = depth;
   }

   inline void backup_principle_variation(int depth, move_t move)
   {
      /* Copy principle variation */
      if (length_of_variation[depth+1] >= (depth+1)) {
         principle_variation[depth][depth] = move;
         for (int c=depth+1; c<length_of_variation[depth+1]; c++)
            principle_variation[c][depth] = principle_variation[c][depth+1];
         length_of_variation[depth] = length_of_variation[depth+1];
      }
   }

};


template <typename kind>
struct game_template_t : public game_t {
   board_t<kind> board;       /* pointer to the current board position */

   /* Rules */
   piece_description_t<kind> pt;
   board_t<kind> root_board;  /* The board position at the root of the search */

   /* State information */
   unmake_info_t<kind> *ui;

   movegen_t<kind> movegen;

   /* Killer moves, storage space requirements must come from the search
    * function.
    */
   move_t killer[MAX_TOTAL_DEPTH][2];
   move_t mate_killer[MAX_TOTAL_DEPTH];

   /* Botwinnik-Markov extension.
    * This is like a killer move or a refutation move, but for NULL moves.
    * When encountering it on consecutive plies, extend the search (or at least sort the move higher in the
    * list).
    */
   move_t null_killer[MAX_TOTAL_DEPTH];

   /* Counter moves, for use with the counter move heuristic.
    * Indexed by from- and to square and side to move.
    */
   move_t counter[8*sizeof(kind)][8*sizeof(kind)][NUM_SIDES];

   /* Combination moves, similar to counter moves except indexed by our own
    * previous moves.
    * Indexed by from- and to square and side to move.
    */
   move_t combo[8*sizeof(kind)][8*sizeof(kind)][2];

#if 0
   /* History reductions, as discussed by Ed Schroeder.
    * The idea is to reduce the depth of moves that have failed low in the
    * past. This requires a history table.
    * This is indexed by colour, piece type and destination square.
    * NB: I tried colour/from square/destination square, and it seems to be
    * worse, at least with the parameters given by Ed Schroeder. Probably
    * because using the piece type means we can catch a bad move more
    * easily even if we moved the piece first (Rf5 exf5 is bad independent
    * of whether the rook was on f8 or f7, this situation catches both).
    */
   int history_reduce[MAX_PIECE_TYPES][NUM_SIDES][8*sizeof(kind)];
#endif

   /* History heuristic */
   int history[NUM_SIDES][MAX_PIECE_TYPES][8*sizeof(kind)];
   int max_history[NUM_SIDES];
   int drop_history[NUM_SIDES][MAX_PIECE_TYPES][8*sizeof(kind)];
   int max_drop_history[NUM_SIDES];

   void init() {
      movegen = movegen_t<kind>();
      output_iteration = default_iteration_output;
      uci_output       = default_uci_output;
      xboard_output    = default_xboard_output;
      error_output     = default_error_output;
      xb_setup         = NULL;
      xb_parent        = NULL;
      check_keyboard   = NULL;
      max_moves = 0;
      move_list = NULL;
      move_clock = NULL;
      ui = NULL;

      board.flag[WHITE].clear();
      board.flag[BLACK].clear();

      transposition_table = NULL;
      eval_table = NULL;

      hash_size = default_hash_size;

      board.clear();
      memset(&pt, 0, sizeof(pt));
      board.piece_types = &pt;

      trace = false;
      show_fail_high = false;
      show_fail_low = false;
      repetition_claim = true;
      random_key = 0;
      random_amplitude = 0;
      random_ply_count = 0;
      random_ok = false;
      analysing = false;
      pondering = false;
      analyse_fen = NULL;
      analyse_new = false;
      analyse_undo = 0;
      option_ms = MATE_SEARCH_ENABLE_DROP;

      fifty_limit = 101;

      movelist = new movelist_t[MAX_TOTAL_DEPTH+2];

      keep_labels = false;
      for (int n=0; n<256; n++) {
         square_to_bit[n] = n;
         bit_to_square[n] = n;
      }

      for (int n=0; n<MAX_TOTAL_DEPTH; n++) {
         killer[n][0] = killer[n][1] = mate_killer[n] = null_killer[n] = 0;
      }
      memset(&pt, 0, sizeof pt);
      memset(&clock, 0, sizeof clock);
      memset(max_history, 0, sizeof max_history);
      memset(history, 0, sizeof history);
      memset(counter, 0, sizeof counter);
      memset(combo, 0, sizeof combo);

      memset(principle_variation, 0, sizeof principle_variation);
      memset(length_of_variation, 0, sizeof length_of_variation);
      memset(best_move, 0, sizeof best_move);

      virtual_files = -1;
      virtual_ranks = -1;

      /* Set some default rules */
      board.rule_flags = 0;
      mate_score = -LEGALWIN;
      stale_score = -LEGALDRAW;
      rep_score = -LEGALDRAW;
      no_piece_score = -LEGALWIN;
      flag_score = -LEGALWIN;
      perpetual = ILLEGAL+1;
      bare_king_score = 0;
      repeat_claim = 2;
      resign_threshold = -ILLEGAL;
      draw_threshold = 0;

      check_score = -LEGALWIN;
      check_limit = 0;

      clock_nodes = 0x00007FFF;
   }
   game_template_t<kind>() { init(); }
   game_template_t<kind>(int files, int ranks) { 
      assert(files*ranks <= 8*sizeof(kind));
      init(); 
      set_board_size(files, ranks);
   }

   ~game_template_t<kind>() {
      free(move_list);
      free(move_clock);
      free(ui);
      free(xb_setup);
      free(start_fen);
      free(name);

      for (int n=0; n<pt.num_piece_types; n++) {
         free(pt.piece_name[n]);
         free(pt.piece_abbreviation[n][WHITE]);
         free(pt.piece_notation[n]);
         free(pt.demotion_string[n]);
      }

      delete[] movelist;

      destroy_hash_table(transposition_table);
      destroy_eval_hash_table(eval_table);
   }

   void assess_piece_mate_potential(
      bitboard_t<kind> reach_from[MAX_PIECE_TYPES][8*sizeof(kind)],
      bitboard_t<kind> attack_from[MAX_PIECE_TYPES][8*sizeof(kind)],
      bitboard_t<kind> attack_to[MAX_PIECE_TYPES][8*sizeof(kind)]
   
   ) {
      int king[NUM_SIDES] = { -1, -1 };
      
      for (side_t side = WHITE; side <= BLACK; side++) {
         if (!(board.bbc[side] & board.royal).is_empty()) {
            king[side] = board.get_piece((board.bbc[side] & board.royal).bitscan());
            continue;
         }

         assert(king[side] == -1);

         for (int n = 0; n<pt.num_piece_types; n++) {
            if (board.holdings[n][side] && (pt.piece_flags[n] & PF_ROYAL)) {
               king[side] = n;
               break;
            }
         }
      }

      if (king[WHITE] != -1 && king[BLACK] != -1) {
         bitboard_t<kind> dkzone = bitboard_t<kind>::board_all & bitboard_t<kind>::board_north & bitboard_t<kind>::board_westward[files/2];
         int num_dks = dkzone.popcount();
#if defined _MSC_VER
         std::vector<int> dks_list(num_dks);
#else
         int dks_list[num_dks];
#endif
         int dki = 0;
         while (!dkzone.is_empty()) {
            bitboard_t<kind> mask = bitboard_t<kind>::board_all;
            if (!(dkzone & bitboard_t<kind>::board_east_edge).is_empty()) mask &= bitboard_t<kind>::board_east_edge;
            if (!(dkzone & mask & bitboard_t<kind>::board_north_edge).is_empty()) mask &= bitboard_t<kind>::board_north_edge;
            int sq = (mask & dkzone).bitscan();
            dkzone.reset(sq);
            dks_list[dki++] = sq;
         }
         /* Mate potential for single pieces: detect if a mate position
          * exists
          */
         for (int n=0; n<pt.num_piece_types; n++) {
            pt.piece_flags[n] |= PF_CANTMATE;
            if ((pt.piece_flags[n] & PF_ROYAL) && pt.piece_maximum[n][WHITE] == 1 && pt.piece_maximum[n][BLACK] == 1) continue;
            for (int dki=0; dki<num_dks; dki++) {
               int dks = dks_list[dki];
               for (int aks = 0; aks < files*ranks; aks++) {
                  if (!(pt.piece_flags[n] & PF_CANTMATE)) break;
                  bitboard_t<kind> dk, ak, dkm, akm;
                  dk.set(dks);
                  ak.set(aks);
                  dkm = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[king[BLACK]], dks, (dk|ak), BLACK);
                  akm = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[king[WHITE]], aks, (dk|ak), WHITE);
                  dkm &= ~dk;

                  /* Kings should not attack eachother.
                   * The attacking king should not block all escape squares (making the position unreachable).
                   * The kings should at least influence eachother, however.
                   */
                  if ( !((ak|akm) & dk).is_empty() ) continue;
                  if ( !((dk|dkm) & ak).is_empty() ) continue;
                  if ( ((ak|akm) & dkm) == dkm ) continue;
                  if ((attack_to[king[WHITE]][aks] & (dkm|dk)).is_empty()) continue;

                  for (int ps = 0; ps < files*ranks; ps++) {
                     if ( (ak|dk).test(ps) ) continue;
                     if ((attack_to[n][ps] & (dkm|dk)).is_empty()) continue;
                     bitboard_t<kind> p, pm;
                     p.set(ps);
                     pm = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[n], ps, (ak|p), WHITE);

                     if ( ((pm | akm) & (dkm|dk)) == (dkm|dk) ) {
                        pt.piece_flags[n] &= ~PF_CANTMATE;
                        break;
                     }
                  }
               }
            }
         }

         /* Mate potential for pairs of pieces.
          * This is slightly more complicated because we need to do some
          * retrograde analysis to test whether the mate can be forced at
          * all.
          * TODO: only consider piece placement where it makes sense:
          * defending king near a corner, and attacking king where it takes
          * away some escape squares. Similarly, the other pieces should at
          * least attack some of the escape squares.
          * TODO: Do not look for mating pairs if royals are not allowed to
          * slide through check, it's not so useful (and far too slow).
          */
         if (!(board.rule_flags & RF_NO_MOVE_PAST_CHECK))
         for (int n1=0; n1<pt.num_piece_types; n1++) {
            for (int n2=n1; n2<pt.num_piece_types; n2++) {
               if (pt.pieces_can_win[n1][n2]) break;
               if ( !(pt.piece_flags[n1] & PF_CANTMATE) || !(pt.piece_flags[n2] & PF_CANTMATE) ) {
                  pt.pieces_can_win[n1][n2] = pt.pieces_can_win[n2][n1] = true;
                  continue;
               }
               if ( (pt.piece_flags[n1] & PF_NORET) || (pt.piece_flags[n2] & PF_NORET) ) continue;
               if ( (pt.piece_flags[n1] & PF_ROYAL) || (pt.piece_flags[n2] & PF_ROYAL) ) continue;

               /* Place defending king */
               for (int dks = pack_rank_file(ranks-1, 0); dks <= pack_rank_file(ranks-1, 1); dks++) {
                  if (pt.pieces_can_win[n1][n2]) continue;

                  /* Place attacking king */
                  for (int aks = 0; aks < files*ranks; aks++) {
                     if (pt.pieces_can_win[n1][n2]) break;
                     bitboard_t<kind> dk, ak, dkm, akm;
                     dk.set(dks);
                     ak.set(aks);
                     dkm = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[king[BLACK]], dks, (dk|ak), BLACK);
                     akm = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[king[WHITE]], aks, (dk|ak), WHITE);
                     dkm &= ~dk;

                     /* Kings should not attack eachother.
                      * The attacking king should not block all escape squares (making the position unreachable).
                      * The kings should at least influence eachother, however.
                      */
                     if ( !((ak|akm) & dk).is_empty() ) continue;
                     if ( !((dk|dkm) & ak).is_empty() ) continue;
                     if ( ((ak|akm) & dkm) == dkm ) continue;
                     if ((attack_to[king[WHITE]][aks] & (dkm|dk)).is_empty()) continue;

                     /* Second and third piece */
                     int nn[2] = {n1, n2};
                     int ps[2];
                     for (ps[0] = 0; ps[0] < files*ranks; ps[0]++) {
                        if ( (ak|dk).test(ps[0]) ) continue;
                        bitboard_t<kind> p[2];
                        p[0].set(ps[0]);

                        if ((attack_to[nn[0]][ps[0]] & (dkm|dk)).is_empty()) continue;
                        for (ps[1] = 0; ps[1] < files*ranks; ps[1]++) {
                           if ( (ak|dk|p[0]).test(ps[1]) ) continue;
                           p[1].clear();
                           p[1].set(ps[1]);

                           if ((attack_to[nn[1]][ps[1]] & (dkm|dk)).is_empty()) continue;

                           bitboard_t<kind> pa[2];
                           pa[0] = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[nn[0]], ps[0], (ak|p[0]|p[1]), WHITE);
                           pa[1] = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[nn[1]], ps[1], (ak|p[0]|p[1]), WHITE);

                           /* Is this mate? If not, skip. */
                           bitboard_t<kind> full_attack = akm | pa[0] | pa[1];
                           if ((full_attack & (dkm|dk)) != (dkm|dk)) continue;

                           /* Identify checking piece; skip double-check. */
                           int c = 0;
                           if (!(pa[1] & dk).is_empty()) c = 1;
                           if (!(pa[0] & dk).is_empty() && c == 1) continue;

                           /* Find all squares the defending king could
                            * have come from, prior to stepping into the
                            * corner.
                            */
                           bitboard_t<kind> pk = dkm & ~(akm | ak | p[1-c]);

                           /* Now find all alternative escape squares */
                           bitboard_t<kind> bb = pk;
                           while(!bb.is_empty()) {
                              int square = bb.bitscan();
                              bb.reset(square);

                              bitboard_t<kind> escape = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[king[BLACK]], square, (dk|ak), BLACK);
                              escape &= ~(ak | akm | pa[1-c]);

                              /* Now find all places the checking piece could
                               * have come from.
                               */
                              bitboard_t <kind> sentry = reach_from[nn[c]][ps[c]] & ~(ak | dk | p[1-c]) & ~attack_from[nn[c]][dks];

                              if (escape == dk) {
                                 bitboard_t<kind> alt_escape = pk;
                                 alt_escape.reset(square);

                                 bitboard_t<kind> sp = sentry;
                                 bool exit = false;
                                 while (!sp.is_empty()) {
                                    int s1 = sp.bitscan();
                                    sp.reset(s1);

                                    bitboard_t<kind> mask = alt_escape | p[c];

                                    mask &= ~attack_from[nn[c]][s1];

                                    if (mask.is_empty()) {
                                       pt.pieces_can_win[n1][n2] = pt.pieces_can_win[n2][n1] = true;
                                       exit = true;
                                       break;
                                    }
                                 }

                                 /* Alternative that distinguishes KFFK and
                                  * KBBK: if the alternate piece can cover
                                  * both its present location and the
                                  * alternate escape, that would also work.
                                  */
                                 sp = p[1-c] | alt_escape;
                                 for (int s2 = 0; s2<files*ranks; s2++) {
                                    if ((reach_from[nn[1-c]][s2] & sp) == sp) {
                                       pt.pieces_can_win[n1][n2] = pt.pieces_can_win[n2][n1] = true;
                                       exit = true;
                                    }
                                    if (exit) break;
                                 }
                                 continue;
                              }

                              /* If there are no alternative escape squares,
                               * the position is unreachable.
                               */
                              if (escape.is_empty()) continue;

                              /* Now find squares where the mating piece could
                               * have covered those squares.
                               */
                              while (!escape.is_empty()) {
                                 int square = escape.bitscan();
                                 escape.reset(square);

                                 sentry &= attack_from[nn[c]][square];
                              }

#if 0
                              if (n1 == 6 && n2 == 7 && !sentry.is_empty()) {
                                 board_t<kind> board;
                                 board.piece_types = &pt;

                                 board.put_piece(king[BLACK], BLACK, dks);
                                 board.put_piece(king[WHITE], WHITE, aks);
                                 board.put_piece(nn[0], WHITE, ps[0]);
                                 board.put_piece(nn[1], WHITE, ps[1]);
                                 board.print();
                                 pa[0].print();
                                 printf("\n");
                                 pa[1].print();
                                 printf("\n");
                                 dk.print();
                              }
#endif

                              /* If this set is not empty, then we could have delivered mate */
                              if (!sentry.is_empty()) pt.pieces_can_win[n1][n2] = pt.pieces_can_win[n2][n1] = true;
                           }
                        }
                     }
                  }
               }
            }
         }
      }

   }

   void initialise_tropism_tables() {
      bitboard_t<kind> occ;
      memset(pt.tropism, 127, sizeof pt.tropism);
      for (int piece = 0; piece<pt.num_piece_types; piece++) {

         /* Make repeated moves until we cover the entire board.
          * NB: the detection for colour bound pieces is a bit stupid, but
          * should work without touching it ever again (even if others are
          * added) because we just keep trying until we've exceeded a safe
          * number of tries.
          * This score is then forced to 0.
          */
         for (int s1=0; s1<files*ranks; s1++) {
            bitboard_t<kind> kn; kn.set(s1);
            int n = 0;
            while (kn != bitboard_t<kind>::board_all && n < 32) {
               bitboard_t<kind> bb = kn;
               while (!bb.is_empty()) {
                  int s2 = bb.bitscan();
                  bb.reset(s2);

                  kn |= movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[piece], s2, occ, WHITE);

                  if (n < pt.tropism[piece][s1][s2])
                     pt.tropism[piece][s1][s2] = n;
               }
               n++;
            }
            for (int s2=0; s2<ranks*files; s2++) {
               if (n < pt.tropism[piece][s1][s2])
                  pt.tropism[piece][s1][s2] = n;
               if (pt.tropism[piece][s1][s2] == 32) {
                  pt.tropism[piece][s1][s2] = 0;
               }
            }
         }

#if 0
         printf("%s tropism table:\n", pt.piece_name[piece]);
         int s1 = files/2;
         for(int r=0; r<ranks; r++) {
            for(int f=0; f<files; f++) {
               int s2 = pack_rank_file(r, f);
               printf(" %2d ", pt.tropism[piece][s1][s2]);
            }
            printf("\n");
         }
#endif
      }
   }

   // FIXME: we only need this because (apparently) MSVC doesn't handle the
   // conversion eval_t*double -> eval_t correctly.
   // Slightly cleaner (on this end) would be to just implement this
   // directly by making eval_t a class.
   static void scale_eval(eval_t &ev, double scl) {
      ev = eval_t(ev*scl);
   }

   /* Crude guestimates for piece values.
    * This is largely a matter of numerology, but it is better than nothing...
    * See http://www.chess.com/forum/view/chess960-chess-variants/mathematics-of-relative-chess-piece-value?quote_id=4383142
    * for a discussion of the relative worth of forward and capture moves.
    */
   void initialise_piece_values() {
      bool set = false;
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_value[n] != 0) continue;
         if (pt.piece_flags[n] & PF_ROYAL) {
            pt.piece_value[n] = LEGALWIN/2;
            continue;
         }
         set = true;

         int cs = pack_rank_file(ranks/2, files/2);
         bitboard_t<kind> init; init.set(cs);
         bitboard_t<kind> occ;
         bitboard_t<kind> forward = bitboard_t<kind>::board_northward[ranks/2];
         bitboard_t<kind> backward = bitboard_t<kind>::board_southward[ranks/2];
         bitboard_t<kind> sideward = bitboard_t<kind>::board_eastward[files/2] | bitboard_t<kind>::board_westward[files/2];
         bitboard_t<kind> move = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[n], cs, occ, WHITE);
         bitboard_t<kind> atk  = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[n], cs, occ, WHITE);

         bitboard_t<kind> board33 = init;
         board33 |= (board33 << 1) | (board33 >> 1);
         board33 |= (board33 << files) | (board33 >> files);
         bitboard_t<kind> board55 = board33;
         board55 |= (board55 << 1) | (board55 >> 1);
         board55 |= (board55 << files) | (board55 >> files);

         /* Get evaluation weight.
          * Penalise short-range pieces for which the 3x3 and 5x5 attack sets coincide.
          * Penalise colour-bound pieces.
          */
         int move_count5   = (move & board55).popcount();
         int move_count3   = (move & board33).popcount();
         int attack_count5 = (move & board55).popcount();
         int attack_count3 = (move & board33).popcount();

         int forward_move_count5   = (move & board55 & forward).popcount();
         int forward_move_count3   = (move & board33 & forward).popcount();
         int forward_attack_count5 = (move & board55 & forward).popcount();
         int forward_attack_count3 = (move & board33 & forward).popcount();

         int backward_move_count5   = (move & board55 & backward).popcount();
         int backward_move_count3   = (move & board33 & backward).popcount();
         int backward_attack_count5 = (move & board55 & backward).popcount();
         int backward_attack_count3 = (move & board33 & backward).popcount();

         int sideward_move_count5   = (move & board55 & sideward).popcount();
         int sideward_move_count3   = (move & board33 & sideward).popcount();
         int sideward_attack_count5 = (move & board55 & sideward).popcount();
         int sideward_attack_count3 = (move & board33 & sideward).popcount();

         if ( ((move|init)&bitboard_t<kind>::board_dark).is_empty() || ((move|init)&bitboard_t<kind>::board_light).is_empty() ) {
            move_count3 = 0;
            attack_count3 = 0;
         }

         /* Captures count for 2/3 of the piece value */
         attack_count5 *= 2;
         attack_count3 *= 2;

         if (forward_move_count5 != backward_move_count5) {
            move_count5   += (2*forward_move_count5 + sideward_move_count5)/3;
            move_count3   += forward_move_count3;
            attack_count5 += (2*forward_attack_count5 + sideward_attack_count5)/3;
            attack_count3 += forward_attack_count3;
         }

         if (sideward_move_count5 == 0) move_count3 = 0;
         if (sideward_attack_count5 == 0) attack_count3 = 0;

         if (move_count3   == move_count5   &&   move_count5 != forward_move_count5  ) move_count3 = 0;
         if (attack_count3 == attack_count5 && attack_count5 != forward_attack_count5) attack_count3 = 0;

         /* Guestimate piece value, normalised such that a rook (with
          * attack rank 12) is ~500.
          * This gets the pawn value wrong, however, since that also
          * depends on the possibility of promotion.
          */
         pt.piece_value[n] = ((attack_count5 + move_count5 + attack_count3 + move_count3) * 500) / (3*12);
      }

      if (!set) return;

      /* Add contribution from promotions */
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_promotion_choice[n] == 0) continue;

         /* Get value of most valuable promotion piece */
         int16_t best_promo = 0;

         piece_bit_t pc = pt.piece_promotion_choice[n];
         while (pc) {
            int k = bitscan32(pc);
            pc ^= (1 << k);

            best_promo = std::max(best_promo, pt.piece_value[k]);
         }
         pt.piece_value[n] += int16_t(best_promo * 0.06);
      }

      /* Demotions.
       * Here there are two effects:
       *  1. If the piece is not unique, it is more valuable if its
       *     demotion piece is weaker.
       *  2. If it is unique, then it depends on how the piece moves and
       *     the value of the piece it demotes to.
       */
      for (int n=0; n<pt.num_piece_types; n++) {
         int k = pt.demotion[n];
         if (k != n) {
            if (pt.piece_value[k] == 0) continue;

            /* Does this piece have a unique move/capture? */
            bool unique = true;
            int id = 0;
            for (int c=0; c<pt.num_piece_types && unique; c++) {
               id = c;
               if (pt.demotion[c] != c) continue;
               unique = !(pt.piece_move_flags[c] == pt.piece_move_flags[n] && pt.piece_capture_flags[c] == pt.piece_capture_flags[n]);
            }

            if (!unique && pt.piece_value[id] != 0) {
               //printf("%s(=%s)->%s (%d %d %d)\n", pt.piece_name[n], pt.piece_name[id], pt.piece_name[k],pt.piece_value[id], pt.piece_value[k], pt.piece_value[id] - pt.piece_value[k]);
               pt.piece_value[n] += 100*(pt.piece_value[id] - pt.piece_value[k])/pt.piece_value[id];
            } else {
               pt.piece_value[n] += pt.piece_value[k] / 20;
            }
         }
      }

      /* Down-scale material if we're playing a drop variant */
      if (board.rule_flags & RF_USE_CAPTURE)
      for (int n=0; n<pt.num_piece_types; n++)
         pt.piece_value[n] /= 3;
   }

   void initialise_piece_evaluation_terms() {
      bitboard_t<kind> reach_from[MAX_PIECE_TYPES][8*sizeof(kind)];
      bitboard_t<kind> attack_from[MAX_PIECE_TYPES][8*sizeof(kind)];
      bitboard_t<kind> attack_to[MAX_PIECE_TYPES][8*sizeof(kind)];
      bitboard_t<kind> moves[MAX_PIECE_TYPES][8*sizeof(kind)][NUM_SIDES];
      int ranks = bitboard_t<kind>::board_ranks;
      int files = bitboard_t<kind>::board_files;
      pt.pawn_index[WHITE] = -1;
      pt.pawn_index[BLACK] = -1;
      pt.royal_pieces = 0;
      pt.defensive_pieces = 0;
      pt.pawn_pieces = 0;
      pt.minor_pieces = 0;
      pt.major_pieces = 0;
      pt.super_pieces = 0;
      pt.shak_pieces = 0;

      memset(reach_from, 0, sizeof reach_from);
      memset(attack_from, 0, sizeof attack_from);

      setup_fen_position(start_fen);

      /* Classify the opening position as "closed" (unbroken pawn chain) or
       * "open" (open files exist).
       * This affects whether centre play is an important concept for pawns
       * or not.
       */

      for (int n=0; n<pt.num_piece_types; n++) {
         /* Assume this piece can not move backward unless proven otherwise */
         pt.piece_flags[n] |= PF_NORET;

         int cs = pack_rank_file(ranks/2, files/2);
         bitboard_t<kind> init; init.set(cs);
         bitboard_t<kind> occ;
         bitboard_t<kind> move = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[n], cs, occ, WHITE);
         bitboard_t<kind> from = init;
         bitboard_t<kind> to = move;

         /* Test whether this piece is colour bound */
         if ( ((from|to) & bitboard_t<kind>::board_dark).is_empty() || ((from|to) & bitboard_t<kind>::board_light).is_empty() ) {
            pt.piece_flags[n] |= PF_PAIRBONUS | PF_COLOURBOUND;
            pt.eval_pair_bonus[n].mg = eval_t(PAIR_BONUS_MG * pt.piece_value[n]);
            pt.eval_pair_bonus[n].eg = eval_t(PAIR_BONUS_EG * pt.piece_value[n]);
         }

         /* Test whether this piece can return to its starting square */
         for (int k = 0; k < files; k++) {
            if ( !(pt.piece_flags[n] & PF_NORET) ) break;

            from = to;
            to.clear();
            while (!from.is_empty() && (to & init).is_empty()) {
               int fs = from.bitscan();
               from.reset(fs);
               to |= movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[n], fs, occ, WHITE);
            }

            if (!(to & init).is_empty())
               pt.piece_flags[n] &= ~PF_NORET;
         }

         /* Hack: the above algorithm detects if the piece can return to
          * its origin for some of its moves, but not for all of its moves.
          * Thus, it fails to detect the irreversibility of forward pawn
          * pushes in Xiangqi.
          * We "fix" this by explicitly testing for this situation. The
          * correct fix is to try each move on the initial square, and only
          * mark the piece as reversible if ALL of them can return.
          */
         if ( (move & bitboard_t<kind>::board_southward[ranks/2]).is_empty() )
            pt.piece_flags[n] |= PF_NORET;

         /* Determine weight of this piece for king attacks */
         pt.king_safety_weight[n] = 1;

         bitboard_t<kind> board33 = init;
         board33 |= (board33 << 1) | (board33 >> 1);
         board33 |= (board33 << files) | (board33 >> files);
         bitboard_t<kind> board55 = board33;
         board55 |= (board55 << 1) | (board55 >> 1);
         board55 |= (board55 << files) | (board55 >> files);

         /* Get evaluation weight.
          * Penalise short-range pieces for which the 3x3 and 5x5 attack sets coincide.
          * Penalise colour-bound pieces.
          */
         int count5 = (move & board55).popcount();
         int count3 = (move & board33).popcount();
         if ( (pt.piece_flags[n] & PF_COLOURBOUND) && !(pt.piece_flags[n] & PF_NORET) ) count3 = 0;

         if (count3 == count5)
            pt.king_safety_weight[n] = count5;
         else
            pt.king_safety_weight[n] = count5 + count3;

         //printf("%10s  %d %d\n", pt.piece_name[n], count3, count5);
         if ((pt.piece_flags[n] & PF_NORET) && (count3 == count5)) {
            if (!(board.bbc[WHITE] & board.bbp[n]).is_empty()) pt.pawn_index[WHITE] = n;
            if (!(board.bbc[BLACK] & board.bbp[n]).is_empty()) pt.pawn_index[BLACK] = n;
         }

         /* Weighting for game-phase */
         pt.phase_weight[n] = pt.king_safety_weight[n];
         if (pt.piece_flags[n] & PF_ROYAL) pt.phase_weight[n] = 0;
         if (pt.pawn_index[WHITE] == n   ) pt.phase_weight[n] = 0;
         if (pt.pawn_index[BLACK] == n   ) pt.phase_weight[n] = 0;

         /* Mark all squares that could be reached from this square for
          * reverse lookup.
          * Collect global information about mobility while we're at it.
          */
         pt.max_moves[n] = 0;
         pt.avg_moves[n] = 0;
         pt.min_moves[n] = ranks*files;
         from = bitboard_t<kind> :: board_all;
         int mobility[8*sizeof(kind)] = { 0 };
         while (!from.is_empty()) {
            int fs = from.bitscan();
            from.reset(fs);

            moves[n][fs][WHITE] = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[n], fs, occ, WHITE);
            moves[n][fs][BLACK] = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[n], fs, occ, BLACK);
            moves[n][fs][WHITE] &= pt.prison[WHITE][n];
            moves[n][fs][BLACK] &= pt.prison[BLACK][n];
            move = moves[n][fs][WHITE];
            int mc = move.popcount();
            mobility[fs] = std::max(1, mc);
            pt.avg_moves[n] += mc;
            if (pt.max_moves[n] < mc) pt.max_moves[n] = mc;
            if (pt.min_moves[n] > mc) pt.min_moves[n] = mc;

            bitboard_t<kind> bb = move;
            while (!bb.is_empty()) {
               int square = bb.bitscan();
               bb.reset(square);

               bitboard_t<kind> move = movegen.generate_move_bitboard_for_flags(pt.piece_move_flags[n], square, occ, WHITE);
               mobility[fs] += move.popcount()/2;
            }

            while (!move.is_empty()) {
               int ts = move.bitscan();
               move.reset(ts);
               reach_from[n][ts].set(fs);
            }

            move = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[n], fs, occ, WHITE);
            attack_to[n][fs] |= move;
            while (!move.is_empty()) {
               int ts = move.bitscan();
               move.reset(ts);
               attack_from[n][ts].set(fs);
            }
         }
         pt.avg_moves[n] = (pt.avg_moves[n] + ranks*files/2) / (ranks*files);

         /* Centre the mobility table */
         int avg = 0;
         for (int square=0; square<files*ranks; square++)
            avg += mobility[square];
         avg = (avg + ranks*files/2) / (ranks*files);
         for (int square=0; square<files*ranks; square++) {    /* Centre the mobility table */
            pt.mobility_score[n][square] = 3*(mobility[square] - avg)*pt.avg_moves[n]/avg;
         }

#if 0
         printf("%s %d\n", pt.piece_name[n], pt.avg_moves[n]);
         for(int r=0; r<ranks; r++) {
            for(int f=0; f<files; f++) {
               int square = pack_rank_file(r, f);
               printf(" %2d ", pt.mobility_score[n][square]);
            }
            printf("\n");
         }
#endif
      }

      initialise_tropism_tables();

      /* De-value pawns in the middle game, so pawns become more valuable
       * in the end game. This has the natural effect of building in a
       * trade-down bonus.
       */
      if (pt.pawn_index[WHITE] >= 0)
        scale_eval(pt.eval_value[pt.pawn_index[WHITE]].mg, PAWN_SCALE_MG);
      if (pt.pawn_index[BLACK] >= 0 && pt.pawn_index[BLACK] != pt.pawn_index[WHITE])
        scale_eval(pt.eval_value[pt.pawn_index[BLACK]].mg, PAWN_SCALE_MG);

      /* Detect defensive pieces, which do not contribute to the game phase. */
      for (int n =0; n<pt.num_piece_types; n++) {
         if ( (pt.prison[WHITE][n] & bitboard_t<kind>::board_north).is_empty() ) pt.defensive_pieces |= (1 << n);
         if ( (pt.prison[BLACK][n] & bitboard_t<kind>::board_south).is_empty() ) pt.defensive_pieces |= (1 << n);

         if (pt.defensive_pieces & (1 << n)) {
            pt.piece_flags[n] |= PF_PAIRBONUS;
            pt.eval_pair_bonus[n].mg = eval_t(PAIR_BONUS_MG * pt.piece_value[n]);
            pt.eval_pair_bonus[n].eg = eval_t(PAIR_BONUS_EG * pt.piece_value[n]);
            pt.phase_weight[n] = 0;
         }
      }

      /* Make lame leapers increase in value as pieces are exchanged.
       * Conversely, hoppers become (much) less valuable in the end game.
       */
      for (int n =0; n<pt.num_piece_types; n++) {
         if (pt.defensive_pieces & (1 << n)) {
            scale_eval(pt.eval_value[n].mg, DEF_SCALE_MG);
            scale_eval(pt.eval_value[n].eg, DEF_SCALE_EG);
            continue;
         }
         if (is_masked_leaper(pt.piece_move_flags[n]) || is_masked_leaper(pt.piece_capture_flags[n])) {
            scale_eval(pt.eval_value[n].mg, LAME_SCALE_MG);
            scale_eval(pt.eval_value[n].eg, LAME_SCALE_EG);
         }
         if (is_hopper(pt.piece_move_flags[n]) || is_hopper(pt.piece_capture_flags[n])) {
            scale_eval(pt.eval_value[n].eg, HOP_SCALE_EG);
         }
      }



      /* Pawn passer/structure masks */
      memset(pt.weak_mask,   0, sizeof pt.weak_mask);
      memset(pt.passer_mask, 0, sizeof pt.passer_mask);
      memset(pt.front_span,  0, sizeof pt.front_span);
      bitboard_t<kind> front_attack_span[NUM_SIDES][8*sizeof(kind)];
      for (side_t side=WHITE; side<NUM_SIDES; side++) {
         if (pt.pawn_index[side] < 0) continue;
         move_flag_t move_flags = pt.piece_move_flags[pt.pawn_index[side]];
         move_flag_t attack_flags = pt.piece_capture_flags[pt.pawn_index[side]];

         if (pt.piece_promotion_choice[pt.pawn_index[side]] == 0) continue;

         for (int square=0; square<files*ranks; square++) {
            bitboard_t <kind> moves;
            moves.set(square);
            front_attack_span[side][square] |= movegen.generate_move_bitboard_from_squares_for_flags(attack_flags, moves, bitboard_t<kind>::board_empty, side);
            int n = 0;
            while (!moves.is_empty() && n < ranks*files) {
               moves = movegen.generate_move_bitboard_from_squares_for_flags(move_flags, moves, bitboard_t<kind>::board_empty, side);
               moves &= ~((bitboard_t<kind>::board_eastward[unpack_file(square)]  |
                           bitboard_t<kind>::board_westward[unpack_file(square)]) & 
                           bitboard_t<kind>::board_rank[unpack_rank(square)]
                           );
               pt.front_span[side][square] |= moves;
               n++;
            }
            front_attack_span[side][square] |= movegen.generate_move_bitboard_from_squares_for_flags(attack_flags, pt.front_span[side][square], bitboard_t<kind>::board_empty, side);
         }
      }
      for (side_t side=WHITE; side<NUM_SIDES; side++) {
         if (pt.pawn_index[side] < 0) continue;

         for (int as=0; as<files*ranks; as++) {
            for (int ds=0; ds<files*ranks; ds++) {
               bitboard_t <kind> block = pt.front_span[next_side[side]][ds] | front_attack_span[next_side[side]][ds];
               block.set(ds);
               if ( !(block & pt.front_span[side][as]).is_empty() )
                  pt.passer_mask[side][as].set(ds);
            }
         }
      }

      /* Weak pawns: pawns that cannot be protected because they are
       * backward or isolated.
       * Pawns are weak if there are no friendly pawns in the registered
       * mask.
       */
      bitboard_t<kind> back_attack_span[NUM_SIDES][8*sizeof(kind)];
      memset(back_attack_span, 0, sizeof back_attack_span);

      for (side_t side=WHITE; side<NUM_SIDES; side++) {
         if (pt.pawn_index[side] < 0) continue;
         move_flag_t move_flags   = pt.piece_move_flags[pt.pawn_index[side]];
         move_flag_t attack_flags = pt.piece_capture_flags[pt.pawn_index[side]];

         if (move_flags == attack_flags) continue;

         bitboard_t<kind> back_span[8*sizeof(kind)];
         memset(back_span, 0, sizeof back_span);

         for (int as=0; as<files*ranks; as++) {
            for (int ds=0; ds<files*ranks; ds++) {
               if (pt.front_span[side][ds].test(as))
                  back_span[as].set(ds);
            }
            back_attack_span[side][as] |= movegen.generate_move_bitboard_from_squares_for_flags(attack_flags, back_span[as], bitboard_t<kind>::board_empty, side);

            pt.weak_mask[side][as] = back_attack_span[side][as];
         }

         //int square = pack_rank_file(ranks/2, files/2);
         //printf("%s\n", square_names[square]);
         //back_span[square].print("\n");
         //back_attack_span[side][square].print("\n");
         //if (side == BLACK) exit(0);
      }

      /* Determine mate potential for pieces */
      assess_piece_mate_potential(reach_from, attack_from, attack_to);

      /* Piece classification, for evaluation purposes:
       *  - Royal: a royal piece type
       *  - Pawn: the most basic piece; can promote
       *  - Minor: a piece that cannot deliver mate (knight/bishop class)
       *  - Major: a piece that can deliver mate (rook class)
       *  - Super: an extremely strong piece (queen or better)
       */
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_flags[n] & PF_ROYAL) pt.royal_pieces |= (1<<n);
         if (pt.piece_flags[n] & PF_SHAK) pt.shak_pieces |= (1<<n);
         if (pt.pawn_index[WHITE] == n) pt.pawn_pieces |= (1<<n);
         if (pt.pawn_index[BLACK] == n) pt.pawn_pieces |= (1<<n);
         if (pt.piece_flags[n] & PF_CANTMATE) pt.minor_pieces |= (1<<n);
         if (!(pt.piece_flags[n] & PF_CANTMATE) && pt.king_safety_weight[n] < 20) pt.major_pieces |= (1<<n);
         if (!(pt.piece_flags[n] & PF_CANTMATE) && pt.king_safety_weight[n] >= 20) pt.super_pieces |= (1<<n);
      }

      /* Test whether it makes sense to defer promotion for different piece
       * types: if the moves of the promoted piece are a superset of the
       * unpromoted piece there is no real reason not to do this.
       */
      pt.deferral_allowed = 0;
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_promotion_choice[n] == 0) continue;
         bool pointless = true;
         for (int k = 0; k<pt.num_piece_types && pointless; k++) {
            if ((pt.piece_promotion_choice[n] & (1<<k)) == 0) continue;
            bool subset = true;

            for (int square=0; square<files*ranks; square++) {
               if ((moves[k][square][WHITE] & moves[n][square][WHITE]) != moves[n][square][WHITE]) {
                  subset = false;
                  break;
               }
            }

            pointless = pointless && subset;

            //if (subset) printf("Promotion of %s->%s is compatible\n", pt.piece_notation[n], pt.piece_notation[k]);
         }
         //if (pointless) printf("Deferral of promotion of %s is pointless\n", pt.piece_name[n]);
         if (!pointless) pt.deferral_allowed |= (1<<n);
      }


      /* Piece-classes are orthogonal, except that royal pieces can also be
       * classed as minor/major/super.
       * Purely defensive pieces are always classified as "minor".
       *
       * For end-game scoring:
       *  major vs (non-defensive) minor -> draw
       *  major+minor vs major -> draw
       *  super vs minor or major -> won
       */
      pt.pawn_pieces &= ~pt.royal_pieces;
      pt.defensive_pieces &= ~(pt.royal_pieces | pt.pawn_pieces);
      pt.minor_pieces &= ~pt.pawn_pieces;
      pt.minor_pieces |= pt.defensive_pieces;
      pt.major_pieces &= ~(pt.minor_pieces | pt.pawn_pieces);
      pt.super_pieces &= ~(pt.major_pieces | pt.minor_pieces | pt.pawn_pieces);

      /* If we're not using a "shak" rule, set all pieces as able to give
       * "shak" (all checks are good for winning).
       */
      if (!(board.rule_flags & RF_USE_SHAKMATE))
         pt.shak_pieces = ~0;

      /* Classify the opening position as "closed" (unbroken pawn chain) or
       * "open" (open files exist).
       * This affects whether centre play is an important concept for pawns
       * or not.
       */
      bitboard_t<kind> pawns;
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.pawn_pieces & (1 << n)) pawns |= board.bbp[n];
      }
      pawns = pawns.fill_south();
      bool open_position = false;
      for (int s = 0; s<files; s++)
         if (pawns.test(s) == 0) open_position = true;

      /* Total scale-factor for game phase */
      pt.phase_scale = 0;
      for (int n=0; n<pt.num_piece_types; n++) {
         pt.phase_scale += pt.phase_weight[n] * board.bbp[n].popcount();
         pt.phase_scale += pt.phase_weight[n] * (board.holdings[n][WHITE] + board.holdings[n][BLACK]);
      }
      if (pt.phase_scale < GAME_PHASE_FLOOR) pt.phase_scale = GAME_PHASE_FLOOR;

      /* Mobility weights, dependent on piece-type.
       * The shape of the function is based on Senpai.
       */
      for (int n=0; n<pt.num_piece_types; n++) {
         for (int k=0; k<files*ranks; k++) {
            pt.eval_mobility[n][k].mg = eval_t(MOBILITY_SCALE_MG*(k - pt.avg_moves[n]));
            pt.eval_mobility[n][k].eg = eval_t(MOBILITY_SCALE_EG*(k - pt.avg_moves[n]));
            //pt.eval_mobility[n][k] = 128*(1.0 - exp(-k));
         }
      }

      /* Identify optional promotion zones.
       * This can be overridden by specifying it explicitly, but the
       * default rule is:
       * Promotions are optional if the piece can still move while in the
       * promotion zone.
       * We skip this if the optional promotion zone has been set from the
       * config file (pzset is true), even if the optional promotion zone
       * is empty.
       */
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.pzset[n]) continue;
         for (side_t side=WHITE; side<NUM_SIDES; side++) {
            if (!pt.optional_promotion_zone[side][n].is_empty()) continue; /* Don't override a user-specified setting */
            for (int square=0; square<files*ranks; square++) {
               if (!pt.promotion_zone[side][n].test(square)) continue;
               
               if (!moves[n][square][side].is_empty())
                  pt.optional_promotion_zone[side][n].set(square);
            }
         }
      }

      /* Restrict drop zones: pieces may not be dropped in locations where
       * they cannot move (the graveyard), unless they have the PF_DROPDEAD
       * flag set.
       */
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_flags[n] & PF_DROPDEAD) continue;
         for (side_t side=WHITE; side<NUM_SIDES; side++) {
            for (int square=0; square<files*ranks; square++) {
               if (moves[n][square][side].is_empty())
                  pt.drop_zone[side][n].reset(square);
            }
            //printf("%s %s\n", (side == WHITE)?"White":"Black", pt.piece_name[n]);
            //pt.drop_zone[side][n].print();
         }
      }



      /* Construct piece-square tables
       * TODO: adjust weights for the piece class.
       */
      for (int n=0; n<pt.num_piece_types; n++) {
         bool use_mobility_table = false;
         for (int square=0; square<files*ranks; square++)
            use_mobility_table = use_mobility_table || (pt.mobility_score[n][square] != 0);

         float centre_weight  = (board.rule_flags & RF_USE_DROPS) ? 1.0f : 0.5f;
         float advance_weight = (board.rule_flags & RF_USE_DROPS) ? 1.0f : 0.0f;

         if (pt.pawn_pieces & (1<<n)) use_mobility_table = false;
         if ((pt.pawn_pieces & (1<<n)) && open_position) centre_weight = 0;

         for (int square=0; square<files*ranks; square++) {
            if (use_mobility_table)
               pt.eval_pst[n][square] = pt.mobility_score[n][square];
            else
               pt.eval_pst[n][square] = int(centre_weight  * PST_CENTRE_Q  * abs(centre_table[square])*centre_table[square] +
                                        centre_weight  * PST_CENTRE_L  * centre_table[square] +
                                        advance_weight * PST_ADVANCE_Q * abs(advance_table[square])*advance_table[square] +
                                        advance_weight * PST_ADVANCE_L * advance_table[square]);

            if (pt.piece_flags[n]&PF_ROYAL) {
               pt.eval_pst[n][square].mg = -4*centre_table[square]
                                           -abs(centre_table[square])*centre_table[square]
                                           -2*advance_table[square]
                                           +4*abs(2*unpack_file(square) - (files-1))/2;
               pt.eval_pst[n][square].eg = centre_table[square]*centre_table[square]*centre_table[square];
            }
         }
         eval_pair_t sum = 0;
         for (int square=0; square<files*ranks; square++)
            sum += pt.eval_pst[n][square];
         sum = (sum + ranks*files/2) / (ranks*files);
         for (int square=0; square<files*ranks; square++)
            pt.eval_pst[n][square] -= sum;

         bitboard_t<kind> prison = pt.prison[WHITE][n] | pt.prison[BLACK][n];
         if ((pt.piece_flags[n]&PF_ROYAL) & (prison != bitboard_t<kind>::board_all)) {
            for (int square=0; square<files*ranks; square++) {
               pt.eval_pst[n][square] = 0;
               if (prison.test(square)) {
                  pt.eval_pst[n][square].mg = -16*advance_table[square] -16*abs(2*unpack_file(square) - (files-1))/2;
                  pt.eval_pst[n][square].eg = centre_table[square]*centre_table[square]*centre_table[square];
               }
            }
         }

         /* Advance bonus, for pawns in the end game */
         if (pt.pawn_pieces & (1 << n)) {
            if (pt.piece_promotion_choice[n]) {
               for (int square=0; square<files*ranks; square++) {
                  int rank = unpack_rank(square) - ranks/2 + 1;
                  int s    = (ranks-1);
                  int x    = advance_table[square];
                  int s2   = s*s;
                  int x2   = x*x;
                  pt.eval_pst[n][square].mg += eval_t(PAWN_ADVANCE_L_MG*x / s);
                  pt.eval_pst[n][square].mg += eval_t(PAWN_ADVANCE_Q_MG*x2 / s2);
                  pt.eval_pst[n][square].eg += eval_t(PAWN_ADVANCE_L_EG*x / s);
                  pt.eval_pst[n][square].eg += eval_t(PAWN_ADVANCE_Q_EG*x2 / s2);
                  if (rank > 0) pt.eval_pst[n][square].mg -= rank;
                  if (board.rule_flags & RF_USE_CAPTURE)
                     pt.eval_pst[n][square].mg += x;
               }
            } else {
               for (int square=0; square<files*(ranks-1); square++) {
                  if ( (pt.prison[WHITE][n] & bitboard_t<kind>::board_north).test(square) ) {
                     pt.eval_pst[n][square].mg += 2*pt.piece_value[n]/3;
                     pt.eval_pst[n][square].eg += pt.piece_value[n];
                  }
               }
            }
         }

         /* Palace tropism */
         for (int royal = 0; royal < pt.num_piece_types; royal++) {
            if ( !(pt.piece_flags[royal] & PF_ROYAL) ) continue;
            if ( pt.prison[WHITE][royal] == bitboard_t<kind>::board_all) continue;
            if ( pt.prison[BLACK][royal] == bitboard_t<kind>::board_all) continue;
            int piece = n;

            if (pt.royal_pieces & (1 << piece)) continue;
            if (pt.defensive_pieces & (1 << piece)) continue;

#if defined _MSC_VER
            std::vector<int> tropism(files*ranks);
#else
            int tropism[files*ranks];
#endif
            int max_trop = 0;
            int avg_trop = 0;
            bitboard_t<kind> palace = pt.prison[BLACK][royal] & bitboard_t<kind>::board_north;

            for (int square = 0; square < ranks*files; square++) {
               tropism[square] = 0;
               //if (palace.test(square)) continue;
               bitboard_t<kind> bb = palace;
               while(!bb.is_empty()) {
                  int s2 = bb.bitscan();
                  bb.reset(s2);
                  tropism[square] += pt.tropism[piece][square][s2];
               }
               max_trop = std::max(max_trop, tropism[square]);
               avg_trop += tropism[square];
            }

            avg_trop = (avg_trop + ranks*files/2) / (ranks*files);
            for (int square = 0; square < ranks*files; square++)
               tropism[square] = avg_trop - tropism[square];

            if (pt.pawn_pieces & (1 << piece)) {
               for (int square=files*(ranks-1); square<files*ranks; square++) {
                  tropism[square] = 0;
                  tropism[square-files] /= 2;
               }
            }

            move_flag_t attack_flags = pt.piece_capture_flags[n];
            int weight = 0;
            if (pt.pawn_pieces & (1 << piece)) weight = 1;
            if (pt.minor_pieces & (1 << piece)) weight = 2;
            if (is_hopper(attack_flags)) weight = 3;
            for (int square=0; square<files*ranks; square++) {
               pt.eval_pst[n][square].mg += weight*tropism[square];
               pt.eval_pst[n][square].eg += 2*weight*tropism[square];
            }

            /* Slider attacks on the palace */
            if (is_slider(attack_flags)) {
               bitboard_t<kind> occ;
               for (int square=0; square<files*ranks; square++) {
                  bitboard_t<kind> attack = movegen.generate_move_bitboard_for_flags(attack_flags, square, occ, WHITE);
                  if (!(attack & palace).is_empty()) {
                     pt.eval_pst[n][square].mg += 5;
                     if (!(attack & palace & bitboard_t<kind>::board_file[unpack_file(square)]).is_empty())
                        pt.eval_pst[n][square].eg += 10;
                  }
               }
            }

            for (int square=0; square<files*ranks; square++) {
               eval_t &mg = pt.eval_pst[n][square].mg;
               eval_t &eg = pt.eval_pst[n][square].eg;
               mg = eval_t(mg * PST_SCALE_PALACE);
               eg = eval_t(eg * PST_SCALE_PALACE);
            }
         }

         /* Flag tropism */
         if (pt.piece_flags[n] & PF_CAPTUREFLAG) {
            bitboard_t<kind> flags = board.flag[n];
#if defined _MSC_VER
            std::vector<int> tropism(files*ranks, 0);
#else
            int tropism[files*ranks];
            memset(tropism, 0, sizeof tropism);
#endif
            int max_trop = 0;
            int avg_trop = 0;

            while (!flags.is_empty()) {
               int fs = flags.bitscan();
               flags.reset(fs);
               for (int square=0; square<files*ranks; square++) {
                  tropism[square] += pt.tropism[n][square][fs];
                  max_trop = std::max(max_trop, tropism[square]);
                  avg_trop += pt.tropism[n][square][fs];
               }
            }
            avg_trop = (avg_trop + ranks*files/2) / (ranks*files);
            for (int square = 0; square < ranks*files; square++)
               tropism[square] = avg_trop - tropism[square];

            int weight = 5;
            if (board.rule_flags & RF_CAPTURE_ANY_FLAG)
               weight = 10;
            for (int square=0; square<files*ranks; square++) {
               if ((pt.piece_flags[n] & PF_ROYAL) == 0)
                  pt.eval_pst[n][square].mg += weight*tropism[square];
               pt.eval_pst[n][square].eg += 2*weight*tropism[square];
            }
         }

#if 0
         printf("%s\n", pt.piece_name[n]);
         for(int r=0; r<ranks; r++) {
            for(int f=0; f<files; f++) {
               int square = pack_rank_file(r, f);
               printf(" % 3d ", pt.eval_pst[n][square].mg);
            }
            printf("\n");
         }
#endif
      }
   }

   void finalise_variant() {
      movegen.apply_board_masks();
      movegen.initialise_super_tables();
      identify_promotion_options();
      identify_castle_partner();

      initialise_piece_values();
      pt.sort_piece_values();

      /* Mapping of squares and bits */
      board.virtual_files = (virtual_files >= 0) ? virtual_files : files;
      board.virtual_ranks = (virtual_ranks >= 0) ? virtual_ranks : ranks;
      for (int n=0; n<128; n++)
         board.bit_to_square[n] = -1;
      for (int n=0; n<256; n++)
         if (square_to_bit[n] >= 0 && square_to_bit[n] < 128) {
            bit_to_square[square_to_bit[n]] = n;
            board.bit_to_square[square_to_bit[n]] = n;
         }

      /* Make sure any gaps in the board are deleted from masks */
      for (side_t side = WHITE; side<=BLACK; side++)
      for (int n=0; n<pt.num_piece_types; n++) {
         pt.promotion_zone[side][n] &= bitboard_t<kind>::board_all;
         pt.optional_promotion_zone[side][n] &= bitboard_t<kind>::board_all;
         pt.special_zone[side][n] &= bitboard_t<kind>::board_all;
         pt.prison[side][n] &= bitboard_t<kind>::board_all;
         pt.drop_zone[side][n] &= bitboard_t<kind>::board_all;
         for (int k=0; k<MAX_PZ; k++)
            pt.promotion[n][k].zone[side] &= bitboard_t<kind>::board_all;
      }

      /* Holding size.
       * Royal pieces and pieces that demote on capture never go into
       * holdings.
       */
      holdsize = 0;
      if (board.rule_flags & RF_USE_HOLDINGS) {
         for (int n=0; n<pt.num_piece_types; n++) {
            if (pt.piece_flags[n] & PF_ROYAL) continue;
            if (pt.demotion[n] != n) continue;
            holdsize++;
         }

         /* However, if the game starts out with pieces in holdings, make sure
          * we include those!
          */
         const char *s = strchr(start_fen, '[');
         if (s) {
            s++;
            int count[NUM_SIDES] = { 0 };

            while (*s && isupper(*s)) count[WHITE]++, s++;
            while (*s && islower(*s)) count[BLACK]++, s++;

            holdsize = std::max(holdsize, std::max(count[WHITE], count[BLACK]));
         }
      }

      /* Test if captures should be mandatory */
      move_flag_t mf = 0;
      for (int n=0; n<pt.num_piece_types; n++)
         mf |= pt.piece_move_flags[n];
      if (mf == 0) board.rule_flags |= RF_FORCE_CAPTURE;

      /* Capture-the-flag, default: all pieces may capture the flag */
      if (board.rule_flags & RF_CAPTURE_THE_FLAG) {
         bool all_may_capture_flag = true;
         for (int n=0; n<pt.num_piece_types; n++)
            if (pt.piece_flags[n] & PF_CAPTUREFLAG) all_may_capture_flag = false;
         if (all_may_capture_flag) for (int n=0; n<pt.num_piece_types; n++)
            pt.piece_flags[n] |= PF_CAPTUREFLAG;
      }

      /* Test for side effects from capture victims */
      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_flags[n] & (PF_ENDANGERED | PF_ASSIMILATE))
            board.rule_flags |= RF_VICTIM_SIDEEFFECT;

         if (pt.piece_allowed_victims[n] != ~0u)
            board.rule_flags |= RF_VICTIM_SIDEEFFECT;
      }

      initialise_piece_evaluation_terms();

      /* Game-ending scores */
      if (perpetual == ILLEGAL+1)
         perpetual = rep_score;

      if (board.rule_flags & RF_USE_DROPS)
         clock_nodes >>= 1;

      castle_san_ok = true;
      for (int c = SHORT; c<NUM_CASTLE_MOVES; c++)
      for (side_t side=WHITE; side<=BLACK; side++)
         if (!movegen.castle_king_dest[c][side].onebit()) castle_san_ok = false;
   }

   virtual void print_eval_parameters(FILE *f = stdout) {
      fprintf(f, "Variant: %s\n\n", get_name());
      for (int n=0; n<pt.num_piece_types; n++) {
         int id = n;
         const char *s = get_piece_notation(id);
         if (isspace(s[0]))
            s = get_piece_abbreviation(WHITE, id);
         fprintf(f, "Value %s % 4d % 4d\n", s, pt.eval_value[id].mg, pt.eval_value[id].eg);
      }
      fprintf(f, "\n");

      for (int n=0; n<pt.num_piece_types; n++) {
         int id = n;
         const char *s = get_piece_notation(id);
         if (isspace(s[0]))
            s = get_piece_abbreviation(WHITE, id);

         fprintf(f, "PST %s MG {\n", s);
         for(int r=0; r<ranks; r++) {
            printf("   ");
            for(int f=0; f<files; f++) {
               int square = pack_rank_file(r, f);
               printf(" % 4d ", pt.eval_pst[n][square].mg);
            }
            printf("\n");
         }
         fprintf(f, "}\n");
         fprintf(f, "PST %s EG {\n", s);
         for(int r=0; r<ranks; r++) {
            printf("   ");
            for(int f=0; f<files; f++) {
               int square = pack_rank_file(r, f);
               printf(" % 4d ", pt.eval_pst[n][square].eg);
            }
            printf("\n");
         }
         fprintf(f, "}\n");
      }

   }

   virtual void load_eval_parameters(FILE *f) {
      char line[4096];

      bool found_variant = false;

      int id = -1;
      int ph = 0;
      int square = 0;

      while (!feof(f)) {
         if (fgets(line, sizeof line, f) == 0)
            continue;
         /* Strip away comments */
         char *s = strstr(line, "#");
         if (s) s[0] = '\0';

         /* Snip end-of-line */
         s = strstr(line, "\n");
         if (s) s[0] = '\0';

         /* Strip trailing space */
         s = line+strlen(line)-1;
         while (s > line && isspace(s[0])) { s[0] = '\0'; s--; }

         /* New variant */
         if (strstr(line, "Variant:") == line) {
            found_variant = false;
            const char *p = line + 8;
            while (isspace(*p) && *p) p++;
            if (strstr(p, get_name()))         /* We've found the variant we're looking for */
               found_variant = true;
            continue;
         }

         /* Have we found the correct variant? */
         if (!found_variant) continue;

         /* What are we trying to read? */
         if ((s = strstr(line, "Value"))) {
            s += 5;
            while (*s && isspace(*s)) s++;
            id = -1;
            for (int n=0; n<pt.num_piece_types; n++) {
               const char *name = get_piece_notation(n);
               if (isspace(name[0]))
                  name = get_piece_abbreviation(WHITE, n);
               if (strstr(s, name) == s) {
                  id = n;
                  break;
               }
            }
            while (*s && !isspace(*s)) s++;
            int mg, eg;
            sscanf(s, "%d %d", &mg, &eg);
            if (id>-1) {
               pt.eval_value[id].mg = mg;
               pt.eval_value[id].eg = eg;
            }
         }

         if ((s = strstr(line, "PST"))) {
            s += 3;
            while (*s && isspace(*s)) s++;
            id = -1;
            for (int n=0; n<pt.num_piece_types; n++) {
               const char *name = get_piece_notation(n);
               if (isspace(name[0]))
                  name = get_piece_abbreviation(WHITE, n);
               if (strstr(s, name) == s) {
                  id = n;
                  break;
               }
            }
            while (*s && !isspace(*s)) s++;
            while (*s && isspace(*s)) s++;
            ph = 0;
            if (strstr(s, "EG")) ph = 1;
            square = 0;
         }

         s = NULL;
         if ((s = strstr(line, "{"))) s++;
         if (!s) s = line;

         if (id == -1) continue;

         while (*s) {
            while (*s && !(isdigit(*s) || *s == '-' || *s == '+')) s++;
            if (*s == 0) break;

            int v = 0;
            sscanf(s, "%d", &v);
            while (*s && !isspace(*s)) s++;

            if (square < files*ranks) {
               if (ph == 0)
                  pt.eval_pst[id][square].mg = v;
               else
                  pt.eval_pst[id][square].eg = v;
            }

            square++;
         }

      }

   }

   int pack_rank_file(int rank, int file) { return bitboard_t<kind>::pack_rank_file(rank, file); }
   void print_board(FILE *file = stdout) const { board.print(file); }
   void print_pieces(void) const {
      pt.print();
      printf("Steppers: %d\n", movegen.number_of_steppers-1);
      printf("Leapers:  %d\n", movegen.number_of_leapers);
      printf("ALeapers: %d\n", movegen.number_of_aleapers);
   }
   void print_bitboards() const { board.print_bitboards(); }
   void generate_moves(movelist_t *movelist) const {
      movegen.generate_moves(movelist, &board, board.side_to_move);
   }
   void print_attacker_bitboard(int square) { movegen.get_all_attackers(&board, bitboard_t<kind>::board_all, square).print(); }
   void print_attack_bitboard(int square) { 
      bitboard_t<kind> test_squares;
      bitboard_t<kind> source_mask;
      source_mask.set(square);
      movegen.generate_attack_bitboard(&board, test_squares, source_mask, board.get_side(square)).print();
   }

   virtual void print_pst(void) {
      for (int n=0; n<pt.num_piece_types; n++) {
         printf("%s:\n", pt.piece_name[n]);
         for (int rank=ranks-1; rank>=0; rank--) {
            printf("%2s ", rank_names[rank]);
            for (int file=0; file<files; file++) {
               int square = pack_rank_file(rank, file);
               printf(" % 4d", pt.eval_pst[n][square].mg);
            }

            printf("    %2s ", rank_names[rank]);
            for (int file=0; file<files; file++) {
               int square = pack_rank_file(rank, file);
               printf(" % 4d", pt.eval_pst[n][square].eg);
            }
            printf("\n");
         }

         printf("   ");
         for (int file=0; file<files; file++)
            printf(" %4s", file_names[file]);
         printf("       ");
         for (int file=0; file<files; file++)
            printf(" %4s", file_names[file]);
         printf("\n");
      }
      printf("Centre:\n");
      for (int rank=ranks-1; rank>=0; rank--) {
         printf("%2s ", rank_names[rank]);
         for (int file=0; file<files; file++) {
            int square = pack_rank_file(rank, file);
            printf(" % 4d", centre_table[square]);
         }
         printf("\n");
      }
      printf("   ");
      for (int file=0; file<files; file++)
         printf(" %4s", file_names[file]);
      printf("\n");

      printf("Advance:\n");
      for (int rank=ranks-1; rank>=0; rank--) {
         printf("%2s ", rank_names[rank]);
         for (int file=0; file<files; file++) {
            int square = pack_rank_file(rank, file);
            printf(" % 4d", advance_table[square]);
         }
         printf("\n");
      }
      printf("   ");
      for (int file=0; file<files; file++)
         printf(" %4s", file_names[file]);
      printf("\n");

#if 0
         for (int square=0; square<files*ranks; square++)

         if ((pt.pawn_pieces & (1<<n)) && open_position) centre_weight = 0;

         for (int square=0; square<files*ranks; square++) {
            if (use_mobility_table)
               pt.eval_pst[n][square] = pt.mobility_score[n][square];
            else
               pt.eval_pst[n][square] = centre_weight * centre_table[square]*centre_table[square];

            if (pt.piece_flags[n]&PF_ROYAL) {
               pt.eval_pst[n][square].mg = -4*centre_table[square]
                                           -centre_table[square]*centre_table[square]
                                           -2*advance_table[square]
                                           +4*abs(2*unpack_file(square) - (files-1))/2;
               pt.eval_pst[n][square].eg = centre_table[square]*centre_table[square]*centre_table[square];
#endif
   }

   virtual const char *get_piece_notation(int id) const {
      return pt.piece_notation[id];
   }

   virtual const char *get_piece_abbreviation(side_t side, int id) const {
      return pt.piece_abbreviation[id][side];
   }

   virtual int get_most_valuable_piece_id(uint32_t mask) const {
      const int *perm = pt.val_perm;

      for (int n=pt.num_piece_types-1; n>=0; n--)
         if (mask & (1<<perm[n])) return perm[n];

      return -1;
   }

   void test_move_game_check() {
      board.check(movegen.player_in_check(&board, board.side_to_move));
   }

   void generate_legal_moves(movelist_t *movelist) const {
      movegen.generate_moves(movelist, &board, board.side_to_move);
      board_t<kind> board_copy = board;

      /* Now filter out the illegal moves */
      int n = 0;
      side_t me = board.side_to_move;
      while(n<movelist->num_moves) {
         unmake_info_t<kind> ui;
         move_t move = movelist->move[n];
         board_copy.makemove(move, &ui);
         bool illegal = movegen.player_in_check(&board_copy, me);

         if ((board.rule_flags & RF_QUIET_PROMOTION) && is_promotion_move(move) ) {
            int square = get_move_to(move);
            int p = board_copy.get_piece(square);
            bitboard_t<kind> atk = movegen.generate_move_bitboard_for_flags(pt.piece_capture_flags[p], square, board_copy.get_occupied(), next_side[board_copy.side_to_move]);
            if (!(atk & board_copy.bbc[board_copy.side_to_move]).is_empty()) illegal = true;
            if (movegen.was_checking_move(&board_copy, board_copy.side_to_move, move)) illegal = true;
         }

         board_copy.unmakemove(move, &ui);
         if (illegal) {
            movelist->num_moves--;
            movelist->move[n] = movelist->move[movelist->num_moves];
         } else {
            n++;
         }
      }
   }

#include "piece_rules.h"
#include "board_rules.h"

#include "fen.h"

   void start_new_game(void)
   {
      board.clear();
      if (max_moves < 1000) max_moves = 1000;

      start_move_count = 0;
      moves_played = last_move = 0;
      random_ok = false;
      fifty_scale_limit = int8_t(fifty_limit * 0.8);
      level = LEVEL_NORMAL;
      multipv = 1;

      move_list = (move_t *)realloc(move_list, max_moves * sizeof *move_list);
      move_clock = (int *)realloc(move_clock, max_moves * sizeof *move_clock);
      ui = (unmake_info_t<kind> *)realloc(ui, max_moves * sizeof *ui);

      setup_fen_position(start_fen);
      memset(see_cache, 0, sizeof(see_cache));

      destroy_hash_table(transposition_table);
      destroy_eval_hash_table(eval_table);
      transposition_table = create_hash_table(hash_size);
      eval_table = create_eval_hash_table(hash_size / 16);
   }

   void set_transposition_table_size(size_t size) { 
      hash_size = default_hash_size = size;

      destroy_hash_table(transposition_table);
      destroy_eval_hash_table(eval_table);

      transposition_table = create_hash_table(hash_size);
      eval_table = create_eval_hash_table(hash_size / 16);
   }

   size_t get_moves_played() { return moves_played; }

   bool player_in_check(side_t side)
   {
      return movegen.player_in_check(&board, side);
   }

   side_t side_piece_on_square(int square) {
      if (board.bbc[WHITE].test(square)) return WHITE;
      if (board.bbc[BLACK].test(square)) return BLACK;
      return NONE;
   }

   side_t get_side_to_move() { return board.side_to_move; }

   void playmove(move_t move)
   {
      board.makemove(move, ui + moves_played);
      move_list[moves_played] = move;

      repetition_hash_table[board.hash&0xFFFF]++;
      if (board.rule_flags & RF_USE_CAPTURE)
      board_repetition_hash_table[board.board_hash&0xFFFF]++;

      for (int c = SHORT; c<NUM_CASTLE_MOVES; c++) {
         bitboard_t<kind> mask = movegen.castle_mask[c][next_side[board.side_to_move]];
         if ((board.init & mask) != (ui[moves_played-1].init & mask)) {
            board.hash ^= flag_key[next_side[board.side_to_move]][c];
            board.board_hash ^= flag_key[next_side[board.side_to_move]][c];
         }
      }

      moves_played++;
   }

   void replaymove()
   {
      playmove(move_list[moves_played]);
   }

   void takeback()
   {
      if (moves_played) {
         moves_played--;
         repetition_hash_table[board.hash&0xFFFF]--;
         if (board.rule_flags & RF_USE_CAPTURE)
         board_repetition_hash_table[board.board_hash&0xFFFF]--;
         board.unmakemove(move_list[moves_played], ui + moves_played);
      }
   }

#include "see.h"
#include "killer.h"
#include "history.h"
#include "search.h"
#include "movestring.h"

   void calculate_pawn_structure(pawn_structure_t<kind> *ps);
   template <bool print>
   eval_t static_evaluation(side_t side_to_move, int alpha = -LEGALWIN, int beta = LEGALWIN);

   int eval() {
      return static_evaluation<true>(board.side_to_move);
   }

#include "betza_string.h"
   static const char *describe_move_flags(move_flag_t flags)
   {
      const char *desc = "compound";
      if ( (flags & ~MF_LEAPER_FLAGS) == 0) {
         desc = "leaper";
         if (is_aleaper(flags)) desc = "asymmetric leaper";
         if (is_double_leaper(flags)) desc = "double leaper";
         if (is_masked_leaper(flags)) desc = "lame leaper";
      } else if ( (flags & ~MF_SLIDER) == 0) {
         desc = "slider";
      } else if ( (flags & ~MF_HOPPER) == 0) {
         desc = "hopper (it must leap over enemy pieces)";
      } else if ( (flags & ~MF_STEPPER) == 0) {
         desc = "stepper";
      } else if ( (flags & ~MF_RIDER) == 0) {
         desc = "rider";
      }

      return desc;
   }

   void print_wiki_rules(void) {
      board_t<kind> backup_board = board;
      int8_t backup_repetition_hash_table[0xFFFF+1];
      int8_t backup_board_repetition_hash_table[0xFFFF+1];
      memcpy(backup_repetition_hash_table, repetition_hash_table, sizeof repetition_hash_table);
      memcpy(backup_board_repetition_hash_table, board_repetition_hash_table, sizeof board_repetition_hash_table);
      bool promotions = false;
      bool drops = false;

      setup_fen_position(start_fen);

      printf("{{AutomaticRules}}\n");
      printf("<div style=\"float:right;\" class=\"diagram\">\n<fen>%s</fen>\nThe opening position for %s.</div>\n", start_fen, name);


      int ranks = (virtual_ranks > 0) ? virtual_ranks : this->ranks;
      int files = (virtual_files > 0) ? virtual_files : this->files;
      printf("=Rules=\n");
      printf("\n");
      printf("%s is played on a %dx%d board.<br>\n", name, files, ranks);
      printf("%s moves first.<br>\n", (board.side_to_move == WHITE) ? "White" : "Black");

      for (int n=0; n<pt.num_piece_types; n++) {
         int nw  = (board.bbp[n]&board.bbc[WHITE]).popcount();
         int nb  = (board.bbp[n]&board.bbc[BLACK]).popcount();
         int nhw = board.holdings[n][WHITE];
         int nhb = board.holdings[n][BLACK];
         const char lc = pt.piece_name[n][strlen(pt.piece_name[n])-1];
         const char *plural = (lc == 's' || lc == 'z') ? "es" : "s";
         if (nw == nb && nhw == nhb) {
            printf("White and black begin the game with %d %s%s (%s)", nw, pt.piece_name[n], (nw == 1) ? "" : plural, pt.piece_abbreviation[n][WHITE]);
            if (nhw) printf(" and %d held in-hand", nhw);
         } else {
            if (nw > 0) {
               printf("White begins the game with %d %s%s (%s)", nw, pt.piece_name[n], (nw == 1) ? "" : plural, pt.piece_abbreviation[n][WHITE]);
               if (nhw) printf(" and %d held in-hand", nhw);
               if (nb) printf("<br>\n");
            }
            if (nb > 0) {
               printf("Black begins the game with %d %s%s (%s)", nb, pt.piece_name[n], (nb == 1) ? "" : plural, pt.piece_abbreviation[n][WHITE]);
               if (nhb) printf(" and %d held in-hand", nhb);
            }
         }
         printf("<br>\n");
      }

      for (int n=0; n<pt.num_piece_types; n++) {
         if (pt.piece_flags[n] & PF_ROYAL) {
            printf("The %s is a royal piece (king).<br>\n", pt.piece_name[n]);
            if (board.rule_flags & RF_KING_TRAPPED)
               printf("The %s may not leave the Palace.<br>\n", pt.piece_name[n]);
            if (board.rule_flags & RF_KING_TABOO)
               printf("The %ss may not face eachother along an open file.<br>\n", pt.piece_name[n]);
            if (board.rule_flags & RF_KING_DUPLECHECK)
               printf("If all %ss are under attack, at least one of them must be taken out of check.<br>\n", pt.piece_name[n]);
         }
      }

      if (board.rule_flags & RF_FORCE_CAPTURE)
         printf("Captures are compulsory.<br>\n");

      if (board.rule_flags & RF_KEEP_CAPTURE)
         printf("Captured pieces go to your hand.<br>\n");

      if (board.rule_flags & RF_RETURN_CAPTURE)
         printf("Captured pieces are returned to your opponent's hand.<br>\n");

      if (board.rule_flags & RF_ALLOW_PICKUP)
         printf("Pieces may be taken in-hand as a move.<br>\n");

      if (board.rule_flags & RF_ALLOW_DROPS)
         printf("Pieces in-hand %s be dropped on an empty square. This counts as a move.<br>\n",
            (board.rule_flags & RF_FORCE_DROPS) ? "must immediately" : "may");

      if (board.rule_flags & RF_GATE_DROPS)
         printf("Pieces in-hand may be gated in when moving a back-rank piece.<br>\n");


      printf("\n");
      printf("==Objectives==\n");
      printf("The game is won:\n");
      if (mate_score < 0)         printf("* If the enemy king is check-mated.\n");
      if (stale_score < 0)        printf("* If the enemy king is stale-mated.\n");
      if (bare_king_score < 0)    printf("* If the enemy only has a king left and cannot take your last piece\n");
      if (rep_score == LEGALLOSS) printf("* If the same position occurs for the third time.\n");
      if (no_piece_score < 0 && board.royal.is_empty())             printf("* If the enemy has no pieces left.\n");
      if (flag_score < 0 && board.rule_flags & RF_CAPTURE_THE_FLAG) printf("* If the enemy flags have been captured.\n");

      printf("\n");
      printf("The game is a draw:\n");
      if (mate_score == 0)      printf("* If the enemy king is check-mated.\n");
      if (stale_score == 0)     printf("* If the enemy king is stale-mated.\n");
      if (bare_king_score == 0) printf("* If the enemy only has a king left and cannot take your last piece\n");
      if (no_piece_score == 0 && board.royal.is_empty())             printf("* If the enemy has no pieces left.\n");
      if (flag_score == 0 && board.rule_flags & RF_CAPTURE_THE_FLAG) printf("* If the enemy flags have been captured.\n");
      if (rep_score == 0)       printf("* If the same position occurs for the third time.\n");
      if (fifty_limit)          printf("* If no progress has been made for %d consecutive moves.\n", (int)fifty_limit/2);
      if (mate_score < 0)       printf("* If there is insufficient material to mate the enemy king.\n");

      printf("\n");
      printf("==Movement of the pieces==\n");
      printf("<table border=\"1px\">\n");
      for (int n=0; n<pt.num_piece_types; n++) {
         int nw = (board.bbp[n]&board.bbc[WHITE]).popcount();
         int nb = (board.bbp[n]&board.bbc[BLACK]).popcount();
         int centre_square = bitboard_t<kind>::pack_rank_file(ranks/2, files/2);
         const char lc = pt.piece_name[n][strlen(pt.piece_name[n])-1];
         const char *plural = (lc == 's' || lc == 'z') ? "es" : "s";

         printf("<tr><th colspan=\"2\"> [[%s]] (%s) </th></tr>\n<tr>\n", pt.piece_name[n], pt.piece_abbreviation[n][WHITE]);

         const char *move_desc = describe_move_flags(pt.piece_move_flags[n]);
         const char *cap_desc = describe_move_flags(pt.piece_capture_flags[n]);
         printf("<td>\n");
         if (pt.piece_move_flags[n] == pt.piece_capture_flags[n] && pt.piece_move_flags[n]) {
            printf("The %s moves and captures as a %s (marked &bull;).<br>\n", pt.piece_name[n], move_desc);
         } else {
            if (pt.piece_move_flags[n])
               printf("The %s moves as a %s (marked &bull;).<br>\n", pt.piece_name[n], move_desc);
            else
               printf("The %s cannot make non-capture moves.<br>\n", pt.piece_name[n]);
            if (pt.piece_capture_flags[n])
               printf("The %s captures as a %s (marked &times;).<br>\n", pt.piece_name[n], move_desc);
            else
               printf("The %s cannot make capture moves.<br>\n", pt.piece_name[n]);
         }
         printf("[[Betza description]]: %s<br>\n", piece_moves_to_betza(n));//move_flags_to_betza(pt.piece_move_flags[n]));

         if (pt.piece_flags[n] & PF_NORET)
            printf("The %s cannot move back to the square it came from.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_CASTLE)
            printf("The %s can castle.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_TAKE_EP)
            printf("The %s can capture en-passant.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_SHAK)
            printf("The %s delivers \"shak\" check.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_NOMATE)
            printf("The %s is not allowed to deliver mate.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPNOCHECK)
            printf("The %s may not be dropped with check.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPNOMATE)
            printf("The %s may not be dropped to deliver mate.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPONEFILE) {
            if (pt.piece_drop_file_maximum[n] == 1)
               printf("The %s may not be dropped if there is already a friendly %s on the same file.<br>\n", pt.piece_name[n], pt.piece_name[n]);
            else
               printf("The %s may not be dropped if there are already %d friendly %s%s on the same file.<br>\n", pt.piece_name[n], pt.piece_drop_file_maximum[n], pt.piece_name[n], plural);
         }

         if (pt.piece_flags[n] & PF_IRON)
            printf("The %s may not be captured.<br>\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPDEAD)
            printf("The %s may be dropped where it cannot move.<br>\n", pt.piece_name[n]);


         if (pt.piece_promotion_choice[n]) {
            printf("The %s can promote to a ", pt.piece_name[n]);
            promotions = true;
            bool first = true;
            piece_bit_t pc = pt.piece_promotion_choice[n];
            while (pc) {
               int k = bitscan32(pc);
               pc ^= (1 << k);
               if (!first) {
                  if (pc == 0)
                     printf(" or ");
                  else
                     printf(", ");
               }
               printf("[[%s]]", pt.piece_name[k]);
               first = false;
            }

            printf(".<br>\n");
         }

         if (pt.demotion[n] != n)
            printf("The %s demotes to %s when captured<br>\n", pt.piece_name[n], pt.piece_name[pt.demotion[n]]);

         char fen[4096] = { 0 };
         int move_board[256];
         memset(move_board, 0, sizeof(move_board));

         bitboard_t<kind> xmark, omark;
         board_t<kind> demo = board;
         demo.clear();
         demo.side_to_move = (nw > 0 || nb == 0) ? WHITE : BLACK;
         demo.put_piece(n, demo.side_to_move, centre_square);
         if (pt.piece_special_move_flags[n]) {
            int f = files/4;
            bitboard_t<kind> bb = pt.special_zone[demo.side_to_move][n] & bitboard_t<kind>::board_file[f];
            f = 1;
            while (bb.is_empty() && f < files) {
               bb = pt.special_zone[demo.side_to_move][n] & bitboard_t<kind>::board_file[f];
               f++;
            }
            if (!bb.is_empty()) {
               int s = bb.bitscan();
               demo.put_piece(n, demo.side_to_move, s);
               move_board[bit_to_square[s]] = -1;
            }
         }

         omark = movegen.generate_moves_bitboard(&demo, bitboard_t<kind>::board_empty, demo.bbp[n], demo.side_to_move);
         xmark = movegen.generate_attack_bitboard(&demo, bitboard_t<kind>::board_empty, demo.bbp[n], demo.side_to_move);

         xmark &= ~omark;

         while (!xmark.is_empty()) {
            int s = xmark.bitscan();
            xmark.reset(s);

            s = bit_to_square[s];

            if (s < 0) continue;
            move_board[s] = 2;
         }
         while (!omark.is_empty()) {
            int s = omark.bitscan();
            omark.reset(s);

            s = bit_to_square[s];

            if (s < 0) continue;
            move_board[s] = 1;
         }

         for (int s = 0; s<files*ranks; s++) {
            int bit = square_to_bit[s];
            if (bit < 0 || !bitboard_t<kind>::board_all.test(bit))
               move_board[s] = 3;
         }

         move_board[bit_to_square[centre_square]] = -1;

         int k = 0;
         for (int r = ranks-1; r>=0; r--) {
            int count = 0;
            for (int f = 0; f < files; f++) {
               int s = f + r * files;

               //printf("%+3d", move_board[s]);

               /* Empty? */
               if (move_board[s] == 0) {
                  count++;
                  continue;
               }

               /* Not empty, do we have a count? */
               if (count) k += snprintf(fen+k, 4096 - k, "%d", count);
               count = 0;

               if (move_board[s] == -1) {
                  k += snprintf(fen+k, 4096-k, "%s", pt.piece_abbreviation[n][demo.side_to_move]);
                  continue;
               }

               const char *colour_string = " .#*";
               k += snprintf(fen+k, 4096-k, "%c", colour_string[move_board[s]]);
            }
            if (count) k += snprintf(fen+k, 4096 - k, "%d", count);
            if (r) k += snprintf(fen+k, 4096 - k, "/");
            //printf("\n");
         }

         //demo.print(stdout, xmark, omark);
         //printf("\n");
         printf("</td>\n<td><fen>");
         printf("%s", fen);
         printf("</fen></td>\n</tr>\n");
      }
      printf("</table>\n\n");

      if (promotions) {
         printf("==Promotion==\n");

         printf("Promotions are allowed when a piece moves into %sthe promotion zone.<br>\n", (board.rule_flags & RF_PROMOTE_IN_PLACE) ? "" : "or out of ");
         if (board.rule_flags & RF_PROMOTE_IN_PLACE)
            printf("Pieces inside the promotion zone can also be promoted later. This counts as a move.<br>\n");

         printf("<table border=\"1px\">\n");
         for (int n=0; n<pt.num_piece_types; n++) {
            if (pt.piece_promotion_choice[n] == 0) continue;
            int nw = (board.bbp[n]&board.bbc[WHITE]).popcount();
            int nb = (board.bbp[n]&board.bbc[BLACK]).popcount();
            const char lc = pt.piece_name[n][strlen(pt.piece_name[n])-1];
            const char *plural = (lc == 's' || lc == 'z') ? "es" : "s";

            side_t side_to_move = (nw > 0 || nb == 0) ? WHITE : BLACK;

            printf("<tr><th colspan=\"2\"> [[%s]] (%s) </th></tr>\n<tr><td>\n", pt.piece_name[n], pt.piece_abbreviation[n][WHITE]);

            bitboard_t<kind> omark = pt.promotion_zone[side_to_move][n];
            bitboard_t<kind> xmark = pt.optional_promotion_zone[side_to_move][n];
            omark &= ~xmark;

            if (!omark.is_empty())
               printf("The %s must promote when it reaches one of the squares marked with &bull;.<br>\n", pt.piece_name[n]);
            if (!xmark.is_empty())
               printf("The %s may promote when it reaches one of the squares marked with &times;.<br>\n", pt.piece_name[n]);

            printf("The %s can promote to a ", pt.piece_name[n]);
            promotions = true;
            bool first = true;
            piece_bit_t pc = pt.piece_promotion_choice[n];
            while (pc) {
               int k = bitscan32(pc);
               pc ^= (1 << k);
               if (!first) {
                  if (pc == 0)
                     printf(" or ");
                  else
                     printf(", ");
               }
               printf("[[%s]]", pt.piece_name[k]);
               first = false;
            }
            printf(".<br>\n");

            if (pt.demotion[n] != n)
               printf("The %s demotes to %s when captured<br>\n", pt.piece_name[n], pt.piece_name[pt.demotion[n]]);
            printf("</td>\n");

            char fen[4096] = { 0 };
            int move_board[256];
            memset(move_board, 0, sizeof(move_board));

            for (side_t side_to_move = (nw?WHITE:BLACK); side_to_move <= (nb?BLACK:WHITE); side_to_move++) {
               bitboard_t<kind> omark = pt.promotion_zone[side_to_move][n];
               bitboard_t<kind> xmark = pt.optional_promotion_zone[side_to_move][n];
               omark &= ~xmark;

               while (!xmark.is_empty()) {
                  int s = xmark.bitscan();
                  xmark.reset(s);

                  s = bit_to_square[s];

                  if (s < 0) continue;
                  move_board[s] = 2 + 3*side_to_move;
               }
               while (!omark.is_empty()) {
                  int s = omark.bitscan();
                  omark.reset(s);

                  s = bit_to_square[s];

                  if (s < 0) continue;
                  move_board[s] = 1 + 3*side_to_move;
               }

               for (int s = 0; s<files*ranks; s++) {
                  int bit = square_to_bit[s];
                  if (bit < 0 || !bitboard_t<kind>::board_all.test(bit))
                     move_board[s] = 3;
               }
            }

            int k = 0;
            for (int r = ranks-1; r>=0; r--) {
               int count = 0;
               for (int f = 0; f < files; f++) {
                  int s = f + r * files;

                  //printf("%+3d", move_board[s]);

                  /* Empty? */
                  if (move_board[s] == 0) {
                     count++;
                     continue;
                  }

                  /* Not empty, do we have a count? */
                  if (count) k += snprintf(fen+k, 4096 - k, "%d", count);
                  count = 0;

                  const char *colour_string = " ,$*.#";
                  k += snprintf(fen+k, 4096-k, "%c", colour_string[move_board[s]]);
               }
               if (count) k += snprintf(fen+k, 4096 - k, "%d", count);
               if (r) k += snprintf(fen+k, 4096 - k, "/");
               //printf("\n");
            }

            //demo.print(stdout, xmark, omark);
            //printf("\n");
            printf("<td><fen>%s</fen></td>\n</tr>\n", fen);
         }
         printf("</table>\n\n");
      }


      printf("[[Category: %dx%d]]\n", files, ranks);
      if (board.rule_flags & RF_USE_DROPS)
      printf("[[Category: Games with drops]]\n");


      board = backup_board;
      memcpy(repetition_hash_table, backup_repetition_hash_table, sizeof repetition_hash_table);
      memcpy(board_repetition_hash_table, backup_board_repetition_hash_table, sizeof board_repetition_hash_table);
   }

   void print_rules(void) {
      board_t<kind> backup_board = board;
      int8_t backup_repetition_hash_table[0xFFFF+1];
      int8_t backup_board_repetition_hash_table[0xFFFF+1];
      memcpy(backup_repetition_hash_table, repetition_hash_table, sizeof repetition_hash_table);
      memcpy(backup_board_repetition_hash_table, board_repetition_hash_table, sizeof board_repetition_hash_table);

      setup_fen_position(start_fen);

      int files = bitboard_t<kind>::board_files;
      int ranks = bitboard_t<kind>::board_ranks;
      printf("Rules of %s\n", name);
      printf("\n");
      printf("%s is played on a %dx%d board\n", name, files, ranks);
      printf("%s moves first.\n", (board.side_to_move == WHITE) ? "White" : "Black");

      printf("\n");
      printf("The game is won:\n");
      if (mate_score < 0)         printf(" * If the enemy king is check-mated.\n");
      if (stale_score < 0)        printf(" * If the enemy king is stale-mated.\n");
      if (bare_king_score < 0)    printf(" * If the enemy only has a king left and cannot take your last piece\n");
      if (rep_score == LEGALLOSS) printf(" * If the same position occurs for the third time.\n");
      if (no_piece_score < 0 && board.royal.is_empty())             printf(" * If the enemy has no pieces left.\n");
      if (flag_score < 0 && board.rule_flags & RF_CAPTURE_THE_FLAG) printf(" * If the enemy flags have been captured.\n");

      printf("\n");
      printf("The game is a draw:\n");
      if (mate_score == 0)      printf(" * If the enemy king is check-mated.\n");
      if (stale_score == 0)     printf(" * If the enemy king is stale-mated.\n");
      if (bare_king_score == 0) printf(" * If the enemy only has a king left and cannot take your last piece\n");
      if (no_piece_score == 0 && board.royal.is_empty())             printf(" * If the enemy has no pieces left.\n");
      if (flag_score == 0 && board.rule_flags & RF_CAPTURE_THE_FLAG) printf(" * If the enemy flags have been captured.\n");
      if (rep_score == 0)       printf(" * If the same position occurs for the third time.\n");
      if (fifty_limit)          printf(" * If no progress has been made for %d consecutive moves.\n", (int)fifty_limit/2);
      if (mate_score < 0)       printf(" * If there is insufficient material to mate the enemy king.\n");

      printf("\n");
      printf("Movement of the pieces:\n");
      for (int n=0; n<pt.num_piece_types; n++) {
         int centre_square = bitboard_t<kind>::pack_rank_file(ranks/2, files/2);
         const char lc = pt.piece_name[n][strlen(pt.piece_name[n])-1];
         const char *plural = (lc == 's' || lc == 'z') ? "es" : "s";
         printf("The %s\n", pt.piece_name[n]);

         const char *move_desc = describe_move_flags(pt.piece_move_flags[n]);
         const char *cap_desc = describe_move_flags(pt.piece_capture_flags[n]);
         if (pt.piece_move_flags[n] == pt.piece_capture_flags[n] && pt.piece_move_flags[n]) {
            printf("The %s moves and captures as a %s (marked +).\n", pt.piece_name[n], move_desc);
         } else {
            if (pt.piece_move_flags[n])
               printf("The %s moves as a %s (marked +).\n", pt.piece_name[n], move_desc);
            else
               printf("The %s cannot make non-capture moves.\n", pt.piece_name[n]);
            if (pt.piece_capture_flags[n])
               printf("The %s captures as a %s (marked *).\n", pt.piece_name[n], move_desc);
            else
               printf("The %s cannot make capture moves.\n", pt.piece_name[n]);
         }
         printf("Betza description: %s\n", piece_moves_to_betza(n));//move_flags_to_betza(pt.piece_move_flags[n]));

         if (pt.piece_flags[n] & PF_NORET)
            printf("The %s cannot move back to the square it came from.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_CASTLE)
            printf("The %s can castle.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_TAKE_EP)
            printf("The %s can capture en-passant.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_SHAK)
            printf("The %s delivers \"shak\" check.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_NOMATE)
            printf("The %s is not allowed to deliver mate.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPNOCHECK)
            printf("The %s may not be dropped with check.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPNOMATE)
            printf("The %s may not be dropped to deliver mate.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPONEFILE) {
            if (pt.piece_drop_file_maximum[n] == 1)
               printf("The %s may not be dropped if there is already a friendly %s on the same file.\n", pt.piece_name[n], pt.piece_name[n]);
            else
               printf("The %s may not be dropped if there are already %d friendly %s%s on the same file.\n", pt.piece_name[n], pt.piece_drop_file_maximum[n], pt.piece_name[n], plural);
         }

         if (pt.piece_flags[n] & PF_COLOURBOUND)
            printf("The %s is colour bound.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_IRON)
            printf("The %s may not be captured.\n", pt.piece_name[n]);

         if (pt.piece_flags[n] & PF_DROPDEAD)
            printf("The %s may be dropped where it cannot move.\n", pt.piece_name[n]);

         if (pt.defensive_pieces & (1 << n))
            printf("The %s is a defensive piece.\n", pt.piece_name[n]);

         if (pt.pawn_pieces & (1 << n))
            printf("The %s is a pawn-class piece.\n", pt.piece_name[n]);

         if (pt.minor_pieces & (1 << n))
            printf("The %s is a minor piece.\n", pt.piece_name[n]);

         if (pt.major_pieces & (1 << n))
            printf("The %s is a major piece.\n", pt.piece_name[n]);

         if (pt.super_pieces & (1 << n))
            printf("The %s is a super piece.\n", pt.piece_name[n]);

         if (pt.pawn_index[WHITE] == n)
            printf("The %s is white's pawn-type piece\n", pt.piece_name[n]);

         if (pt.pawn_index[BLACK] == n)
            printf("The %s is black's pawn-type piece\n", pt.piece_name[n]);

         //printf("The %s has safety weight %d.\n", pt.piece_name[n], pt.king_safety_weight[n]);

         int nw = (board.bbp[n]&board.bbc[WHITE]).popcount();
         int nb = (board.bbp[n]&board.bbc[BLACK]).popcount();
         if (nw == nb) printf("White and black begin the game with %d %s%s\n", nw, pt.piece_name[n], (nw == 1) ? "" : (lc == 's' || lc == 'z') ? "es" : "s");
         else {
         if (nw > 0) printf("White begins the game with %d %s%s\n", nw, pt.piece_name[n], (nw == 1) ? "" : (lc == 's' || lc == 'z') ? "es" : "s");
         if (nb > 0) printf("Black begins the game with %d %s%s\n", nb, pt.piece_name[n], (nb == 1) ? "" : (lc == 's' || lc == 'z') ? "es" : "s");
         }

         if (pt.piece_flags[n] & PF_ROYAL) printf("The %s is a royal piece (king).\n", pt.piece_name[n]);

         if (pt.piece_promotion_choice[n]) {
            printf("The %s can promote to a ", pt.piece_name[n]);
            bool first = true;
            piece_bit_t pc = pt.piece_promotion_choice[n];
            while (pc) {
               int k = bitscan32(pc);
               pc ^= (1 << k);
               if (!first) {
                  if (pc == 0)
                     printf(" or ");
                  else
                     printf(", ");
               }
               printf("%s", pt.piece_name[k]);
               first = false;
            }

            printf(".\n");

            bitboard_t<kind> pz[2], opz[2];
            pz[WHITE] = pt.promotion_zone[WHITE][n];
            pz[BLACK] = pt.promotion_zone[BLACK][n];
            opz[WHITE] = pt.optional_promotion_zone[WHITE][n];
            opz[BLACK] = pt.optional_promotion_zone[BLACK][n];
            if (nw) {
               printf("A white %s promotes when it reaches ", pt.piece_name[n]); 
               bool first = true;
               while(!pz[WHITE].is_empty()) {
                  int square = pz[WHITE].bitscan();
                  pz[WHITE].reset(square);
                  if (!first) {
                     if (pz[WHITE].is_empty())
                        printf(" or ");
                     else
                        printf(", ");
                  }
                  first = false;
                  printf("%s", square_names[square]);
               }
               printf(".\n");

               if (!opz[WHITE].is_empty()) {
                  printf("Promotion is optional on ");
                  bool first = true;
                  while(!opz[WHITE].is_empty()) {
                     int square = opz[WHITE].bitscan();
                     opz[WHITE].reset(square);
                     if (!first) {
                        if (opz[WHITE].is_empty())
                           printf(" or ");
                        else
                           printf(", ");
                     }
                     first = false;
                     printf("%s", square_names[square]);
                  }
                  printf(".\n");
               }
            }

            if (nb) {
               printf("A black %s promotes when it reaches ", pt.piece_name[n]); 
               bool first = true;
               while(!pz[BLACK].is_empty()) {
                  int square = pz[BLACK].bitscan();
                  pz[BLACK].reset(square);
                  if (!first) {
                     if (pz[BLACK].is_empty())
                        printf(" or ");
                     else
                        printf(", ");
                  }
                  first = false;
                  printf("%s", square_names[square]);
               }
               printf(".\n");

               if (!opz[BLACK].is_empty()) {
                  printf("Promotion is optional on ");
                  bool first = true;
                  while(!opz[BLACK].is_empty()) {
                     int square = opz[BLACK].bitscan();
                     opz[BLACK].reset(square);
                     if (!first) {
                        if (opz[BLACK].is_empty())
                           printf(" or ");
                        else
                           printf(", ");
                     }
                     first = false;
                     printf("%s", square_names[square]);
                  }
                  printf(".\n");
               }
            }
         }

         if (pt.demotion[n] != n)
            printf("The %s demotes to %s when captured\n", pt.piece_name[n], pt.piece_name[pt.demotion[n]]);

         bitboard_t<kind> xmark, omark;
         board_t<kind> demo = board;
         demo.clear();
         demo.side_to_move = (nw > 0 || nb == 0) ? WHITE : BLACK;
         demo.put_piece(n, demo.side_to_move, centre_square);
         if (pt.piece_special_move_flags[n]) {
            int f = files/4;
            bitboard_t<kind> bb = pt.special_zone[demo.side_to_move][n] & bitboard_t<kind>::board_file[f];
            f = 1;
            while (bb.is_empty() && f < files) {
               bb = pt.special_zone[demo.side_to_move][n] & bitboard_t<kind>::board_file[f];
               f++;
            }
            if (!bb.is_empty())
               demo.put_piece(n, demo.side_to_move, bb.bitscan());
         }
         xmark = movegen.generate_moves_bitboard(&demo, bitboard_t<kind>::board_empty, demo.bbp[n], demo.side_to_move);
         omark = movegen.generate_attack_bitboard(&demo, bitboard_t<kind>::board_empty, demo.bbp[n], demo.side_to_move);

         demo.print(stdout, xmark, omark);
         if (is_aleaper(pt.piece_move_flags[n]) || is_stepper(pt.piece_move_flags[n])) {
            demo.clear();
            demo.side_to_move = next_side[demo.side_to_move];
            demo.put_piece(n, demo.side_to_move, centre_square);

            if (pt.piece_special_move_flags[n]) {
               int f = files/4;
               bitboard_t<kind> bb = pt.special_zone[demo.side_to_move][n] & bitboard_t<kind>::board_file[f];
               f = 1;
               while (bb.is_empty() && f < files) {
                  bb = pt.special_zone[demo.side_to_move][n] & bitboard_t<kind>::board_file[f];
                  f++;
               }
               if (!bb.is_empty())
                  demo.put_piece(n, demo.side_to_move, bb.bitscan());
            }

            xmark = movegen.generate_moves_bitboard(&demo, bitboard_t<kind>::board_empty, demo.bbp[n], demo.side_to_move);
            omark = movegen.generate_attack_bitboard(&demo, bitboard_t<kind>::board_empty, demo.bbp[n], demo.side_to_move);

            demo.print(stdout, xmark, omark);
         }
         printf("\n");
      }

      board = backup_board;
      memcpy(repetition_hash_table, backup_repetition_hash_table, sizeof repetition_hash_table);
      memcpy(board_repetition_hash_table, backup_board_repetition_hash_table, sizeof board_repetition_hash_table);
   }

   void write_piece_descriptions(bool xb = false) const
   {
      if (xb) {
         for (int n=0; n<pt.num_piece_types; n++)
            printf("piece %s& %s\n", pt.piece_abbreviation[n][WHITE], piece_moves_to_betza(n));
      } else {
         printf("\n%s:\n", name);
         for (int n=0; n<pt.num_piece_types; n++)
            printf("   %-20s (%s): %s\n", pt.piece_name[n], pt.piece_abbreviation[n][WHITE], piece_moves_to_betza(n));
      }
   }

} ATTRIBUTE_ALIGNED(16);

#include "evaluate.h"

#endif
