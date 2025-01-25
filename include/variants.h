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
#ifndef VARIANTS_H
#define VARIANTS_H

#include <climits>

typedef game_t *(*new_variant_game_t)(const char *shortname);

game_t *create_standard_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBN", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
   game->name = strdup("Chess");

   game->finalise_variant();

   return game;
}

game_t *create_seirawan_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fh = fb | fn;
   move_flag_t fe = fr | fn;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",       "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",       "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",       "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",       "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fh, fh, 0,  pz, "",       "Hawk",   "H,h", "H", 825);
   game->add_piece_type(fe, fe, 0,  pz, "",       "Elephant","E,e","E", 875);
   game->add_piece_type(fk, fk, kf, pz, "",       "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QHERBN", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KGFDCBQkgfdcbq - 0 1");
   game->name = strdup("Seirawan");

   game->add_rule(RF_GATE_DROPS);

   game->finalise_variant();

   return game;
}

game_t *create_crazyhouse_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   float m_scale = 0.5f;
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", int(m_scale*325));
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", int(m_scale*325));
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", int(m_scale*450));
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", int(m_scale*675));
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "Q~R~B~N~", "Pawn",   "P,p", " ", int(m_scale*150));

   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N~,n~", "N~", int(m_scale*375));
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B~,b~", "B~", int(m_scale*375));
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R~,r~", "R~", int(m_scale*500));
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q~,q~", "Q~", int(m_scale*725));

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR [-] w KQkq - 0 1");
   game->name = strdup("Crazyhouse");

   game->add_rule(RF_ALLOW_DROPS | RF_KEEP_CAPTURE);

   game->finalise_variant();

   /* Pawns may not be dropped on the first or last ranks */
   for (int n = 0; n<game->pt.num_piece_types; n++) {
      game->pt.drop_zone[WHITE][n] &= game->pt.drop_zone[BLACK][n];
      game->pt.drop_zone[BLACK][n] &= game->pt.drop_zone[WHITE][n];
   }

   return game;
}

game_t *create_chessgi_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBN", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR [-] w KQkq - 0 1");
   game->xb_setup  = strdup("(PNBRQKpnbrqk) 8x8+6_crazyhouse");
   game->name = strdup("Chessgi");

   game->add_rule(RF_ALLOW_DROPS | RF_KEEP_CAPTURE);

   game->finalise_variant();

   return game;
}

game_t *create_twilight_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBN", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR [-] w KQkq - 0 1");
   game->name = strdup("Twilight Chess");

   game->add_rule(RF_ALLOW_DROPS | RF_ALLOW_PICKUP);

   game->finalise_variant();

   return game;
}

game_t *create_shatranj_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("leap (2,2)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 400);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 100);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 800);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 200);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, 0,  pp, "Q",    "Pawn",   "P,p", " ", 100);

   /* Stale mate is a loss */
   game->stale_score = -LEGALWIN;

   /* A bare king is a loss (except if we can bare the enemy king on the next move, but the search takes care
    * of that).
    */
   game->add_rule(RF_USE_BARERULE);
   game->bare_king_score = -LEGALWIN;

   game->start_fen = strdup("rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w 0 1");
   game->name = strdup("Shatranj");

   game->finalise_variant();

   return game;
}

game_t *create_courier_game(const char *)
{
   int files = 12;
   int ranks = 8;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(12, 8);

   move_flag_t fe = game->movegen.define_piece_move("leap (2,2)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t ff = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fw = game->movegen.define_piece_move("leap (1,0)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint128_t> pz[2];
   bitboard_t<uint128_t> pp[2] = {bitboard_t<uint128_t>::board_rank[7], bitboard_t<uint128_t>::board_rank[0]};

   game->add_piece_type(fn, fn, 0,  pz, "",  "Knight",   "N,n", "N", 400);
   game->add_piece_type(fb, fb, 0,  pz, "",  "Bishop",   "B,b", "B", 500);
   game->add_piece_type(fr, fr, 0,  pz, "",  "Rook",     "R,r", "R", 800);
   game->add_piece_type(ff, ff, 0,  pz, "",  "Ferz",     "F,f", "F", 200);
   game->add_piece_type(fe, fe, 0,  pz, "",  "Elephant", "E,e", "E", 250);
   game->add_piece_type(fw, fw, 0,  pz, "",  "Wazir",    "W,w", "W", 225);
   game->add_piece_type(fk, fk, 0,  pz, "",  "Man",      "M,m", "M", 300);
   game->add_piece_type(fk, fk, kf, pz, "",  "King",     "K,k", "K",   0);
   game->add_piece_type(fp, fc, 0,  pp, "F", "Pawn",     "P,p", " ", 100);

   /* Set the FEN string for the starting position */
   game->start_fen = strdup("rnebmk1wbenr/1ppppp1pppp1/6f5/p5p4p/P5P4P/6F5/1PPPPP1PPPP1/RNEBMK1WBENR w - -");
   game->name = strdup("Courier chess");

   game->finalise_variant();

   return game;
}

game_t *create_berolina_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step NE, NW");
   move_flag_t fc = game->movegen.define_piece_move("step N");
   move_flag_t f2 = game->movegen.define_piece_move("step 2NE, 2NW");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBN", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
   game->name = strdup("Berolina Chess");

   game->finalise_variant();

   return game;
}

game_t *create_knightmate_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fk, fk, 0,  pz, "",     "Man",    "M,m", "M", 300);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 450);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fn, fn, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBM", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rmbqkbmr/pppppppp/8/8/8/8/PPPPPPPP/RMBQKBMR w KQkq - 0 1");
   game->name = strdup("Knightmate");

   game->finalise_variant();

   return game;
}

game_t *create_shatar_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t ff = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fq = ff | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;
   uint32_t mf = PF_NOMATE | PF_SHAK;
   uint32_t sf = PF_SHAK;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, mf, pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, sf, pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, sf, pz, "",     "Berse",  "J,j", "J", 725);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, 0,  pp, "J",    "Pawn",   "P,p", " ", 100);

   game->add_rule(RF_USE_SHAKMATE);

   game->start_fen = strdup("rnbjkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBJKBNR w - - 0 1");
   //game->start_fen = strdup("rnbjkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBJKBNR w - - 0 1");
   game->xb_setup = strdup("(PNBR..........J......Kpnbr..........j......k) 8x8+0_fairy");
   game->name = strdup("Shatar");

   game->finalise_variant();

   return game;
}

game_t *create_spartan_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq  = fb | fr;
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t fw2 = game->movegen.define_piece_move("step 2N");
   move_flag_t ff  = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fw  = fb | fn;
   move_flag_t fg  = fr | ff;
   move_flag_t flm = game->movegen.define_piece_move("aleap (-1,0)|(1,0)|(1,1)|(-1,1)|(1,-1)|(-1,-1)|(2,2)|(-2,2)|(2,-2)|(-2,-2)");
   move_flag_t flc = game->movegen.define_piece_move("leap (1,1)|(2,2)");
   move_flag_t fc  = game->movegen.define_piece_move("leap (1,0)|(2,0)");
   move_flag_t fbh = game->movegen.define_piece_move("step NE,NW");
   move_flag_t fbc = game->movegen.define_piece_move("step N");
   move_flag_t fb2 = game->movegen.define_piece_move("leap (2,2)") | fbh;
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint64_t> rank2 = bitboard_t<uint64_t>::board_rank[1];
   bitboard_t<uint64_t> rank7 = bitboard_t<uint64_t>::board_rank[6];

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   game->add_piece_type( fn,  fn, 0,  pz, "",     "Knight",     "N,n", "N", 325);
   game->add_piece_type( fb,  fb, 0,  pz, "",     "Bishop",     "B,b", "B", 325);
   game->add_piece_type( fr,  fr, 0,  pz, "",     "Rook",       "R,r", "R", 500);
   game->add_piece_type( fq,  fq, 0,  pz, "",     "Queen",      "Q,q", "Q", 975);
   game->add_piece_type( fk,  fk, kf, pz, "",     "King",       "K,k", "K", 475);
   game->add_piece_type( fg,  fg, 0,  pz, "",     "General",    "G,g", "G", 725);
   game->add_piece_type( fw,  fw, 0,  pz, "",     "Warlord",    "W,w", "W", 825);
   game->add_piece_type(flm, flc, 0,  pz, "",     "Lieutenant", "L,l", "L", 300);
   game->add_piece_type( fc,  fc, 0,  pz, "",     "Captain",    "C,c", "C", 300);
   game->add_piece_type(fwp, fwc, 0,  pp, "QRBN", "Pawn",       "P,p", " ", 100);
   game->add_piece_type(fbh, fbc, 0,  pp, "KGWLC","Hoplite",    "H,h", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", rank2, fw2);
   game->add_special_move("h", rank7, fb2);

   /* Spartans can have two kings */
   game->set_maximum_number_of_kings(BLACK, 2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);

   game->start_fen = strdup("lgkcckwl/hhhhhhhh/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ -");
   game->name = strdup("Spartan chess");

   game->add_rule(RF_KING_DUPLECHECK);

   game->finalise_variant();

   return game;
}

game_t *create_super_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fe = fn | fr;
   move_flag_t fs = fb | fn;
   move_flag_t fa = fb | fr | fn;
   move_flag_t fv = game->movegen.define_piece_move("leap (1,0)|(1,1)|(1,2)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fe, fe, 0,  pz, "",     "Empress","E,e", "E", 900);
   game->add_piece_type(fs, fs, 0,  pz, "",     "Princess","S,s","S", 875);
   game->add_piece_type(fa, fa, 0,  pz, "",     "Amazon", "A,a", "A",1225);
   game->add_piece_type(fv, fv, 0,  pz, "",     "Veteran","V,v", "V", 750);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBNAESV", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->set_maximum_number_of_pieces("Q", WHITE, 1);
   game->set_maximum_number_of_pieces("A", WHITE, 1);
   game->set_maximum_number_of_pieces("E", WHITE, 1);
   game->set_maximum_number_of_pieces("S", WHITE, 1);
   game->set_maximum_number_of_pieces("V", WHITE, 1);
   game->set_maximum_number_of_pieces("R", WHITE, 2);
   game->set_maximum_number_of_pieces("N", WHITE, 2);
   game->set_maximum_number_of_pieces("B", WHITE, 2);
   game->set_maximum_number_of_pieces("Q", BLACK, 1);
   game->set_maximum_number_of_pieces("A", BLACK, 1);
   game->set_maximum_number_of_pieces("E", BLACK, 1);
   game->set_maximum_number_of_pieces("S", BLACK, 1);
   game->set_maximum_number_of_pieces("V", BLACK, 1);
   game->set_maximum_number_of_pieces("R", BLACK, 2);
   game->set_maximum_number_of_pieces("N", BLACK, 2);
   game->set_maximum_number_of_pieces("B", BLACK, 2);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
   game->xb_setup = strdup("(PNBRQ..SE.......V.AKpnbrq..se.......v.ak) 8x8+9_fairy");
   game->name = strdup("Super Chess");

   game->finalise_variant();

   return game;
}

game_t *create_test_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq  = fb | fr;
   move_flag_t fa  = fb | fn;
   move_flag_t fc  = fr | fn;
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t fw2 = game->movegen.define_piece_move("step 2N");
   move_flag_t ff  = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fw  = game->movegen.define_piece_move("leap (1,0)");
   move_flag_t fs  = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fg  = game->movegen.define_piece_move("aleap (1,0)|(-1,0)|(0,1)|(0,-1)|(1,1)|(-1,1)");
   move_flag_t fd  = game->movegen.define_piece_move("leap (2,0)");
   move_flag_t fe  = game->movegen.define_piece_move("leap (2,2)");
   move_flag_t fh  = game->movegen.define_piece_move("leap ((0,1)+(1,1))&(2,1)");
   move_flag_t fz  = game->movegen.define_piece_move("leap (3,2)");
   move_flag_t flm = game->movegen.define_piece_move("leap (0,0)|(1,1)|(1,0)|(2,0)|(2,1)|(2,2)");
   move_flag_t flc = game->movegen.define_piece_move("leap ((1,1)|(1,0))+((1,1)|(1,0))");
   move_flag_t fdk = fr | ff;
   move_flag_t fdh = fb | fw;
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   game->add_piece_type( fn,  fn, 0,  pz, "",     "Knight",     "N,n", "N");
   game->add_piece_type( fh,  fh, 0,  pz, "",     "Horse",      "H,h", "H");
   game->add_piece_type( fb,  fb, 0,  pz, "",     "Bishop",     "B,b", "B");
   game->add_piece_type( fr,  fr, 0,  pz, "",     "Rook",       "R,r", "R");
   game->add_piece_type( fq,  fq, 0,  pz, "",     "Queen",      "Q,q", "Q");
   game->add_piece_type( fa,  fa, 0,  pz, "",     "Archbishop", "A,a", "A");
   game->add_piece_type( fc,  fc, 0,  pz, "",     "Chancellor", "C,c", "C");
   game->add_piece_type( fk,  fk, kf, pz, "",     "King",       "K,k", "K");
   game->add_piece_type( ff,  ff, 0,  pz, "",     "Ferz",       "F,f", "F");
   game->add_piece_type( fw,  fw, 0,  pz, "",     "Wazir",      "W,w", "W");
   game->add_piece_type( fz,  fz, 0,  pz, "",     "Zebra",      "Z,z", "Z");
   game->add_piece_type( flc, flc,0,  pz, "",     "Lion",       "L,l", "L");
   game->add_piece_type( fs,  fs, 0,  pz, "",     "Silver general", "S,s", "S");
   game->add_piece_type( fg,  fg, 0,  pz, "",     "Gold general", "G,g", "G");
   game->add_piece_type( fd,  fd, 0,  pz, "",     "Dabbabah",   "D,d", "D");
   //game->add_piece_type( fe,  fe, 0,  pz, "",     "Elephant",   "E,e", "E");
   game->add_piece_type(fdk, fdk, 0,  pz, "",     "Dragon king","J,j", "J");
   game->add_piece_type(fdh, fdh, 0,  pz, "",     "Dragon horse","I,i", "I");
   game->add_piece_type(fwp, fwc, pf, pp, "QACRBN", "Pawn",     "P,p", " ");

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, fw2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
   game->name = strdup("test");

   game->finalise_variant();

   return game;
}

game_t *create_shogi_game(const char *)
{
   int files = 9;
   int ranks = 9;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("aleap (1,2)|(-1,2)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fp  = game->movegen.define_piece_move("step N");
   move_flag_t fl  = game->movegen.define_piece_move("step 8N");
   move_flag_t ff  = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fw  = game->movegen.define_piece_move("leap (1,0)");
   move_flag_t fs  = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fg  = game->movegen.define_piece_move("aleap (1,0)|(-1,0)|(0,1)|(0,-1)|(1,1)|(-1,1)");
   move_flag_t fdk = fr | ff;
   move_flag_t fdh = fb | fw;
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_DROPNOMATE | PF_DROPONEFILE;

   bitboard_t<uint128_t> pp[2];
   bitboard_t<uint128_t> pz[2] = { bitboard_t<uint128_t>::board_rank[8]|bitboard_t<uint128_t>::board_rank[7]|bitboard_t<uint128_t>::board_rank[6],
                                   bitboard_t<uint128_t>::board_rank[0]|bitboard_t<uint128_t>::board_rank[1]|bitboard_t<uint128_t>::board_rank[2] };
   float m_scale = 0.3f;
   game->add_piece_type( fp,  fp, pf, pz, "+",    "Pawn",            "P,p", "P", int( 80*m_scale));//  50);
   game->add_piece_type( fl,  fl, 0,  pz, "+",    "Lance",           "L,l", "L", int(225*m_scale));// 200);
   game->add_piece_type( fn,  fn, 0,  pz, "+",    "Knight",          "N,n", "N", int(250*m_scale));// 250);
   game->add_piece_type( fs,  fs, 0,  pz, "+",    "Silver general",  "S,s", "S", int(375*m_scale));// 350);
   game->add_piece_type( fb,  fb, 0,  pz, "+",    "Bishop",          "B,b", "B", int(575*m_scale));// 550);
   game->add_piece_type( fr,  fr, 0,  pz, "+",    "Rook",            "R,r", "R", int(650*m_scale));// 650);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Gold general",    "G,g", "G", int(450*m_scale));// 400);
   game->add_piece_type( fk,  fk, kf, pp, "",     "King",            "K,k", "K");

   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted pawn",   "+P,+p", "+P", int(530*m_scale));// 270);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted lance",  "+L,+l", "+L", int(480*m_scale));// 300);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted knight", "+N,+n", "+N", int(500*m_scale));// 330);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted silver", "+S,+s", "+S", int(490*m_scale));// 370);
   game->add_piece_type(fdh, fdh, 0,  pp, "",     "Dragon horse",    "+B,+b", "+B", int(825*m_scale));// 700);
   game->add_piece_type(fdk, fdk, 0,  pp, "",     "Dragon king",     "+R,+r", "+R", int(950*m_scale));// 800);

   game->start_fen = strdup("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL [-] w 0 1");
   game->name = strdup("shogi");

   game->add_rule(RF_ALLOW_DROPS);
   game->add_rule(RF_KEEP_CAPTURE);

   /* TODO: perpetual checking is not allowed */
   game->perpetual = -ILLEGAL;
   game->repeat_claim = 3;

   game->finalise_variant();

   return game;
}

game_t *create_minishogi_game(const char *)
{
#define kind uint64_t
   unsigned int files = 5;
   unsigned int ranks = 5;
   if (8*sizeof(kind) < files*ranks) return NULL;
   game_template_t<kind> *game = new game_template_t<kind>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fp  = game->movegen.define_piece_move("step N");
   move_flag_t ff  = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fw  = game->movegen.define_piece_move("leap (1,0)");
   move_flag_t fs  = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fg  = game->movegen.define_piece_move("aleap (1,0)|(-1,0)|(0,1)|(0,-1)|(1,1)|(-1,1)");
   move_flag_t fdk = fr | ff;
   move_flag_t fdh = fb | fw;
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_DROPNOMATE | PF_DROPONEFILE;

   bitboard_t<kind> pp[2];
   bitboard_t<kind> pz[2] = { bitboard_t<kind>::board_rank[4], bitboard_t<kind>::board_rank[0] };
   game->add_piece_type( fp,  fp, pf, pz, "+",    "Pawn",            "P,p", "P", 100);
   game->add_piece_type( fs,  fs, 0,  pz, "+",    "Silver general",  "S,s", "S", 400);
   game->add_piece_type( fb,  fb, 0,  pz, "+",    "Bishop",          "B,b", "B", 450);
   game->add_piece_type( fr,  fr, 0,  pz, "+",    "Rook",            "R,r", "R", 500);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Gold general",    "G,g", "G", 450);
   game->add_piece_type( fk,  fk, kf, pp, "",     "King",            "K,k", "K");

   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted pawn",   "+P,+p", "+P", 550);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted silver", "+S,+s", "+S", 500);
   game->add_piece_type(fdh, fdh, 0,  pp, "",     "Dragon horse",    "+B,+b", "+B", 650);
   game->add_piece_type(fdk, fdk, 0,  pp, "",     "Dragon king",     "+R,+r", "+R", 700);

   game->start_fen = strdup("rbsgk/4p/5/P4/KGSBR [-] w 0 1");
   game->xb_setup = strdup("(P.BR.S...G.+.++.+Kp.br.s...g.+.++.+k) 5x5+5_shogi");
   game->name = strdup("minishogi");

   game->add_rule(RF_ALLOW_DROPS);
   game->add_rule(RF_KEEP_CAPTURE);

   /* FIXME: repetition is scored as a loss, but it should be scored as a loss *for white*,
    * unless it is a check-by-repetition.
    */
   game->rep_score = -ILLEGAL;
   game->repeat_claim = 3;

   game->finalise_variant();

   return game;
#undef kind
}

game_t *create_shoshogi_game(const char *)
{
   int files = 9;
   int ranks = 9;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("aleap (1,2)|(-1,2)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fp  = game->movegen.define_piece_move("step N");
   move_flag_t fl  = game->movegen.define_piece_move("step 8N");
   move_flag_t ff  = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fw  = game->movegen.define_piece_move("leap (1,0)");
   move_flag_t fs  = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fg  = game->movegen.define_piece_move("aleap (1,0)|(-1,0)|(0,1)|(0,-1)|(1,1)|(-1,1)");
   move_flag_t fe  = game->movegen.define_piece_move("aleap (1,0)|(-1,0)|(0,1)|(1,1)|(-1,1)|(1,-1)|(-1,-1)");
   move_flag_t fdk = fr | ff;
   move_flag_t fdh = fb | fw;
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_DROPNOMATE | PF_DROPONEFILE;

   bitboard_t<uint128_t> pp[2];
   bitboard_t<uint128_t> pz[2] = { bitboard_t<uint128_t>::board_rank[8]|bitboard_t<uint128_t>::board_rank[7]|bitboard_t<uint128_t>::board_rank[6],
                                   bitboard_t<uint128_t>::board_rank[0]|bitboard_t<uint128_t>::board_rank[1]|bitboard_t<uint128_t>::board_rank[2] };
   game->add_piece_type( fp,  fp, pf, pz, "+",    "Pawn",            "P,p", "P",  75);
   game->add_piece_type( fl,  fl, 0,  pz, "+",    "Lance",           "L,l", "L", 250);
   game->add_piece_type( fn,  fn, 0,  pz, "+",    "Knight",          "N,n", "N", 300);
   game->add_piece_type( fs,  fs, 0,  pz, "+",    "Silver general",  "S,s", "S", 400);
   game->add_piece_type( fb,  fb, 0,  pz, "+",    "Bishop",          "B,b", "B", 650);
   game->add_piece_type( fr,  fr, 0,  pz, "+",    "Rook",            "R,r", "R", 750);
   game->add_piece_type( fe,  fe, 0,  pz, "+",    "Drunk elephant",  "E,e", "E", 550);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Gold general",    "G,g", "G", 450);
   game->add_piece_type( fk,  fk, kf, pp, "",     "King",            "K,k", "K");

   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted pawn",   "+P,+p", "+P", 450);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted lance",  "+L,+l", "+L", 450);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted knight", "+N,+n", "+N", 450);
   game->add_piece_type( fg,  fg, 0,  pp, "",     "Promoted silver", "+S,+s", "+S", 450);
   game->add_piece_type( fk,  fk, kf, pp, "",     "Crown prince",    "+E,+e", "+E", 700);
   game->add_piece_type(fdh, fdh, 0,  pp, "",     "Dragon horse",    "+B,+b", "+B", 775);
   game->add_piece_type(fdk, fdk, 0,  pp, "",     "Dragon king",     "+R,+r", "+R", 900);

   game->start_fen = strdup("lnsgkgsnl/1r2e2b1/ppppppppp/9/9/9/PPPPPPPPP/1B2E2R1/LNSGKGSNL [-] w 0 1");
   game->xb_setup = strdup("(PNBRLSE..G.+.++.++Kpnbrlse..g.+.++.++k) 9x9+0_shogi");
   game->name = strdup("shoshogi");

   game->add_rule(RF_USE_BARERULE);
   game->bare_king_score = -LEGALWIN;
   game->repeat_claim = 3;

   game->finalise_variant();

   return game;
}

game_t *create_tori_game(const char *)
{
   int files = 7;
   int ranks = 7;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t flq = game->movegen.define_piece_move("step 6N,6SE,SW"); // Left Quail
   move_flag_t frq = game->movegen.define_piece_move("step 6N,6SW,SE"); // Right Quail
   move_flag_t fs  = game->movegen.define_piece_move("step N");
   move_flag_t ff  = game->movegen.define_piece_move("aleap (-1,-1)|(-1,0)|(-1,1)|(0,1)|(1,1)|(1,0)|(1,-1)");
   move_flag_t fc  = game->movegen.define_piece_move("aleap (-1,-1)|(0,-1)|(-1,1)|(1,1)|(0,1)|(1,-1)");
   move_flag_t fe  = game->movegen.define_piece_move("step 6NE,6NW,6S,2SE,2SW") | fk;
   move_flag_t fp  = game->movegen.define_piece_move("aleap (-1,-1)|(1,-1)|(0,2)");
   move_flag_t fg  = game->movegen.define_piece_move("aleap (2,2)|(-2,2)|(0,-2)");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_DROPNOMATE | PF_DROPONEFILE;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = { bitboard_t<uint64_t>::board_rank[6]|bitboard_t<uint64_t>::board_rank[5],
                                  bitboard_t<uint64_t>::board_rank[0]|bitboard_t<uint64_t>::board_rank[1] };
   game->add_piece_type( fk,  fk, kf, pz, "",     "Phoenix",    "K,k", "K");
   game->add_piece_type( ff,  ff, 0,  pp, "+",    "Falcon",     "F,f", "F");
   game->add_piece_type( fc,  fc, 0,  pz, "",     "Crane",      "C,c", "C");
   game->add_piece_type(flq, flq, 0,  pz, "",     "Left quail", "L,l", "L");
   game->add_piece_type(frq, frq, 0,  pz, "",     "Right quail","R,r", "R");
   game->add_piece_type( fp,  fp, 0,  pz, "",     "Pheasant",   "P,p", "P");
   int st =
   game->add_piece_type( fs,  fs, pf, pp, "+",    "Swallow",    "S,s", "S");
   game->add_piece_type( fg,  fg, 0,  pz, "",     "Goose",      "+S,+s","+S");
   game->add_piece_type( fe,  fe, 0,  pz, "",     "Eagle",      "+F,+f","+F");

   game->pt.piece_drop_file_maximum[st] = 2;

   game->start_fen = strdup("rpckcpl/3f3/sssssss/2s1S2/SSSSSSS/3F3/LPCKCPR [-] w 0 1");
   game->xb_setup = strdup("(S.....FLR.C+.....+.PKs.....flr.c+.....+.pk) 7x7+6_shogi");
   game->name = strdup("Tori Shogi");

   game->add_rule(RF_ALLOW_DROPS);
   game->add_rule(RF_KEEP_CAPTURE);

   // TODO: repetition rules
   game->rep_score = -ILLEGAL;

   game->finalise_variant();

   /* Promotions are never optional */
   for (int n = 0; n<game->pt.num_piece_types; n++) {
      game->pt.optional_promotion_zone[WHITE][n].clear();
      game->pt.optional_promotion_zone[BLACK][n].clear();
   }

   return game;
}

game_t *create_chinese_game(const char *)
{
   int files = 9;
   int ranks = 10;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fc = game->movegen.define_piece_move("hop (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap ((0,1)+(1,1))&(1,2)");
   move_flag_t fe = game->movegen.define_piece_move("leap ((1,1)+(1,1))&(2,2)");
   move_flag_t ff = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)");
   move_flag_t fp = game->movegen.define_piece_move("aleap (1,0)|(-1,0)|(0,1)");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint128_t> pz[2];
   game->add_piece_type(fn, fn, 0,  pz, "", "Horse",    "H,h", "H", 400);
   game->add_piece_type(fr, fr, 0,  pz, "", "Rook",     "R,r", "R", 950);
   game->add_piece_type(fr, fc, 0,  pz, "", "Cannon",   "C,c", "C", 450);
   int gi = game->add_piece_type(ff, ff, 0,  pz, "", "Guard",    "A,a", "A",  75);
   int ei = game->add_piece_type(fe, fe, 0,  pz, "", "Elephant", "E,e", "E",  75);
   int pi = game->add_piece_type(fp, fp, 0,  pz, "", "Pawn",     "P,p", "P", 100);
   int ki = game->add_piece_type(fk, fk, kf, pz, "", "King",     "K,k", "K",   0);

   /* Zones for restricted piece movement */
   bitboard_t<uint128_t> south = bitboard_t<uint128_t>::board_south;
   bitboard_t<uint128_t> north = bitboard_t<uint128_t>::board_north;;
   bitboard_t<uint128_t> pawnf;
   for (int f = 0; f<files; f+=2)
      pawnf |= bitboard_t<uint128_t>::board_file[f];

   /* Guards and kings are restricted to the castles */
   bitboard_t<uint128_t> palace;
   for (int r = 0; r<=2; r++) for (int f = 3; f<=5; f++) {
      palace.set(f + 9*r);
      palace.set(f + 9*(9-r));
   }

   game->pt.prison[WHITE][ki] = palace & south;
   game->pt.prison[BLACK][ki] = palace & north;
   game->pt.prison[WHITE][gi] = palace & south;
   game->pt.prison[BLACK][gi] = palace & north;
   game->pt.prison[WHITE][ei] = south;
   game->pt.prison[BLACK][ei] = north;
   game->pt.prison[WHITE][pi] = pawnf | north;
   game->pt.prison[BLACK][pi] = pawnf | south;

   game->add_rule(RF_KING_TABOO | RF_KING_TRAPPED | RF_USE_CHASERULE);

   game->start_fen = strdup("rheakaehr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RHEAKAEHR w - - 0 1");
   game->name = strdup("XiangQi");

   game->fifty_limit = 0;
   game->stale_score = -LEGALWIN;
   //game->rep_score   = -LEGALWIN;

   game->finalise_variant();

   return game;
}

game_t *create_grand_game(const char *)
{
   int files = 10;
   int ranks = 10;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fq  = fb | fr;
   move_flag_t fc  = fb | fn;
   move_flag_t fm  = fr | fn;
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t fw2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint128_t> pz[2];
   bitboard_t<uint128_t> pi[2] = {bitboard_t<uint128_t>::board_rank[2], bitboard_t<uint128_t>::board_rank[7]};
   bitboard_t<uint128_t> pp[2] = {bitboard_t<uint128_t>::board_rank[7]|bitboard_t<uint128_t>::board_rank[8]|bitboard_t<uint128_t>::board_rank[9],
                                  bitboard_t<uint128_t>::board_rank[0]|bitboard_t<uint128_t>::board_rank[1]|bitboard_t<uint128_t>::board_rank[2]};
   game->add_piece_type( fn,  fn, 0,  pz, "",       "Knight",    "N,n", "N", 275);
   game->add_piece_type( fb,  fb, 0,  pz, "",       "Bishop",    "B,b", "B", 350);
   game->add_piece_type( fr,  fr, 0,  pz, "",       "Rook",      "R,r", "R", 475);
   game->add_piece_type( fq,  fq, 0,  pz, "",       "Queen",     "Q,q", "Q", 950);
   game->add_piece_type( fc,  fc, 0,  pz, "",       "Cardinal",  "A,a", "A", 825);
   game->add_piece_type( fm,  fm, 0,  pz, "",       "Marshal",   "C,c", "C", 875);
   game->add_piece_type( fk,  fk, kf, pz, "",       "King",      "K,k", "K",   0);
   game->add_piece_type(fwp, fwc, pf, pp, "QCARBN", "Pawn",      "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, fw2);

   game->set_maximum_number_of_pieces("Q", WHITE, 1);
   game->set_maximum_number_of_pieces("C", WHITE, 1);
   game->set_maximum_number_of_pieces("A", WHITE, 1);
   game->set_maximum_number_of_pieces("R", WHITE, 2);
   game->set_maximum_number_of_pieces("N", WHITE, 2);
   game->set_maximum_number_of_pieces("B", WHITE, 2);
   game->set_maximum_number_of_pieces("Q", BLACK, 1);
   game->set_maximum_number_of_pieces("C", BLACK, 1);
   game->set_maximum_number_of_pieces("A", BLACK, 1);
   game->set_maximum_number_of_pieces("R", BLACK, 2);
   game->set_maximum_number_of_pieces("N", BLACK, 2);
   game->set_maximum_number_of_pieces("B", BLACK, 2);

   game->start_fen = strdup("r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R w - -");
   game->name = strdup("Grand chess");

   game->finalise_variant();

   return game;
}

// http://www.chessvariants.org/index/msdisplay.php?itemid=MSopulentchess
game_t *create_opulent_game(const char *)
{
   int files = 10;
   int ranks = 10;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)|leap(1,0)");
   move_flag_t fw  = game->movegen.define_piece_move("leap (1,3)|leap(1,1)");
   move_flag_t fl  = game->movegen.define_piece_move("leap (0,2)|(0,3)|leap(1,1)");
   move_flag_t fN  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fq  = fb | fr;
   move_flag_t fa  = fb | fN;
   move_flag_t fc  = fr | fN;
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t fw2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint128_t> pz[2];
   bitboard_t<uint128_t> pi[2] = {bitboard_t<uint128_t>::board_rank[2], bitboard_t<uint128_t>::board_rank[7]};
   bitboard_t<uint128_t> pp[2] = {bitboard_t<uint128_t>::board_rank[7]|bitboard_t<uint128_t>::board_rank[8]|bitboard_t<uint128_t>::board_rank[9],
                                  bitboard_t<uint128_t>::board_rank[0]|bitboard_t<uint128_t>::board_rank[1]|bitboard_t<uint128_t>::board_rank[2]};
   game->add_piece_type( fn,  fn, 0,  pz, "",         "Knight",    "N,n", "N", 300);
   game->add_piece_type( fw,  fw, 0,  pz, "",         "Wizard",    "W,w", "W", 300);
   game->add_piece_type( fl,  fl, 0,  pz, "",         "Lion",      "L,l", "L", 300);
   game->add_piece_type( fb,  fb, 0,  pz, "",         "Bishop",    "B,b", "B", 350);
   game->add_piece_type( fr,  fr, 0,  pz, "",         "Rook",      "R,r", "R", 475);
   game->add_piece_type( fq,  fq, 0,  pz, "",         "Queen",     "Q,q", "Q", 950);
   game->add_piece_type( fa,  fa, 0,  pz, "",         "Archbishop","A,a", "A", 825);
   game->add_piece_type( fc,  fc, 0,  pz, "",         "Chancellor","C,c", "C", 875);
   game->add_piece_type( fk,  fk, kf, pz, "",         "King",      "K,k", "K",   0);
   game->add_piece_type(fwp, fwc, pf, pp, "QACRBNWL", "Pawn",      "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, fw2);

   game->set_maximum_number_of_pieces("Q", WHITE, 1);
   game->set_maximum_number_of_pieces("C", WHITE, 1);
   game->set_maximum_number_of_pieces("A", WHITE, 1);
   game->set_maximum_number_of_pieces("R", WHITE, 2);
   game->set_maximum_number_of_pieces("N", WHITE, 2);
   game->set_maximum_number_of_pieces("B", WHITE, 2);
   game->set_maximum_number_of_pieces("W", WHITE, 2);
   game->set_maximum_number_of_pieces("L", WHITE, 2);
   game->set_maximum_number_of_pieces("Q", BLACK, 1);
   game->set_maximum_number_of_pieces("M", BLACK, 1);
   game->set_maximum_number_of_pieces("C", BLACK, 1);
   game->set_maximum_number_of_pieces("R", BLACK, 2);
   game->set_maximum_number_of_pieces("N", BLACK, 2);
   game->set_maximum_number_of_pieces("B", BLACK, 2);
   game->set_maximum_number_of_pieces("W", BLACK, 2);
   game->set_maximum_number_of_pieces("L", BLACK, 2);

   game->start_fen = strdup("rw6wr/clbnqknbla/pppppppppp/10/10/10/10/PPPPPPPPPP/CLBNQKNBLA/RW6WR w - -");
   game->xb_setup = strdup("(PNBRQ..AC....W.......LKpnbrq..ac....w.......lk) 10x10+0_grand");
   game->name = strdup("Opulent chess");

   game->finalise_variant();

   return game;
}

game_t *create_greatshatranj_game(const char *)
{
   int files = 10;
   int ranks = 8;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fg  = game->movegen.define_piece_move("leap (1,0)|(1,1)|(2,0)|(2,2)");
   move_flag_t fm  = game->movegen.define_piece_move("leap (1,0)|(2,0)|(1,2)");
   move_flag_t fh  = game->movegen.define_piece_move("leap (1,1)|(2,2)|(1,2)");
   move_flag_t fe  = game->movegen.define_piece_move("leap (1,1)|(2,2)");
   move_flag_t fw  = game->movegen.define_piece_move("leap (1,0)|(2,0)");
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint128_t> pz[2];
   bitboard_t<uint128_t> pi[2] = {bitboard_t<uint128_t>::board_rank[1], bitboard_t<uint128_t>::board_rank[6]};
   bitboard_t<uint128_t> pp[2] = {bitboard_t<uint128_t>::board_rank[7], bitboard_t<uint128_t>::board_rank[0]};
   game->add_piece_type( fn,  fn, 0,  pz, "",       "Knight",    "N,n", "N", 325);
   game->add_piece_type( fg,  fg, 0,  pz, "",       "General",   "G,g", "G", 650);
   game->add_piece_type( fm,  fm, 0,  pz, "",       "Minister",  "M,m", "M", 650);
   game->add_piece_type( fh,  fh, 0,  pz, "",       "High Priest","H,h","H", 650);
   game->add_piece_type( fe,  fe, 0,  pz, "",       "Elephant",  "E,e", "E", 300);
   game->add_piece_type( fw,  fw, 0,  pz, "",       "Woody",     "W,w", "W", 300);
   game->add_piece_type( fk,  fk, 0,  pz, "",       "Soldier",   "S,s", "S", 300);
   game->add_piece_type( fk,  fk, kf, pz, "",       "King",      "K,k", "K",   0);
   game->add_piece_type(fwp, fwc, 0,  pp, "NGMHEWS","Pawn",      "P,p", " ", 100);

   game->set_maximum_number_of_pieces("N", WHITE, 2);
   game->set_maximum_number_of_pieces("G", WHITE, 1);
   game->set_maximum_number_of_pieces("M", WHITE, 1);
   game->set_maximum_number_of_pieces("H", WHITE, 1);
   game->set_maximum_number_of_pieces("E", WHITE, 2);
   game->set_maximum_number_of_pieces("W", WHITE, 2);
   game->set_maximum_number_of_pieces("N", BLACK, 2);
   game->set_maximum_number_of_pieces("G", BLACK, 1);
   game->set_maximum_number_of_pieces("M", BLACK, 1);
   game->set_maximum_number_of_pieces("H", BLACK, 1);
   game->set_maximum_number_of_pieces("E", BLACK, 2);
   game->set_maximum_number_of_pieces("W", BLACK, 2);

   game->start_fen = strdup("wnegkmhenw/pppppppppp/10/10/10/10/PPPPPPPPPP/WNEGKMHENW w KQkq - 0 1");
   game->xb_setup = strdup("(PN....E...S..HWGMKpn....e...s..hwgmk)");

   game->name = strdup("Great Shatranj");

   game->finalise_variant();

   return game;
}

game_t *create_capablanca_game(const char *)
{
   int files = 10;
   int ranks = 8;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fq  = fb | fr;
   move_flag_t fa  = fb | fn;
   move_flag_t fc  = fr | fn;
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t fw2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint128_t> pz[2];
   bitboard_t<uint128_t> pi[2] = {bitboard_t<uint128_t>::board_rank[1], bitboard_t<uint128_t>::board_rank[6]};
   bitboard_t<uint128_t> pp[2] = {bitboard_t<uint128_t>::board_rank[7], bitboard_t<uint128_t>::board_rank[0]};
   game->add_piece_type( fn,  fn, 0,  pz, "",       "Knight",    "N,n", "N", 275);
   game->add_piece_type( fb,  fb, 0,  pz, "",       "Bishop",    "B,b", "B", 350);
   game->add_piece_type( fr,  fr, 0,  pz, "",       "Rook",      "R,r", "R", 475);
   game->add_piece_type( fq,  fq, 0,  pz, "",       "Queen",     "Q,q", "Q", 950);
   game->add_piece_type( fa,  fa, 0,  pz, "",       "Archbishop","A,a", "A", 825);
   game->add_piece_type( fc,  fc, 0,  pz, "",       "Chancellor","C,c", "C", 875);
   game->add_piece_type( fk,  fk, kf, pz, "",       "King",      "K,k", "K",   0);
   game->add_piece_type(fwp, fwc, pf, pp, "QACRBN", "Pawn",      "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, fw2);

   game->deduce_castle_flags(WHITE, 5, 8, 9);
   game->deduce_castle_flags(WHITE, 5, 2, 0);
   game->deduce_castle_flags(BLACK, 75, 78, 79);
   game->deduce_castle_flags(BLACK, 75, 72, 70);

   game->start_fen = strdup("rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq -");
   game->name = strdup("Capablanca chess");

   game->finalise_variant();

   return game;
}

game_t *create_gothic_game(const char *shortname)
{
   game_t *game = create_capablanca_game(shortname);

   if (game) {
      free(game->start_fen);
      free(game->name);
      game->start_fen = strdup("rnbqckabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQCKABNR w KQkq -");
      game->name = strdup("Capablanca chess (Gothic)");
   }

   return game;
}

game_t *create_embassy_game(const char *shortname)
{
   game_t *game = create_capablanca_game(shortname);

   if (game) {
      free(game->start_fen);
      free(game->name);
      game->start_fen = strdup("rnbqkcabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQKCABNR w KQkq -");
      game->xb_setup = strdup("(PNBRQ..AC............Kpnbrq..ac............k) 10x8+0_capablanca");
      game->name = strdup("Capablanca chess (Embassy)");

      game->deduce_castle_flags(WHITE,  4,  1,  0);
      game->deduce_castle_flags(WHITE,  4,  7,  9);
      game->deduce_castle_flags(BLACK, 74, 71, 70);
      game->deduce_castle_flags(BLACK, 74, 77, 79);
   }

   return game;
}

game_t *create_micro_game(const char *)
{
#define kind uint64_t
   int files = 5;
   int ranks = 5;
   game_template_t<kind> *game = new game_template_t<kind>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fq  = fb | fr;
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<kind> pz[2];
   bitboard_t<kind> pp[2] = {bitboard_t<kind>::board_rank[4], bitboard_t<kind>::board_rank[0]};
   game->add_piece_type( fn,  fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type( fb,  fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type( fr,  fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type( fq,  fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type( fk,  fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fwp, fwc, 0,  pp, "QRBN", "Pawn",   "P,p", " ", 100);

   game->start_fen = strdup("kqbnr/ppppp/5/PPPPP/KQBNR w - -");
   game->xb_setup  = strdup("(PNBRQKpnbrqk) 5x5+0_fairy");
   game->name = strdup("Gardner minichess");
   // http://www.chessvariants.org/small.dir/hpmini.html

   game->finalise_variant();

   return game;
#undef kind
}

game_t *create_losalamos_game(const char *)
{
   int files = 6;
   int ranks = 6;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb  = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr  = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fq  = fb | fr;
   move_flag_t fn  = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk  = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fwp = game->movegen.define_piece_move("step N");
   move_flag_t fwc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[5], bitboard_t<uint64_t>::board_rank[0]};
   game->add_piece_type( fn,  fn, 0,  pz, "",    "Knight", "N,n", "N", 575);
   game->add_piece_type( fr,  fr, 0,  pz, "",    "Rook",   "R,r", "R", 650);
   game->add_piece_type( fq,  fq, 0,  pz, "",    "Queen",  "Q,q", "Q",1220);
   game->add_piece_type( fk,  fk, kf, pz, "",    "King",   "K,k", "K",   0);
   game->add_piece_type(fwp, fwc, 0,  pp, "QRN", "Pawn",   "P,p", " ", 100);

   game->start_fen = strdup("rnqknr/pppppp/6/6/PPPPPP/RNQKNR w - -");
   game->xb_setup  = strdup("(PN.RQ................Kpn.rq................k) 6x6+0_fairy");
   game->name = strdup("Los Alamos chess");

   game->finalise_variant();

   return game;
}

game_t *create_pocketknight_game(const char *shortname)
{
   game_t *game = create_standard_game(shortname);

   if (game) {
      free(game->start_fen);
      free(game->name);
      game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR [Nn] w KQkq -");
      game->xb_setup  = strdup("(PNBRQKpnbrqk) 8x8+2_bughouse");    // Hack: we need a holding-size of 2, otherwise the knight drops out.
      game->name = strdup("Pocket Knight Chess");
      game->add_rule(RF_ALLOW_DROPS);
   }

   return game;
}

game_t *create_kingofthehill_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 2N");
   uint32_t kf = PF_ROYAL | PF_CAPTUREFLAG;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   bitboard_t<uint64_t> pi[2] = {bitboard_t<uint64_t>::board_rank[1], bitboard_t<uint64_t>::board_rank[6]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fb, fb, 0,  pz, "",     "Bishop", "B,b", "B", 325);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fq, fq, 0,  pz, "",     "Queen",  "Q,q", "Q", 975);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, pf, pp, "QRBN", "Pawn",   "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE, 4, 6, 7);
   game->deduce_castle_flags(WHITE, 4, 2, 0);
   game->deduce_castle_flags(BLACK, 60, 62, 63);
   game->deduce_castle_flags(BLACK, 60, 58, 56);

   for (side_t side=WHITE; side<=BLACK; side++) {
      game->place_flag(side, 27);
      game->place_flag(side, 28);
      game->place_flag(side, 35);
      game->place_flag(side, 36);
   }
   game->add_rule(RF_CAPTURE_ANY_FLAG);

   game->start_fen = strdup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
   game->xb_setup  = strdup("(PNBRQKpnbrqk) 8x8+0_fairy");
   game->name = strdup("King of the hill");

   game->finalise_variant();

   return game;
}

game_t *create_sittuyin_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fm = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fs = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_PROMOTEWILD;

   bitboard_t<uint64_t> diag = bitboard_t<uint64_t>::board_diagonal[7] | bitboard_t<uint64_t>::board_antidiagonal[7];
   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = { diag & diag.board_homeland[BLACK], diag & diag.board_homeland[WHITE] };
            game->add_piece_type(fn, fn, 0,  pz, "",  "Knight", "N,n", "N", 325);
            game->add_piece_type(fs, fs, 0,  pz, "",  "Silver general",  "S,s", "S", 275);
            game->add_piece_type(fm, fm, 0,  pz, "",  "Ferz",   "F,f", "F", 100);
   int ri = game->add_piece_type(fr, fr, 0,  pz, "",  "Rook",   "R,r", "R", 500);
            game->add_piece_type(fk, fk, kf, pz, "",  "King",   "K,k", "K",   0);
   int pi = game->add_piece_type(fp, fc, pf, pp, "F", "Pawn",   "P,p", " ",  55);

   game->add_rule(RF_FORCE_DROPS | RF_PROMOTE_IN_PLACE | RF_QUIET_PROMOTION | RF_PROMOTE_BY_MOVE);
   game->set_maximum_number_of_pieces("F", WHITE, 1);
   game->set_maximum_number_of_pieces("F", BLACK, 1);

   for (int n = 0; n<game->pt.num_piece_types; n++) {
      if (n == ri) {
         game->pt.drop_zone[WHITE][n] = bitboard_t<uint64_t>::board_rank[0];
         game->pt.drop_zone[BLACK][n] = bitboard_t<uint64_t>::board_rank[7];
      } else {
         game->pt.drop_zone[WHITE][n] = bitboard_t<uint64_t>::board_rank[0] | bitboard_t<uint64_t>::board_rank[1] | bitboard_t<uint64_t>::board_rank[2];
         game->pt.drop_zone[BLACK][n] = bitboard_t<uint64_t>::board_rank[7] | bitboard_t<uint64_t>::board_rank[6] | bitboard_t<uint64_t>::board_rank[5];
      }
      game->pt.optional_promotion_zone[WHITE][n] = game->pt.promotion_zone[WHITE][n];
      game->pt.optional_promotion_zone[BLACK][n] = game->pt.promotion_zone[BLACK][n];
   }

   /* Only moves out of the promotion zone should promote */
   game->pt.entry_promotion_zone[WHITE][pi].clear();
   game->pt.entry_promotion_zone[BLACK][pi].clear();

   /* Set the FEN string for the starting position */
   game->start_fen = strdup("8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1");
   game->xb_setup  = strdup("(PN.R.F....SKpn.r.f....sk) 8x8+6_bughouse");
   game->name = strdup("Sittuyin");

   game->finalise_variant();

   for (int n = 0; n<game->pt.num_piece_types; n++) {
      game->pt.optional_promotion_zone[WHITE][n] = game->pt.promotion_zone[WHITE][n];
      game->pt.optional_promotion_zone[BLACK][n] = game->pt.promotion_zone[BLACK][n];
   }

   return game;
}


game_t *create_makruk_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fm = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fs = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[5], bitboard_t<uint64_t>::board_rank[2]};
   game->add_piece_type(fn, fn, 0,  pz, "",  "Knight", "N,n", "N", 325);
   game->add_piece_type(fs, fs, 0,  pz, "",  "Silver general",  "S,s", "S", 275);
   game->add_piece_type(fm, fm, 0,  pz, "",  "Met",    "M,m", "M", 150);
   game->add_piece_type(fr, fr, 0,  pz, "",  "Rook",   "R,r", "R", 500);
   game->add_piece_type(fk, fk, kf, pz, "",  "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, 0,  pp, "M", "Pawn",   "P,p", " ",  80);

   /* Set the FEN string for the starting position */
   game->start_fen = strdup("rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - -");
   game->name = strdup("Makruk");

   game->finalise_variant();

   /* Sixth-rank promotions are never optional */
   for (int n = 0; n<game->pt.num_piece_types; n++) {
      game->pt.optional_promotion_zone[WHITE][n].clear();
      game->pt.optional_promotion_zone[BLACK][n].clear();
   }

   return game;
}


game_t *create_aiwok_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fa = game->movegen.define_piece_move("leap (1,1)|(1,2)") | fr;
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fs = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[5], bitboard_t<uint64_t>::board_rank[2]};
   game->add_piece_type(fn, fn, 0,  pz, "",  "Knight", "N,n", "N", 325);
   game->add_piece_type(fs, fs, 0,  pz, "",  "Silver general",  "S,s", "S", 275);
   game->add_piece_type(fa, fa, 0,  pz, "",  "Ai-Wok", "A,a", "A",1050);
   game->add_piece_type(fr, fr, 0,  pz, "",  "Rook",   "R,r", "R", 500);
   game->add_piece_type(fk, fk, kf, pz, "",  "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, 0,  pp, "A", "Pawn",   "P,p", " ",  80);

   /* Set the FEN string for the starting position */
   game->start_fen = strdup("rnsaksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKASNR w - -");
   game->xb_setup  = strdup("(PN.R...A..SKpn.r...a..sk) 8x8+0_makruk");
   game->name = strdup("Ai-Wok");

   game->finalise_variant();

   /* Sixth-rank promotions are never optional */
   for (int n = 0; n<game->pt.num_piece_types; n++) {
      game->pt.optional_promotion_zone[WHITE][n].clear();
      game->pt.optional_promotion_zone[BLACK][n].clear();
   }

   return game;
}


game_t *create_asean_game(const char *)
{
   int files = 8;
   int ranks = 8;
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;

   game->set_board_size(files, ranks);

   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fm = game->movegen.define_piece_move("leap (1,1)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fs = game->movegen.define_piece_move("aleap (0,1)|(1,1)|(1,-1)|(-1,-1)|(-1,1)");
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   uint32_t kf = PF_ROYAL;

   bitboard_t<uint64_t> pz[2];
   bitboard_t<uint64_t> pp[2] = {bitboard_t<uint64_t>::board_rank[7], bitboard_t<uint64_t>::board_rank[0]};
   game->add_piece_type(fn, fn, 0,  pz, "",     "Knight", "N,n", "N", 325);
   game->add_piece_type(fs, fs, 0,  pz, "",     "Bishop", "B,b", "B", 275);
   game->add_piece_type(fm, fm, 0,  pz, "",     "Queen",  "Q,q", "Q", 150);
   game->add_piece_type(fr, fr, 0,  pz, "",     "Rook",   "R,r", "R", 500);
   game->add_piece_type(fk, fk, kf, pz, "",     "King",   "K,k", "K",   0);
   game->add_piece_type(fp, fc, 0,  pp, "QRBN", "Pawn",   "P,p", " ",  80);

   /* Set the FEN string for the starting position */
   game->start_fen = strdup("rnbqkbnr/8/pppppppp/8/8/PPPPPPPP/8/RNBQKBNR  w - -");
   game->name = strdup("ASEAN");

   game->finalise_variant();

   return game;
}

game_t *create_omega_game(const char *)
{
   int files = 12;
   int ranks = 10;
   game_template_t<uint128_t> *game = new game_template_t<uint128_t>;

   game->set_board_size(files, ranks);

   move_flag_t fb = game->movegen.define_piece_move("slide (A,D)");
   move_flag_t fr = game->movegen.define_piece_move("slide (H,V)");
   move_flag_t fn = game->movegen.define_piece_move("leap (1,2)");
   move_flag_t fk = game->movegen.define_piece_move("leap (1,0)|(1,1)");
   move_flag_t fh = game->movegen.define_piece_move("leap (0,1)|(0,2)|(2,2)");
   move_flag_t fw = game->movegen.define_piece_move("leap (1,1)|(1,3)");
   move_flag_t fq = fb | fr;
   move_flag_t fp = game->movegen.define_piece_move("step N");
   move_flag_t fc = game->movegen.define_piece_move("step NE, NW");
   move_flag_t f2 = game->movegen.define_piece_move("step 3N");
   uint32_t kf = PF_ROYAL;
   uint32_t pf = PF_SET_EP | PF_TAKE_EP;

   bitboard_t<uint128_t> pz[2];
   bitboard_t<uint128_t> pp[2] = {bitboard_t<uint128_t>::board_rank[9], bitboard_t<uint128_t>::board_rank[0]};
   bitboard_t<uint128_t> pi[2] = {bitboard_t<uint128_t>::board_rank[1], bitboard_t<uint128_t>::board_rank[8]};
   game->add_piece_type(fn, fn, 0,  pz, "",       "Knight",   "N,n", "N", 250);
   game->add_piece_type(fb, fb, 0,  pz, "",       "Bishop",   "B,b", "B", 400);
   game->add_piece_type(fw, fw, 0,  pz, "",       "Wizard",   "W,w", "W", 350);
   game->add_piece_type(fh, fh, 0,  pz, "",       "Champion", "C,c","C", 375);
   game->add_piece_type(fr, fr, 0,  pz, "",       "Rook",     "R,r", "R", 600);
   game->add_piece_type(fq, fq, 0,  pz, "",       "Queen",    "Q,q", "Q", 975);
   game->add_piece_type(fk, fk, kf, pz, "",       "King",     "K,k", "K",   0);
   int ind = game->add_piece_type(fp, fc, pf, pp, "QRBCWN", "Pawn",     "P,p", " ", 100);

   /* Add special move types for pawns on their initial squares */
   game->add_special_move("P", pi, f2);

   game->deduce_castle_flags(WHITE,   6,   8,   9);
   game->deduce_castle_flags(WHITE,   6,   4,   2);
   game->deduce_castle_flags(BLACK, 114, 116, 117);
   game->deduce_castle_flags(BLACK, 114, 112, 110);

   /* Now hack the movement tables to off-set the wizard squares by one rank */
   bitboard_t<uint128_t> ws = bitboard_t<uint128_t>::board_corner;
   bitboard_t<uint128_t> bb;
   for (int n=0; n<16; n++) {
      bitboard_t<uint128_t>::board_rank[n] &= ~ws;
      bitboard_t<uint128_t>::board_file[n] &= ~ws;
   }
   for (int n=0; n<32; n++) {
      bitboard_t<uint128_t>::board_diagonal[n] &= ~ws;
      bitboard_t<uint128_t>::board_antidiagonal[n] &= ~ws;
   }
   bitboard_t<uint128_t>::board_dark  ^= ws;
   bitboard_t<uint128_t>::board_light ^= ws;

   /* Remap the wizard squares on the diagonals */
   bitboard_t<uint128_t>::diagonal_nr[0]--;
   bitboard_t<uint128_t>::diagonal_nr[files*ranks-1]++;
   bitboard_t<uint128_t>::anti_diagonal_nr[files-1]--;
   bitboard_t<uint128_t>::anti_diagonal_nr[files*(ranks-1)]++;
   for (int n=0; n<32; n++) {
      bitboard_t<uint128_t>::board_diagonal[n].clear();
      bitboard_t<uint128_t>::board_antidiagonal[n].clear();
   }
   for (int sq=0; sq<files*ranks; sq++) {
      int n;

      n = bitboard_t<uint128_t>::diagonal_nr[sq];
      bitboard_t<uint128_t>::board_diagonal[n].set(sq);

      n = bitboard_t<uint128_t>::anti_diagonal_nr[sq];
      bitboard_t<uint128_t>::board_antidiagonal[n].set(sq);
   }

   /* Adjust leaper tables */
   for (int n=0; n<8; n++)
      game->movegen.step_mask[n] &=~ws;
   for (int n=0; n<game->movegen.number_of_leapers; n++) {
      for (int s=0; s<files*ranks; s++)
         game->movegen.leaper[n][s] &= ~ws;
   }
   for (int n=0; n<game->movegen.number_of_aleapers; n++) {
      for (int s=0; s<files*ranks; s++) {
         game->movegen.aleaper[WHITE][n][s] &= ~ws;
         game->movegen.aleaper[BLACK][n][s] &= ~ws;
      }
   }
   for (int n=0; n<game->movegen.number_of_steppers; n++) {
      for (int s=0; s<files*ranks; s++) {
         game->movegen.stepper_step[n][WHITE][s] &= ~ws;
         game->movegen.stepper_step[n][BLACK][s] &= ~ws;
      }
   }

   /* Correct leaper tables for wizard squares */
   bb = ws & bitboard_t<uint128_t>::board_south;
   while (!bb.is_empty()) {
      int sq = bb.bitscan();
      bb.reset(sq);

      for (int n=0; n<game->movegen.number_of_leapers; n++) {
         bitboard_t<uint128_t> moves = game->movegen.leaper[n][sq] >> files;
         game->movegen.leaper[n][sq] = moves;
         while (!moves.is_empty()) {
            int s2 = moves.bitscan();
            moves.reset(s2);
            game->movegen.leaper[n][s2].set(sq);
         }
      }
   }
   bb = ws & bitboard_t<uint128_t>::board_north;
   while (!bb.is_empty()) {
      int sq = bb.bitscan();
      bb.reset(sq);

      for (int n=0; n<game->movegen.number_of_leapers; n++) {
         bitboard_t<uint128_t> moves = game->movegen.leaper[n][sq] << files;
         game->movegen.leaper[n][sq] = moves;
         while (!moves.is_empty()) {
            int s2 = moves.bitscan();
            moves.reset(s2);
            game->movegen.leaper[n][s2].set(sq);
         }
      }
   }

   /* Pawns can never reach wizard squares */
   game->pt.prison[WHITE][ind] &= ~ws;
   game->pt.prison[BLACK][ind] &= ~ws;

   /* Map squares on the 12x12 board to the 12x10 bitboard representation */
   game->virtual_files = 12;
   game->virtual_ranks = 12;
   for (int sq = 0; sq<12*12; sq++)
      game->square_to_bit[sq] = -1;
   game->top_left = 12*11;
   for (int r = 0; r<ranks; r++)
   for (int f = 0; f<files; f++) {
      int bit = f+r*files;
      int sq  = f + (1+r)*12;
      if ((f == 0) || (f == files-1)) {
         if (r == 0) sq -= 12;
         if (r == ranks-1) sq += 12;
      }
      game->square_to_bit[sq] = bit;
   }

   bb = (bitboard_t<uint128_t>::board_east_edge | bitboard_t<uint128_t>::board_west_edge) & ~ws;
   while (!bb.is_empty()) {
      int sq = bb.bitscan();
      bb.reset(sq);

      game->remove_square(sq);
   }

   game->start_fen = strdup("wcrnbqkbnrcw/*pppppppppp*/*10*/*10*/*10*/*10*/*10*/*10*/*PPPPPPPPPP*/WCRNBQKBNRCW w KQkq -");
   game->start_fen = strdup("w**********w/*crnbqkbnrc*/*pppppppppp*/*10*/*10*/*10*/*10*/*10*/*10*/*PPPPPPPPPP*/*CRNBQKBNRC*/W**********W w KQkq -");

   game->xb_setup  = strdup("(PNBRQ..C.W...........Kpnbrq..c.w...........k) 12x12+0_fairy");

   game->name = strdup("Omega chess");

   game->finalise_variant();

   /* Relabel squares so their names are consistent with the 12x12 board rather than the 12x10 */
   for (int n=0; n<128; n++) if (square_names[n]) square_names[n][0] = 0;
   for (int r = 0; r<12; r++) {
      for (int f = 0; f<12; f++) {
         //printf("% 4d ", game->square_to_bit[f+r*12]);
         if (game->square_to_bit[f+r*12] >= 0) {
            snprintf(square_names[game->square_to_bit[f+r*12]], 10, "%c%d", 'a'+f, 1+r);
            //printf("%4s", square_names[game->square_to_bit[f+r*12]]);
         } else {
            //printf("    ");
         }
      }
      //printf("\n");
   }
   keep_labels = true;

   return game;
}

template <typename kind>
struct file_piece_description_t {
   bitboard_t<kind> special_zone[2];
   bitboard_t<kind> promotion_zone[MAX_PZ][2];
   bitboard_t<kind> optional_promotion_zone[2];
   bitboard_t<kind> prison_zone[2];
   bitboard_t<kind> block_zone[2];
   bitboard_t<kind> drop_zone[2];
   bitboard_t<kind> entry_promotion_zone[2];
   char *name;
   char *abbr;
   char *symbol;
   char *promotion[MAX_PZ];
   char *demotion;
   char *allowed_victim;
   move_flag_t move, capture, special, initial;
   uint32_t flags;
   int value;
   int max[2];
   bool set_capture;
   bool set_prison;
   bool set_block;
   bool set_optional_promotion;
   bool set_drop_zone;
   bool set_entry_promotion_zone;
   int pz_count;
};

template <typename kind>
struct board_zone_description_t {
   char *name;
   bitboard_t<kind> zone;
};

template <typename kind>
static void add_piece_to_game(game_template_t<kind> *game, const file_piece_description_t<kind> *pd)
{
   move_flag_t cflags = pd->move;
   if (pd->set_capture) cflags = pd->capture;
   int id = game->add_piece_type(pd->move, cflags, pd->flags, pd->promotion_zone[0], pd->promotion[0], pd->name, pd->symbol, pd->abbr, pd->value);

   if (pd->special) {
      char *s = strdup(pd->symbol);
      char *p = strchr(s, ',');
      if (p) *p = 0;
      game->add_special_move(s, pd->special_zone, pd->special);
      free(s);
   }

   if (pd->initial) {
      char *s = strdup(pd->symbol);
      char *p = strchr(s, ',');
      if (p) *p = 0;
      game->add_initial_move(s, pd->initial);
      free(s);
   }

   if (pd->max[WHITE] > 0)
      game->pt.piece_maximum[id][WHITE] = pd->max[WHITE];

   if (pd->max[BLACK] > 0)
      game->pt.piece_maximum[id][BLACK] = pd->max[BLACK];

   if (pd->set_prison) {
      game->pt.prison[WHITE][id] = pd->prison_zone[WHITE];
      game->pt.prison[BLACK][id] = pd->prison_zone[BLACK];
   }

   if (pd->set_block) {
      game->pt.block[WHITE][id] = pd->block_zone[WHITE];
      game->pt.block[BLACK][id] = pd->block_zone[BLACK];
   }

   if (pd->set_drop_zone) {
      game->pt.drop_zone[WHITE][id] = pd->drop_zone[WHITE];
      game->pt.drop_zone[BLACK][id] = pd->drop_zone[BLACK];
   }

   for (int k=1; k<pd->pz_count; k++) {
      game->pt.promotion[id][k].zone[WHITE] = pd->promotion_zone[k][WHITE];
      game->pt.promotion[id][k].zone[BLACK] = pd->promotion_zone[k][BLACK];
      game->pt.promotion[id][k].string = strdup(pd->promotion[k]);
   }

   if (pd->set_optional_promotion) {
      game->pt.optional_promotion_zone[WHITE][id] = pd->optional_promotion_zone[WHITE];
      game->pt.optional_promotion_zone[BLACK][id] = pd->optional_promotion_zone[BLACK];
      game->pt.promotion_zone[WHITE][id] |= pd->optional_promotion_zone[WHITE];
      game->pt.promotion_zone[BLACK][id] |= pd->optional_promotion_zone[BLACK];
      game->pt.pzset[id] = true;
   }

   if (pd->set_entry_promotion_zone) {
      game->pt.entry_promotion_zone[WHITE][id] = pd->entry_promotion_zone[WHITE];
      game->pt.entry_promotion_zone[BLACK][id] = pd->entry_promotion_zone[BLACK];
   }

   if (pd->demotion)
      game->pt.demotion_string[id] = strdup(pd->demotion);

   if (pd->allowed_victim)
      game->pt.allowed_victim[id] = strdup(pd->allowed_victim);
}


template <typename kind>
game_template_t<kind> *parse_game_description(FILE *f, const char *variant_name, int files, int ranks)
{
   board_zone_description_t<kind> zone[8*sizeof(kind)+2];
   file_piece_description_t<kind> *pd = NULL;
   int num_zones = 0;

   game_template_t<kind> *game = new game_template_t<kind>;
   game->set_board_size(files, ranks);
   game->name = strdup(variant_name);

   char line[4096];

   zone[num_zones].name = strdup("empty");
   zone[num_zones].zone.clear();
   num_zones++;

   zone[num_zones].name = strdup("all");
   zone[num_zones].zone = bitboard_t<kind>::board_all;
   num_zones++;

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

      /* Skip empty lines */
      if (s[0] == '\0') continue;

      /* New variant - we are done */
      if (strstr(line, "Variant:") == line) {
         break;
      }

      /* Set board size - can't occur more than once!*/
      if (strstr(line, "Board:") == line) {
         delete game;
         return NULL;
      }

      /* Starting position (FEN) */
      if (strstr(line, "FEN:") == line) {
         char *s = line+4;
         s = strstr(s, "\"");
         if (!s) continue;
         s++;
         char *eof = strstr(s, "\"");
         if (!eof) continue;
         eof[0] = '\0';
         game->start_fen = strdup(s);
         if (game->xb_setup == NULL) game->xb_setup = strdup("(PNBRQFEACWMOHIJGDVLSUKpnbrqfeacwmohijgdvlsuk)");

         continue;
      }

      /* Piece description for XBoard */
      if ((strstr(line, "XBoard pieces:") == line) || (strstr(line, "WinBoard pieces:") == line)) {
         char *s = strstr(line, ":") + 1;
         while (*s && isspace(*s)) s++;
         s = strstr(s, "\"");
         if (!s) continue;
         s++;
         char *eof = strstr(s, "\"");
         if (!eof) continue;
         if (strstr(s, " "))
            eof = strstr(s, " ");
         eof[0] = '\0';
         
         size_t len = eof - s;
         free(game->xb_setup);
         game->xb_setup = (char *)malloc(len+4);
         snprintf(game->xb_setup, len+3, "(%s)", s);
         
         continue;
      }

      /* Piece description for XBoard */
      if ((strstr(line, "XBoard parent:") == line) || (strstr(line, "WinBoard parent:") == line)) {
         char *s = line+14;
         s = strstr(s, "\"");
         if (!s) continue;
         s++;
         char *eof = strstr(s, "\"");
         if (!eof) continue;
         eof[0] = '\0';

         free(game->xb_parent);
         game->xb_parent = (char *)malloc(51);
         snprintf(game->xb_parent, 50, "%s", s);

         continue;
      }

      /* Define a region of the board */
      if (strstr(line, "Zone:") == line) {
         char *name = line+5;
         while(isspace(*name)) name++;
         char *s = strstr(name, "=");
         char *sq;
         if (!s) continue;
         sq = s+1;
         s[0] = '\0'; s--;
         while(isspace(*s)) s--;
         s[1] = '\0';
         zone[num_zones].name = strdup(name);

         s = strtok(sq, ",");
         while(isspace(*s)) s++;
         char file;
         int rank;
         sscanf(s, "%c%d", &file, &rank);
         file -= 'a';
         rank--;
         zone[num_zones].zone.clear();
         zone[num_zones].zone.set(game->pack_rank_file(rank, file));
         while ((s = strtok(NULL, ","))) {
            sscanf(s, "%c%d", &file, &rank);
            file -= 'a';
            rank--;
            zone[num_zones].zone.set(game->pack_rank_file(rank, file));
         }
         num_zones++;
         continue;
      }

      /* Exclude a region on the board (for the purpose of move generation) */
      if (strstr(line, "Exclude:") == line) {
         char *s = line+8;
         while(isspace(*s)) s++;
         char *sq = s;
         char file;
         int rank;

         s = strtok(sq, ",");
         while(isspace(*s)) s++;
         sscanf(s, "%c%d", &file, &rank);
         file -= 'a';
         rank--;
         game->remove_square(game->pack_rank_file(rank, file));
         while ((s = strtok(NULL, ","))) {
            sscanf(s, "%c%d", &file, &rank);
            file -= 'a';
            rank--;
            game->remove_square(game->pack_rank_file(rank, file));
         }
         continue;
      }

      /* Capture-the-flag, white flag (for black to capture) */
      if (strstr(line, "WhiteFlag:") == line) {
         char *s = line+10;
         while(isspace(*s)) s++;
         char *sq = s;
         char file;
         int rank;

         s = strtok(sq, ",");
         sscanf(sq, "%c%d", &file, &rank);
         file -= 'a';
         rank--;
         game->place_flag(BLACK, game->pack_rank_file(rank, file));
         while ((s = strtok(NULL, ","))) {
            sscanf(s, "%c%d", &file, &rank);
            file -= 'a';
            rank--;
            game->place_flag(BLACK, game->pack_rank_file(rank, file));
         }
         continue;
      }

      /* Capture-the-flag, black flag (for white to capture) */
      if (strstr(line, "BlackFlag:") == line) {
         char *s = line+10;
         while(isspace(*s)) s++;
         char *sq = s;
         char file;
         int rank;

         s = strtok(sq, ",");
         sscanf(sq, "%c%d", &file, &rank);
         file -= 'a';
         rank--;
         game->place_flag(WHITE, game->pack_rank_file(rank, file));
         while ((s = strtok(NULL, ","))) {
            sscanf(s, "%c%d", &file, &rank);
            file -= 'a';
            rank--;
            game->place_flag(WHITE, game->pack_rank_file(rank, file));
         }
         continue;
      }

      /* Define a new piece */
      if (strstr(line, "Piece:") == line) {
         if (pd) {
            assert(game);
            add_piece_to_game(game, pd);
            free(pd->name);
            free(pd->abbr);
            free(pd->symbol);
            for (int k=0; k<MAX_PZ; k++) free(pd->promotion[k]);
            free(pd->demotion);
            free(pd);
         }
         pd = (file_piece_description_t<kind> *)calloc(1, sizeof *pd);
         s = line + 6;
         while(isspace(*s)) s++;
         while(*s == ' ') s++;
         pd->name = strdup(s);
         for (side_t side = WHITE; side < NUM_SIDES; side++) {
            pd->prison_zone[side] = bitboard_t<kind>::board_all;
            pd->block_zone[side]  = bitboard_t<kind>::board_empty;
            pd->drop_zone[side]   = bitboard_t<kind>::board_all;
         }
         continue;
      }

      if (strstr(line, "Move:") == line && pd) {
         s = line + 5;
         while(*s && isspace(*s)) s++;
         pd->move |= game->movegen.define_piece_move(s);
         continue;
      }

      if (strstr(line, "Drop zone:") == line && pd) {
         s = line + 7;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->drop_zone[WHITE] = zone[n].zone;
               break;
            }

         s = p;
         while (isspace(*s)) s++;
         p = strstr(p, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->drop_zone[BLACK] = zone[n].zone;
               break;
            }

         pd->set_drop_zone = true;

         /* Do not override the drop table to remove the graveyard */
         pd->flags |= PF_DROPDEAD;
      }

      if (strstr(line, "Prison:") == line && pd) {
         s = line + 7;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->prison_zone[WHITE] = zone[n].zone;
               break;
            }

         s = p;
         while (*s && isspace(*s)) s++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->prison_zone[BLACK] = zone[n].zone;
               break;
            }

         pd->set_prison = true;
      }

      if (strstr(line, "Block:") == line && pd) {
         s = line + 7;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->block_zone[WHITE] = zone[n].zone;
               break;
            }

         s = p;
         while (*s && isspace(*s)) s++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->block_zone[BLACK] = zone[n].zone;
               break;
            }

         pd->set_block = true;
      }

      if (strstr(line, "Special:") == line && pd) {
         s = line + 8;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->special_zone[WHITE] = zone[n].zone;
               break;
            }

         s = p;
         while (isspace(*s)) s++;
         p = strstr(p, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->special_zone[BLACK] = zone[n].zone;
               break;
            }

         while (isspace(*p)) p++;
         pd->special |= game->movegen.define_piece_move(p);
         continue;
      }

      if (strstr(line, "Initial:") == line && pd) {
         s = line + 8;
         while(*s && isspace(*s)) s++;
         pd->initial |= game->movegen.define_piece_move(s);
         continue;
      }

      if (strstr(line, "Capture:") == line && pd) {
         s = line + 8;
         while(isspace(*s)) s++;
         pd->capture |= game->movegen.define_piece_move(s);
         pd->set_capture = true;
         continue;
      }

      if (strstr(line, "Castle:") == line && pd) {
         side_t side = WHITE;
         int from, to, rfrom;
         char file;
         int rank;

         s = line + 7;
         while(isspace(*s)) s++;

         /* Field 1: side */
         if (strstr(s, "black") == s) side = BLACK;
         s += 5;

         /* From square */
         while(isspace(*s)) s++;
         sscanf(s, "%c%d", &file, &rank);
         file -= 'a';
         rank--;
         from = game->pack_rank_file(rank, file);

         /* Rook from-square */
         char *rs = strstr(s, "with");
         if (!rs) continue;
         rs+=4;
         while(isspace(*rs)) rs++;
         sscanf(rs, "%c%d", &file, &rank);
         file -= 'a';
         rank--;
         rfrom = game->pack_rank_file(rank, file);

         /* To square */
         s = strstr(s, "-");
         while (s && s < rs) {
            s++;
            while(isspace(*s)) s++;
            sscanf(s, "%c%d", &file, &rank);
            file -= 'a';
            rank--;
            to = game->pack_rank_file(rank, file);

            /* Calculate castle masks */
            game->deduce_castle_flags(side, from, to, rfrom);
            s = strstr(s, ",");
         }

         continue;
      }

      if (strstr(line, "Symbol:") == line && pd) {
         s = line + 7;
         while (isspace(*s)) s++;
         char *p = strstr(s, "\"");
         if (!p) continue;
         p++;
         s = strstr(p, "\"");
         if (!s) continue;
         s++; s[-1] = '\0';
         pd->abbr = strdup(p);

         p = strstr(s, "\"");
         if (!p) continue;
         p++;
         s = strstr(p, "\"");
         if (!s) continue;
         s++; s[-1] = '\0';
         pd->symbol = strdup(p);
         continue;
      }

      if (strstr(line, "Promotion:") == line && pd) {
         s = line + 10;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->promotion_zone[pd->pz_count][WHITE] = zone[n].zone;
               break;
            }
         s = p;
         while (isspace(*s)) s++;
         p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->promotion_zone[pd->pz_count][BLACK] = zone[n].zone;
               break;
            }

         p = strstr(p, "\"");
         p++;
         s = strstr(p, "\"");
         if (s) s[0] = '\0';

         pd->promotion[pd->pz_count] = strdup(p);
         pd->pz_count++;
         continue;
      }

      if (strstr(line, "Optional promotion:") == line && pd) {
         s = line + 19;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->promotion_zone[pd->pz_count][WHITE] = zone[n].zone;
               pd->optional_promotion_zone[WHITE] |= zone[n].zone;
               break;
            }
         s = p;
         while (isspace(*s)) s++;
         p = strstr(s, ",");
         if (p) {
            p[0] = '\0';
            p++;
         }

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->promotion_zone[pd->pz_count][BLACK] = zone[n].zone;
               pd->optional_promotion_zone[BLACK] |= zone[n].zone;
               break;
            }
         pd->set_optional_promotion = true;

         if (p) {
            p = strstr(p, "\"");
            if(!p) continue;
            p++;
            s = strstr(p, "\"");
            if (s) s[0] = '\0';

            pd->promotion[pd->pz_count] = strdup(p);
         } else
            continue;

         pd->pz_count++;
         continue;
      }

      if (strstr(line, "Entry promotion:") == line && pd) {
         s = line + 16;
         while (isspace(*s)) s++;
         char *p = strstr(s, ",");
         if (!p) continue;
         p[0] = '\0'; p++;

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->entry_promotion_zone[WHITE] = zone[n].zone;
               break;
            }
         s = p;
         while (isspace(*s)) s++;
         p = strstr(s, ",");
         if (p) {
            p[0] = '\0';
            p++;
         }

         /* Find the corresponding zone */
         for(int n=0; n<num_zones; n++)
            if (strstr(s, zone[n].name) == s) {
               pd->entry_promotion_zone[BLACK] = zone[n].zone;
               break;
            }
         pd->set_entry_promotion_zone = true;

         continue;
      }

      if (strstr(line, "Demotion:") == line && pd) {
         s = line + 9;
         while (isspace(*s)) s++;
         char *p = s;

         p = strstr(p, "\"");
         if(!p) continue;
         p++;
         s = strstr(p, "\"");
         if (s) s[0] = '\0';

         pd->demotion = strdup(p);
         continue;
      }

      if (strstr(line, "Allowed victims:") == line && pd) {
         s = line + 16;
         while (isspace(*s)) s++;
         char *p = s;

         p = strstr(p, "\"");
         if(!p) continue;
         p++;
         s = strstr(p, "\"");
         if (s) s[0] = '\0';

         pd->allowed_victim = strdup(p);
         continue;
      }

      if (strstr(line, "Flags:") == line && pd) {
         char *p;
         s = line + 6;
         while(isspace(*s)) s++;
         while((p = strtok(s, ","))) {
            s = NULL;
            while(isspace(*p)) p++;
            while(*p == ' ') p++;
            if (strstr(p, "royal") == p) {
               pd->flags |= PF_ROYAL;
            } else if (strstr(p, "castle") == p) {
               /* Ignore, deprecated */
            } else if (strstr(p, "set_ep") == p) {
               pd->flags |= PF_SET_EP;
            } else if (strstr(p, "take_ep") == p) {
               pd->flags |= PF_TAKE_EP;
            } else if (strstr(p, "drop_no_check") == p) {
               pd->flags |= PF_DROPNOCHECK;
            } else if (strstr(p, "drop_no_mate") == p) {
               pd->flags |= PF_DROPNOMATE;
            } else if (strstr(p, "drop_one_file") == p) {
               pd->flags |= PF_DROPONEFILE;
            } else if (strstr(p, "drop_dead") == p) {
               pd->flags |= PF_DROPDEAD;
            } else if (strstr(p, "no_mate") == p) {
               pd->flags |= PF_NOMATE;
            } else if (strstr(p, "shak") == p) {
               pd->flags |= PF_SHAK;
            } else if (strstr(p, "assimilate") == p) {
               pd->flags |= PF_ASSIMILATE;
            } else if (strstr(p, "no_retaliate") == p) {
               pd->flags |= PF_NO_RETALIATE;
            } else if (strstr(p, "endangered") == p) {
               pd->flags |= PF_ENDANGERED;
            } else if (strstr(p, "iron") == p) {
               pd->flags |= PF_IRON;
            } else if (strstr(p, "capture_flag") == p) {
               pd->flags |= PF_CAPTUREFLAG;
            }
         }
         continue;
      }

      if (strstr(line, "Value:") == line && pd) {
         sscanf(line+6, "%d", &pd->value);
         continue;
      }

      if (strstr(line, "Max:") == line && pd) {
         if (strstr(line, ","))
            sscanf(line+5, "%d,%d", &pd->max[0], &pd->max[1]);
         else {
            sscanf(line+5, "%d", &pd->max[0]);
            pd->max[1] = pd->max[0];
         }
         continue;
      }

      if (strstr(line, "Rule:") == line) {
         s = line + 5;
         while(isspace(*s)) s++;
         if (strstr(s, "checkmate") == s) {
            if (strstr(s, "loss")) {
               game->mate_score = LEGALWIN;
            } else if (strstr(s, "win")) {
               game->mate_score = -LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->mate_score = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->mate_score = ILLEGAL;
            }
         }
         if (strstr(s, "stalemate") == s) {
            if (strstr(s, "loss")) {
               game->stale_score = LEGALWIN;
            } else if (strstr(s, "win")) {
               game->stale_score = -LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->stale_score = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->stale_score = ILLEGAL;
            }
         }
         if (strstr(s, "repeat") == s) {
            int count = -1;
            if (sscanf(s, "repeat%d", &count) == 1 && count >= 0) {
               game->repeat_claim  = count-1;
               if (game->repeat_claim <= 0) game->repeat_claim = INT_MAX;
               if (strstr(s, "loss")) {
                  game->rep_score = -LEGALWIN;
               } else if (strstr(s, "win")) {
                  game->rep_score = LEGALWIN;
               } else if (strstr(s, "draw")) {
                  game->rep_score = LEGALDRAW;
               } else if (strstr(s, "illegal")) {
                  game->rep_score = -ILLEGAL;
               }
            }
         }
         if (strstr(s, "check") == s) {
            int count = -1;
            if (sscanf(s, "check%d", &count) == 1 && count >= 1) {
               game->check_limit  = count;
               if (strstr(s, "loss")) {
                  game->check_score = LEGALWIN;
               } else if (strstr(s, "win")) {
                  game->check_score = -LEGALWIN;
               } else if (strstr(s, "draw")) {
                  game->check_score = LEGALDRAW;
               } else if (strstr(s, "illegal")) {
                  game->check_score = -ILLEGAL;
               }
            }
         }
         if (strstr(s, "perpetual") == s) {
            if (strstr(s, "loss")) {
               game->perpetual = -LEGALWIN;
            } else if (strstr(s, "win")) {
               game->perpetual = LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->perpetual = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->perpetual = -ILLEGAL;
            } else if (strstr(s, "repeat")) {
               game->perpetual = ILLEGAL+1;
            }
         }
         if (strstr(s, "loneking") == s) {
            if (strstr(s, "loss")) {
               game->bare_king_score = -LEGALWIN;
            } else if (strstr(s, "win")) {
               game->bare_king_score = LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->bare_king_score = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->bare_king_score = ILLEGAL;
            }
         }
         if (strstr(s, "nopieces") == s) {
            if (strstr(s, "loss")) {
               game->no_piece_score = -LEGALWIN;
            } else if (strstr(s, "win")) {
               game->no_piece_score = LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->no_piece_score = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->no_piece_score = ILLEGAL;
            }
         }
         if (strstr(s, "captureanyflag") == s) {
            game->board.rule_flags |= RF_CAPTURE_ANY_FLAG;
            if (strstr(s, "loss")) {
               game->flag_score = LEGALWIN;
            } else if (strstr(s, "win")) {
               game->flag_score = -LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->flag_score = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->flag_score = ILLEGAL;
            }
         }
         if (strstr(s, "captureallflags") == s) {
            game->board.rule_flags |= RF_CAPTURE_ALL_FLAG;
            if (strstr(s, "loss")) {
               game->flag_score = LEGALWIN;
            } else if (strstr(s, "win")) {
               game->flag_score = -LEGALWIN;
            } else if (strstr(s, "draw")) {
               game->flag_score = LEGALDRAW;
            } else if (strstr(s, "illegal")) {
               game->flag_score = ILLEGAL;
            }
         }
         if (strstr(s, "movecountdraw") == s) {
            char *p = strstr(s, "=");
            if (p) {
               p++;
               int n = 50;
               sscanf(p, "%d", &n);
               n = (2*n)+1;
               if (n>127) n = 127;

               game->fifty_limit = n;
            }
         }

         if (strstr(s, "taboo")) game->board.rule_flags |= RF_KING_TABOO;

         if (strstr(s, "keep capture"))      game->board.rule_flags |= RF_KEEP_CAPTURE;
         if (strstr(s, "return capture"))    game->board.rule_flags |= RF_RETURN_CAPTURE;
         if (strstr(s, "forced capture"))    game->board.rule_flags |= RF_FORCE_CAPTURE;
         if (strstr(s, "duplecheck"))        game->board.rule_flags |= RF_KING_DUPLECHECK;
         if (strstr(s, "allow pickup"))      game->board.rule_flags |= RF_ALLOW_PICKUP;
         if (strstr(s, "allow drop"))        game->board.rule_flags |= RF_ALLOW_DROPS;
         if (strstr(s, "force drop"))        game->board.rule_flags |= RF_FORCE_DROPS;
         if (strstr(s, "gate drop"))         game->board.rule_flags |= RF_GATE_DROPS;
         if (strstr(s, "promote here"))      game->board.rule_flags |= RF_PROMOTE_IN_PLACE;
         if (strstr(s, "promote drop"))      game->board.rule_flags |= RF_PROMOTE_ON_DROP;
         if (strstr(s, "special init"))      game->board.rule_flags |= RF_SPECIAL_IS_INIT;
         if (strstr(s, "bare rule"))         game->board.rule_flags |= RF_USE_BARERULE;
         if (strstr(s, "chase rule"))        game->board.rule_flags |= RF_USE_CHASERULE;
         if (strstr(s, "shak rule"))         game->board.rule_flags |= RF_USE_SHAKMATE;
         if (strstr(s, "check any king"))    game->board.rule_flags |= RF_CHECK_ANY_KING;
         if (strstr(s, "en-passant check"))  game->board.rule_flags |= RF_NO_MOVE_PAST_CHECK;
         if (strstr(s, "quiet promotion"))   game->board.rule_flags |= RF_QUIET_PROMOTION;
         if (strstr(s, "promote by move"))   game->board.rule_flags |= RF_PROMOTE_BY_MOVE;
      }
   }

   /* Add the last remaining piece */
   if (pd) {
      add_piece_to_game(game, pd);
      free(pd->name);
      free(pd->abbr);
      free(pd->symbol);
      for (int n=0; n<pd->pz_count; n++)
         free(pd->promotion[n]);
      free(pd->demotion);
      free(pd);
      pd = NULL;
   }

   /* Free memory */
   int n;
   for (n=0; n<num_zones; n++)
      free(zone[n].name);

   /* Finalise game description */
   game->finalise_variant();

   return game;
}

game_t *create_game_from_file(const char *filename, const char *variant_name)
{
   game_t *game = NULL;
   int files = 0;
   int ranks = 0;
   FILE *f;

   f = fopen(filename, "r");
   if (!f) return game;
   char line[4096];

   bool found_variant = false;

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
         if (game) break;                          /* We're done loading the variant we're looking for */
         if (strstr(line+8, variant_name)) {       /* We've found the variant we're looking for */
            found_variant = true;
         }
         continue;
      }

      /* Have we found the correct variant? */
      if (!found_variant) continue;

      /* Set board size */
      if (strstr(line, "Board:") == line) {
         s = line+6;
         sscanf(s, "%dx%d", &files, &ranks);

         /* Check whether the input is valid */
         if (files > 16 || ranks > 16 || (files*ranks > 128)) {
            fclose(f);

            char msg[256];
            int n = 0;
            if (files > 16) n += snprintf(msg+n, 256-n, "Board has too many files.");
            if (ranks > 16) n += snprintf(msg+n, 256-n, "Board has too many ranks.");
            if (files*ranks>128) n += snprintf(msg+n, 256-n, "Board has too many files.");
            if (game->xboard_output) game->xboard_output("tellusererror %s\n", msg);
            return NULL;
         }

         if (files * ranks <= 64)
            game = parse_game_description<uint64_t>(f, variant_name, files, ranks);
         else
            game = parse_game_description<uint128_t>(f, variant_name, files, ranks);

         break;
      }

      continue;
   }
   fclose(f);

   return game;
}


#endif

