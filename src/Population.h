/*
 * File: Population.h
 * Author: David Brotz
 */

#ifndef __POPULATION_H
#define __POPULATION_H

#include "World.h"
#include "sys/LinkedList.h"

#define AGE_MATURE (0)
#define AGE_DEATH (1)

typedef struct lua_State lua_State;
struct HashTable;

struct Population {
	int Id;
	char* Name;
	int Nutrition; //How much is eaten per day.
	int Meat; //In pounds.
	int Milk; //Fluid ounces.
	double MaleRatio;
	struct Constraint** Ages;
	struct Good** Outputs; //Contains struct Good*.
	struct FoodBase** Eats;
	int EatsSize;
};

struct Animal {
	int Id;
	int Type;
	int X;
	int Y;
	int(*Think)(struct Object*);
	int Gender;
	int Nutrition;
	DATE Age;
	struct ActorJob* Action;
	const struct Population* PopType;
};

struct AnimalDep {
	struct FoodBase** Tbl;
	struct Array* Animals;
	int Nutrition;
};

struct Population* CreatePopulation(const char* _Name, int _Nutrition, int _Meat, int _Milk, struct Constraint** _Ages, double _MaleRatio);
struct Population* CopyPopulation(const struct Population* _Population);
int PopulationCmp(const void* _One, const void* _Two);
int PopulationFoodCmp(const void* _One, const void* _Two);
void DestroyPopulation(struct Population* _Population);

struct Population* PopulationLoad(lua_State* _State, int _Index);

struct Animal* CreateAnimal(const struct Population* _Pop, int _Age,  int _Nutrition, int _X, int _Y);
int AnimalCmp(const void* _One, const void* _Two);
void DestroyAnimal(struct Animal* _Animal);
/*!
 * Returns a power set that contains all FoodBase*'s that are eaten by Population*'s in _Table.
 */
struct Array* AnimalFoodDep(const struct HashTable* _Table);
struct InputReq** AnimalTypeCount(const struct Array* _Animals, int* _Size);

#endif
