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
#ifndef BITS64_H
#define BITS64_H

#include <stdint.h>
#include "bool.h"

#if defined _MSC_VER
#  include <intrin.h>
#  if !defined _M_AMD64 && !defined _M_X64
inline void _BitScanForward64(unsigned long *res, uint64_t b)
{
   unsigned __int32 l, h;
   l = uint32_t(b & 0xffffffffu);
   h = uint32_t(b >> 32);
   if (l) {
      _BitScanForward(res, l);
   } else {
      _BitScanForward(res, h);
      *res += 32;
   }
}

inline void _BitScanReverse64(unsigned long *res, uint64_t b)
{
   unsigned __int32 l, h;
   l = uint32_t(b & 0xffffffffu);
   h = uint32_t(b >> 32);
   if (h) {
      _BitScanReverse(res, h);
      *res += 32;
   } else {
      _BitScanReverse(res, l);
   }
}
#endif
#endif


static inline bool onebit64(uint64_t x)
{
   return (x & (x-1)) == 0;
}

static inline int bitscan64(uint64_t x) {
#ifdef __GNUC__
   return __builtin_ctzll (x);
#elif defined _MSC_VER
   unsigned long res;
   _BitScanForward64(&res, x);
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

static inline int lsb64(uint64_t x) {
#ifdef __GNUC__
   return __builtin_ctzll (x);
#elif defined _MSC_VER
	unsigned long res;
	_BitScanForward64(&res, x);
	return (int)res;
#else
   int n = 0;

   assert(x);
   while ((x&((uint64_t)1<<n)) == 0) n++;

   return n;
#endif
}

static inline int msb64(uint64_t x) {
#ifdef __GNUC__
   return 63 - __builtin_clzll (x);
#elif defined _MSC_VER
	unsigned long res;
	_BitScanReverse64(&res, x);
	return (int)res;
#else
   int n = 63;

   assert(x);
   while ((x&((uint64_t)1<<n)) == 0) n--;

   return n;
#endif
}

static inline uint64_t sshift64(uint64_t x, int s)
{
   signed char left  =   (signed char) s;
   signed char right = -((signed char)(s >> 8) & left);
   return (x >> right) << (right + left);
}


static inline int bitscan16(uint16_t x) {
#ifdef __GNUC__
   return __builtin_ctz (x);
#elif defined _MSC_VER
   unsigned long res;
   _BitScanForward(&res, x);
   return res;
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
static inline int popcount64(uint64_t x)
{
#ifdef __GNUC__
    return __builtin_popcountll(x);
#else
    const uint64_t k1 = 0x5555555555555555ll;
    const uint64_t k2 = 0x3333333333333333ll;
    const uint64_t k4 = 0x0f0f0f0f0f0f0f0fll;
    const uint64_t kf = 0x0101010101010101ll;
    x =  x       - ((x >> 1)  & k1);
    x = (x & k2) + ((x >> 2)  & k2);
    x = (x       +  (x >> 4)) & k4;
    x = (x * kf) >> 56;
    return (int) x;
#endif
}
#endif
