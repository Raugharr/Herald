/*
 * File: World.h
 * Author: David Brotz
 */

#ifndef __WORLD_H
#define __WORLD_H

#include "Herald.h"

#include "sys/LinkedList.h"
#include "sys/KDTree.h"
#include "video/MapRenderer.h"

#define WorldGetTile(_X, _Y) ((_Y) / g_WorldSize + (_X))

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

extern struct GameWorld g_GameWorld;
extern struct RBTree* g_GoodDeps;
extern struct Array* g_AnFoodDep;
extern struct RBTree g_Families;
extern struct KDTree g_ObjPos;
extern int g_TemperatureList[];
extern int* g_AvgTempMap[MONTHS];

#define GameWorldInsertSettlement(_GameWorld, _Settlement) 																				\
{																																		\
	SDL_Rect _AABB = {(_Settlement)->StartPos.X, (_Settlement)->StartPos.Y, 															\
	(_Settlement)->EndPos.X - (_Settlement)->StartPos.X, (_Settlement)->EndPos.Y - (_Settlement)->StartPos.Y};							\
	LnkLstPushBack(&(_GameWorld)->Settlements, (_Settlement));																			\
	QTInsertAABB(&(_GameWorld)->SettlementIndex, (_Settlement), &_AABB);																\
}

struct GameWorld {
	int IsPaused;
	DATE Date;
	struct MapRenderer* MapRenderer;
	struct QuadTree SettlementIndex;
	struct RBTree* GoodDeps;
	struct Array* AnFoodDeps;
	struct RBTree Families;
	struct KDTree ObjPos;
	struct BigGuy* Player;
	struct LinkedList Settlements;
	struct RBTree BigGuys;
	struct RBTree BigGuyStates;
};

enum {
	TERRAIN_GRASS
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

struct WorldTile* CreateWorldTile();
void DestroyWorldTile(struct WorldTile* _Tile);

void WorldInit(int _Area);
void WorldQuit();
void GameWorldEvents(const struct KeyMouseState* _State, struct GameWorld* _World);
void GameWorldDraw(const struct KeyMouseState* _State, struct GameWorld* _World);
void CreateTempMap(int _Length);
int World_Tick();

#endif
