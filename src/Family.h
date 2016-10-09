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

#define FamilyAddAnimal(_Family, _Animal) AnimalArrayInsert(&(_Family)->Animals, (_Animal))
#define FamilyTakeAnimal(_Family, _Animal) AnimalArrayRemove(&(_Family)->Animals, (_Animal))

struct Person;
struct Array;
struct Field;
struct Constraint;
struct FamilyType;
struct Good;
struct Settlement;
typedef struct lua_State lua_State;

enum {
	CASTE_THRALL,
	CASTE_LOWCLASS,
	CASTE_HIGHCLASS,
	CASTE_WARRIOR,
	CASTE_NOBLE,
	CASTE_SIZE
};

struct CasteGoodReq {
	struct GoodBase* Base;
	struct Good* GoodPtr; //If the family's good array contains Base GoodPtr points to the Good* in the array.
	int MinQuantity;
};

struct Caste {
	int Type;
	struct LinkedList JobList; //List of jobs this caste should perform and when.
	struct Behavior* Behavior;
};

struct Family {
	struct Object Object;
	const char* Name;
	struct Person* People[FAMILY_PEOPLESZ];
	struct Settlement* HomeLoc;
	struct Family* Parent;
	const struct Family* Owner; //Used if Caste is CASTE_SERF.
	const struct Profession* Profession;
	const struct Caste* Caste;
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
	bool IsAlive;
};

//FIXME: Remove Family_Init and Family_Quit as they are not specifically related to Family.h and should be moved somewhere else that initializes data.
void Family_Init(struct Array* _Array);
void Family_Quit();
struct Family* CreateFamily(const char* _Name, struct Settlement* _Location, struct Family* _Parent);
struct Family* CreateRandFamily(const char* _Name, int _Size, struct Family* _Parent, struct Constraint * const * const _AgeGroups, 
	struct Constraint * const * const _BabyAvg, int _X, int _Y, struct Settlement* _Location, struct FamilyType** _FamilyTypes, const struct Caste* _Caste);
void DestroyFamily(struct Family* _Family);
struct Food* FamilyMakeFood(struct Family* _Family);
void FamilyWorkField(struct Family* _Family);
void FamilyObjThink(struct Object* _Obj);
void FamilyThink(struct Object* _Obj);
/**
 * Returns how many people are in _Family.
 */
int FamilySize(const struct Family* _Family);
/**
 * Creates a new family that contains _Male and _Female as the husband and wife.
 */
void Marry(struct Person* _Male, struct Person* _Female);
/**
 * NOTE: _Family is not fully created yet and has no people in it, thus _FamilySize is needed
 */
void FamilyAddGoods(struct Family* _Family, int _FamilySize, lua_State* _State, struct FamilyType** _FamilyTypes, struct Settlement* _Location);
/*
 * Takes _Quantity amount from _Good and inserts it into _Family's Good array, creating a new good if _Family's
 * good array does not contain the good. If _Quantity is equal to _Good->Quantity _Good is destroyed.
 */
void FamilyGetGood(struct Family* _Family, struct Good* _Good, int _Quantity);
/*
 * Takes _Quantity from _Index in _Family's good array.
 */
struct Good* FamilyTakeGood(struct Family* _Family, int _Index, int _Quantity);
/**
 * Returns the yearly requirement of nutrition needed to feed the people in the family _Family.
 */
int FamilyNutReq(const struct Family* _Family);
/**
 * Returns the amount of nutrition the family currently has.
 */
int FamilyGetNutrition(const struct Family* _Family);
struct Settlement* FamilyGetSettlement(struct Family* _Family);
/*
 * Returns how many acres _Family owns.
 */
int FamilyCountAcres(const struct Family* _Family);
/*
 *	Returns how many nutritional units the family is expected to harvest from their fields.
 */
int FamilyExpectedYield(const struct Family* _Family);

/*
 * FIXME: Merge with CountAnimalTypes found in Population.h
 */
int FamilyCountAnimalTypes(const struct Family* _Family);
void FamilySlaughterAnimals(struct Family* _Family);
void FamilyShearAnimals(struct Family* _Family);
int FamilyWorkModifier(const struct Family* _Family);
int FamilyCanMake(const struct Family* _Family, const struct GoodBase* _Good);
int FamilyGetWealth(const struct Family* _Family);
static inline int StoredFoodSufficient(const struct Family* _Family) {
	uint32_t _FoodOwned = FamilyGetNutrition(_Family);
	uint32_t _FoodReq = FamilyNutReq(_Family);
	
	if((_FoodOwned / _FoodReq) >= (YEAR_DAYS + (YEAR_DAYS / 2)))
		return 1;
	return 0;
}

#endif

