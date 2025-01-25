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
#ifndef PIECES_H
#define PIECES_H

#include <stdint.h>
#include "moveflag.h"

#define MAX_PIECE_TYPES    32 //16
#define PIECE_BITS          5

#define MAX_STEPPER_TYPES  16
#define MAX_LEAPER_TYPES   16
#define MAX_RIDER_TYPES    16

typedef uint32_t piece_bit_t;

/* Bitfield colours */
typedef enum side_t { NONE=-1, WHITE, BLACK, NUM_SIDES } side_t;
static const side_t next_side[NUM_SIDES] = { BLACK, WHITE };

/* Castle move options */
enum { SHORT, LONG, NUM_CASTLE_MOVES };

#ifdef __cplusplus
inline side_t& operator++(side_t& side, int)
{
   assert(side >= WHITE);
   assert(side <= BLACK);
	const int i = static_cast<int>(side)+1;
	side = static_cast<side_t>(i);
	return side;
}
#endif

/* For the generalised description of moves, we need to pack the piece type and the colour bit into one bit
 * field. This feels a bit like a hack...
 */
static inline uint8_t piece_for_side(int piece, side_t side)
{
   return piece | (side << PIECE_BITS);
}

static inline side_t side_for_piece(int piece)
{
#ifdef __cplusplus
   side_t side = static_cast<side_t>((piece >> PIECE_BITS) & 0x01);
   return side;
#else
   return (piece >> PIECE_BITS) & 0x01;
#endif
}

static inline uint8_t neutral_piece(int piece)
{
   return piece & ((1 << PIECE_BITS)-1);
}


#endif
