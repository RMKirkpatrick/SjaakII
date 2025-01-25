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
#include "softexp.h"

/* Stupid implementation of the exponential function.
 * First brings its argument in the range [0, 1] while collecting
 * appropriate integer powers of e, then uses the Pade approximation to
 * evaluate the exponential on the interval [0, 1].
 *
 * Included here because for obscure reasons cross-compiled 64 bit Windows
 * binaries crash in Windows when the libc exp() function is called.
 */
double myexp(double x)
{
   double x2, x3, x4;
   const double ee = 2.71828182845904523536028747135266250;
   double y = 1.;
   while (x > 1.) {
      y *= ee;
      x -= 1.0;
   }
   while (x < 0.) {
      y /= ee;
      x += 1.0;
   }
   x2 = x*x;
   x3 = x2*x;
   x4 = x2*x2;

   y *= (1. + 4./7.*x+1./7.*x2+2./105.*x3+1./840*x4) / (1 - 3./7.*x + 1./14. * x2 - 1/210.*x3);

   return y;
}
