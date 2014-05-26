/*
 * File: Population.h
 * Author: David Brotz
 */

#ifndef __POPULATION_H
#define __POPULATION_H

#include "LinkedList.h"

struct Population {
	const char* Name;
	int AdultFood;
	int ChildFood;
	int AdultAge;
	int Quantity;
	struct LinkedList Output; //Contains struct Good*.
	struct LinkedList PopList;//Contains struct Population*.
};

struct Population* CreatePopulation(const char* _Name, int _AdultFood, int _ChildFood, int _AdultAge);
struct Population* CopyPopulation(const struct Population* _Population, int _Quantity);
void DestroyPopulation(struct Population* _Population);

#endif
