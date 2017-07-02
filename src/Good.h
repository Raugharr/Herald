/*
 * File: Good.h
 * Author: David Brotz
 */

#ifndef __GOOD_H
#define __GOOD_H

#include "sys/LinkedList.h"
#include "sys/HashTable.h"
#include "sys/Array.h"

#include "Date.h"

#include <SDL2/SDL.h>

#define MELEE_RANGE (0)
#define OUNCE (16)
#define BUSHEL (60)
#define GALLON (128)
#define GoodType(Good) (ffs((Good)->Category) - 1)
#define ToPound(Quantity) ((Quantity) / OUNCE)
#define ToOunce(Quantity) ((Quantity) * OUNCE)
#define ToBushel(Quantity) ((Quantity) / BUSHEL)
#define ToGallon(Quantity) ((Quantity) / GALLON)
#define FOOD_MAXPARTS (10)
#define GoodNutrition(Good) (ToPound((Good)->Quantity) * ((struct FoodBase*)(Good)->Base)->Nutrition)

typedef struct lua_State lua_State;
struct HashTable;
struct Object;
struct Good;
struct GoodOutput;
struct Settlement;

extern struct Good*(*g_GoodCopy[])(const struct Good*);

enum {
	GOOD_FOOD = (1 << 0),
	GOOD_INGREDIENT = (1 << 1),
	GOOD_ANIMAL = (1 << 2),
	GOOD_SEED = (1 << 3),
	GOOD_TOOL = (1 << 4),
	GOOD_MATERIAL = (1 << 5),
	GOOD_WEAPON = (1 << 6),
	GOOD_ARMOR = (1 << 7),
	GOOD_OTHER = (1 << 8),
	GOOD_FLOUR = (1 << 9),
	GOOD_SIZE = 10
};

enum {
	ETOOL_PLOW = (1 << 0),
	ETOOL_REAP = (1 << 1),
	ETOOL_CUT = (1 << 2),
	ETOOL_LOGGING = (1 << 3)
};

enum {
	EWEAPON_SEAX,
	EWEAPON_SPEAR,
	EWEAPON_SWORD,
	EWEAPON_JAVELIN,
	EWEAPON_BOW,
};

enum {
	EARMOR_BODY,
	EARMOR_SHIELD
};

enum EQuantity {
	QUANTITY_OUNCE,
	QUANTITY_POUND,
	OUANTITY_FOUNCE,
	OUANTITY_GALLON,
	QUANTITY_BUSHEL
};

struct GoodQuantity {
	uint16_t Quantity;
	uint8_t Type;	
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
	uint32_t Id;
	char* Name;
	struct InputReq** InputGoods;
	uint32_t Category;
	uint32_t Cost; //Value of good.
	uint8_t IGSize; //InputGood size.
};

struct Good {
	uint32_t Id;
	int32_t Quantity; //!Described either as fluid ounces, ounces, or per item.
	const struct GoodBase* Base;
};

struct WeaponBase {
	struct GoodBase Base;
	int8_t WeaponType;
	int8_t  MeleeAttack;
	int8_t RangeAttack;
	int8_t Range;
};

struct ArmorBase {
	struct GoodBase Base;
	uint8_t ArmorType;
	uint8_t Defence;
};

struct ToolBase {
	struct GoodBase Base;
	uint8_t Function;
	uint8_t Quality;
};

struct FoodBase {
	struct GoodBase Base;
	double Nutrition;
};

struct Food {
	uint32_t Id;
	int32_t Quantity;
	const struct FoodBase* Base;
	int Parts;
};

/**
 * @Brief struct that contains a list of each Good that is required as well
 * as if the Good is obtainable.
 */
struct GoodDep {
	struct Array* DepTbl; //Contains struct InputReq*. with Req as a GoodDep*.
	const struct GoodBase* Good;
};

int GoodDepCmp(const struct GoodDep* One, const struct GoodDep* Two);
int GoodBaseDepCmp(const struct GoodBase* Weapon, const struct GoodDep* Pair);
int InputReqGoodCmp(const struct InputReq* One, const struct Good* Two);

struct GoodBase* CreateGoodBase(struct GoodBase* Weapon, const char* Name, int Category);
struct GoodBase* CopyGoodBase(const struct GoodBase* Good);
int GoodBaseCmp(const void* One, const void* Two);
void DestroyGoodBase(struct GoodBase* Weapon);

struct BuyRequest* CreateBuyRequest(struct Family* Family, const struct GoodBase* Base, int Quantity);
void DestroyBuyRequest(struct BuyRequest* BuyReq);

int GoodInpGdCmp(const void* One, const void* Two);

struct Good* GoodCopy(const struct Good* Good);
struct Good* FoodGoodCopy(const struct Good* Good);

struct Good* CreateGood(const struct GoodBase* Base);
int GoodCmp(const void* One, const void* Two);
void DestroyGood(struct Good* Weapon);

int GoodGBaseCmp(const struct GoodBase* One, const struct Good* Two);
int GoodBaseGoodCmp(const struct GoodBase* One, const struct Good* Two);

struct ToolBase* CreateToolBase(const char* Name, int Category, int Function, int Quality);
void DestroyToolBase(struct ToolBase* Tool);

struct FoodBase* CreateFoodBase(const char* Name, int Category, int Nutrition);
void DestroyFoodBase(struct FoodBase* Food);

struct Food* CreateFood(const struct FoodBase* Base);
void DestroyFood(struct Food* Food);

struct ClothingBase* CreateClothingBase(const char* Name, int Category);
void DestroyClothingBase(struct ClothingBase* Clothing);

struct WeaponBase* CreateWeaponBase(const char* Name, int Category);
void DestroyWeaponBase(struct WeaponBase* Weapon);

/**
 * @Brief Reads a table from Index from State that contains data about a Good.
 * @Return NULL if the table is invalid.
 * @Return Good* if the table is valid.
 */
struct GoodBase* GoodLoad(lua_State* State, int Index);
int GoodLoadInput(lua_State* State, struct GoodBase* Weapon);
int GoodLoadOutput(lua_State* State, struct GoodBase* Weapon);
void GoodLoadConsumableInput(lua_State* State, struct GoodBase* Weapon, struct LinkedList* List);
void ClothingBaseLoad(lua_State* State, struct GoodBase* Weapon, int* Locations);
int WeaponBaseLoad(lua_State* State, struct WeaponBase* Weapon);
int ArmorBaseLoad(lua_State* State, struct ArmorBase* Armor);
struct GoodDep* CreateGoodDep(const struct GoodBase* Weapon);
void DestroyGoodDep(struct GoodDep* GoodDep);
/**
 * @Brief Simple wrapper that adds the correct ICallback and SCallback to RBTRee.
 * Creates an RBTree* that contains struct GoodDep* containing a struct GoodBase* and
 * a table of all GoodBases that need the good.
 * The returned RBTree* can be deleted with DestroyRBTree as normal.
 */
struct RBTree* GoodBuildDep(const struct HashTable* GoodList);

/**
 * Iterates through all of Good's input goods and searches Tree for them. If they are not found a GoodDep* is created
 * and a GoodDep* containing Good is added to its DepTbl. Each input good will then have GoodDependencies called on them.
 * A GoodDep* containing Good is returned.
 */
struct GoodDep* GoodDependencies(struct RBTree* Tree, const struct GoodBase* Good);
/**
 * Returns a double containing the value of Base's nutritional value by adding the
 * sum of the nutritional values of the goods used to create Base together.
 */
double GoodNutVal(struct GoodBase* Base);
/**
 * Creates an array of InputReq* that contain Good and the amount possible to make with the Good*'s in Goods.
 */
struct InputReq** GoodBuildList(const struct Array* Goods, int* Size, int Categories);
/**
 *  Returns how many objects of Good that can be made from the items in Goods.
 *  Returns 0 if none can be created.
 */
int GoodCanMake(const struct GoodBase* Array, const struct Array* Goods);
/**
 * Returns the good that has the most quantity that is in the category Category.
 */
struct Good* GoodMostAbundant(struct Array* Goods, int Category);
/**
 * Checks if Seller already has a SellRequest of type Base at their settlement if
 * they do add Quantity to the SellRequest. Otherwise a new SellRequest is created
 * at the settlement.
 */
void GoodSell(const struct Family* Seller, const struct GoodBase* Base, int Quantity);
/**
 * Checks for a SellRequest that contains Base. If such a SellRequest exists then attempt
 * to buy Quantity amount. If the SellRequest does not have enough items to statisfy Quantity
 * GoodBuy will continue iterating through all SellRequests to find another valid SellRequest.
 * If Quantity amount of goods are not able to be bought then a BuyRequest will be created for
 * the remaining amount.
 */
int GoodBuy(struct Family* Family, const struct GoodBase* Base, int Quantity);
int GoodPay(struct Family* Buyer, const struct SellRequest* SellReq);
const struct GoodBase* GoodPayInKind(const struct Family* Buyer, int Cost, const struct GoodBase* PayGood, int* Quantity);
void SellItem(struct Family* Buyer, const struct SellRequest* SellReq);
int GoodGetValue(const struct GoodBase* Base);
struct Good* GoodMake(const struct GoodBase* Base, int Quantity, struct Array* Inputs);
const struct LinkedList* GoodGetCategory(const char* Category);
/*
 * Takes Quantity amount from Good and inserts it into Family's Good array, creating a new good if Family's
 * good array does not contain the good. If Quantity is equal to Good->Quantity Good is destroyed.
 */
void ArrayAddGood(struct Array* GoodList, struct Good* Good, uint32_t Quantity);
/*
 * Takes Quantity from Index in Family's good array.
 */
struct Good* ArrayRemoveGood(struct Array* GoodList, uint32_t Index, uint32_t Quantity);
/**
 * Returns the yearly requirement of nutrition needed to feed the people in the family Family.
 */

static inline struct Good* CheckGoodTbl(struct Array* Array, const char* GoodName, const struct HashTable* Search) {
	const struct GoodBase* GoodBase = HashSearch(Search, GoodName);
	struct Good* Good = NULL;

	if((Good = LinearSearch(GoodBase, Array->Table, Array->Size, (CompCallback) GoodBaseGoodCmp)) == NULL) {
		Good = CreateGood(GoodBase);
		ArrayInsertSort(Array, Good, GoodCmp);
	}
	return Good;
}
#endif
