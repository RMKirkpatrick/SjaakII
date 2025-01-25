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
#ifndef MOVE_H
#define MOVE_H

#include <stdint.h>
#include "assert.h"
#include "pieces.h"
#include "bool.h"

/* Define structure to hold a move.
 * Because of the specific requirements of variants or other games, this has to be a fairly flexible data
 * structure. Each move can consist of (in that order):
 *  N pickups (N = 0, 1, 2, 3)
 *  N swaps   (N = 0, 1, 2, 3) 
 *  N drops   (N = 0, 1, 2, 3)
 *  N store/retrieve (N = 0, 1)
 * A "pickup" stores a square (7 bits).
 * A "swap" stores the from - and to-square (7+7 = 14 bits).
 * A "drop" stores a square and a piece to be dropped there (7+5+1 = 13 bits).
 * A "store" stores a piece in the holdings (1+5+1 = 7 bits), a "retrieve" does the opposite
 * The extra bits ("4+1") for each piece are used to encode the colour of the piece.
 *
 * The order in which the components should be resolved is:
 *  1. pickups (so all destination squares are empty)
 *  2. swaps (which move pieces on the board)
 *  3. drops (which may happen on the square that was just cleared by the move)
 *  4. store/retrieve (could be any order really)
 *
 * Normal moves then consist of one swap (14 bits)
 * Captures consist of a pickup and a swap (7+14 = 21 bits)
 * Promotions consist of one (or two, for captures) pickup(s) and a drop (7(+7)+13 = 20(27) bits)
 * Castling consists of two swaps (14+14=28 bits)
 * Gating moves add an extra drop:
 * A castle+gate in this case takes up 28+13 = 41 bits (48 including retrieve)
 * A capture+gate takes 21+13 = 34 bits (41 including retrieve)
 *
 * For printing of SAN moves alone it is convenient to have the "primary" piece
 * type as well (4+1 bits).
 *  bits
 *   0-1  Number of pickups (2 bits)
 *   2-3  Number of "swaps" (2 bits)
 *   4    Number of pieces taken/from holdings (0 or 1, 1 bit)
 *   5-6  Number of drops (2 bits)
 *   7-11 Primary piece (5 bits)
 *  12-59 Move actions (48 bits)
 *  60-63 Move flags (4 bits)
 * We even have room to expand the number of piece types to 32, which makes
 * implementing variants with many pieces easier (or possible at all).
 */
typedef uint64_t move_t;

/* Offsets and test masks */
#define MOVE_SIDE_BITS        1
#define MOVE_PIECE_BITS       (PIECE_BITS)
#define MOVE_SQUARE_BITS      7

#define MOVE_PICKUP_SIZE      (MOVE_SQUARE_BITS)
#define MOVE_DROP_SIZE        (MOVE_SQUARE_BITS + MOVE_PIECE_BITS + MOVE_SIDE_BITS)
#define MOVE_SWAP_SIZE        (MOVE_SQUARE_BITS + MOVE_SQUARE_BITS)

#define MOVE_SQUARE_MASK      ((1<<MOVE_SQUARE_BITS)-1)
#define MOVE_PIECE_MASK       ((1<<MOVE_PIECE_BITS)-1)
#define MOVE_DROP_MASK        ((1<<MOVE_DROP_SIZE)-1)
#define MOVE_PICKUP_MASK      ((1<<MOVE_PICKUP_SIZE)-1)
#define MOVE_HOLDING_MASK     ((1<<(MOVE_PIECE_BITS + MOVE_SIDE_BITS + 1))-1)
#define MOVE_SWAP_MASK        ((1<<MOVE_SWAP_SIZE)-1)
#define MOVE_SIDE_MASK        0x0001

#define MOVE_SET_ENPASSANT    0x8000000000000000ll
#define MOVE_KEEP_TURN        0x4000000000000000ll
#define MOVE_RESET50          0x2000000000000000ll

#define DROP_SQUARE_SHIFT     (MOVE_PIECE_BITS+MOVE_SIDE_BITS)

#define MOVE_PICK_SHIFT       0
#define MOVE_SWAP_SHIFT       2
#define MOVE_DROP_SHIFT       5
#define MOVE_HOLD_SHIFT       4
#define MOVE_PIECE_SHIFT      7

#define MOVE_HEADER_SIZE      (MOVE_PIECE_SHIFT + MOVE_PIECE_BITS + MOVE_SIDE_BITS)

#define MOVE_SLOT1            (MOVE_HEADER_SIZE)
#define MOVE_SLOT2            (MOVE_SLOT1 + MOVE_PICKUP_SIZE)
#define MOVE_SLOT3            (MOVE_SLOT2 + MOVE_PICKUP_SIZE)

extern char piece_symbol_string[MAX_PIECE_TYPES+1];
extern char piece_psymbol_string[MAX_PIECE_TYPES+1];
extern char piece_drop_string[MAX_PIECE_TYPES+1];

#ifdef __cplusplus
const char *move_to_string(move_t move, char *buffer = NULL);
const char *move_to_lan_string(move_t move, bool castle_san = true, bool castle_kxr = true, char *buffer = NULL);
#endif

/**********************************************************************
 *                      static inline functions                       *
 **********************************************************************/
static inline int get_move_pickups(move_t move)
{
   return (move >> MOVE_PICK_SHIFT) & 0x03;
}

static inline int get_move_drops(move_t move)
{
   return (move >> MOVE_DROP_SHIFT) & 0x03;
}

static inline int get_move_swaps(move_t move)
{
   return (move >> MOVE_SWAP_SHIFT) & 0x03;
}

static inline int get_move_holdings(move_t move)
{
   return (move >> MOVE_HOLD_SHIFT) & 0x01;
}



static inline move_t encode_number_pickups(int count)
{
   assert(count <= 3);
   return count << MOVE_PICK_SHIFT;
}

static inline move_t encode_number_drops(int count)
{
   assert(count <= 3);
   return count << MOVE_DROP_SHIFT;
}

static inline move_t encode_number_swaps(int count)
{
   assert(count <= 3);
   return count << MOVE_SWAP_SHIFT;
}

static inline move_t encode_number_holdings(int count)
{
   assert(count <= 1);
   return count << MOVE_HOLD_SHIFT;
}

static inline move_t encode_move_piece(int piece)
{
   return (piece << MOVE_PIECE_SHIFT);
}

/* Get the first, second or third pickup */
static inline int get_move_pickup(move_t move, int n)
{
   int shift;
   assert (n<=get_move_pickups(move));
   shift = n * MOVE_PICKUP_SIZE;

   return (move >> (MOVE_SLOT1 + shift)) & MOVE_PICKUP_MASK;
}

/* Get the first, second or third drop */
static inline int get_move_drop(move_t move, int n)
{
   int shift;
   assert (n<=get_move_drops(move));
   shift = MOVE_DROP_SIZE*n + MOVE_SWAP_SIZE*get_move_swaps(move) + MOVE_PICKUP_SIZE*get_move_pickups(move);

   return (move >> (MOVE_SLOT1 + shift)) & MOVE_DROP_MASK;
}

static inline int get_move_swap(move_t move, int n)
{
   int shift;
   assert (n<=get_move_swaps(move));
   shift = MOVE_SWAP_SIZE*n + MOVE_PICKUP_SIZE*get_move_pickups(move);

   return (move >> (MOVE_SLOT1 + shift)) & MOVE_SWAP_MASK;
}

static inline int get_move_holding(move_t move)
{
   int shift = MOVE_SWAP_SIZE*get_move_swaps(move) + MOVE_DROP_SIZE*get_move_drops(move) + MOVE_PICKUP_SIZE*get_move_pickups(move);
   return (move >> (MOVE_SLOT1 + shift)) & MOVE_HOLDING_MASK;
}

static inline int decode_swap_from(int swap)
{
   return swap & MOVE_SQUARE_MASK;
}

static inline int decode_swap_to(int swap)
{
   return (swap >> MOVE_SQUARE_BITS) & MOVE_SQUARE_MASK;
}

/* Decode drop and pickup piece/squares */
static inline int decode_drop_square(int drop)
{
   return (drop >> DROP_SQUARE_SHIFT) & MOVE_SQUARE_MASK;
}

static inline int decode_drop_piece(int drop)
{
   return drop & MOVE_PIECE_MASK;
}

static inline side_t decode_drop_side(int drop)
{
   return side_for_piece(drop);
}

static inline int decode_pickup_square(int pickup)
{
   return pickup;
}

/* The code for holdings is almost the same as for drops */
static inline int decode_holding_count(int hold)
{
   return 2*decode_drop_square(hold) - 1;
}
#define decode_holding_piece decode_drop_piece
#define decode_holding_side decode_drop_side

static inline uint64_t encode_swap(int from, int to)
{
   return from | to << MOVE_SQUARE_BITS;
}

static inline uint64_t encode_pickup(int from)
{
   return from & MOVE_SQUARE_MASK;
}

static inline uint64_t encode_drop(int piece, int to)
{
   return piece | to<<DROP_SQUARE_SHIFT;
}

static inline uint64_t encode_holding(int piece, int count)
{
   return piece | count<<DROP_SQUARE_SHIFT;
}

/* Encode a normal (non-capture) move */
static inline move_t encode_normal_move(uint8_t piece, uint8_t from, uint8_t to)
{
   return encode_number_swaps(1)   |
          encode_move_piece(piece) |
          encode_swap(from, to) << MOVE_SLOT1;
}

/* Encode an en-passant capture, where the pickup square is different from the destination square */
static inline move_t encode_en_passant_capture(uint8_t piece, uint8_t from, uint8_t to, uint8_t ep)
{
   int swap_shift = MOVE_PICKUP_SIZE;

   return encode_number_pickups(1) |
          encode_number_swaps(1)   |
          encode_move_piece(piece) |
          encode_pickup(ep)         <<  MOVE_SLOT1 |
          encode_swap  (from,   to) << (MOVE_SLOT1 + swap_shift) | MOVE_RESET50;
}

/* Encode a normal capture */
static inline move_t encode_normal_capture(uint8_t piece, uint8_t from, uint8_t to)
{
   return encode_en_passant_capture(piece, from, to, to);
}

/* Encode an en-passant capture, where the pickup square is different from the destination square */
static inline move_t encode_double_capture(uint8_t piece, uint8_t from, uint8_t to, uint8_t to2)
{
   int swap_shift = 2*MOVE_PICKUP_SIZE;

   return encode_number_pickups(2) |
          encode_number_swaps(1)   |
          encode_move_piece(piece) |
          encode_pickup(to2)        <<  MOVE_SLOT1 |
          encode_pickup(to)         << (MOVE_SLOT1 + MOVE_PICKUP_SIZE) |
          encode_swap  (from,   to) << (MOVE_SLOT1 + swap_shift) | MOVE_RESET50;
}

/* Encode a normal (non-capture) promotion */
static inline move_t encode_normal_promotion(uint8_t piece, uint8_t from, uint8_t to, uint8_t tpiece)
{
   return encode_number_pickups(1) |
          encode_number_drops(1)   |
          encode_move_piece(piece) |
          encode_pickup(from)         << MOVE_SLOT1 |
          encode_drop  (tpiece, to)   << MOVE_SLOT2 | MOVE_RESET50;
}

/* Encode a capture promotion */
static inline move_t encode_capture_promotion(uint8_t piece, uint8_t from, uint8_t to, uint8_t tpiece)
{
   return encode_number_pickups(2) |
          encode_number_drops(1) |
          encode_move_piece(piece) |
          encode_pickup(from)       << MOVE_SLOT1 |
          encode_pickup(to)         << MOVE_SLOT2 |
          encode_drop  (tpiece, to) << MOVE_SLOT3 | MOVE_RESET50;
}

/* Encode a castling move */
static inline move_t encode_castle_move(uint8_t piece, uint8_t from, uint8_t to, uint8_t p2, uint8_t f2, uint8_t t2) 
{
   (void)p2;
   return encode_number_swaps(2)   |
          encode_move_piece(piece) |
          encode_swap(from, to) <<  MOVE_SLOT1 |
          encode_swap(f2,   t2) << (MOVE_SLOT1+MOVE_SWAP_SIZE);
}

/* Encode a drop */
static inline move_t encode_drop_move(uint8_t piece, uint8_t to)
{
   return encode_number_drops(1) | encode_move_piece(piece) | encode_drop(piece, to) << MOVE_SLOT1 | MOVE_RESET50;
}

/* Encode a pickup */
static inline move_t encode_pickup_move(uint8_t piece, uint8_t to)
{
   return encode_number_pickups(1) | encode_move_piece(piece) | encode_pickup(to) << MOVE_SLOT1 | MOVE_RESET50;
}

/* Add store/retrieval information to a move */
static inline move_t add_move_store(move_t move, uint8_t piece, int8_t count)
{
   int shift;
   assert(count < 2);
   assert(count > -2);
   assert((get_move_pickups(move) + get_move_drops(move)) < 5);

   count  = (count + 1) / 2;
   shift = MOVE_SLOT1 +
           MOVE_DROP_SIZE * get_move_drops(move) +
           MOVE_SWAP_SIZE * get_move_swaps(move) +
           MOVE_PICKUP_SIZE * get_move_pickups(move);

   return move | encode_number_holdings(1) | encode_holding(piece, count) << shift;
}
#define add_move_retrieve(move, piece, count) add_move_store((move), (piece), -(count))

/* Add gate information to a move: one extra drop, plus removing from
 * holdings.
 */
static inline move_t add_move_gate(move_t move, uint8_t piece, uint8_t to)
{
   int drops = get_move_drops(move);
   int swaps = get_move_swaps(move);
   int pickups = get_move_pickups(move);
   int shift = MOVE_DROP_SIZE*drops + MOVE_SWAP_SIZE*get_move_swaps(move) + MOVE_PICKUP_SIZE*get_move_pickups(move);

   move &= ~encode_number_drops(3);
   move |= encode_number_drops(drops + 1) | encode_drop(piece, to) << (MOVE_SLOT1 + shift);
   move = add_move_retrieve(move, piece, 1);
   return move;
}

/* Query move types:
 * A normal move is one pickup, one drop (by the same piece)
 * A capture is two pickups (from two different sides), one drop
 * A promotion is a drop by a different piece type than a capture
 * A drop has no pickups
 * A "shot" is a pickup without a drop
 * Castling is two pickups and two drops
 */

static inline bool is_capture_move(move_t move)
{
   return get_move_pickups(move) > get_move_drops(move);
}

static inline bool is_double_capture_move(move_t move)
{
   return get_move_pickups(move) > get_move_drops(move)+1;
}

static inline bool is_promotion_move(move_t move)
{
   unsigned int drop_piece;
   if (get_move_drops(move) != 1 || get_move_pickups(move) < 1)
      return false;
   drop_piece = decode_drop_piece(get_move_drop(move, 0));
   return drop_piece != ((move >> MOVE_PIECE_SHIFT) & MOVE_PIECE_MASK);
}

static inline bool is_irreversible_move(move_t move)
{
   return (move & MOVE_RESET50) != 0;
}

static inline bool is_drop_move(move_t move)
{
   return (get_move_pickups(move) == 0) && (get_move_swaps(move) == 0) && (get_move_drops(move) == 1);
}

static inline bool is_pickup_move(move_t move)
{
   return (get_move_pickups(move) == 1) && (get_move_swaps(move) == 0) && (get_move_drops(move) == 0);
}

static inline bool is_castle_move(move_t move)
{
   return (get_move_swaps(move) == 2) || ((get_move_pickups(move) == 2) && (get_move_drops(move) == 2));
}

static inline bool is_gate_move(move_t move)
{
   return !is_drop_move(move) && (get_move_drops(move) > get_move_pickups(move));
}

/* Convenience macros: the piece type of the first pickup and the first pickup and drop squares
 * By lucky coincidence, this works correctly for pure drop-moves too.
 */
static inline int get_move_piece(move_t move)
{
   return (move >> MOVE_PIECE_SHIFT) & MOVE_PIECE_MASK;
}

static inline int get_move_promotion_piece(move_t move)
{
   uint16_t p = get_move_drop(move, 0);

   return decode_drop_piece(p);
}

static inline int get_move_drop_square(move_t move)
{
   uint16_t p;
   assert(is_drop_move(move) || is_gate_move(move));
   p = get_move_drop(move, 0);

   return decode_drop_square(p);
}

static inline int get_move_from(move_t move)
{
   assert(!is_drop_move(move));
   uint16_t p;
   if (get_move_swaps(move)) {
      int swap = get_move_swap(move, 0);
      return decode_swap_from(swap);
   }
   p = get_move_pickup(move, 0);

   return decode_pickup_square(p);
}

static inline int get_move_to(move_t move)
{
   uint16_t p;
   if (get_move_swaps(move)) {
      int swap = get_move_swap(move, 0);
      return decode_swap_to(swap);
   }
   p = get_move_drop(move, 0);

   return decode_drop_square(p);
}

static inline int get_castle_move_from2(move_t move)
{
   uint16_t p;
   if (get_move_swaps(move) == 2) {
      int swap = get_move_swap(move, 1);
      return decode_swap_from(swap);
   }
   p = get_move_pickup(move, 1);

   return decode_pickup_square(p);
}

static inline int get_castle_move_to2(move_t move)
{
   uint16_t p;
   if (get_move_swaps(move) == 2) {
      int swap = get_move_swap(move, 1);
      return decode_swap_to(swap);
   }
   p = get_move_drop(move, 1);

   return decode_drop_square(p);
}

static inline int get_move_capture_square(move_t move)
{
   uint16_t p = 0;
   
   if (get_move_swaps(move))
      p = get_move_pickup(move, 0);
   else
      p = get_move_pickup(move, 1);

   return decode_pickup_square(p);
}

#endif

