/* Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

#include "sys/LinkedList.h"

#include "video/AABB.h"
#include "video/Point.h"

#define SETTLEMENT_MINBG (3)
#define SettlementRaiseFyrd(_Settlement, _ArmyGoal) CreateArmy((_Settlement), &(_Settlement)->FirstPart->Pos, (_Settlement)->Government, (_Settlement)->Government->Leader, (_ArmyGoal))

struct BigGuy;
struct Family;
struct Government;
struct MapRenderer;
struct Settlement;
struct Army;
struct ArmyGoal;
struct Tile;
struct Object;

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
	SDL_Rect Pos;
	char* Name;
	struct Person* People;
	struct LinkedList BigGuys;
	struct LinkedList Sprites;
	struct Government* Government;
	int NumPeople;
	int NumFamilies;
	struct SettlementPart* FirstPart;
	struct SettlementPart* LastPart;
};

struct SettlementPart {
	int NumPeople;
	SDL_Point Pos;
	struct Settlement* Owner;
	struct LinkedList Families;
	struct Person* People;
	struct SettlementPart* Next;
	struct SettlementPart* Prev;
};

void LocationGetArea(const struct Location* _Location, SDL_Rect* _AABB);

struct Settlement* CreateSettlement(int _X, int _Y, int _Width, int _Height, const char* _Name, int _GovType);
void DestroySettlement(struct Settlement* _Location);

void SettlementThink(struct Settlement* _Settlement);
void SettlementDraw(const struct MapRenderer* _Renderer, struct Settlement* _Settlement);

struct SettlementPart* SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family, int* _X, int* _Y);
void SettlementAddTile(struct Settlement* _Location, const struct Tile* _Tile);
int SettlementIsFriendly(const struct Settlement* _Location, struct Army* _Army);
void SettlementGetCenter(const struct Settlement* _Location, SDL_Point* _Pos);
void SettlementAddPerson(struct SettlementPart* _Settlement, struct Person* _Person);
void SettlementRemovePerson(struct SettlementPart* _Settlement, struct Person* _Person);
/*
 * _Families should be of size _Settlement->NumFamilies.
 */
void SettlementGetFamilies(struct Settlement* _Settlement, struct Family** _Families);
/*
 * _People should be of size _Settlement->NumPeople.
 */
void SettlementGetPeople(struct Settlement* _Settlement, struct Person** _People);
int SettlementCountWarriors(const struct Settlement* _Settlement);
void TribalCreateBigGuys(struct Settlement* _Settlement);
int SettlementPartGetNutrition(const struct SettlementPart* _Part);
int SettlementGetNutrition(const struct Settlement* _Settlement);
#endif
