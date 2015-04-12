/* Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

struct Person;

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

struct CityLocation {
	int Type;
	int StartX;
	int StartY;
	int EndX;
	int EndY;
	char* Name;
	struct CityPlanner Planner;
	struct Person* People;
	struct Person* Leader;
};

struct CityLocation* CreateCityLocation(int _X, int _Y, int _Width, int _Length, const char* _Name);
void DestroyCityLocation(struct CityLocation* _Location);

void CityLocationPickLeader(struct CityLocation* _Location);

void PlaceBuilding(struct CityLocation* _Location, int _Width, int _Length, int* _X, int* _Y);
#endif
