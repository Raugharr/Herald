/* Author: David Brotz
 * File: Location.c
 */

#include "Location.h"

#include "Person.h"

#include <stdlib.h>
#include <string.h>

struct CityLocation* CreateCityLocation(int _X, int _Y, int _Width, int _Length, const char* _Name) {
	struct CityLocation* _Loc = (struct CityLocation*) malloc(sizeof(struct CityLocation));

	_Loc->Type = ELOC_SETTLEMENT;
	_Loc->StartX = _X;
	_Loc->StartY = _Y;
	_Loc->EndX = _X + _Width;
	_Loc->EndY = _Y + _Length;
	_Loc->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Loc->People = NULL;
	_Loc->Leader = NULL;
	strcpy(_Loc->Name, _Name);
	return _Loc;
}

void DestroyCityLocation(struct CityLocation* _Location) {
	free(_Location->Name);
	free(_Location);
}

void CityLocationPickLeader(struct CityLocation* _Location) {
	struct Person* _Person = _Location->People;

	while(_Person != NULL) {
		if(_Person->Gender == EMALE && DateToDays(_Person->Age) > ADULT_AGE) {
			_Location->Leader = _Person;
			break;
		}
		_Person = _Person->Next;
	}
}

void PlaceBuilding(struct CityLocation* _Location, int _Width, int _Length, int* _X, int* _Y) {
	*_X = _Location->Planner.BuildingX;
	*_Y = _Location->Planner.BuildingY;
	_Location->Planner.BuildingY += _Length;
	if(_Location->Planner.BuildingY > _Location->EndY) {
		_Location->Planner.BuildingY = 0;
		_Location->Planner.BuildingX += _Width;
	}
}

