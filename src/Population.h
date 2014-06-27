/*
 * File: Population.h
 * Author: David Brotz
 */

#ifndef __POPULATION_H
#define __POPULATION_H

#include "World.h"
#include "sys/LinkedList.h"

typedef struct lua_State lua_State;

struct Population {
	int Id;
	char* Name;
	int AdultFood;
	int ChildFood;
	int Meat; //In pounds.
	int Milk; //Fluid ounces.
	double MalePercent;
	struct Constraint** Ages;
	struct Array* Output; //Contains struct Good*.
};

struct Animal {
	int Id;
	int Gender;
	int Nutrition;
	DATE Age;
	struct Population* PopType;
};

struct Population* CreatePopulation(const char* _Name, int _AdultFood, int _ChildFood, struct Constraint** _Ages, int _Meat, int _Milk);
struct Population* CopyPopulation(const struct Population* _Population);
void DestroyPopulation(struct Population* _Population);
struct Population* PopulationLoad(lua_State* _State, int _Index);

struct Animal* CreateAnimal(struct Population* _Pop, int _Age);
void DestroyAnimal(struct Animal* _Animal);
void AnimalUpdate(struct Animal* _Animal);
void AnimalDeath();

#endif
