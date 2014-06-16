/*
 * File: World.h
 * Author: David Brotz
 */

#ifndef __WORLD_H
#define __WORLD_H

#define YEAR(__Date) (__Date >> 9)
#define MONTH(__Date) ((__Date >> 5) & 15)
#define MONTHS (12)
#define DAY(__Date) (__Date & 31)
#define TO_DATE(__Year, __Month, __Day) (DAY(__Day) | (_Month << 5) | (_Year << 9))

struct Array;
struct RBTree;

#define DATE int

extern DATE g_Date;
extern struct Array* g_World;
extern struct RBTree* g_BuildDep;

//Each tile represents a mile of the world.
struct WorldTile {
	int Temperature;
};

void World_Init(int _Area);
void World_Quit();
void NextDay(int* _Date);
int World_Tick();
int MonthToInt(const char* _Month);
int DaysBetween(int _DateOne, int _DateTwo);
int DateToDays(int _Date);
int DateAdd(int _Left, int _Right);
int DateSub(int _Left, int _Right);

#endif
