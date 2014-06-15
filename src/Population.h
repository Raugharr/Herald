/*
 * File: Population.h
 * Author: David Brotz
 */

#ifndef __POPULATION_H
#define __POPULATION_H

#include "sys/LinkedList.h"

typedef struct lua_State lua_State;

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
struct Population* PopulationLoad(lua_State* _State, int _Index);

#endif
