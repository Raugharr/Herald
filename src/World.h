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

extern struct GameWorld g_GameWorld;
extern struct RBTree* g_GoodDeps;
extern struct Array* g_AnFoodDep;
extern struct RBTree g_Families;
extern struct KDTree g_ObjPos;
extern struct BigGuy* g_Player;
extern struct LinkedList g_Settlements;
extern struct RBTree g_BigGuys;
extern struct RBTree g_BigGuyState;
extern int g_TemperatureList[];
extern int* g_AvgTempMap[MONTHS];

struct GameWorld {
	DATE Date;
	struct MapRenderer MapRenderer;
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

int LuaRegisterPersonItr(lua_State* _State);
int LuaWorldGetPlayer(lua_State* _State);
int LuaWorldGetPersons(lua_State* _State);
int LuaWorldGetDate(lua_State* _State);
int LuaWorldTick(lua_State* _State);

void WorldInit(int _Area);
void WorldQuit();
void CreateTempMap(int _Length);
int World_Tick();

#endif
