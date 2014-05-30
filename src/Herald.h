/*
 * File: Herald.h
 * Author: David Brotz
 */

#ifndef __HERALD_H
#define __HERALD_H

#include "sys/HashTable.h"

#define TO_YEARS(__MONTHS) (__MONTHS * 12)

#define YEAR(__Date) (__Date >> 9) 
#define MONTH(__Date) ((__Date >> 5) & 15)
#define DAY(__Date) (__Date & 31)

enum {
	BABY = 0,
	CHILD,
	TEENAGER,
	ADULT,
	SENIOR
};

extern int g_Date;

extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
extern struct HashTable g_Buildings;
extern struct HashTable g_Strings;

//extern struct Constraint** g_AgeDistr;
extern struct Constraint** g_AgeConstraints;
extern struct Constraint** g_AgeGroups;
//TODO: g_FamilySize and g_AgeAvg are only used for generation of manor's and should only be exposed to the function that does this.
extern struct Constraint** g_FamilySize;//Average number of families sharing the same last name.
extern struct Constraint** g_AgeAvg;//Distribution of ages.
//TODO: Found more accurate USA census data that should replace this Constraint
//! Percentages of how the probability of the number of children a family will have 3.125 will have 0 babies, etc.
extern struct Constraint** g_BabyAvg;

extern struct LinkedList* g_ManorList;

void HeraldInit();
void HeraldDestroy();
struct Array* LoadFile(const char* _File, char _Delimiter);
void World_Init();
void World_Quit();

#endif
