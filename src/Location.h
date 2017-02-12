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
#include "Faction.h"

#include "sys/LinkedList.h"

#include "video/AABB.h"

#define SETTLEMENT_MINBG (3)
#define MILE_ACRE (640)
#define SETTLEMENT_SPACE (MILE_ACRE / 3)
#define HARVEST_YEARS (3)
#define SettlementRaiseFyrd(Settlement, ArmyGoal) CreateArmy((Settlement), (Settlement)->Government->Leader, (ArmyGoal))

struct BigGuy;
struct Family;
struct Government;
struct MapRenderer;
struct Settlement;
struct Army;
struct ArmyGoal;
struct Tile;
struct Object;
struct BulletinItem;
struct Retinue;

struct Settlement {
	struct Object Object;
	SDL_Point Pos;
	char* Name;
	//Look into replacing this with an array as People should not be frequently leaving/joining a settlement.
	struct Person* People;
	struct Sprite* Sprite;
	struct Government* Government;
	struct BuyRequest* BuyOrders;
	struct SellRequest* Market;
	struct BulletinItem* Bulletin;
	struct Retinue* Retinues; 
	struct Faction Factions;
	struct LinkedList Families; // List of struct Family*
	struct LinkedList BigGuys; // List of struct BigGuy*
	struct LinkedList FreeWarriors; //List of struct Person*
	struct Field Meadow; //Common area that anyone can use to feed their animals.
	DATE LastRaid;
	uint16_t NumPeople;
	//Migrants are not accounted for.
	uint16_t YearDeaths; //Record of deaths in this settlement this year.
	uint16_t YearBirths;
	uint16_t AdultMen;
	uint16_t AdultWomen;
	uint16_t MaxWarriors;
	uint16_t FreeAcres;
	uint16_t UsedAcres;
	uint16_t StarvingFamilies;
	uint16_t CasteCount[CASTE_SIZE];
	uint8_t Stats[BGSKILL_SIZE];
	uint8_t CasteHappiness[CASTE_SIZE];
	/**
	 * Modifier to how many pounds are harvested from each field in this
	 * settlement. Changes every year and is dependant on the previous year.
	 */
	uint8_t HarvestMod[HARVEST_YEARS];
};

void LocationGetPoint(const struct Settlement* Location, SDL_Point* Point);

struct Settlement* CreateSettlement(int X, int Y, const char* Name, int GovType);
void DestroySettlement(struct Settlement* Location);

void SettlementThink(struct Settlement* Settlement);
void SettlementDraw(const struct MapRenderer* Renderer, struct Settlement* Settlement);

/**
 * Adds a family to the settlement.
 */
void SettlementPlaceFamily(struct Settlement* Location, struct Family* Family);
void SettlementRemoveFamily(struct Settlement* Location, struct Family* Family);
/**
 * Returns 1 if _Location is determed to be non-hostile to _Army.
 * Otherwise returns 0.
 */
int SettlementIsFriendly(const struct Settlement* Location, struct Army* Army);
void SettlementGetCenter(const struct Settlement* Location, SDL_Point* Pos);
/**
 *\brief Adds a person to the settlement.
 */
void SettlementAddPerson(struct Settlement* Settlement, struct Person* Person);
void SettlementRemovePerson(struct Settlement* Settlement, struct Person* Person);
/**
 * \return The number of adult men who own a weapon and are capable of fighting.
 */
int SettlementCountWarriors(const struct Settlement* Settlement);
void TribalCreateBigGuys(struct Settlement* Settlement, double CastePercent[CASTE_SIZE]);
int SettlementBigGuyCt(const struct Settlement* Settlement);
int SettlementAdultPop(const struct Settlement* Settlement);

/**
 * \return The cumulative nutritional value the people in _Settlement have.
 */
int SettlementGetNutrition(const struct Settlement* Settlement);
/**
 *	\return How much nutritional value is required in a year by Settlement.
 */
int SettlementYearlyNutrition(const struct Settlement* Settlement);
int SettlementCountAcres(const struct Settlement* Settlement);
int SettlementExpectedYield(const struct Settlement* Settlement);
float HarvestModifier(uint8_t (* const HarvestYears)[HARVEST_YEARS]);
static inline const struct LnkLst_Node* SettlementPlots(const struct Settlement* Settlement) {
	return NULL;
}
/**
 * \return the first found Plot that is of type PlotType and contains PlotData.
 */
struct Plot* SettlementFindPlot(const struct Settlement* Settlement, int PlotType, void* PlotData);
struct Retinue* SettlementAddRetinue(struct Settlement* Settlement, struct BigGuy* Leader);

static inline int SettlementAllocAcres(struct Settlement* Settlement, int Acres) {
	if(Settlement->FreeAcres - Acres < 0)
		return 0;
	Settlement->UsedAcres += Acres;
	Settlement->FreeAcres -= Acres;
	return 1;
}
#endif
