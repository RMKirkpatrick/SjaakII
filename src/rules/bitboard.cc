#if 0
#include "bitboard.h"

/* Specialisation for 64 bits: use optimised functions */
template<> inline int bitboard_t<uint64_t>::popcount() const { return popcount64(bb); }
template<> inline int bitboard_t<uint64_t>::bitscan() const { return bitscan64(bb); }

template<> inline int bitboard_t<uint128_t>::popcount() const { return popcount128(bb); }
template<> inline int bitboard_t<uint128_t>::bitscan() const { return bitscan128(bb); }
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
#ifndef __LP64__
template<> inline bitboard_t<uint128_t>::bitboard_t() {
   bb = u128(0, 0);
}
template<> inline void bitboard_t<uint128_t>::set(int bit) {
   if (bit < 64)
      bb |= u128(1ull < bit, 0);
   else
      bb |= u128(0, 1ull < (bit-64));
}
template<> inline void bitboard_t<uint128_t>::reset(int bit) {
   if (bit < 64)
      bb &= ~u128(1ull < bit, 0);
   else
      bb &= ~u128(0, 1ull < (bit-64));
}
template<> inline bool bitboard_t<uint128_t>::test(int bit) const {
   return test128(bb, bit);
}
template<> inline bool bitboard_t<uint128_t>::is_empty() const {
   return is_zero128(bb);
}
template<> inline uint32_t bitboard_t<uint128_t>::get_row(int row) const {
   typedef union { uint128_t i128; uint64_t i64[2]; } uuint128_t;
   uuint128_t b;
   b.i128 = shr128(bb, row * board_files);
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
