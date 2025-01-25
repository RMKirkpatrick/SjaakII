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
#ifndef BITS128_H
#define BITS128_H

#include <stdint.h>

#include "bits64.h"

#if defined __x86_64__
#define HAVE_UINT128_T 1
#endif

#if HAVE_UINT128_T
typedef __uint128_t uint128_t; 

static inline uint128_t u128(uint64_t u1, uint64_t u2)
{
   return (uint128_t)u2 << 64 | u1;
}

static inline bool onebit128(uint128_t x)
{
   return (x & (x-1)) == 0;
}

static inline int bitscan128(uint128_t x) {
   if (x & (uint128_t)0xFFFFFFFFFFFFFFFFll<<64)
      return 64 + bitscan64(x>>64);
   else
      return bitscan64(x);
}

static inline int lsb128(uint128_t x) {
   if ((x & (uint128_t)0xFFFFFFFFFFFFFFFFll) == 0)
      return 64 + lsb64(x>>64);
   else
      return lsb64(x);
}

static inline int msb128(uint128_t x) {
   if (x & (uint128_t)0xFFFFFFFFFFFFFFFFll<<64)
      return 64 + msb64(x>>64);
   else
      return msb64(x);
}

static inline int popcount128(uint128_t x)
{
   return popcount64(x >> 64) + popcount64(x);
}

static inline uint128_t shr128(uint128_t x, int bits)
{
   return x >> bits;
}

static inline uint128_t shl128(uint128_t x, int bits)
{
   return x << bits;
}

static inline uint128_t mul128(uint128_t x, uint128_t y)
{
   return x * y;
}

static inline uint128_t sshift128(uint128_t x, int s)
{
   if (s>0)
      return shl128(x, s);
   return shr128(x, -s);
#if 0
   signed char left  =   (signed char) s;
   signed char right = -((signed char)(s >> 8) & left);
   return shl128(shr128(x, right), right + left);
#endif
}

static inline bool test128(uint128_t x, int bit)
{
   return shr128(x, bit) & 0x01;
}

static inline bool is_zero128(uint128_t x)
{
   return x == 0;
}

static inline bool is_equal128(uint128_t x, uint128_t y)
{
   return x == y;
}

#else
#ifdef __cplusplus

struct uint128_t {
   uint64_t i64[2];

   uint128_t() { i64[0] = 0; i64[1] = 0; }
   uint128_t(int i) { i64[0] = i; i64[1] = 0; }
   uint128_t(uint64_t u1, uint64_t u2) { i64[0] = u1; i64[1] = u2; }

   inline uint128_t operator << (const int bits) const {
      uint64_t u1 = i64[0];
      uint64_t u2 = i64[1];

      if (bits >= 64) {
         u2 = u1 << (bits-64);
         u1 = 0;
      } else if (bits > 0) {
         uint64_t uu = u1 >> (64 - bits);
         u2 <<= bits;
         u1 <<= bits;
         u2 |= uu;
      }
      return uint128_t(u1, u2);
   }

   inline uint128_t operator >> (const int bits) const {
      uint64_t u1 = i64[0];
      uint64_t u2 = i64[1];

      if (bits >= 64) {
         u1 = u2 >> (bits-64);
         u2 = 0;
      } else if (bits > 0) {
         uint64_t uu = u2 << (64 - bits);
         u1 >>= bits;
         u2 >>= bits;
         u1 |= uu;
      }
      return uint128_t(u1, u2);
   }

   inline bool operator == (const uint128_t y) const {
      return (i64[0] == y.i64[0]) && (i64[1] == y.i64[1]);
   }

   inline bool operator != (const uint128_t y) const {
      return !(*this == y);
   }

   inline uint128_t operator |=(const uint128_t &x) {
      i64[0] |= x.i64[0];
      i64[1] |= x.i64[1];
      return *this;
   }
   inline uint128_t operator &=(const uint128_t &x) {
      i64[0] &= x.i64[0];
      i64[1] &= x.i64[1];
      return *this;
   }
   inline uint128_t operator ^=(const uint128_t &x) {
      i64[0] ^= x.i64[0];
      i64[1] ^= x.i64[1];
      return *this;
   }
   inline uint128_t operator +=(const uint128_t &x) {
      i64[0] += x.i64[0];
      i64[1] += x.i64[1];
      return *this;
   }
   inline uint128_t operator -=(const uint128_t &x) {
      i64[0] -= x.i64[0];
      i64[1] -= x.i64[1];
      return *this;
   }

   inline uint128_t operator ~ () const {
      return uint128_t(~i64[0], ~i64[1]);
   }

   inline uint128_t operator | (const uint128_t &x) const {
      return uint128_t(*this) |= x;
   }
   inline uint128_t operator & (const uint128_t &x) const {
      return uint128_t(*this) &= x;
   }
   inline uint128_t operator ^ (const uint128_t &x) const {
      return uint128_t(*this) ^= x;
   }
   inline uint128_t operator + (const uint128_t &x) const {
      return uint128_t(*this) += x;
   }
   inline uint128_t operator - (const uint128_t &x) const {
      return uint128_t(*this) -= x;
   }
};

inline uint128_t operator - (int lhs, const uint128_t &x) {
   return uint128_t(lhs) -= x;
}

inline uint128_t operator + (int lhs, const uint128_t &x) {
   return uint128_t(lhs) += x;
}

static inline uint128_t u128(uint64_t u1, uint64_t u2)
{
   return uint128_t(u1, u2);
}

static inline bool onebit128(uint128_t x)
{
   uint64_t u1 = x.i64[0];
   uint64_t u2 = x.i64[1];
   return onebit64(u1 ^ u2) && onebit64(u1 | u2);
}

static inline int bitscan128(uint128_t x) {
   if (x.i64[1])
      return 64 + bitscan64(x.i64[1]);
   else
      return bitscan64(x.i64[0]);
}

static inline int lsb128(uint128_t x) {
   if (x.i64[0] == 0)
      return 64 + lsb64(x.i64[1]);
   else
      return lsb64(x.i64[0]);
}

static inline int msb128(uint128_t x) {
   if (x.i64[1])
      return 64 + msb64(x.i64[1]);
   else
      return msb64(x.i64[0]);
}

static inline int popcount128(uint128_t x)
{
   return popcount64(x.i64[0]) + popcount64(x.i64[1]);
}

static inline uint128_t shr128(uint128_t x, int bits)
{
   return x >> bits;
}

static inline uint128_t shl128(uint128_t x, int bits)
{
   return x << bits;
}

static inline bool test128(uint128_t x, int bit)
{
   if (bit < 64)
      return (x.i64[0] >> bit) & 0x01;
   return (x.i64[1] >> (bit-64)) & 0x01;
}

static inline bool is_zero128(uint128_t x)
{
   uint64_t x1 = x.i64[0];
   uint64_t x2 = x.i64[1];
   return (x1 | x2) == 0;
}

static inline bool is_equal128(uint128_t x, uint128_t y)
{
   return x == y;
}

#else

typedef uint64_t uint128_t __attribute__ ((vector_size(sizeof(uint64_t)*2), aligned(8))); 

static inline uint128_t u128(uint64_t u1, uint64_t u2)
{
   return (uint128_t){u1, u2};
}

static inline bool onebit128(uint128_t x)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   uint64_t u1 = xx.i64[0];
   uint64_t u2 = xx.i64[1];
   return onebit64(u1 ^ u2) && onebit64(u1 | u2);
}

static inline int bitscan128(uint128_t x) {
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   if (xx.i64[1])
      return 64 + bitscan64(xx.i64[1]);
   else
      return bitscan64(xx.i64[0]);
}

static inline int lsb128(uint128_t x) {
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   if (xx.i64[0] == 0)
      return 64 + lsb64(xx.i64[1]);
   else
      return lsb64(xx.i64[0]);
}

static inline int msb128(uint128_t x) {
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   if (xx.i64[1])
      return 64 + msb64(xx.i64[1]);
   else
      return msb64(xx.i64[0]);
}

static inline int popcount128(uint128_t x)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   return popcount64(xx.i64[0]) + popcount64(xx.i64[1]);
}

static inline uint128_t shr128(uint128_t x, int bits)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t y;
   y.i128 = x;
   uint64_t u1 = y.i64[0];
   uint64_t u2 = y.i64[1];

   if (bits >= 64) {
      u1 = u2 >> (bits-64);
      u2 = 0;
   } else if (bits > 0) {
      uint64_t uu = u2 << (64 - bits);
      u1 >>= bits;
      u2 >>= bits;
      u1 |= uu;
   }
   return (uint128_t){u1, u2};
}

static inline uint128_t shl128(uint128_t x, int bits)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   uint64_t u1 = xx.i64[0];
   uint64_t u2 = xx.i64[1];

   if (bits >= 64) {
      u2 = u1 << (bits-64);
      u1 = 0;
   } else if (bits > 0) {
      uint64_t uu = u1 >> (64 - bits);
      u2 <<= bits;
      u1 <<= bits;
      u2 |= uu;
   }
   return (uint128_t){u1, u2};
}

static inline uint128_t mul128(uint128_t x, uint128_t y)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx, yy;
   xx.i128 = x;
   yy.i128 = y;
   uint64_t x1 = xx.i64[0];
   uint64_t x2 = xx.i64[1];
   uint64_t y1 = yy.i64[0];
   uint64_t y2 = yy.i64[1];
   uuint128_t r, q, w;

   /* Multiply upper word */
   w.i128 = u128(0, y1*x2 + x1*y2);

   /* Lower word, but we have to be careful about carries */
   q.i128 = u128(y1,0);
   r.i128 = u128(0,0);
   while (x1 > 0) {
      if (x1 & 0x1) {
         uint64_t r1 = r.i64[0];
         r.i128 += q.i128;

         /* Carry */
         r.i64[1] += (r.i64[0] < r1 || r.i64[0] < q.i64[0]);
      }
      int n = 1;
      x1 >>= 1;
      while (x1 && !(x1 & 0x1)) {x1>>=1; n++;}
      q.i128 = shl128(q.i128, n);
   }
   
   q.i128 = r.i128 + w.i128;
   if (q.i64[0] < r.i64[0] || q.i64[0] < w.i64[0]) q.i64[1]++;

   return q.i128;
}

static inline uint128_t sshift128(uint128_t x, int s)
{
   if (s>0)
      return shl128(x, s);
   return shr128(x, -s);
#if 0
   signed char left  =   (signed char) s;
   signed char right = -((signed char)(s >> 8) & left);
   return shl128(shr128(x, right), right + left);
#endif
}

static inline bool test128(uint128_t x, int bit)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   x = shr128(x, bit);
   uuint128_t xx; xx.i128 = x;
   uint64_t u1 = xx.i64[0];
   return u1 & 0x01;
}

static inline bool is_zero128(uint128_t x)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx; xx.i128 = x;
   uint64_t x1 = xx.i64[0];
   uint64_t x2 = xx.i64[1];
   return (x1 | x2) == 0;
}

static inline bool is_equal128(uint128_t x, uint128_t y)
{
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t xx, yy;
   xx.i128 = x;
   yy.i128 = y;
   uint64_t x1 = xx.i64[0];
   uint64_t x2 = xx.i64[1];
   uint64_t y1 = yy.i64[0];
   uint64_t y2 = yy.i64[1];

   return x1 == y1 && x2 == y2;
}
#endif

#endif

#endif
