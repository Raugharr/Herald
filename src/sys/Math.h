/*
 * File: Random.h
 * Author: David Brotz
 */

#ifndef __MATH_H
#define __MATH_H

#ifndef ffs
#define ffs(_Int) __builtin_ffs(_Int)
#endif

#ifndef bsc //bit set count
#define bsc(_Int) __builtin_popcount(_Int)
#endif

#ifndef CHAR_BITS
#define CHAR_BITS (8)
#endif

#define PowTwo(_Pow) (_Pow) > 0 && !((_Pow) & ((_Pow) - 1))

#define IsNeg(_Int) ((_Int) < 0)
//_Int becomes _Num more integers away from 0.
#define AbsAdd(_Int, _Num) ((+1 | ((_Int) >> sizeof(int) * (CHAR_BITS - 1))) * (_Num))
#define IntSignedness(_Val) (((_Val) != 0) | -(int)((unsigned int)((int) (_Val)) >> (sizeof(int) * CHAR_BITS - 1)))

void MathInit();

unsigned int Random(unsigned int _Min, unsigned int _Max);
int Rand();
void Srand(int _Seed);

int min(int _One, int _Two);
int max(int _One, int _Two);
int Abs(int _Num);

double Normalize(int _Num, int _Min, int _Max);

#endif
