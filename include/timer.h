/*  Sjaak, a program for playing chess variants
 *  Copyright (C) 2011  Evert Glebbeek
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
#ifndef TIMER_H
#define TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bool.h"

uint64_t get_timer(void);

typedef struct chess_clock_t {
   /* various things having to do with time management */
   int time_left;          /* Total time left on the clock, in msec */
   int time_inc;           /* Time increment per move, in msec */
   int time_per_move;      /* Time per move, in msec */
   int extra_time;         /* Extra time allocated for this move */
   uint64_t start_time;    /* Timer value when the search started */
   int movestogo;          /* Number of moves until the next cycle */
   int movestotc;          /* Number of moves per cycle */
   int root_moves_played;  /* Total number of moves played so far in the game */
   size_t max_nodes;       /* Maximum number of nodes to search */
   size_t nodes_searched;  /* Number of nodes currently searched */
   bool pondering;         /* true: not our move, ignore clock but mind keyboard */

   bool (*check_clock)(const struct chess_clock_t *clock);
} chess_clock_t;

void start_clock(chess_clock_t *clock);

int peek_timer(const chess_clock_t *clock);

int get_chess_clock_time_for_move(const chess_clock_t *clock);

void set_ponder_timer(chess_clock_t *clock);
void set_infinite_time(chess_clock_t *clock);
void set_time_per_move(chess_clock_t *clock, int msec);
void set_time_for_game(chess_clock_t *clock);

#ifdef __cplusplus
}
#endif

#endif
