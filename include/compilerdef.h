/*  Sjaak, a program for playing chess
 *  Copyright (C) 2011, 2014, 2015  Evert Glebbeek
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
#ifndef COMPILERDEF_H
#define COMPILERDEF_H

/* Various compiler-specific definitions and declarations. Mainly to deal
 * with differences between GNU and Microsoft compiler families.
 */


#if defined _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#  define _CRT_NONSTDC_NO_WARNINGS
#  if !defined _HAS_EXCEPTIONS
#    define _HAS_EXCEPTIONS 0
#  endif

#  define expect(x,y) (x)
#	define prefetch(x) (void)0

#  define ATTRIBUTE_FORMAT_PRINTF
#  define ATTRIBUTE_ALIGNED(x)
#  define ATTRIBUTE_UNUSED

#  define PRIu64 "I64u"

#  define strdup _strdup

#   if _MSC_VER < 1900
#	  if defined __cplusplus
extern "C" {
#	  endif
int snprintf(char *buf, size_t size, const char *fmt, ...);
#	  if defined __cplusplus
}
#	  endif
#endif

#else
/* Assume GNU */

#  include <inttypes.h>

#  define expect(x,y) __builtin_expect((x), (y))
#	define prefetch(x) __builtin_prefetch(x)

#  define ATTRIBUTE_FORMAT_PRINTF __attribute__((format(printf, 1, 2)))
#  define ATTRIBUTE_ALIGNED(x)    __attribute__((aligned(x)))
#  define ATTRIBUTE_UNUSED        __attribute__((unused))

#endif

#endif
