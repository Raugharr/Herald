/*
 * File: World.h
 * Author: David Brotz
 */

#ifndef __WORLD_H
#define __WORLD_H

#include "Herald.h"

#define YEAR(__Date) (__Date >> 9)
#define MONTH(__Date) ((__Date >> 5) & 15)
#define MONTHS (12)
#define DAY(__Date) (__Date & 31)
#define TO_DATE(__Year, __Month, __Day) (DAY(__Day) | (_Month << 5) | (_Year << 9))

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

void WorldInit(int _Area);
void WorldQuit();
void NextDay(int* _Date);
int World_Tick();


#endif
