/*
 * File: Math.c
 * Author: David Brotz
 */

#include "Math.h"

#include <stdlib.h>

#define MATH_RAND_MAX (0xFFFFFFFFFFFFFFFF)
#ifndef RND_TWIST
 static uint64_t g_SeedX = 521288629;
 static uint64_t g_SeedY = 362436069;
#else
#define RAND_ARRAYSZ (624)

static int g_RandIndex = 0;
static int g_RandArray[RAND_ARRAYSZ];
#endif

void MathInit() {
#ifdef RND_TWIST
	g_RandIndex = 0;
	g_RandArray[0] = 521288629;
	for(int i = 1; i < (RAND_ARRAYSZ - 1); ++i) {
		g_RandArray[i] = 0x6C078965 * g_RandArray[i - 1] ^ ((g_RandArray[i - 1] >> 30) + i);
	}
#endif
}

#ifdef RND_TWIST
void RandGenNumbers() {
	int y = 0;

	for(int i = 0; i < RAND_ARRAYSZ; ++i) {
		y = (g_RandArray[i] & 0x80000000) + (g_RandArray[(i + 1) & RAND_ARRAYSZ] % 0x7FFFFFFF);
		g_RandArray[i] = g_RandArray[(i + 397) % RAND_ARRAYSZ] ^ (y >> 1);
		if((g_RandArray[i] & 1) == 1)
			g_RandArray[i] = g_RandArray[i] ^ 0x9908B0DF;
	}
}
#endif

uint64_t Rand() {
#ifdef RND_TWIST
	 int y = g_RandArray[g_RandIndex];

	 if(g_RandIndex == 0)
		 RandGenNumbers();
	 y = y ^ (y >> 11);
	 y = y ^ ((y << 7) & 0xD2C5680);
	 y = y ^ ((y << 15) & 0xEFC6000);
	 y = y ^ (y >> 18);
	 g_RandIndex = (g_RandIndex + 1) & RAND_ARRAYSZ;
	 return y;
#else
	 g_SeedX = 1800 * (g_SeedX & 0xFFFFFFFF) + (g_SeedX >> 32);
	 g_SeedY = 30903 * (g_SeedY & 0xFFFFFFFF) + (g_SeedY >> 32);
	 return (g_SeedX << 32) + (g_SeedY & 0xFFFFFFFF);
#endif
 }

uint64_t Random(uint64_t _Min, uint64_t _Max) {
	uint64_t _Rand = 0;

	while((_Rand = Rand()) > (MATH_RAND_MAX - ((MATH_RAND_MAX % _Max) + 1))) {}
	//_Rand = Rand();
	return _Rand % (_Max - _Min + 1) + _Min;
}

void Srand(int _Seed) {
	//g_SeedX = _Seed << 16;
	//g_SeedY = _Seed >> 16;
}

int min(int _One, int _Two) {
	return (_One <= _Two) ? (_One) : (_Two);
}

int max(int _One, int _Two) {
	return (_One >= _Two) ? (_One) : (_Two);
}

int Abs(int _Num) {
	int _Mask = (_Num >> (sizeof(int) * CHAR_BIT - 1));

	return (_Num ^ _Mask) - _Mask;
}

void RandTable(double* _Table, int** _IntTable, int _TableSz, int _Amount) {
	double _IdxAmt = 0.0;
	double _Percent = 0.0;
	int _PercentInt = 0;
	int _Temp = (int) _IdxAmt;
	int _CurrAmount = _Amount;

	for(int i = 0; i < _TableSz; ++i) {
		_IdxAmt = _CurrAmount * _Table[i];
		_Temp = (int) _IdxAmt;
		if(NumberIsInt(_IdxAmt) != 0) {
			_CurrAmount -= _Temp;
			(*_IntTable)[i] = _Temp;
			continue;
		}
		_Percent = _IdxAmt - _Temp;
		_PercentInt = _Percent * 100;
		(*_IntTable)[i] = _Temp;
		_CurrAmount -= _Temp;
		if(_CurrAmount > 0 && Random(0, 100) <= _PercentInt) {
			++((*_IntTable)[i]);
			--_CurrAmount;
		}
	}
	while(_CurrAmount > 0) {
		for(int i = 0; i < _TableSz; ++i) {
			_IdxAmt = _CurrAmount * _Table[i];
			_Temp = (int) _IdxAmt;
			if(NumberIsInt(_IdxAmt) != 0) {
				continue;
			}
			_Percent = _IdxAmt - _Temp;
			_PercentInt = _Percent * 100;
			if(Random(0, 100) <= _PercentInt) {
				++((*_IntTable)[i]);
				--_CurrAmount;
				if(_CurrAmount <= 0)
					return;
			}
		}
	}
}

double Normalize(int _Num, int _Min, int _Max) {
	return ((double)_Num) / (_Min + _Max);
}

int NextPowTwo(int _Num) {
	return (1 << ((sizeof(int) * CHAR_BIT) - clz(_Num)));
}

int PrevPowTwo(int _Num) {
	return (1 << ((sizeof(int) * CHAR_BIT) - clz(_Num) - 1));
}

/**
 * Credit: https://gist.github.com/orlp/3551590
 */
int64_t Ipow(int64_t base, uint8_t exp) {
    static const uint8_t highest_bit_set[] = {
        0, 1, 2, 2, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 255, // anything past 63 is a guaranteed overflow with base > 1
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
    };

    uint64_t result = 1;

    switch (highest_bit_set[exp]) {
    case 255: // we use 255 as an overflow marker and return 0 on overflow/underflow
        if (base == 1) {
            return 1;
        }
        
        if (base == -1) {
            return 1 - 2 * (exp & 1);
        }
        
        return 0;
    case 6:
        if (exp & 1) result *= base;
        exp >>= 1;
        base *= base;
    case 5:
        if (exp & 1) result *= base;
        exp >>= 1;
        base *= base;
    case 4:
        if (exp & 1) result *= base;
        exp >>= 1;
        base *= base;
    case 3:
        if (exp & 1) result *= base;
        exp >>= 1;
        base *= base;
    case 2:
        if (exp & 1) result *= base;
        exp >>= 1;
        base *= base;
    case 1:
        if (exp & 1) result *= base;
    default:
        return result;
    }
}
