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

#if defined _MSC_VER
#  define USE_GETTICKCOUNT
#  include <windows.h>
#  undef min
#else
#  include <unistd.h>
#  if defined HAVE_CLOCK_GETTIME && defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 199309L && defined _POSIX_TIMERS && _POSIX_TIMERS>0 && defined _POSIX_MONOTONIC_CLOCK
#     define USE_CLOCK_GETTIME
#     include <time.h>
#  else
#     include <sys/time.h>
#  endif
#endif

#include <stdint.h>
#include <limits.h>
#include "assert.h"
#include "timer.h"
#include "keypressed.h"

/* returns the current value of the counter, in micro seconds */
uint64_t get_timer(void)
{
#ifdef USE_CLOCK_GETTIME
   struct timespec t;
   clock_gettime(0, &t);

   return t.tv_sec*1000000 + t.tv_nsec/1000; 
#elif defined USE_GETTICKCOUNT
   return (uint64_t)GetTickCount() * 1000;
#else
   struct timeval t;
   gettimeofday(&t, NULL);
   
   return t.tv_sec*1000000 + t.tv_usec; 
#endif
}

static int min(int x, int y) { return (x<y)?x:y; }

/* Start the clock for the current player */
void start_clock(chess_clock_t *clock)
{
   assert(clock);
   clock->start_time = get_timer();
   clock->extra_time = 0;
}

/* Get the time elapsed since the last call to start_clock(), in ms */
int peek_timer(const chess_clock_t *clock)
{
   return (int)((get_timer() - clock->start_time) / 1000);
}

/* Calculate time allocated for this a move */
int get_chess_clock_time_for_move(const chess_clock_t *clock)
{
   int num_moves = clock->movestogo;
   int time_for_move;

   if (clock->pondering) {
      if (!keyboard_input_waiting())
         return INT_MAX;
      else
         return 0;
   }

   if (clock->time_per_move)
      return clock->time_per_move;

   if (clock->check_clock == NULL)
      return INT_MAX;


   /* No set number of moves to be played within the time limit, make some
    * sort of estimate for how many moves we may want to play in the time
    * remaining.
    */
   if (num_moves == 0) {
      if (clock->root_moves_played < 20)
         num_moves = 15+clock->root_moves_played;
      else
         num_moves = 20;
   }

   if (num_moves < 0) num_moves = 10;

   /* Base time for this move: the total time left divided by the number of
    * moves.
    */
   time_for_move = clock->time_left/num_moves;

   /* We want to preserve a small buffer to avoid time losses */
   if (num_moves < 5)
      time_for_move = (int)(time_for_move*0.95);

   /* Adjust: we can spare at least half the increment */
   if (clock->time_left > clock->time_inc/2)
      time_for_move += clock->time_inc/2;

   /* We may have allocated some extra time for this move, for instance
    * because our best move failed low.
    */
   time_for_move += min(clock->extra_time, clock->time_left);

   /* If we're short on time and have an increment, then play quickly so we
    * gain some extra time to finish the game.
    */
   if (clock->time_left < clock->time_inc * 5)
      time_for_move = 3*clock->time_inc/4;

   return min(time_for_move, clock->time_left - clock->time_left/8);
}

/* Time management: check if a fixed time per move has passed since the
 * clock was started.
 */
static bool check_time_per_move_clock(const chess_clock_t *clock)
{
   int time_passed = peek_timer(clock);

   if (time_passed >= clock->time_per_move)
      return true;

   return false;
}

static bool check_time_for_game(const chess_clock_t *clock)
{
   int time_passed = peek_timer(clock);
   int time_for_move = get_chess_clock_time_for_move(clock);

   /* If we've expended our time for this move, then abort */
   if (time_passed >= time_for_move)
      return true;

   return false;
}

static bool check_keyboard(const chess_clock_t *clock)
{
   return keyboard_input_waiting();
}

void set_time_per_move(chess_clock_t *clock, int msec)
{
   clock->time_per_move = msec;
   clock->check_clock = check_time_per_move_clock;
}

void set_ponder_timer(chess_clock_t *clock)
{
   clock->check_clock = check_keyboard;
}

void set_infinite_time(chess_clock_t *clock)
{
   clock->check_clock = NULL;
}

void set_time_for_game(chess_clock_t *clock)
{
   clock->time_per_move = 0;
   clock->check_clock = check_time_for_game;
}
