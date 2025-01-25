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
#ifndef BITBOARD_H
#define BITBOARD_H
#include <algorithm>
#include <cstring>
#include "assert.h"
#include "bits.h"
#include "squares.h"

template<typename kind>
class bitboard_t {
   public:
      kind bb;

   public:
      static int board_files, board_ranks;
      static uint32_t rank_mask;
      static uint32_t file_mask;
      static uint8_t diagonal_nr[sizeof(kind)*8];
      static uint8_t anti_diagonal_nr[sizeof(kind)*8];

      static bitboard_t<kind> king_zone[2][sizeof(kind)*8];
      static bitboard_t<kind> neighbour_board[sizeof(kind)*8];
      static bitboard_t<kind> square_bitboards[sizeof(kind)*8];
      static bitboard_t<kind> board_empty;
      static bitboard_t<kind> board_all;
      static bitboard_t<kind> board_edge;
      static bitboard_t<kind> board_corner;
      static bitboard_t<kind> board_east_edge;
      static bitboard_t<kind> board_west_edge;
      static bitboard_t<kind> board_north_edge;
      static bitboard_t<kind> board_south_edge;
      static bitboard_t<kind> board_light;
      static bitboard_t<kind> board_dark;
      static bitboard_t<kind> board_centre;
      static bitboard_t<kind> board_xcentre;
      static bitboard_t<kind> board_xxcentre;
      static bitboard_t<kind> board_rank[16];
      static bitboard_t<kind> board_file[16];
      static bitboard_t<kind> board_file_mask;
      static bitboard_t<kind> board_south;
      static bitboard_t<kind> board_north;
      static bitboard_t<kind> board_northward[16];
      static bitboard_t<kind> board_southward[16];
      static bitboard_t<kind> board_eastward[16];
      static bitboard_t<kind> board_westward[16];
      static bitboard_t<kind> board_homeland[2];

      static bitboard_t<kind> board_diagonal[32];
      static bitboard_t<kind> board_antidiagonal[32];

      static bitboard_t<kind> board_between[sizeof(kind)*8][sizeof(kind)*8];

      static void initialise_bitboards(int files, int ranks);

      bitboard_t() : bb(0) { }
      bitboard_t(const kind &b) : bb(b) { }

      kind value() { return bb; }

      bitboard_t<kind> operator = (const bitboard_t<kind>& b) {
         bb = b.bb;
         return *this;
      }

      /* Default function definitions, should work with any normal integer
       * type, but can be overridden if a more optimal solution is
       * possible.
       */
      void clear() { bb ^= bb; };
      bool onebit() const {
         return (bb & (bb-1)) == 0;
      }
      bool twobit() const {
         if (onebit()) return false;
         kind b = bb & (bb-1);
         return (b & (b-1)) == 0;
      }
      int popcount() const {
         kind x = bb;
         int count = 0;
         while (x) {
            count++;
            x &= x-1;
         }
         return count;
      }
      int bitscan() const {
         kind x = bb;
         int i = 0;
         while (x && !(x&1)) {
            i++;
            x>>=1;
         }
         return i;
      }
      int msb() const {
         kind x = bb;
         int n = 8*sizeof(kind)-1;

         assert(x);
         while ((x&((kind)1<<n)) == 0) n--;

         return n;
      }
      int lsb() const {
         kind x = bb;
         int n = 0;

         assert(x);
         while ((x&((kind)1<<n)) == 0) n++;

         return n;
      }
      inline void set(int bit) {
         assert(bit < 8*sizeof(kind));
         bb |= (((kind)1) << bit);
      }
      inline void reset(int bit) {
         assert(bit < 8*sizeof(kind));
         bb &= ~(((kind)1) << bit);
      }
      bool test(int bit) const {
         assert(bit < 8*sizeof(kind));
         return (bb & (((kind)1) << bit)) != 0;
      }
      bitboard_t<kind> sshift(int bits) const {
         if (bits>0)
            return bitboard_t<kind>(bb << bits);
         return bitboard_t<kind>(bb >> (-bits));
      }
      inline bool is_empty() const { return bb == 0; }

      /* Default definitions of operators */
      inline bool operator == (const bitboard_t<kind>& b2) const {
         return bb == b2.bb;
      }

      inline bool operator != (const bitboard_t<kind>& b2) const {
         return !(*this == b2);
      }

      inline bitboard_t<kind> operator << (const int bits) const {
         return bitboard_t<kind>(bb << bits);
      }
      inline bitboard_t<kind> operator >> (const int bits) const {
         return bitboard_t<kind>(bb >> bits);
      }

      inline bitboard_t<kind> operator ~() const {
         return bitboard_t<kind>(~bb);
      }
      inline bitboard_t<kind> operator |(const bitboard_t<kind> &b2) const {
         return bitboard_t<kind>(bb | b2.bb);
      }
      inline bitboard_t<kind> operator &(const bitboard_t<kind> &b2) const {
         return bitboard_t<kind>(bb & b2.bb);
      }
      inline bitboard_t<kind> operator ^(const bitboard_t<kind> &b2) const {
         return bitboard_t<kind>(bb ^ b2.bb);
      }

      inline bitboard_t<kind>& operator |=(const bitboard_t<kind> &b2) {
         bb |= b2.bb;
         return *this;
      }
      inline bitboard_t<kind>& operator &=(const bitboard_t<kind> &b2) {
         bb &= b2.bb;
         return *this;
      }
      inline bitboard_t<kind>& operator ^=(const bitboard_t<kind> &b2) {
         bb ^= b2.bb;
         return *this;
      }

      inline uint32_t get_rank(int rank) const {
         assert(rank < board_ranks);
         bitboard_t<kind> b = (*this) >> (rank * board_files);
         return (uint32_t)(b.bb & rank_mask);
      }

      inline uint32_t get_file(int file) const {
#if 0
         uint32_t file_bits = 0;
         int n;

         int bit = file;
         for (n=0; n<board_ranks; n++) {
            if (test(bit)) file_bits |= (1<<n);
            bit += board_files;
         }
         return file_bits;
#else
         int shift = 1;
         bitboard_t<kind> b(bb);
         b = (b >> file) & board_file_mask;

         do {
            b |= (b >> (shift * board_files)) << shift;
            shift <<= 1;
            //b |= (b >> board_files) << 1;
            //shift++;
         } while (shift < board_ranks);
         return (uint32_t)(b.bb & file_mask);
#endif
      }

      bitboard_t<kind> fill_north() const {
         int shift = 1;
         bitboard_t<kind> b(bb);

         do {
            b |= (b << (shift * board_files));
            shift <<= 1;
         } while (shift < board_ranks);

         return b;
      }

      inline bitboard_t<kind> fill_south() const {
         int shift = 1;
         bitboard_t<kind> b(bb);

         do {
            b |= (b >> (shift * board_files));
            shift <<= 1;
         } while (shift < board_ranks);

         return b;
      }

      char *rank_string(int rank, char *buffer = NULL) const {
         static char static_buffer[256];
         if (buffer == NULL) buffer = static_buffer;
         char *s = buffer;

         uint32_t bits = get_rank(rank);
         for (int bit = 0; bit < board_files; bit++) {
            if (bits & (1 << bit))
               *s = '1';
            else
               *s = '.';
            s++;
         }
         *s = '\0';

         return buffer;
      }

      void print(const char *msg = NULL) const {
         for (int n = 0; n<board_ranks; n++)
            printf("%s\n", rank_string(board_ranks-n-1));
         if (msg) printf("%s", msg);
      }

      static inline int pack_rank_file(int rank, int file)
      {
         return file + (rank * board_files);
      }
};

template<typename kind> int bitboard_t<kind>::board_files;
template<typename kind> int bitboard_t<kind>::board_ranks;
template<typename kind> uint32_t bitboard_t<kind>::rank_mask;
template<typename kind> uint32_t bitboard_t<kind>::file_mask;
template<typename kind> uint8_t bitboard_t<kind>::diagonal_nr[sizeof(kind)*8];
template<typename kind> uint8_t bitboard_t<kind>::anti_diagonal_nr[sizeof(kind)*8];

template<typename kind> bitboard_t<kind> bitboard_t<kind>::king_zone[2][sizeof(kind)*8];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::neighbour_board[sizeof(kind)*8];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::square_bitboards[sizeof(kind)*8];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_empty;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_all;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_edge;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_corner;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_east_edge;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_west_edge;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_north_edge;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_south_edge;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_light;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_dark;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_centre;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_xcentre;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_xxcentre;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_rank[16];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_file[16];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_file_mask;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_south;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_north;
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_northward[16];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_southward[16];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_eastward[16];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_westward[16];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_homeland[2];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_diagonal[32];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_antidiagonal[32];
template<typename kind> bitboard_t<kind> bitboard_t<kind>::board_between[sizeof(kind)*8][sizeof(kind)*8];

template<typename kind>
inline void bitboard_t<kind>::initialise_bitboards(int files, int ranks)
{
   board_ranks = ranks;
   board_files = files;

   assert(ranks * files <= 8*sizeof(kind));

   rank_mask = (1<<files)-1;
   file_mask = (1<<ranks)-1;

   int size = ranks * files;
   int n;

   for (n=0; n<size; n++) {
      square_bitboards[n].clear();
      square_bitboards[n].set(n);
   }
   board_empty.clear();
   board_all.clear();
   board_edge.clear();
   board_corner.clear();
   board_east_edge.clear();
   board_west_edge.clear();
   board_north_edge.clear();
   board_south_edge.clear();
   board_light.clear();
   board_dark.clear();
   board_south.clear();
   board_north.clear();

   for (n=0; n<16; n++) {
      board_rank[n].clear();
      board_file[n].clear();
      board_northward[n].clear();
      board_southward[n].clear();
      board_eastward[n].clear();
      board_westward[n].clear();
   }

   memset(board_diagonal, 0, sizeof board_diagonal);
   memset(board_antidiagonal, 0, sizeof board_antidiagonal);

   int bb_size = sizeof(kind)*8;
   for (n=0; n<bb_size; n++) {
      int f = n % files;
      int r = n / files;
      board_rank[r].set(n);
      board_file[f].set(n);
      if ((f^r)&1)
         board_light |= square_bitboards[n];
      else
         board_dark |= square_bitboards[n];

      if (f == 0)
         board_west_edge |= square_bitboards[n];

      if (f == (files-1))
         board_east_edge |= square_bitboards[n];

      if (r == 0)
         board_south_edge |= square_bitboards[n];

      if ((r == (ranks-1)) || n > size)
         board_north_edge |= square_bitboards[n];
   }
   board_edge = board_south_edge |
                board_north_edge |
                board_east_edge |
                board_west_edge;
   board_corner = (board_south_edge | board_north_edge) & (board_east_edge | board_west_edge);

   for (n=0; n<size; n++)
      board_all |= square_bitboards[n];

   /* Neighbourhoods */
   for (n=0; n<size; n++) {
      neighbour_board[n] = square_bitboards[n] | (square_bitboards[n] << files) | (square_bitboards[n] >> files);
      neighbour_board[n] |= (neighbour_board[n] & ~board_west_edge) >> 1;
      neighbour_board[n] |= (neighbour_board[n] & ~board_east_edge) << 1;

      king_zone[0][n] = king_zone[1][n] = neighbour_board[n];
      king_zone[0][n] |= neighbour_board[n] << files;
      king_zone[1][n] |= neighbour_board[n] >> files;

      neighbour_board[n] &= ~square_bitboards[n];
      king_zone[0][n]    &= ~square_bitboards[n];
      king_zone[1][n]    &= ~square_bitboards[n];
   }

   /* Set up diagonals and anti-diagonals
    * For the mapping of diagonal numbers, we pretend the board is square; it may not be, but this is easiest.
    */
   int s = files;
   if (ranks > files) s = ranks;
   for (n = 0; n<32; n++) {
      diagonal_nr[n] = 255;
      anti_diagonal_nr[n] = 255;
   }
   for (n = 0; n<size; n++) {
      int f = n % files;
      int r = n / files;
      diagonal_nr[n] = r + (s-1) - f;
      anti_diagonal_nr[n] = r + f;

      board_diagonal[diagonal_nr[n]] |= square_bitboards[n];
      board_antidiagonal[anti_diagonal_nr[n]] |= square_bitboards[n];
   }

   /* North/south bitboards */
   bitboard_t<kind>::board_south = bitboard_t<kind>::board_north = bitboard_t<kind>::board_empty;
   for (n=0; n<ranks/2; n++)
      board_south |= board_rank[n];
   for (; n<ranks; n++)
      board_north |= board_rank[n];

   board_homeland[0] = board_south;
   board_homeland[1] = board_north;

   /* Northward/southward bitboards */
   for (n=0; n<ranks; n++) {
      int c;
      for (c=0; c<n; c++)       board_southward[n] |= board_rank[c];
      for (c=n+1; c<ranks; c++) board_northward[n] |= board_rank[c];
   }

   /* Eastward/westward bitboards */
   for (n=0; n<files; n++) {
      int c;
      for (c=0; c<n; c++)       board_westward[n] |= board_file[c];
      for (c=n+1; c<files; c++) board_eastward[n] |= board_file[c];
   }

   /* Centre and extended centre */
   bitboard_t<kind> centre_files, centre_ranks;
   for (int f = files/2 - 1; f<files/2+1+(files&1); f++)
      centre_files |= board_file[f];
   for (int r = ranks/2 - 1; r<ranks/2+1+(ranks&1); r++)
      centre_ranks |= board_rank[r];
   board_centre = centre_files & centre_ranks;

   for (int f = std::max(0, files/2 - 2); f<std::min(files/2+2+(files&1), 16); f++)
      centre_files |= board_file[f];
   for (int r = std::max(0, ranks/2 - 2); r<std::min(ranks/2+2+(ranks&1), 16); r++)
      centre_ranks |= board_rank[r];
   board_xcentre = centre_files & centre_ranks;

   board_xxcentre = board_all & ~(board_xcentre | board_edge);
   board_xcentre &= ~board_centre;

   /* Connecting rays */
   memset(board_between, 0, sizeof board_between);
   int board_size = board_files * board_ranks;
   for (int square = 0; square < board_size; square++) {
      int attack;
      for (attack = square+1; attack<board_size; attack++) {
         int rank = unpack_rank(square);
         int file = unpack_file(square);
         if (rank == unpack_rank(attack)) {
            for (int n=file;n<=unpack_file(attack);n++)
               board_between[square][attack] |= square_bitboards[pack_rank_file(rank, n)];
         }
         if (file == unpack_file(attack)) {
            for (int n=rank;n<=unpack_rank(attack);n++)
               board_between[square][attack] |= square_bitboards[pack_rank_file(n, file)];
         }
         if (diagonal_nr[square] == diagonal_nr[attack]) {
            for (int n=square;n<=(attack);n+=board_files+1)
               board_between[square][attack] |= square_bitboards[n];
         }
         if (bitboard_t<kind>::anti_diagonal_nr[square] == anti_diagonal_nr[attack]) {
            for (int n=square;n<=(attack);n+=board_files-1)
               board_between[square][attack] |= square_bitboards[n];
         }
         board_between[square][attack].reset(square);
         board_between[square][attack].reset(attack);
         board_between[attack][square] = board_between[square][attack];
      }
   }
   board_file_mask = board_file[0];
}

/* Specialisation for 32 bits: use optimised functions */
template<> inline int bitboard_t<uint32_t>::popcount() const { return popcount32(bb); }
template<> inline int bitboard_t<uint32_t>::bitscan() const { return bitscan32(bb); }
template<> inline int bitboard_t<uint32_t>::lsb() const { return lsb32(bb); }
template<> inline int bitboard_t<uint32_t>::msb() const { return msb32(bb); }

/* Specialisation for 64 bits: use optimised functions */
template<> inline int bitboard_t<uint64_t>::popcount() const { return popcount64(bb); }
template<> inline int bitboard_t<uint64_t>::bitscan() const { return bitscan64(bb); }
template<> inline int bitboard_t<uint64_t>::lsb() const { return lsb64(bb); }
template<> inline int bitboard_t<uint64_t>::msb() const { return msb64(bb); }

template<> inline int bitboard_t<uint128_t>::popcount() const { return popcount128(bb); }
template<> inline int bitboard_t<uint128_t>::bitscan() const { return bitscan128(bb); }
template<> inline int bitboard_t<uint128_t>::lsb() const { return lsb128(bb); }
template<> inline int bitboard_t<uint128_t>::msb() const { return msb128(bb); }
template<> inline bool bitboard_t<uint128_t>::onebit() const { return onebit128(bb); }
template<> inline bitboard_t<uint128_t> bitboard_t<uint128_t>::operator << (const int bits) const {
   return bitboard_t<uint128_t>(shl128(bb, bits));
}
template<> inline bitboard_t<uint128_t> bitboard_t<uint128_t>::operator >> (const int bits) const {
   return bitboard_t<uint128_t>(shr128(bb, bits));
}
template<> inline bool bitboard_t<uint128_t>::test(int bit) const {
   return test128(bb, bit);
}
template<> inline bool bitboard_t<uint128_t>::is_empty() const {
   return is_zero128(bb);
}
#ifndef HAVE_UINT128_T
template<> inline uint32_t bitboard_t<uint128_t>::get_rank(int rank) const {
   uint128_t b;
   b = shr128(bb, rank * board_files);
   return b.i64[0] & rank_mask;
}
template<> inline uint32_t bitboard_t<uint128_t>::get_file(int file) const {
   uint32_t file_bits = 0;
   int n;

   int bit = file;
   for (n=0; n<board_ranks; n++) {
      if (test(bit)) file_bits |= (1<<n);
      bit += board_files;
   }
   return file_bits;
}

#endif

#endif
