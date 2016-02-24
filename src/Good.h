/*
 * File: Good.h
 * Author: David Brotz
 */

#ifndef __GOOD_H
#define __GOOD_H

#include "sys/LinkedList.h"
#include "Date.h"

#include <SDL2/SDL.h>

#define MELEE_RANGE (0)
#define OUNCE (16)
#define TO_POUND(_Quantity) (_Quantity * OUNCE);
#define FOOD_MAXPARTS (10)
#define CheckGoodTbl(_GoodTbl, _GoodName, _Good, _X, _Y)																						\
{																																				\
	struct GoodBase* _GoodBase = HashSearch(&g_Goods, (_GoodName));																				\
	if(((_Good) = LinearSearch(_GoodBase, (_GoodTbl)->Table, (_GoodTbl)->Size, (CompCallback) GoodBaseGoodCmp)) == NULL) {						\
		(_Good) = CreateGood(_GoodBase, (_X), (_Y));																							\
		ArrayInsertSort((_GoodTbl), _Good, GoodCmp);																				\
	}																																			\
}

typedef struct lua_State lua_State;
struct HashTable;
struct Object;
struct Good;
struct GoodOutput;
struct Settlement;

extern char* g_PersonBodyStr[];
extern struct Good*(*g_GoodCopy[])(const struct Good*);

enum {
	GOOD_FOOD,
	GOOD_INGREDIENT,
	GOOD_ANIMAL,
	GOOD_SEED,
	GOOD_TOOL,
	GOOD_MATERIAL,
	GOOD_CLOTHING,
	GOOD_WEAPON,
	GOOD_ARMOR,
	GOOD_OTHER,
	GOOD_SIZE
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

struct BuyRequest {
	DATE Time;
	const struct GoodBase* Base;
	struct Family* Owner;
	int Quantity;
	struct BuyRequest* Next;
	struct BuyRequest* Prev;
};

struct SellRequest {
	const struct GoodBase* Base;
	const struct Family* Owner;
	int Quantity;
	int Cost;
	struct SellRequest* Next;
	struct SellRequest* Prev;
};

struct GoodBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize; //InputGood size.
};

struct Good {
	int Id;
	SDL_Point Pos;
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
	int Quality;
};

struct FoodBase {
	int Id;
	char* Name;
	int Category;
	struct InputReq** InputGoods;
	int IGSize;
	double Nutrition;
};

struct Food {
	int Id;
	SDL_Point Pos;
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

int GoodDepCmp(const struct GoodDep* _One, const struct GoodDep* _Two);
int GoodBaseDepCmp(const struct GoodBase* _Weapon, const struct GoodDep* _Pair);
int InputReqGoodCmp(const struct InputReq* _One, const struct Good* _Two);

struct GoodBase* InitGoodBase(struct GoodBase* _Weapon, const char* _Name, int _Category);
struct GoodBase* CopyGoodBase(const struct GoodBase* _Good);
int GoodBaseCmp(const void* _One, const void* _Two);
void DestroyGoodBase(struct GoodBase* _Weapon);

struct BuyRequest* CreateBuyRequest(struct Family* _Family, const struct GoodBase* _Base, int _Quantity);
void DestroyBuyRequest(struct BuyRequest* _BuyReq);

int GoodInpGdCmp(const void* _One, const void* _Two);

struct Good* GoodCopy(const struct Good* _Good);
struct Good* FoodGoodCopy(const struct Good* _Good);

struct Good* CreateGood(const struct GoodBase* _Base, int _X, int _Y);
int GoodCmp(const void* _One, const void* _Two);
void DestroyGood(struct Good* _Weapon);

int GoodGBaseCmp(const struct GoodBase* _One, const struct Good* _Two);
int GoodBaseGoodCmp(const struct GoodBase* _One, const struct Good* _Two);

struct ToolBase* CreateToolBase(const char* _Name, int _Category, int _Function, int _Quality);
void DestroyToolBase(struct ToolBase* _Tool);

struct FoodBase* CreateFoodBase(const char* _Name, int _Category, int _Nutrition);
void DestroyFoodBase(struct FoodBase* _Food);

struct Food* CreateFood(const struct FoodBase* _Base, int _X, int _Y);
void DestroyFood(struct Food* _Food);

struct ClothingBase* CreateClothingBase(const char* _Name, int _Category);
void DestroyClothingBase(struct ClothingBase* _Clothing);

struct WeaponBase* CreateWeaponBase(const char* _Name, int _Category);
void DestroyWeaponBase(struct WeaponBase* _Weapon);

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
 * Iterates through all of _Good's input goods and searches _Tree for them. If they are not found a GoodDep* is created
 * and a GoodDep* containing _Good is added to its DepTbl. Each input good will then have GoodDependencies called on them.
 * A GoodDep* containing _Good is returned.
 */
struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct GoodBase* _Good);
/**
 * Returns a double containing the value of _Base's nutritional value by adding the
 * sum of the nutritional values of the goods used to create _Base together.
 */
double GoodNutVal(struct GoodBase* _Base);
/**
 * Creates an array of InputReq* that contain Good and the amount possible to make with the Good*'s in _Goods.
 */
struct InputReq** GoodBuildList(const struct Array* _Goods, int* _Size, int _Categories);
/**
 *  Returns how many objects of _Good that can be made from the items in _Goods.
 *  Returns 0 if none can be created.
 */
int GoodCanMake(const struct GoodBase* _Array, const struct Array* _Goods);
/**
 * Returns the good that has the most quantity that is in the category _Category.
 */
struct Good* GoodMostAbundant(struct Array* _Goods, int _Category);
/**
 * Checks if _Seller already has a SellRequest of type _Base at their settlement if
 * they do add _Quantity to the SellRequest. Otherwise a new SellRequest is created
 * at the settlement.
 */
void GoodSell(const struct Family* _Seller, const struct GoodBase* _Base, int _Quantity);
/**
 * Checks for a SellRequest that contains _Base. If such a SellRequest exists then attempt
 * to buy _Quantity amount. If the SellRequest does not have enough items to statisfy _Quantity
 * GoodBuy will continue iterating through all SellRequests to find another valid SellRequest.
 * If _Quantity amount of goods are not able to be bought then a BuyRequest will be created for
 * the remaining amount.
 */
int GoodBuy(struct Family* _Family, const struct GoodBase* _Base, int _Quantity);
int GoodPay(struct Family* _Buyer, const struct SellRequest* _SellReq);
const struct GoodBase* GoodPayInKind(const struct Family* _Buyer, int _Cost, const struct GoodBase* _PayGood, int* _Quantity);
void SellItem(struct Family* _Buyer, const struct SellRequest* _SellReq);
int GoodGetValue(const struct GoodBase* _Base);
struct Good* GoodMake(const struct GoodBase* _Base, int _Quantity, struct Array* _Inputs, int _X, int _Y);
const struct LinkedList* GoodGetCategory(const char* _Category);
#endif
