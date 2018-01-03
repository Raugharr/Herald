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
#define ffs(Int) __builtin_ffs(Int)
#endif

#ifndef bsc //bit set count
#define bsc(Int) __builtin_popcount(Int)
#endif

#ifndef clz
#define clz(Int) __builtin_clz(Int)
#endif

#ifndef ctz
#define ctz(Int) __builtin_ctz(Int)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define IsPowTwo(Pow) (Pow) > 0 && !((Pow) & ((Pow) - 1))
#define IsNeg(Int) ((Int) < 0)
//_Int becomes Num more integers away from 0.
#define AbsAdd(Int, Num) ((+1 | ((Int) >> sizeof(int) * (CHAR_BIT - 1))) * (Num))
#define IntSignedness(Val) (((Val) != 0) | -(int)((unsigned int)((int) (Val)) >> (sizeof(int) * CHAR_BIT - 1)))
#define sqrt3 (1.732050807568877)
#define RND_TWIST
#ifdef RND_TWIST
#define MATH_RAND_MAX (UINT32_MAX)
#else
#define MATH_RAND_MAX (UINT64_MAX)
#endif

typedef struct SDL_Point SDL_Point;

void MathInit();

double NormalRandom(void);
uint64_t Random(uint64_t Min, uint64_t Max);
uint64_t Rand();
uint8_t RandByte();
void Srand(int Seed);

int min(int One, int Two);
int max(int One, int Two);
int Abs(int Num);
static inline int NumberIsInt(double Number) {
	return Number == (int)Number;
}

void NormalizeTable(double* Table, int TableSz);
/**
 * \brief Takes a table Table that contains TableSz elements who's sum is equal to 1.
 * Each element of Table is multiplied by Amount and the result is placed into IntTable.
 * If the sum of the multiplication is a number and not an integer a random number between 0
 * and 100 is generated, if the percentile of the number is less than the generated integer 
 * the number is ceiled. Example 3 -> 3, 3.6 has a 60% chance of becomming 4.
 * \param Table an array that is of TableSz length and who's elements are summed to 1.
 * \param IntTable an array of TableSize that will be filled with the returning values.
 * \param TableSz The size of both Table and IntTable.
 * \param Amount The sum of the elements that are contained in IntTable.
 */ 
void RandTable(double* Table, int** IntTable, int TableSz, int Amount);
double Normalize(int Num, int Min, int Max);
int NextPowTwo(int Num);
int PrevPowTwo(int Num);
int64_t Ipow(int64_t base, uint8_t exp);
uint32_t Isqrt(uint32_t num);
static inline double NormalDistribution(double Var, uint32_t Variance, uint32_t Mean) {
	return pow(((double)1) / (sqrt(2 * Variance * M_PI)), (-pow(Var - Mean, 2)) / (2 * Variance));
}

static inline double Quadratic(uint32_t Num, uint32_t Min, uint32_t Max) {
	uint32_t Mult = (Num - Min);
	uint32_t MaxMult = (Max - Min);
	Mult *= Mult;
	MaxMult *= MaxMult;
	return Mult / ((double)MaxMult);
}

void Centroid(const SDL_Point** Arr, int ArrSz, SDL_Point* Center);
#endif
