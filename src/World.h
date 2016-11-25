/*
 * Author: David Brotz
 * File: World.h
 */

#ifndef __WORLD_H
#define __WORLD_H

#include "Herald.h"
#include "Date.h"
#include "Family.h"

#include "sys/LinkedList.h"
#include "sys/RBTree.h"
#include "sys/ITree.h"
#include "sys/Queue.h"

#include "video/MapRenderer.h"

typedef struct lua_State lua_State;
typedef struct SDL_Texture SDL_Texture;

struct Array;
struct Person;
struct RBTree;
struct TaskPool;
struct BigGuy;
struct GameWorld;
struct KeyMouseState;
struct Path;
struct Actor;
struct Policy;

extern struct GameWorld g_GameWorld;
extern int g_TemperatureList[];
extern int* g_AvgTempMap[MONTHS];
extern struct Caste g_Castes[CASTE_SIZE];

#define GameWorldInsertSettlement(_GameWorld, _Settlement) 																				\
{																																		\
	LnkLstPushBack(&(_GameWorld)->Settlements, (_Settlement));																			\
	QTInsertAABB(&(_GameWorld)->SettlementIndex, (_Settlement), &(_Settlement)->Pos);													\
}
#define WORLD_DECAY (101)

enum {
	SUBTIME_ARMY,
	SUBTIME_BATTLE,
	SUBTIME_SIZE
};

enum {
	TERRAIN_GRASS
};

enum {
	MOUSESTATE_DEFAULT,
	MOUSESTATE_RAISEARMY,
	MOUSESTATE_SIZE
};

struct SubTimeObject {
	void (*Callback)(void*);
	void* (*Next)(void*);
	void* (*Prev)(void*);
	void* List;
};

struct GameWorld {
	DATE Date;
	uint32_t Tick;
	struct MapRenderer* MapRenderer;
	struct QuadTree SettlementMap;
	struct RBTree* GoodDeps; //Tree consisting of
	struct Array* AnFoodDeps;
	struct RBTree Families;
	struct BigGuy* Player;
	struct LinkedList Settlements;
	struct RBTree BigGuys;
	struct RBTree Agents;
	struct RBTree ActionHistory;
	struct RBTree PlotList;
	struct IntTree PersonRetinue; //Mapping of a person to their retinue.
	struct LinkedList MissionFrame;
	struct FoodBase** HumanEats;
	struct FoodBase** HumanDrinks;
	struct Policy* Policies;
	struct Constraint** BabyAvg;
	struct Constraint** AgeGroups;
	struct Queue FreeWarriors;
	float DecayRate[WORLD_DECAY];
	uint8_t PolicySz;
	uint8_t IsPaused;
};

struct FamilyType {
	double Percent;
	char* LuaFunc;
};

struct BigGuy* PickPlayer();
int IsPlayerGovernment(const struct GameWorld* _World, const struct Settlement* _Settlement);

/*
 * NOTE: Does this actually work or does it just return everything in a square distance away?
 */
void WorldSettlementsInRadius(struct GameWorld* _World, const SDL_Point* _Point, int _Radius, struct LinkedList* _List);

struct FoodBase** LoadHumanFood(lua_State* _State, struct FoodBase** _FoodArray, const char* _LuaTable);
void WorldInit(int _Area);
void WorldQuit();

int GameDefaultClick(const struct Object* _One, const struct Object* _Two);
int GameFyrdClick(const struct Object* _One, const struct Object* _Two);
void GameWorldEvents(const struct KeyMouseState* _State, struct GameWorld* _World);
void GameWorldDraw(const struct GameWorld* _World);
void CreateTempMap(int _Length);
int World_Tick();

void WorldPathCallback(struct Army* _Army, struct Path* _Path);

void** SubTimeGetList(int _Type);
void SetClickState(struct Object* _Data, int _State);
struct Settlement* WorldGetSettlement(struct GameWorld* _World, SDL_Point* _Pos);

#endif
