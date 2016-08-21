/**
 * Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

#include "Crop.h"
#include "Good.h"
#include "Retinue.h"
#include "BigGuy.h"

#include "sys/LinkedList.h"

#include "video/AABB.h"

#define SETTLEMENT_MINBG (3)
#define SETTLEMENT_SPACE (640 / 4)
#define HARVEST_YEARS (3)
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
struct Retinue;

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
	struct Sprite* Sprite;
	struct Government* Government;
	struct BuyRequest* BuyOrders;
	struct SellRequest* Market;
	int NumPeople;
	int YearDeaths; //Record of deaths in this settlement this year.
	int YearBirths;
	DATE LastRaid;
	uint16_t MaxWarriors;
	uint16_t FreeAcres;
	uint16_t UsedAcres;
	struct BulitinItem* Bulitin;
	/**
	 * Modifier to how many pounds are harvested from each field in this
	 * settlement. Changes every year and is dependant on the previous year.
	 */
	struct LinkedList Families;
	struct LinkedList BigGuys;
	struct LinkedList FreeWarriors;
	struct LinkedList Retinues;
	struct Field Meadow; //Common area that anyone can use to feed their animals.
	uint8_t Stats[BGSKILL_SIZE];
	uint8_t HarvestMod[HARVEST_YEARS];
	uint8_t CurrHarvestYear;
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
void SettlementRemoveFamily(struct Settlement* _Location, struct Family* _Family);
/**
 * Returns 1 if _Location is determed to be non-hostile to _Army.
 * Otherwise returns 0.
 */
int SettlementIsFriendly(const struct Settlement* _Location, struct Army* _Army);
void SettlementGetCenter(const struct Settlement* _Location, SDL_Point* _Pos);
/**
 *\brief Adds a person to the settlement.
 */
void SettlementAddPerson(struct Settlement* _Settlement, struct Person* _Person);
void SettlementRemovePerson(struct Settlement* _Settlement, struct Person* _Person);
/**
 * \return The number of adult men who own a weapon and are capable of fighting.
 */
int SettlementCountWarriors(const struct Settlement* _Settlement);
void TribalCreateBigGuys(struct Settlement* _Settlement, double _CastePercent[CASTE_SIZE]);
int SettlementBigGuyCt(const struct Settlement* _Settlement);
int SettlementAdultPop(const struct Settlement* _Settlement);

/**
 * \return The cumulative nutritional value the people in _Settlement have.
 */
int SettlementGetNutrition(const struct Settlement* _Settlement);
/**
 *	\return How much nutritional value is required in a year by _Settlement.
 */
int SettlementYearlyNutrition(const struct Settlement* _Settlement);
int SettlementCountAcres(const struct Settlement* _Settlement);
int SettlementExpectedYield(const struct Settlement* _Settlement);
float HarvestModifier(const uint8_t (* const _HarvestYears)[HARVEST_YEARS]);
static inline const struct LnkLst_Node* SettlementPlots(const struct Settlement* _Settlement) {
	return NULL;
}
/**
 * \return the first found Plot that is of type _PlotType and contains _PlotData.
 */
struct Plot* SettlementFindPlot(const struct Settlement* _Settlement, int _PlotType, void* _PlotData);
static inline void SettlementAddRetinue(struct Settlement* _Settlement, struct Retinue* _Retinue) {
	LnkLstPushBack(&_Settlement->Retinues, _Retinue);
}

static inline int SettlementAllocAcres(struct Settlement* _Settlement, int _Acres) {
	if(_Settlement->UsedAcres + _Acres > _Settlement->FreeAcres)
		return 0;
	_Settlement->UsedAcres += _Acres;
	_Settlement->FreeAcres -= _Acres;
	return 1;
}
#endif
