/* Testing Code */

#include <limits.h>
#include <math.h>

/* Routines used by floation point test code */

/* Convert from bit level representation to floating point number */
float u2f(unsigned u) {
  union {
    unsigned u;
    float f;
  } a;
  a.u = u;
  return a.f;
}

/* Convert from floating point number to bit-level representation */
unsigned f2u(float f) {
  union {
    unsigned u;
    float f;
  } a;
  a.f = f;
  return a.u;
}

/* Copyright (C) 1991-2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* We do support the IEC 559 math functionality, real and complex.  */
/* wchar_t uses ISO/IEC 10646 (2nd ed., published 2011-03-15) /
   Unicode 6.0.  */
/* We do not support C11 <threads.h>.  */
//1
long test_bitNor(long x, long y)
{
  return ~(x|y);
}
//2
long test_anyOddBit(long x) {
  int i;
  for (i = 1; i < 64; i+=2)
      if (x & (1L<<i))
   return 1L;
  return 0L;
}
long test_negate(long x) {
  return -x;
}
long test_sign(long x) {
    if (x == 0)
 return 0;
    return (x < 0) ? -1L : 1L;
}
//3
long test_logicalShift(long x, long n) {
  unsigned long u = (unsigned long) x;
  unsigned long shifted = u >> n;
  return (long) shifted;
}
long test_subtractionOK(long x, long y)
{
  __int128 ldiff = (__int128) x - y;
  return (long) (ldiff == (long) ldiff);
}
long test_isLess(long x, long y)
{
    return (long) (x < y);
}
//4
long test_bang(long x)
{
    return (long) !x;
}
long test_howManyBits(long x) {
    long cnt;
    if (x < 0)
 /* Flip the bits if x is negative */
 x = ~x;
    unsigned long a = (unsigned long) x;
    for (cnt=1; a; a>>=1, cnt++)
        ;
    return cnt;
}
//float
unsigned test_floatNegate(unsigned uf) {
    float f = u2f(uf);
    float nf = -f;
    if (isnan(f))
 return uf;
    else
 return f2u(nf);
}
int test_floatIsLess(unsigned uf, unsigned ug) {
    float f = u2f(uf);
    float g = u2f(ug);
    return f < g;
}
unsigned test_floatScale4(unsigned uf) {
  float f = u2f(uf);
  float tf = 4*f;
  if (isnan(f))
    return uf;
  else
    return f2u(tf);
}
unsigned test_floatUnsigned2Float(unsigned u) {
    float f = (float) u;
    return f2u(f);
}
