/* Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

#include "sys/LinkedList.h"

#include "video/AABB.h"
#include "video/Point.h"

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
	struct Sprite* Sprite; //FIXME: Should only me one sprite not a list.
	struct Government* Government;
	int NumPeople;
	struct LinkedList Families;
};

void LocationGetPoint(const struct Location* _Location, SDL_Point* _Point);

struct Settlement* CreateSettlement(int _X, int _Y, const char* _Name, int _GovType);
void DestroySettlement(struct Settlement* _Location);

void SettlementThink(struct Settlement* _Settlement);
void SettlementDraw(const struct MapRenderer* _Renderer, struct Settlement* _Settlement);

void SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family);
int SettlementIsFriendly(const struct Settlement* _Location, struct Army* _Army);
void SettlementGetCenter(const struct Settlement* _Location, SDL_Point* _Pos);
void SettlementAddPerson(struct Settlement* _Settlement, struct Person* _Person);
void SettlementRemovePerson(struct Settlement* _Settlement, struct Person* _Person);
int SettlementCountWarriors(const struct Settlement* _Settlement);
void TribalCreateBigGuys(struct Settlement* _Settlement);
int SettlementGetNutrition(const struct Settlement* _Settlement);
#endif
