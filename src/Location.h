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
#include "sys/Array.h"

#include "video/AABB.h"

#define SETTLEMENT_MINBG (3)
#define MILE_ACRE (640)
#define SETTLEMENT_SPACE (520 * 7)
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

enum ESelp {
	SELP_REG = (1 << 0),//Regular person.
	SELP_BIG = (1 << 1),//Big guy.
	SELP_ANY = SELP_REG | SELP_BIG
};

enum EFine {
	FINE_MURDER,
	FINE_SIZE
};

enum {
	SET_HAMLET,
	SET_VILLAGE,
	SET_TOWN,
	SET_CITY,
	SET_SIZE
};

enum EFineL {
	FINE_LLIGHT,
	FINE_LNORMAL,
	FINE_LHEAVY
};
/**
 * Struct used in QueryPeople to determine which people should be selected and how many
 * people should be selected.
 */
struct PersonSelector {
	uint16_t Count; //Will select Count number of people. If there arent enough valid people will then select them all.
	int8_t Gender; //Gender of people to return use (EMALE | EFEMALE) to return both.
	int8_t Adult;
	int8_t Caste;
	//Type of person this is, see ESELP enumeration for valid values.
	int8_t PType;
	struct Person* Target;
	bool Relatives;//Only select relatives.
};

struct SettlementSelector {
	const struct Settlement* Target;
	uint16_t Distance; //Distance from Target to query search results.
	uint16_t Count;
};

struct Settlement {
	struct Object Object;
	SDL_Point Pos;
	char* Name;
	//Look into replacing this with an array as People should not be frequently leaving/joining a settlement.
	struct Array People;
	//List of crisis that have been started here.
	struct Array Crisis;
	//List of families that are slaves, used to lookup who owns what slave family.
	//Sorted by Owner's family id.
	struct Array Slaves;
	struct Sprite* Sprite;
	struct Government* Government;
	struct BuyRequest* BuyOrders;
	struct SellRequest* Market;
	struct Array Bulletin;
	struct Retinue* Retinues; 
	struct LinkedList Families; // List of struct Family*
	struct LinkedList BigGuys; // List of struct BigGuy*
	struct LinkedList FreeWarriors; //List of struct Person*
	//Men who are looking for wives.
	struct Array Suitors;
	//Women who are looking for husbands.
	struct Array Brides;
	DATE LastRaid;
	struct {
		int32_t SlowSpoiled;
		int32_t FastSpoiled;
		int32_t AnimalFood; //Accounts for all nutrition that is assumed to be grown from wild plants on the farmers fallow fields. Since it is not harvested
		// this variable should be set to 0 at harvest time.
	} Food;
	//Migrants are not accounted for.
	uint16_t YearDeaths; //Record of deaths in this settlement this year.
	uint16_t YearBirths;
	uint16_t AdultMen;
	uint16_t AdultWomen;
	uint16_t MaxWarriors;
	//How many acres remain to give out as farming land.
	uint16_t FreeAcres;
	//Number of farming acres in use.
	uint16_t UsedAcres;
	uint16_t NumPeople;
	uint16_t CasteCount[CASTE_SIZE];
	uint8_t Stats[BGSKILL_SIZE];
	uint8_t HarvestYear;
	struct {
		const struct Crop* Crop; //What crop is growing in the meadow.
		uint16_t Acres;
		int32_t MonthNut;//How much food this field generates per month.
		int32_t NutRem;//How much food is unused.
	} Meadow;
	/**
	 * Modifier to how many pounds are harvested from each field in this
	 * settlement. Changes every year and is dependant on the previous year.
	 */
	uint8_t HarvestMod[HARVEST_YEARS];
	struct Faction Factions;
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
void InitPersonSelector(struct PersonSelector* Selector);
//Returns a list of people that is at most Selector->Count people that are in the InList that satisfy the selctor's parameters.
struct Person** QueryPeople(struct Person** const InList, uint32_t InListSz, const struct PersonSelector* Selector, uint32_t* OutListSz);
struct Settlement** QuerySettlement(const struct SettlementSelector* Selector, uint32_t* OutListSz);
bool LocationCreateField(struct Family* Family, int Acres);
static inline int SettlementType(const struct Settlement* Settlement) {
	if(Settlement->NumPeople < 100) return SET_HAMLET;
	else if (Settlement->NumPeople < 500) return SET_VILLAGE;
	else if(Settlement->NumPeople < 1000) return SET_TOWN;
	return SET_CITY;
}
#endif
