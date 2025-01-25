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
#include "pieces.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint64_t piece_key[MAX_PIECE_TYPES][2][128];
extern uint64_t hold_key[MAX_PIECE_TYPES][2][128];
extern uint64_t side_to_move_key;
extern uint64_t flag_key[2][8];
extern uint64_t en_passant_key[128];

void initialise_hash_keys(void);

#ifdef __cplusplus
}
#endif
