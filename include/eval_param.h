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

/* Integer constants are used within the evaluation itself, floating point
 * values are only used when initialising the evaluation tables at startup.
 */

#define GAME_PHASE_FLOOR      80

#define SAFE_MOB_WEIGHT        8       // Weight for safe mobility squares
#define MOB_SCALE             16       // Overall mobility weight
#define MOB_FORWARD_BLOCKED    5       // Penalty for forward mobility blocked by pieces

#define KS_ATTACK_WEIGHT    256
#define KS_SHELTER_WEIGHT    32
#define KING_SAFETY_WEIGHT  416        // Overall scale-factor for king safety
// Weight vs. STS score:
// 512 (3126)
// 256 (3044)
// 384 (3118)
// 448 (3117)
// 416 (3128)

#define PST_HOLDINGS          5        // PST value for a piece in holdings

#define PST_SPACE_MG          5        // PST value for empty squares on the own side of the board

#define PST_CENTRE_Q          1.0
#define PST_CENTRE_L          0.0
#define PST_ADVANCE_Q         0.0
#define PST_ADVANCE_L         0.0

#define PST_SCALE_PALACE      1.1      // Scale factor for PST in games with a palace

#define MOBILITY_SCALE_MG   128.0      // Scale factor for mobility, middle game
// Weight vs. STS score:
//  96 (8001, 923)
// 112 (8143, 932)
// 120 (8055, 932)
// 128 (8234, 940)
// 130 (8110, 922)
// 140 (8086, 926)
// 192 (8094, 931)
#define MOBILITY_SCALE_EG   128.0      // Scale factor for mobility, end game
// Weight vs. STS score:
// 120 (8114, 928)
// 128 (8234, 940)
// 140 (8007, 916)

#define PASSER_RANK_BASE      4
#define PASSER_RANK_SCALE   128

#define PAWN_ADVANCE_Q_EG    98.0      // Bonus for advancing pawns in the end game, quadratic term
#define PAWN_ADVANCE_L_EG     0.0      // Bonus for advancing pawns in the end game, linear term
#define PAWN_ADVANCE_Q_MG     0.0      // Bonus for advancing pawns in the middle game, quadratic term
#define PAWN_ADVANCE_L_MG     0.0      // Bonus for advancing pawns in the middle game, linear term

#define ROOK_BASE_PAWN_MG     5        // Bonus for a rook that attacks base of enemy pawn structure, middle game
#define ROOK_BASE_PAWN_EG    10        // Bonus for a rook that attacks base of enemy pawn structure, end game

#define WEAK_PAWN_BASE_MG     5        // Penalty for a weak pawn, middle game
#define WEAK_PAWN_BASE_EG    10        // Penalty for a weak pawn, end game

#define LOOSE_MINOR_PENALTY   5        // Penalty for an undefended minor on the enemy side of the board

#define PAIR_BONUS_MG         0.10     // Fraction of piece value
#define PAIR_BONUS_EG         0.15     // Fraction of piece value

#define SLIDER_OPENFILE_MG   10        // Slider bonus on open file, middle game
#define SLIDER_OPENFILE_EG   15        // Slider bonus on open file, end game

#define HOPPER_KINGFILE_MG   50        // Hopper on same file as enemy king, middle game
#define HOPPER_KINGFILE_EG    0        // Hopper on same file as enemy king, middle game

#define DEF_PROTECT           7        // Bonus for defensive pieces that protect eachother
#define DEF_SHIELD_FILE      10        // Bonus for defensive pieces on king file

#define FUTILITY_DEPTH        3

#define PAWN_SCALE_MG         0.8      // Fraction of nominal piece value
#define LAME_SCALE_MG         0.9      // Fraction of nominal piece value
#define LAME_SCALE_EG         1.1      // Fraction of nominal piece value
#define DEF_SCALE_MG          1.1      // Fraction of nominal piece value
#define DEF_SCALE_EG          0.8      // Fraction of nominal piece value
#define HOP_SCALE_EG          0.7      // Fraction of nominal piece value

#define TEMPO_DROP_WEIGHT     2        // Weighting of in-hand material for the tempo bonus
#define TEMPO_DROP_MAX        32767    // Absolute limit for the tempo bonus
