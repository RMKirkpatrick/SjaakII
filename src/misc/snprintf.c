/*  Sjaak, a program for playing chess
 *  Copyright (C) 2011, 2014, 2015  Evert Glebbeek
 *
 *  This source file written by Martin Sedlak, modified by Evert Glebbeek.
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
#	include "compilerdef.h"
#	include <stdint.h>
#	include <stdarg.h>
#	include <stdio.h>
#	if _MSC_VER < 1900
int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	int res;
	va_list ap;
	va_start(ap, fmt);
	res = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return res;
}
#	endif
#endif
