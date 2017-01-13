/*
 * Author: David Brotz
 * File: Family.h
 */

#ifndef __FAMILY_H
#define __FAMILY_H

#include "Herald.h"

#include "sys/Array.h"

#include <stdbool.h>

#define CHILDREN_SIZE (8)
#define FAMILY_PEOPLESZ (CHILDREN_SIZE + 2)
#define FAMILY_BUILDINGCT (2)
#define FAMILY_FIELDCT (2)

#define HUSBAND (0)
#define WIFE (1)
#define CHILDREN (2)

#define FamilyAddAnimal(Family, Animal) AnimalArrayInsert(&(Family)->Animals, (Animal))
#define FamilyTakeAnimal(Family, Animal) AnimalArrayRemove(&(Family)->Animals, (Animal))

/*
 * NOTE: Instead of FamilyThink feeding everyone have each person decide where they will eat from.\
 * To ensure that every person gets a fair share of the family's food if there isnt enough, have the family track how many people will take food from it.
 * By doing this the game will then allow people to be able to eat from other sources such as if a person is in the army or else where.
 * This would also then allow the game to parralise the function where a person eats food.
 */

struct Person;
struct Array;
struct Field;
struct Constraint;
struct FamilyType;
struct Good;
struct Settlement;
struct GameWorld;
typedef struct lua_State lua_State;

enum CasteEnum {
	CASTE_THRALL,
	CASTE_FARMER,
	CASTE_CRAFTSMAN,
	CASTE_LOWNOBLE,
	CASTE_PRIEST,
	CASTE_WARRIOR,
	CASTE_NOBLE,
	CASTE_SIZE
};

enum CasteGroupEnum {
	CASTE_GSLAVE,
	CASTE_GCOMMON,
	CASTE_GNOBLE
};

struct CasteGoodReq {
	struct GoodBase* Base;
	struct Good* GoodPtr; //If the family's good array contains Base GoodPtr points to the Good* in the array.
	uint32_t MinQuantity;
};

struct Family {
	struct Object Object;
	const char* Name;
	struct Person* People[FAMILY_PEOPLESZ];
	struct Settlement* HomeLoc;
	struct Family* Parent;
	const struct Family* Owner; //Used if Caste is CASTE_SERF.
	const struct Profession* Profession;
	//FIXME: We should assume that a person will only have at most 2 fields.
	// By replacing the array with a static one we should be able to save space.
	struct Field* Fields[FAMILY_FIELDCT];
	struct Building* Buildings[FAMILY_BUILDINGCT];
	struct Array Goods;
	struct Array Animals;
	struct {
		int16_t SlowSpoiled;
		int16_t FastSpoiled;
		int16_t AnimalFood; //Accounts for all nutrition that is assumed to be grown from wild plants on the farmers fallow fields. Since it is not harvested
		// this variable should be set to 0 at harvest time.
	} Food;
	uint8_t NumChildren;
	uint8_t FieldCt;
	uint8_t BuildingCt;
	uint8_t Caste;
	uint8_t Faction;
	bool IsAlive;
	/*union {
		struct {
			struct Field* Fields[FAMILY_FIELDCT];
		} Commoner;
		struct {
			struct Family* Owner;
		} Slave;
	}*/
};

//FIXME: Remove Family_Init and Family_Quit as they are not specifically related to Family.h and should be moved somewhere else that initializes data.
void Family_Init(struct Array* Array);
void Family_Quit();
struct Family* CreateFamily(const char* Name, struct Settlement* Location, struct Family* Parent);
struct Family* CreateRandFamily(const char* Name, int Size, struct Family* Parent, struct Constraint * const * const AgeGroups, 
	struct Constraint * const * const BabyAvg, struct Settlement* Location, uint8_t Caste);
void DestroyFamily(struct Family* Family);
struct Food* FamilyMakeFood(struct Family* Family);
void FamilyWorkField(struct Family* Family);
void FamilyObjThink(struct Object* Obj);
void FamilyThink(struct Object* Obj);
/**
 * Returns how many people are in Family.
 */
int FamilySize(const struct Family* Family);
/**
 * Creates a new family that contains Male and Female as the husband and wife.
 */
void Marry(struct Person* Male, struct Person* Female);
/**
 * NOTE: Family is not fully created yet and has no people in it, thus FamilySize is needed
 */
void FamilyAddGoods(struct Family* Family, int FamilySize, lua_State* State, struct FamilyType** FamilyTypes, struct Settlement* Location);
int FamilyNutReq(const struct Family* Family);
/**
 * Returns the amount of nutrition the family currently has.
 */
int FamilyGetNutrition(const struct Family* Family);
struct Settlement* FamilyGetSettlement(struct Family* Family);
/*
 * Returns how many acres Family owns.
 */
int FamilyCountAcres(const struct Family* Family);
/*
 *	Returns how many nutritional units the family is expected to harvest from their fields.
 */
int FamilyExpectedYield(const struct Family* Family);

/*
 * FIXME: Merge with CountAnimalTypes found in Population.h
 */
int FamilyCountAnimalTypes(const struct Family* Family);
void FamilySlaughterAnimals(struct Family* Family);
void FamilyShearAnimals(struct Family* Family);
int FamilyWorkModifier(const struct Family* Family);
int FamilyCanMake(const struct Family* Family, const struct GoodBase* Good);
int FamilyGetWealth(const struct Family* Family);
static inline int StoredFoodSufficient(const struct Family* Family) {
	uint32_t FoodOwned = FamilyGetNutrition(Family);
	uint32_t FoodReq = FamilyNutReq(Family);
	
	if((FoodOwned / FoodReq) >= (YEAR_DAYS + (YEAR_DAYS / 2)))
		return 1;
	return 0;
}

void FamilyRemovePerson(struct Family* Family, struct Person* Person);
bool FamilyAddPerson(struct Family* Family, struct Person* Person);

void CreateFarmerFamilies(struct GameWorld* World, struct Settlement* Settlement, struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg);
void CreateWarriorFamilies(struct Settlement* Settlement, struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg);
#endif

