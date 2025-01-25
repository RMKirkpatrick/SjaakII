/*  Sjaak, a program for playing chess variants
 *  Copyright (C) 2016  Evert Glebbeek
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
#include "ansi.h"

#if defined _WIN32 || defined _WIN64
#undef DATADIR
#include "compilerdef.h"
#include <windows.h>
#include <stdio.h>
void ansi_code(const char *s)
{
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   WORD attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
   int ansi_code = 0;
   CONSOLE_SCREEN_BUFFER_INFO   csbi;

   if (GetConsoleScreenBufferInfo(hConsole, &csbi))
      attr = csbi.wAttributes;
   
   sscanf(s + 2, "%d", &ansi_code);
   switch (ansi_code) {
      case 0:
         attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
         break;
      case 1:
         attr |= FOREGROUND_INTENSITY;
         break;
      case 30:
         attr &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
         break;
      case 37:
         attr |=  (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
         break;
      case 44:
         attr &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
         attr |= BACKGROUND_BLUE;
         break;
      case 45:
         attr &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
         attr |= BACKGROUND_BLUE | BACKGROUND_RED;
         break;
      case 46:
         attr &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
         attr |= BACKGROUND_BLUE | BACKGROUND_GREEN;
         break;
      default:
         break;
   }

   SetConsoleTextAttribute(hConsole, attr);
}
#else
#include <stdio.h>
void ansi_code(const char *s)
{
   printf("%s", s);
}
#endif

