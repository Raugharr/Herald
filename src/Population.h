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
	const struct Population* const PopType;
	struct {
		uint8_t Years;
		uint8_t Months;
	} Age;
	int16_t Nutrition;
	uint8_t Gender;
};

struct AnimalDep {
	struct FoodBase** Tbl;
	struct Array Animals;
	uint16_t Nutrition;
	uint8_t TblSz;
};

struct Population* CreatePopulation(const char* Name, int Nutrition, int Meat, int Milk, struct Constraint** Ages,
	 double MaleRatio, int FMRatio, double ReproduceMin, double ReproduceMax, int SpaceReq, uint8_t Wealth);
struct Population* CopyPopulation(const struct Population* Population);
int PopulationCmp(const void* One, const void* Two);
int PopulationFoodCmp(const void* One, const void* Two);
void DestroyPopulation(struct Population* Population);

struct Population* PopulationLoad(lua_State* State, int Index);

struct Animal* CreateAnimal(const struct Population* Pop, uint8_t Years, uint8_t Months, int16_t Nutrition, int X, int Y);
int AnimalCmp(const void* One, const void* Two);
void DestroyAnimal(struct Animal* Animal);
void AnimalObjThink(struct Object* Obj);
void AnimalThink(struct Object* Object);
/*!
 * Returns a power set that contains all FoodBase*'s that are eaten by Population*'s in Table.
 */
struct Array* AnimalFoodDep(const struct HashTable* Table);
struct InputReq** AnimalTypeCount(const struct Array* Animals, int* Size);
void AnimalArrayInsert(struct Array* Array, struct Animal* Animal);
struct Animal* AnimalArrayRemove(struct Array* Array, int Index);

int CountAnimal(const struct Population* PopType, const struct Animal** List, size_t ListSz);
int AnimalsReproduce(const struct Population* Population, int MaleCt, int FemaleCt);
static inline bool AnimalMature(const struct Animal* Animal) {
	return Animal->Age.Years >= Animal->PopType->Ages[AGE_MATURE]->Min;	
}

#endif
