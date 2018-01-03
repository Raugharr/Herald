/*
 * File: Math.c
 * Author: David Brotz
 */

#include "Math.h"

#include "Log.h"
#include <assert.h>

#include <stdlib.h>
#include <SDL2/SDL.h>


#ifndef RND_TWIST
 static uint64_t g_SeedX = 521288629;
 static uint64_t g_SeedY = 362436069;
#else
#define RAND_ARRAYSZ (624)

static uint32_t g_RandIndex = 0;
static uint32_t g_RandArray[RAND_ARRAYSZ];
#endif

static uint8_t g_RandByteIdx = 0;
//Simple rng generator table taken from doom source code.
uint8_t g_RandTbl[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

void MathInit() {
#ifdef RND_TWIST
	g_RandIndex = 0;
	g_RandArray[0] = 521288629;
	for(int i = 1; i < RAND_ARRAYSZ; ++i) {
		g_RandArray[i] = 0x6C078965 * g_RandArray[i - 1] ^ ((g_RandArray[i - 1] >> 30) + i);
	}
	g_RandIndex = RAND_ARRAYSZ;
#endif
}

#ifdef RND_TWIST
void RandGenNumbers() {
	uint32_t x = 0;
	uint32_t xA = 0;

	for(int i = 0; i < RAND_ARRAYSZ; ++i) {
		x = (g_RandArray[i] & 0x7FFFFFFF) + (g_RandArray[(i + 1) & RAND_ARRAYSZ] % 0x7FFFFFFF);
		xA = x >> 1;
		if((x & 1) == 1) xA ^= 0x9908B0DF;
		g_RandArray[i] = g_RandArray[(i + 397) % RAND_ARRAYSZ] ^ xA;
	}
	g_RandIndex = 0;
}
#endif

uint64_t Rand() {
#ifdef RND_TWIST
	 uint32_t y = 0;
	 uint32_t i = g_RandIndex;

	if(g_RandIndex >= RAND_ARRAYSZ) {
		RandGenNumbers();
		i = g_RandIndex;
	}
	++g_RandIndex;
	y = g_RandArray[i];
	y ^= (y >> 11);
	y ^= ((y << 7) & 0xD2C5680UL);
	y ^= ((y << 15) & 0xEFC6000UL);
	y ^= (y >> 18);
	//g_RandIndex = (g_RandIndex + 1) & RAND_ARRAYSZ;
	return y;
#else
	 g_SeedX = 1800 * (g_SeedX & 0xFFFFFFFF) + (g_SeedX >> 32);
	 g_SeedY = 30903 * (g_SeedY & 0xFFFFFFFF) + (g_SeedY >> 32);
	 return (g_SeedX << 32) + (g_SeedY & 0xFFFFFFFF);
#endif
 }

double Gaussian(void) {
	static double Fact, v = 0;
	static int Phase = 0;
	double u, Z, S, U1, U2 = 0;

	if(Phase) {
		Z = v * Fact;
	} else {
		do {
			U1 = (double)Rand() / MATH_RAND_MAX;
			U2 = (double)Rand() / MATH_RAND_MAX;
			u = 2.0 * U1 - 1.0;
			v = 2.0 * U2 - 1.0;
			S = u * u + v * v;
		} while(S >= 1);
		Fact = sqrt(-2.0 * log(S) / S);
		Z = u * Fact;
	}
	Phase = 1 - Phase;
	return Z;
}

double NormalRandom(void) {
	static double Sigma = 1.0 / 8.0;
	
	while(true) {
		double Z = Gaussian() * Sigma + 0.5;

		if(Z >= 0 && Z <= 1) return Z;
	}
}

uint64_t Random(uint64_t Min, uint64_t Max) {
	uint64_t RandNum = 0;

	//assert(Min < Max);
	if(Max == 0)
		return 0;
	while((RandNum = Rand()) > (MATH_RAND_MAX - ((MATH_RAND_MAX % Max) + 1))) {}
	//_Rand = Rand();
	return RandNum % (Max - Min + 1) + Min;
}

void Srand(int Seed) {
	//g_SeedX = Seed << 16;
	//g_SeedY = Seed >> 16;
}

int min(int One, int Two) {
	return (One <= Two) ? (One) : (Two);
}

int max(int One, int Two) {
	return (One >= Two) ? (One) : (Two);
}

int Abs(int Num) {
	int Mask = (Num >> (sizeof(int) * CHAR_BIT - 1));

	return (Num ^ Mask) - Mask;
}

void NormalizeTable(double* Table, int TableSz) {
	double Total = 0;

	for(int i = 0; i < TableSz; ++i) {
		Total += Table[i];
	}
	while(Total < 1.0) {
		double Diff = 1 / Total;

		Total = 0;
		for(int i = 0; i < TableSz; ++i) {
			Table[i] = Table[i] * Diff;
			Total += Table[i];
		}
	}
}

void RandTable(double* Table, int** IntTable, int TableSz, int Amount) {
	double IdxAmt = 0.0;
	double Percent = 0.0;
	int PercentInt = 0;
	int Temp = (int) IdxAmt;
	int CurrAmount = Amount;

	if(TableSz == 0) return;
	for(int i = 0; i < TableSz; ++i) {
		IdxAmt = Amount * Table[i];//_CurrAmount * Table[i];
		Temp = (int) IdxAmt;
		if(NumberIsInt(IdxAmt) != 0) {
			CurrAmount -= Temp;
			(*IntTable)[i] = Temp;
			continue;
		}
		Percent = IdxAmt - Temp;
		PercentInt = Percent * 100;
		(*IntTable)[i] = Temp;
		CurrAmount -= Temp;
		if(CurrAmount > 0 && Random(0, 100) <= PercentInt) {
			++((*IntTable)[i]);
			--CurrAmount;
		}
	}
	while(CurrAmount > 0) {
		for(int i = 0; i < TableSz; ++i) {
			IdxAmt = CurrAmount * Table[i];
			Temp = (int) IdxAmt;
			if(NumberIsInt(IdxAmt) != 0) {
				continue;
			}
			Percent = IdxAmt - Temp;
			PercentInt = Percent * 100;
			if(Random(1, 100) <= PercentInt) {
				++((*IntTable)[i]);
				--CurrAmount;
				if(CurrAmount <= 0)
					return;
			}
		}
	}
}

uint8_t RandByte() {
	g_RandByteIdx = (g_RandByteIdx + 1) & 0xFF;
	return g_RandTbl[g_RandByteIdx];
}

double Normalize(int Num, int Min, int Max) {
	return ((double)Num) / (Min + Max);
}

int NextPowTwo(int Num) {
	return (1 << ((sizeof(int) * CHAR_BIT) - clz(Num)));
}

int PrevPowTwo(int Num) {
	return (1 << ((sizeof(int) * CHAR_BIT) - clz(Num) - 1));
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

uint32_t Isqrt(uint32_t num) {
    uint32_t res = 0;
    uint32_t bit = 1 << 30;
 
    // "bit" starts at the highest power of four <= the argument.
    while (bit > num)
        bit >>= 2;
        
    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        }
        else
            res >>= 1;
        bit >>= 2;
    }
    return res;
}

void Centroid(const SDL_Point** Arr, int ArrSz, SDL_Point* Center) {
	int x = 0;
	int y = 0;

	for(int i = 0; i < ArrSz; ++i) {
		x += Arr[i]->x;
		y += Arr[i]->y;
	}
	Center->x = x / ArrSz;
	Center->y = y / ArrSz;
}
