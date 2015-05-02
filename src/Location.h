/* Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

#include "sys/LinkedList.h"

struct BigGuy;
struct Family;
struct Government;

enum {
	ELOC_SETTLEMENT,
	ELOC_FOREST
};

struct Location {
	int Type;
	int StartX;
	int StartY;
	int EndX;
	int EndY;
};

struct CityPlanner {
	int BuildingX;
	int BuildingY;
};

struct Settlement {
	int Type;
	int StartX;
	int StartY;
	int EndX;
	int EndY;
	char* Name;
	struct CityPlanner Planner;
	struct Person* People;
	struct LinkedList BigGuys;
	struct LinkedList Families;
	struct Government* Government;
	int NumPeople;
};

struct Settlement* CreateSettlement(int _X, int _Y, int _Width, int _Length, const char* _Name, int _GovType);
void DestroySettlement(struct Settlement* _Location);

void SettlementPickLeader(struct Settlement* _Location);
void SettlementThink(struct Settlement* _Settlement);

int SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family, int* _X, int* _Y);
void PlaceBuilding(struct Settlement* _Location, int _Width, int _Length, int* _X, int* _Y);
#endif
