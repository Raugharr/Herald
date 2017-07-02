/*
 * File: Random.h
 * Author: David Brotz
 */

#ifndef __MATH_H
#define __MATH_H

#include <limits.h>
#include <math.h>
#include <stdint.h>

#ifndef ffs
#define ffs(_Int) __builtin_ffs(_Int)
#endif

#ifndef bsc //bit set count
#define bsc(_Int) __builtin_popcount(_Int)
#endif

#ifndef clz
#define clz(_Int) __builtin_clz(_Int)
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define IsPowTwo(_Pow) (_Pow) > 0 && !((_Pow) & ((_Pow) - 1))
#define IsNeg(_Int) ((_Int) < 0)
//_Int becomes _Num more integers away from 0.
#define AbsAdd(_Int, _Num) ((+1 | ((_Int) >> sizeof(int) * (CHAR_BIT - 1))) * (_Num))
#define IntSignedness(_Val) (((_Val) != 0) | -(int)((unsigned int)((int) (_Val)) >> (sizeof(int) * CHAR_BIT - 1)))
#define sqrt3 (1.732050807568877)

void MathInit();

uint64_t Random(uint64_t _Min, uint64_t _Max);
uint64_t Rand();
uint8_t RandByte();
void Srand(int _Seed);

int min(int _One, int _Two);
int max(int _One, int _Two);
int Abs(int _Num);
static inline int NumberIsInt(double _Number) {
	return _Number == (int)_Number;
}

/**
 * \brief Takes a table _Table that contains _TableSz elements who's sum is equal to 1.
 * Each element of _Table is multiplied by _Amount and the result is placed into _IntTable.
 * If the sum of the multiplication is a number and not an integer a random number between 0
 * and 100 is generated, if the percentile of the number is less than the generated integer 
 * the number is ceiled. Example 3 -> 3, 3.6 has a 60% chance of becomming 4.
 * \param _Table an array that is of _TableSz length and who's elements are summed to 1.
 * \param _IntTable an array of _TableSize that will be filled with the returning values.
 * \param _TableSz The size of both _Table and _IntTable.
 * \param _Amount The sum of the elements that are contained in _IntTable.
 */ 
void RandTable(double* _Table, int** _IntTable, int _TableSz, int _Amount);
double Normalize(int _Num, int _Min, int _Max);
int NextPowTwo(int _Num);
int PrevPowTwo(int _Num);
int64_t Ipow(int64_t base, uint8_t exp);
static inline double NormalDistribution(double _Var, uint32_t _Variance, uint32_t _Mean) {
	return pow(((double)1) / (sqrt(2 * _Variance * M_PI)), (-pow(_Var - _Mean, 2)) / (2 * _Variance));
}

static inline double Quadratic(uint32_t _Num, uint32_t _Min, uint32_t _Max) {
	uint32_t _Mult = (_Num - _Min);
	uint32_t _MaxMult = (_Max - _Min);
	_Mult *= _Mult;
	_MaxMult *= _MaxMult;
	return _Mult / ((double)_MaxMult);
}
#endif
