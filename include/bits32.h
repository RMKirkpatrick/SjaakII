/*  Sjaak, a program for playing chess variants
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
#ifndef BITS32_H
#define BITS32_H

#include <stdint.h>
#include "bool.h"

#if defined _MSC_VER
#  include <intrin.h>
#endif

static inline bool onebit32(uint32_t x)
{
   return (x & (x-1)) == 0;
}

static inline int lsb32(uint32_t x) {
#ifdef __GNUC__
   return __builtin_ctz (x);
#elif defined _MSC_VER
	unsigned long res;
	_BitScanForward(&res, x);
	return (int)res;
#else
   int n = 0;

   assert(x);
   while ((x&((uint32_t)1<<n)) == 0) n++;

   return n;
#endif
}

static inline int msb32(uint32_t x) {
#ifdef __GNUC__
   return 31 - __builtin_clz(x);
#elif defined _MSC_VER
	unsigned long res;
	_BitScanReverse(&res, x);
	return (int)res;
#else
   int n = 31;

   assert(x);
   while ((x&((uint32_t)1<<n)) == 0) n--;

   return n;
#endif
}

static inline uint32_t sshift32(uint32_t x, int s)
{
   signed char left  =   (signed char) s;
   signed char right = -((signed char)(s >> 8) & left);
   return (x >> right) << (right + left);
}

static inline int bitscan32(uint32_t x) {
#ifdef __GNUC__
   return __builtin_ctz (x);
#elif defined _MSC_VER
	unsigned long res;
	_BitScanForward(&res, x);
	return (int)res;
#else
   int i = 0;
   assert(x);
   while (!(x & 1)) {
	   i++;
	   x >>= 1;
   }
   return i;
#endif
}

/* Return the number of bits set on a bitboard
 * From http://chessprogramming.wikispaces.com/Population+Count
 */
static inline int popcount32(uint32_t x)
{
#ifdef __GNUC__
    return __builtin_popcount(x);
#else
    const uint32_t k1 = 0x55555555;
    const uint32_t k2 = 0x33333333;
    const uint32_t k4 = 0x0f0f0f0f;
    const uint32_t kf = 0x01010101;
    x =  x       - ((x >> 1)  & k1);
    x = (x & k2) + ((x >> 2)  & k2);
    x = (x       +  (x >> 4)) & k4;
    x = (x * kf) >> 24;
    return (int) x;
#endif
}
#endif

