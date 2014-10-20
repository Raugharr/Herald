/*
 * File: World.h
 * Author: David Brotz
 */

#ifndef __WORLD_H
#define __WORLD_H

#include "Herald.h"

typedef struct lua_State lua_State;

struct Array;
struct RBTree;

extern DATE g_Date;
extern struct Array* g_World;
extern struct RBTree* g_GoodDeps;
extern struct Array* g_AnFoodDep;
extern struct RBTree g_Families;
extern struct KDTree g_ObjPos;

struct FamilyType {
	double Percent;
	char* LuaFunc;
};

//Each tile represents a mile of the world.
struct WorldTile {
	int Temperature;
};

int LuaRegisterPersonItr(lua_State* _State);
int LuaGetPersons(lua_State* _State);

void WorldInit(int _Area);
void WorldQuit();
int World_Tick();


#endif
