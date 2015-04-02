/*
 * File: Zone.c
 * Author: David Brotz
 */

#include "Zone.h"

#include "Herald.h"
#include "Good.h"
#include "sys/LinkedList.h"

#include <stdlib.h>

int ZoneFieldReq(const struct Zone* _Zone, const struct LinkedList* _List) {
	int _Area = _Zone->Width * _Zone->Length;
	struct LnkLst_Node* _Itr = NULL;

	if(_List->Size != _Area)
		return 0;
	_Itr = _List->Front;
	while(_Itr != NULL) {
		if(((struct Object*)_Itr->Data)->Type != OBJECT_CROP)
			return 0;
		_Itr = _Itr->Next;
	}
	return 1;
}

int ZoneOneHouseReq(const struct Zone* _Zone, const struct LinkedList* _List) {
	int _Bed = 0;
	int _Cooking = 1;
	struct LnkLst_Node* _Itr = _List->Front;

	while(_Itr->Next != NULL) {
		if(((struct Object*)_Itr->Data)->Type == OBJECT_GOOD)
			if(((struct Good*)_Itr->Data)->Base->Category == EFURNITURE)
				_Bed = 1;
		_Itr = _Itr->Next;
	}
	return (_Bed == 1 && _Cooking == 1) ? (1) : (0);
}

int ZoneNoReq(const struct Zone* _Zone, const struct LinkedList* _List) {
	return 1;
}

int ZoneOneHousePlacement(const struct Zone* _Zone, const struct Object* _Obj, int* _X, int* _Y) {
	struct Good* _Good = NULL;

	if(_Obj->Type != OBJECT_GOOD)
		return 0;
	_Good = (struct Good*)_Obj;
	if(_Good->Base->Category == EFURNITURE) {
		*_X = _Zone->X;
		*_Y = _Zone->Y;
	}
	return 1;
}

int ZoneStoragePlacement(const struct Zone* _Zone, const struct Object* _Obj, int* _X, int* _Y) {
	*_X = _Zone->X;
	*_Y = _Zone->Y;
	return 1;
}

int ZoneNoPlacement(const struct Zone* _Zone, const struct Object* _Obj, int* _X, int* _Y) {
	return 0;
}

struct ZoneBase g_Zones[] = {
		{ZONE_FIELD, "Field", ACRE_WIDTH, ACRE_LENGTH, ZoneFieldReq, ZoneNoPlacement},
		{ZONE_ONERHOUSE, "One Room House", 8, 8, ZoneOneHouseReq, ZoneOneHousePlacement},
		{ZONE_STORAGE, "Storage", 6, 6, ZoneNoReq, ZoneStoragePlacement},
};

int g_ZoneCt = 3;
int g_ZoneId = 0;

struct Zone* CreateZone(struct ZoneBase* _Base, int _Width, int _Length) {
	struct Zone* _Zone = (struct Zone*) malloc(sizeof(struct Zone));

	_Zone->Id = g_ZoneId++;
	_Zone->X = 0;
	_Zone->Y = 0;
	_Zone->Width = _Width;
	_Zone->Length = _Length;
	_Zone->Base = _Base;
	_Zone->Owner = NULL;
	return _Zone;
}

void DestroyZone(struct Zone* _Zone) {
	free(_Zone);
}

int NextZoneId() {return ++g_ZoneId;}
