/*
 * File: World.h
 * Author: David Brotz
 */

#ifndef __WORLD_H
#define __WORLD_H

#include "Herald.h"

typedef struct lua_State lua_State;

struct Array;
struct Person;
struct RBTree;

extern DATE g_Date;
extern struct Array* g_World;
extern struct RBTree* g_GoodDeps;
extern struct Array* g_AnFoodDep;
extern struct RBTree g_Families;
extern struct KDTree g_ObjPos;
extern struct Person* g_Player;

struct FamilyType {
	double Percent;
	char* LuaFunc;
};

//Each tile represents a mile of the world.
struct WorldTile {
	int Temperature;
};

struct Person* PickPlayer();

int LuaRegisterPersonItr(lua_State* _State);
int LuaWorldGetPlayer(lua_State* _State);
int LuaWorldGetPersons(lua_State* _State);
int LuaWorldGetDate(lua_State* _State);
int LuaWorldTick(lua_State* _State);

void WorldInit(int _Area);
void WorldQuit();
int World_Tick();


#endif
