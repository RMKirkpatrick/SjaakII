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
#ifndef EVAL_TYPES_H
#define EVAL_TYPES_H

#include "compilerdef.h"

typedef int16_t eval_t;

struct eval_pair_t {
   eval_t mg, eg;

   eval_pair_t () { mg = eg = 0; }
   eval_pair_t (int v) { mg = eg = v; }
   eval_pair_t (int m, int e) { mg = m; eg = e; }


   inline eval_pair_t operator = (const eval_pair_t p) {
      mg = p.mg;
      eg = p.eg;
      return *this;
   }

   inline eval_pair_t operator = (const int v) {
      mg = v;
      eg = v;
      return *this;
   }

   inline eval_pair_t operator << (const int bits) const {
      return eval_pair_t(mg << bits, eg << bits);
   }
   inline eval_pair_t operator >> (const int bits) const {
      return eval_pair_t(mg >> bits, eg >> bits);
   }

   inline eval_pair_t operator +=(const eval_pair_t &rhs) {
      mg += rhs.mg;
      eg += rhs.eg;
      return *this;
   }
   inline eval_pair_t operator -=(const eval_pair_t &rhs) {
      mg -= rhs.mg;
      eg -= rhs.eg;
      return *this;
   }
   inline eval_pair_t operator *=(const eval_pair_t &rhs) {
      mg *= rhs.mg;
      eg *= rhs.eg;
      return *this;
   }
   inline eval_pair_t operator /=(const eval_pair_t &rhs) {
      mg /= rhs.mg;
      eg /= rhs.eg;
      return *this;
   }

   inline eval_pair_t operator +=(const int &rhs) {
      mg += rhs;
      eg += rhs;
      return *this;
   }
   inline eval_pair_t operator -=(const int &rhs) {
      mg -= rhs;
      eg -= rhs;
      return *this;
   }
   inline eval_pair_t operator *=(const int &rhs) {
      mg *= rhs;
      eg *= rhs;
      return *this;
   }
   inline eval_pair_t operator /=(const int &rhs) {
      mg /= rhs;
      eg /= rhs;
      return *this;
   }


   inline const eval_pair_t operator +(const eval_pair_t &rhs) const {
      return eval_pair_t(*this) += rhs;
   }
   inline const eval_pair_t operator -(const eval_pair_t &rhs) const {
      return eval_pair_t(*this) -= rhs;
   }
   inline const eval_pair_t operator *(const eval_pair_t &rhs) const {
      return eval_pair_t(*this) *= rhs;
   }
   inline const eval_pair_t operator /(const eval_pair_t &rhs) const {
      return eval_pair_t(*this) /= rhs;
   }

   inline const eval_pair_t operator +(const int &rhs) const {
      return eval_pair_t(*this) += eval_pair_t(rhs);
   }
   inline const eval_pair_t operator -(const int &rhs) const {
      return eval_pair_t(*this) -= eval_pair_t(rhs);
   }
   inline const eval_pair_t operator *(const int &rhs) const {
      return eval_pair_t(*this) *= eval_pair_t(rhs);
   }
   inline const eval_pair_t operator /(const int &rhs) const {
      return eval_pair_t(*this) /= eval_pair_t(rhs);
   }

   inline const eval_pair_t operator *(const float &rhs) const {
      return eval_pair_t(int(this->mg * rhs), int(this->eg * rhs));
   }

   inline eval_t interpolate(int x, int scale) const {
      //if (x < 0) x = 0;
      //if (x > scale) x = scale;
      if (expect(scale == 0, false)) return mg;
      return (scale * mg + (scale - x) * (eg-mg) ) / scale;
   }

   inline eval_pair_t operator -() const {
      return eval_pair_t(-mg, -eg);
   }
};

/* Pawn structure tables */
template <typename kind>
struct pawn_structure_t {
   bitboard_t<kind> open;
   bitboard_t<kind> passed;
   bitboard_t<kind> stop;
   bitboard_t<kind> weak;
   bitboard_t<kind> attacked[NUM_SIDES];
   eval_t shelter_score[NUM_SIDES][16];
};


#endif
