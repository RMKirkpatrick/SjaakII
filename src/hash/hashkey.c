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
#include <stdint.h>
#include "hashkey.h"
#include "genrand.h"

uint64_t piece_key[MAX_PIECE_TYPES][2][128];
uint64_t hold_key[MAX_PIECE_TYPES][2][128];
uint64_t side_to_move_key;
uint64_t flag_key[2][8];
uint64_t en_passant_key[128];

uint64_t genrand64(void)
{
   return ((uint64_t)genrandui())<<32 | genrandui();
}

void initialise_hash_keys(void)
{
   int colour, piece, square;
   sgenrand(0x1422CE55);

   for (piece = 0; piece < MAX_PIECE_TYPES; piece++)
      for (colour = 0; colour<2; colour++)
         for (square = 0; square<128; square++)
            piece_key[piece][colour][square] = genrand64();

   for (square = 0; square<128; square++)
      en_passant_key[square] = genrand64();

   for (colour = 0; colour<2; colour++)
      for (square = 0; square<8; square++)
         flag_key[colour][square] = genrand64();

   side_to_move_key = genrand64();

   for (piece = 0; piece < MAX_PIECE_TYPES; piece++)
      for (colour = 0; colour<2; colour++)
         for (square = 0; square<128; square++)
            hold_key[piece][colour][square] = square ? genrand64() : 0;
}

