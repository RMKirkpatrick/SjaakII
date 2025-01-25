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
#include <stdio.h>
#include "bool.h"
#include "keypressed.h"

#if defined _WIN32 || defined _WIN64
#define WINDOWS
#endif

#if defined __unix__ || defined __APPLE__
#define UNIX
#endif

#ifdef WINDOWS
#undef DATADIR
#include <windows.h>
#include <conio.h>
#ifndef min
#define min(x,y) ( ((x)<(y))?(x):(y) )
#endif
#endif

#ifdef UNIX
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#if defined WINDOWS || defined UNIX
const bool ponder_ok = true;
#else
const bool ponder_ok = false;
#endif

/* Determine whether there is input waiting in the standard input stream.
 * A variation of this code is present in at least OliThink, Beowulf,
 * Crafty and Stockfish.
 * This version adapted from Stockfish.
 */
bool keyboard_input_waiting(void)
{
#ifdef WINDOWS
   static bool virgin = true;
   static bool pipe = false;
   static HANDLE inh;
   DWORD dw;

#if defined(FILE_CNT)
   if (stdin->_cnt > 0)
      return stdin->_cnt;
#endif
   if (virgin) {
      virgin = false;
      inh = GetStdHandle(STD_INPUT_HANDLE);
      pipe = !GetConsoleMode(inh, &dw);
      if (!pipe) {
         SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
         FlushConsoleInputBuffer(inh);
      }
   }
   if (pipe) {
      if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL))
         return true;
      return dw;
   } else {
      INPUT_RECORD rec[256];
      DWORD recCnt;
      DWORD i;

      // Count the number of unread input records, including keyboard,
      // mouse, and window-resizing input records.
      GetNumberOfConsoleInputEvents(inh, &dw);
      if (dw <= 0)
         return false;

      // Read data from console without removing it from the buffer
      if (!PeekConsoleInput(inh, rec, min(dw, 256), &recCnt))
         return false;

      // Search for at least one keyboard event
      for (i = 0; i < recCnt; i++)
         if (rec[i].EventType == KEY_EVENT)
            return true;

      return false;
   }

#elif defined UNIX
   struct timeval timeout;
   fd_set readfds;

   FD_ZERO(&readfds);
   FD_SET(fileno(stdin), &readfds);
   /* Set to timeout immediately */
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;
   select(16, &readfds, 0, 0, &timeout);

   return (FD_ISSET(fileno(stdin), &readfds));
#else
#warning  cannot determine keyboard output!
   return false;
#endif
}
