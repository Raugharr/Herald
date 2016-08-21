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

#include "video/MapRenderer.h"

typedef struct lua_State lua_State;
typedef struct SDL_Texture SDL_Texture;

struct Array;
struct Person;
struct RBTree;
struct TaskPool;
struct BigGuy;
struct WorldTile;
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
	int IsPaused;
	DATE Date;
	int Tick;
	struct MapRenderer* MapRenderer;
	struct QuadTree SettlementMap;
	struct RBTree* GoodDeps; //Tree consisting of
	struct Array* AnFoodDeps;
	struct RBTree Families;
	struct BigGuy* Player;
	struct LinkedList Settlements;
	struct RBTree BigGuys;
	/*
	 * NOTE: Is only inserted into and not searched, should be removed as it has no apparent use.
	 */
	struct RBTree BigGuyStates;
	struct RBTree Agents;
	struct RBTree Crisis;
	struct RBTree ActionHistory;
	struct RBTree PlotList;
	struct LinkedList MissionData;
	struct FoodBase** HumanEats;
	struct FoodBase** HumanDrinks;
	struct Policy* Policies;
	struct Constraint** BabyAvg;
	struct Constraint** AgeGroups;
	int PolicySz;
};

struct FamilyType {
	double Percent;
	char* LuaFunc;
};

//Each tile represents a mile of the world.
struct WorldTile {
	float Forest;
	float Unbuildable;
	int Temperature;
};

struct BigGuy* PickPlayer();
int IsPlayerGovernment(const struct GameWorld* _World, const struct Settlement* _Settlement);

struct WorldTile* CreateWorldTile();
void DestroyWorldTile(struct WorldTile* _Tile);
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
