/* Author: David Brotz
 * File: Location.c
 */

#include "Location.h"

void PlaceBuilding(struct CityLocation* _Location, int _Width, int _Length, int* _X, int* _Y) {
	_X = _Location->Planner.BuildingX;
	_Y = _Location->Planner.BuildingY;
	_Location->Planner.BuildingY += _Length;
	if(_Location->Planner.BuildingY > _Location->EndY) {
		_Location->Planner.BuildingY = 0;
		_Location->Planner.BuildingX += _Width;	
	}
}

