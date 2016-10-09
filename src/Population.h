/*
 * File: Population.h
 * Author: David Brotz
 */

#ifndef __POPULATION_H
#define __POPULATION_H

#include "World.h"
#include "Family.h"

#include "sys/Constraint.h"
#include "sys/LinkedList.h"

#include <stdbool.h>
#include <inttypes.h>

#define AGE_MATURE (0)
#define AGE_DEATH (1)
#define MILK_NUT (3)

typedef struct lua_State lua_State;
struct HashTable;

struct Population {
	uint32_t Id;
	char* Name;
	double MaleRatio; //Precentage of children that will be male.
	uint32_t Meat; //How much meat  is generated in pounds when the animal is slaughtered.
	uint32_t Milk; //Fluid ounces per day.
	uint32_t FMRatio; //How many females a male can reproduce with per season.
	uint16_t GestationTime; //How many days it takes to birth a child.
	uint16_t MaxNutrition;
	struct Constraint** Ages;
	struct {
		uint16_t Min;
		uint16_t Max;	
	} ReproduceRate; 
	struct Good** Outputs; //Contains struct Good*.
	struct FoodBase** Eats;
	struct {
		const struct GoodBase* Skin;
		double Pounds;
	} Skin;

	struct {
		const struct GoodBase* Hair;
		double Pounds;
		uint8_t Shearable;
	} Hair;
	uint8_t EatsSize;
	uint8_t SpaceReq;
	uint8_t Wealth; //How much wealth an animal is worth 100 is equal to 1 wealth.
	uint8_t Nutrition; //How much is eaten per day.
};

struct Animal {
	struct Object Object;
	SDL_Point Pos;
	DATE Age;
	const struct Population* const PopType;
	int16_t Nutrition;
	uint16_t Gender;
};

struct AnimalDep {
	struct FoodBase** Tbl;
	struct Array* Animals;
	int Nutrition;
};

struct Population* CreatePopulation(const char* _Name, int _Nutrition, int _Meat, int _Milk, struct Constraint** _Ages,
	 double _MaleRatio, int _FMRatio, double _ReproduceMin, double _ReproduceMax, int _SpaceReq, uint8_t _Wealth);
struct Population* CopyPopulation(const struct Population* _Population);
int PopulationCmp(const void* _One, const void* _Two);
int PopulationFoodCmp(const void* _One, const void* _Two);
void DestroyPopulation(struct Population* _Population);

struct Population* PopulationLoad(lua_State* _State, int _Index);

struct Animal* CreateAnimal(const struct Population* _Pop, int _Age,  int _Nutrition, int _X, int _Y);
int AnimalCmp(const void* _One, const void* _Two);
void DestroyAnimal(struct Animal* _Animal);
void AnimalThink(struct Object* _Object);
/*!
 * Returns a power set that contains all FoodBase*'s that are eaten by Population*'s in _Table.
 */
struct Array* AnimalFoodDep(const struct HashTable* _Table);
struct InputReq** AnimalTypeCount(const struct Array* _Animals, int* _Size);
void AnimalArrayInsert(struct Array* _Array, struct Animal* _Animal);
struct Animal* AnimalArrayRemove(struct Array* _Array, int _Index);

int CountAnimal(const struct Population* _PopType, const struct Animal** _List, size_t _ListSz);
int AnimalsReproduce(const struct Population* _Population, int _MaleCt, int _FemaleCt);
static inline bool AnimalMature(const struct Animal* _Animal) {
	return _Animal->Age >= _Animal->PopType->Ages[AGE_MATURE]->Min;	
}

#endif
