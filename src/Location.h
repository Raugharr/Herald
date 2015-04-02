/* Author: David Brotz
 * File: Location.h
 */

#ifndef __LOCATION_H
#define __LOCATION_H

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
};

/*
 * TODO: Use a data structure to store the location of all buildings, then lookup
 * their location to determine if the new building is colliding with one already.
 */
void PlaceBuilding(struct CityLocation* _Location, int _Width, int _Length, int* _X, int* _Y);
#endif
