/*
 * File: Random.h
 * Author: David Brotz
 */

#ifndef __RANDOM_H
#define __RANDOM_H

#ifndef ffs
#define ffs(_Int) __builtin_ffs(_Int)
#endif

unsigned int Random(unsigned int _Min, unsigned int _Max);
int Rand();
void Srand(int _Seed);

#endif
