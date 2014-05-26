/*
 * File: Herald.h
 * Author: David Brotz
 */

#ifndef __HERALD_H
#define __HERALD_H

#include "HashTable.h"

#define AGEDIST_SIZE (17)
#define TO_YEARS(__MONTHS) (__MONTHS * 12)

struct Array {
	void** Table;
	int Size;
};

enum {
	BABY = 0,
	CHILD,
	TEENAGER,
	ADULT,
	SENIOR
};

extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
extern struct HashTable g_Buildings;
extern struct HashTable g_Strings;

extern struct Constraint** g_AgeDistr;
extern struct Constraint** g_AgeConstraints;
extern struct Constraint** g_AgeGroups;
extern struct Constraint** g_FamilySize;//Average number of families sharing the same last name.
extern struct Constraint** g_AgeAvg;//Distribution of ages.
//! Percentages of how the probability of the number of children a family will have 3.125 will have 0 babies, etc.
extern struct Constraint** g_BabyAvg;

void HeraldInit();
void HeraldDestroy();
void LoadCSV(const char* _File, char*** _Array, int* _Size);
void SetArray(void*** _Array, int _Size, void* _Value);

#endif
