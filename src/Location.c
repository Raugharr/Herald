/* Author: David Brotz
 * File: Location.c
 */

#include "Location.h"

#include "BigGuy.h"
#include "Person.h"
#include "Family.h"
#include "Government.h"
#include "sys/KDTree.h"

#include <stdlib.h>
#include <string.h>

struct Settlement* CreateSettlement(int _X, int _Y, int _Width, int _Length, const char* _Name, int _GovType) {
	struct Settlement* _Loc = (struct Settlement*) malloc(sizeof(struct Settlement));

	_Loc->Type = ELOC_SETTLEMENT;
	_Loc->StartX = _X;
	_Loc->StartY = _Y;
	_Loc->EndX = _X + _Width;
	_Loc->EndY = _Y + _Length;
	_Loc->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Loc->People = NULL;
	_Loc->Government = CreateGovernment(_GovType);
	_Loc->NumPeople = 0;
	_Loc->BigGuys.Size = 0;
	_Loc->BigGuys.Front = NULL;
	_Loc->Families.Back = NULL;
	_Loc->Families.Front = NULL;
	_Loc->Families.Back = NULL;
	strcpy(_Loc->Name, _Name);
	return _Loc;
}

void DestroySettlement(struct Settlement* _Location) {
	free(_Location->Government);
	free(_Location->Name);
	free(_Location);
}

void SettlementPickLeader(struct Settlement* _Location) {
	struct Person* _Person = _Location->People;

	while(_Person != NULL) {
		if(_Person->Gender == EMALE && DateToDays(_Person->Age) > ADULT_AGE) {
			_Location->Government->Leader = CreateBigGuy(_Person); //NOTE: Make sure we aren't making a big guy when the person is already a big guy.
			break;
		}
		_Person = _Person->Next;
	}
}

void SettlementThink(struct Settlement* _Settlement) {
	struct LnkLst_Node* _Itr = _Settlement->Families.Front;

	while(_Itr != NULL) {
		FamilyThink((struct Family*)_Itr->Data);
		_Itr = _Itr->Next;
	}
}

int SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family, int* _X, int* _Y) {
	int _Point[] = {_Location->StartX, _Location->StartY};

	while(_Point[0] < _Location->EndX) {
		++_Point[0];
		while(_Point[1] < _Location->EndY) {
			if(KDSearchNode(&g_ObjPos, _Point) == NULL) {
				*_X = _Point[0];
				*_Y = _Point[1];
				_Location->NumPeople += FamilySize(_Family);
				LnkLstPushBack(&_Location->Families, _Family);
				return 1;
			}
			++_Point[1];
		}
	}
	return 0;
}

void PlaceBuilding(struct Settlement* _Location, int _Width, int _Length, int* _X, int* _Y) {
	*_X = _Location->Planner.BuildingX;
	*_Y = _Location->Planner.BuildingY;
	_Location->Planner.BuildingY += _Length;
	if(_Location->Planner.BuildingY > _Location->EndY) {
		_Location->Planner.BuildingY = 0;
		_Location->Planner.BuildingX += _Width;
	}
}

