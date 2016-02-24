/*
 * Author: David Brotz
 * File: Family.h
 */

#ifndef __FAMILY_H
#define __FAMILY_H

#include "Herald.h"

#include "Population.h"

#define CHILDREN_SIZE (8)
#define FAMILY_PEOPLESZ (CHILDREN_SIZE + 2)

#define HUSBAND (0)
#define WIFE (1)
#define CHILDREN (2)

#define FamilyAddAnimal(_Family, _Animal) AnimalArrayInsert((_Family)->Animals, (_Animal))
#define FamilyTakeAnimal(_Family, _Animal) AnimalArrayRemove((_Family)->Animals, (_Animal))

struct Person;
struct Array;
struct Field;
struct Constraint;
struct FamilyType;
struct Good;
struct Settlement;
typedef struct lua_State lua_State;

enum {
	CASTE_SERF,
	CASTE_PEASANT,
	CASTE_CRAFTSMAN,
	CASTE_WARRIOR,
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
	int Id;
	int Type;
	ObjectThink Think;
	/*
	 * LastThink is not used by Object and should be removed.
	 */
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	int NumChildren;
	const char* Name;
	struct Person* People[FAMILY_PEOPLESZ];
	struct Array* Fields; //Used for if Caste is CASTE_FARMER.
	struct Array* Buildings;
	struct Array* Goods;
	struct Array* Animals;
	struct Settlement* HomeLoc;
	struct Family* Parent;
	const struct Family* Owner; //Used if Caste is CASTE_SERF.
	struct Profession* Profession;
	struct Caste* Caste;
};

//FIXME: Remove Family_Init and Family_Quit as they are not specifically related to Family.h and should be moved somewhere else that initializes data.
void Family_Init(struct Array* _Array);
void Family_Quit();
struct Family* CreateFamily(const char* _Name, struct Settlement* _Location, struct Family* _Parent);
struct Family* CreateRandFamily(const char* _Name, int _Size, struct Family* _Parent, struct Constraint** _AgeGroups, struct Constraint** _BabyAvg, int _X, int _Y, struct Settlement* _Location);
void DestroyFamily(struct Family* _Family);
struct Food* FamilyMakeFood(struct Family* _Family);
void FamilyWorkField(struct Family* _Family);
int FamilyThink(struct Family* _Family);
/**
 * Returns how many people are in _Family.
 */
int FamilySize(const struct Family* _Family);
/**
 * Creates a new family that contains _Male and _Female as the husband and wife.
 */
void Marry(struct Person* _Male, struct Person* _Female);
/**
 * REFACTOR: This function shouldn't be accessing Lua tables. The tables should instead be loaded into a struct and the references given to this function.
 */
void FamilyAddGoods(struct Family* _Family, lua_State* _State, struct FamilyType** _FamilyTypes, int _X, int _Y, struct Settlement* _Location);
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
int FamilyCountAnimalTypes(struct Family* _Family);
void FamilySlaughterAnimals(struct Family* _Family);
void FamilyShearAnimals(struct Family* _Family);
int FamilyWorkModifier(const struct Family* _Family);
int FamilyCanMake(const struct Family* _Family, const struct GoodBase* _Good);

#endif

