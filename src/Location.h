/* Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

#include "Crop.h"
#include "Good.h"

#include "sys/LinkedList.h"

#include "video/AABB.h"

#define SETTLEMENT_MINBG (3)
#define SettlementRaiseFyrd(_Settlement, _ArmyGoal) CreateArmy((_Settlement), &(_Settlement)->Pos, (_Settlement)->Government, (_Settlement)->Government->Leader, (_ArmyGoal))

struct BigGuy;
struct Family;
struct Government;
struct MapRenderer;
struct Settlement;
struct Army;
struct ArmyGoal;
struct Tile;
struct Object;
struct BulitinItem;

enum {
	ELOC_SETTLEMENT,
	ELOC_FOREST
};

struct Location {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	int LocType;
	SDL_Rect Pos;
};

struct Settlement {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	int LocType;
	SDL_Point Pos;
	char* Name;
	struct Person* People;
	struct LinkedList BigGuys;
	struct Sprite* Sprite;
	struct Government* Government;
	int NumPeople;
	int YearDeaths; //Record of deaths in this settlement this year.
	int YearBirths;
	/**
	 * Modifier to how many pounds are harvested from each field in this
	 * settlement. Changes every year and is dependant on the previous year.
	 */
	float HarvestMod;
	struct LinkedList Families;
	struct Field Meadow; //Common area that anyone can use to feed their animals.
	struct BuyRequest* BuyOrders;
	struct SellRequest* Market;
	int Glory;
	struct BulitinItem* Bulitin;
	DATE LastRaid;
};

void LocationGetPoint(const struct Location* _Location, SDL_Point* _Point);

struct Settlement* CreateSettlement(int _X, int _Y, const char* _Name, int _GovType);
void DestroySettlement(struct Settlement* _Location);

void SettlementThink(struct Settlement* _Settlement);
void SettlementDraw(const struct MapRenderer* _Renderer, struct Settlement* _Settlement);

/**
 * Adds a family to the settlement.
 */
void SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family);
/**
 * Returns 1 if _Location is determed to be non-hostile to _Army.
 * Otherwise returns 0.
 */
int SettlementIsFriendly(const struct Settlement* _Location, struct Army* _Army);
void SettlementGetCenter(const struct Settlement* _Location, SDL_Point* _Pos);
void SettlementAddPerson(struct Settlement* _Settlement, struct Person* _Person);
void SettlementRemovePerson(struct Settlement* _Settlement, struct Person* _Person);
/**
 * Counts how many adult men who own a weapon and are capable of fighting.
 */
int SettlementCountWarriors(const struct Settlement* _Settlement);
void TribalCreateBigGuys(struct Settlement* _Settlement);
int SettlementBigGuyCt(const struct Settlement* _Settlement);
int SettlementAdultPop(const struct Settlement* _Settlement);

/**
 * Returns the cumulative nutritional value the people in _Settlement have.
 */
int SettlementGetNutrition(const struct Settlement* _Settlement);
int SettlementYearlyNutrition(const struct Settlement* _Settlement);
int SettlementCountAcres(const struct Settlement* _Settlement);
int SettlementExpectedYield(const struct Settlement* _Settlement);
#endif
