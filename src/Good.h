/*
 * File: Good.h
 * Author: David Brotz
 */

#ifndef __GOOD_H
#define __GOOD_H

#include "sys/LinkedList.h"

#define MELEE_RANGE (0)
#define OUNCE (16)
#define TO_POUND(_Quantity) (_Quantity * OUNCE);
#define FOOD_MAXPARTS (10)
#define CheckGoodTbl(_GoodTbl, _GoodName, _Good, _X, _Y)																						\
{																																				\
	struct GoodBase* _GoodBase = HashSearch(&g_Goods, (_GoodName));																				\
	if(((_Good) = LinearSearch(_GoodBase, (_GoodTbl)->Table, (_GoodTbl)->Size, (int(*)(const void*, const void*))GoodBaseGoodCmp)) == NULL) {	\
		(_Good) = CreateGood(_GoodBase, (_X), (_Y));																							\
		ArrayInsertSort((_GoodTbl), _Good, GoodCmp);																							\
	}																																			\
}

typedef struct lua_State lua_State;
struct HashTable;
struct Object;
struct Good;
struct GoodOutput;

extern char* g_PersonBodyStr[];
extern struct GoodOutput** g_GoodOutputs;
extern int g_GoodOutputsSz;
extern struct Good*(*g_GoodCopy[])(const struct Good*);

enum {
	EFOOD,
	EINGREDIENT,
	EANIMAL,
	ESEED,
	ETOOL,
	EMATERIAL,
	ECLOTHING,
	EFURNITURE,
	EGOOD,
	EWEAPON,
	EARMOR,
	EOTHER
};

enum {
	ETOOL_PLOW = (1 << 0),
	ETOOL_REAP = (1 << 1),
	ETOOL_CUT = (1 << 2),
	ETOOL_LOGGING = (1 << 3)
};

enum {
	EWEAPON_SPEAR,
	EWEAPON_SWORD,
	EWEAPON_JAVELIN,
	EWEAPON_BOW,
};

enum {
	EARMOR_BODY,
	EARMOR_SHIELD
};

struct GoodBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
};

struct GoodMaker {
	struct GoodBase* Maker;
	int Time;	
};

struct GoodOutput {
	struct GoodBase* Output;
	struct GoodMaker** Makers;
};

struct Good {
	int Id;
	int Type;
	int X;
	int Y;
	int(*Think)(struct Object*);
	const struct GoodBase* Base;
	int Quantity; //!Described either as fluid ounces, ounces, or per item.
};

struct WeaponBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
	int WeaponType;
	int MeleeAttack;
	int RangeAttack;
	int Charge;
	int Range;
};

struct ArmorBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
	int ArmorType;
	int Defence;
	int IsShield;
};

struct ToolBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
	int Function;
};

struct FoodBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
	int Nutrition;
};

struct Food {
	int Id;
	int Type;
	int X;
	int Y;
	int(*Think)(struct Object*);
	const struct FoodBase* Base;
	int Quantity;
	int Parts;
};

struct ClothingBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
	int* Locations;
};

/**
 * @Brief struct that contains a list of each Good that is required as well
 * as if the Good is obtainable.
 */
struct GoodDep {
	struct Array* DepTbl; //Contains struct InputReq*. with Req as a GoodDep*.
	const struct GoodBase* Good;
};

int GoodOutputCmp(const void* _One, const void* _Two);

int GoodDepCmp(const struct GoodDep* _One, const struct GoodDep* _Two);
int GoodBaseDepCmp(const struct GoodBase* _Weapon, const struct GoodDep* _Pair);
int InputReqGoodCmp(const struct InputReq* _One, const struct Good* _Two);

struct GoodBase* InitGoodBase(struct GoodBase* _Weapon, const char* _Name, int _Category);
struct GoodBase* CopyGoodBase(const struct GoodBase* _Good);
int GoodBaseCmp(const void* _One, const void* _Two);
void DestroyGoodBase(struct GoodBase* _Weapon);

int GoodInpGdCmp(const void* _One, const void* _Two);

struct Good* GoodCopy(const struct Good* _Good);
struct Good* FoodGoodCopy(const struct Good* _Good);

struct Good* CreateGood(const struct GoodBase* _Base, int _X, int _Y);
int GoodCmp(const void* _One, const void* _Two);
void DestroyGood(struct Good* _Weapon);

int GoodGBaseCmp(const struct Good* _One, const struct GoodBase* _Two);
int GoodBaseGoodCmp(const struct GoodBase* _One, const struct Good* _Two);

struct ToolBase* CreateToolBase(const char* _Name, int _Category, int _Function);
void DestroyToolBase(struct ToolBase* _Tool);

struct FoodBase* CreateFoodBase(const char* _Name, int _Category, int _Nutrition);
void DestroyFoodBase(struct FoodBase* _Food);

struct Food* CreateFood(const struct FoodBase* _Base, int _X, int _Y);
void DestroyFood(struct Food* _Food);

struct ClothingBase* CreateClothingBase(const char* _Name, int _Category);
void DestroyClothingBase(struct ClothingBase* _Clothing);

struct WeaponBase* CreateWeaponBase(const char* _Name, int _Category);
void DestroyWeaponBase(struct WeaponBase* _Weapon);

int GoodThink(struct Good* _Weapon);
/**
 * @Brief Reads a table from _Index from _State that contains data about a Good.
 * @Return NULL if the table is invalid.
 * @Return Good* if the table is valid.
 */
struct GoodBase* GoodLoad(lua_State* _State, int _Index);
int GoodLoadInput(lua_State* _State, struct GoodBase* _Weapon);
int GoodLoadOutput(lua_State* _State, struct GoodBase* _Weapon);
void GoodLoadConsumableInput(lua_State* _State, struct GoodBase* _Weapon, struct LinkedList* _List);
void ClothingBaseLoad(lua_State* _State, struct GoodBase* _Weapon, int* _Locations);
int WeaponBaseLoad(lua_State* _State, struct WeaponBase* _Weapon);
int ArmorBaseLoad(lua_State* _State, struct ArmorBase* _Armor);
struct GoodDep* CreateGoodDep(const struct GoodBase* _Weapon);
void DestroyGoodDep(struct GoodDep* _GoodDep);
/**
 * @Brief Simple wrapper that adds the correct ICallback and SCallback to RBTRee.
 * Creates an RBTree* that contains struct GoodDep* containing a struct GoodBase* and
 * a table of all GoodBases that need the good.
 * The returned RBTree* can be deleted with DestroyRBTree as normal.
 */
struct RBTree* GoodBuildDep(const struct HashTable* _GoodList);
/**
 * @Brief Returns an empty GoodDep*. Every GoodDep* in _Tree that is a prerequisite of _Good
 * Will have this empty GoodDep* added to their DepTbl.
 */
struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct GoodBase* _Weapon);
int GoodNutVal(struct GoodBase* _Base);
/**
 * Creates an array of InputReq* that contain Good and the amount possible to make with the Good*'s in _Goods.
 */
struct InputReq** GoodBuildList(const struct Array* _Goods, int* _Size, int _Categories);
/**
 *  Returns how many objects of _Good that can be made from the items in _Goods.
 *  Returns 0 if none can be created.
 */
int GoodCanMake(const struct GoodBase* _Weapon, const struct Array* _Goods);
struct Good* GoodMostAbundant(struct Array* _Goods, int _Category);
#endif
