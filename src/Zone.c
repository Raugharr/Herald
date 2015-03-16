/*
 * File: Zone.c
 * Author: David Brotz
 */

#include "Zone.h"

#include "Herald.h"

#include <stdlib.h>

struct ZoneBase g_Zones[] = {
		{ZONE_FIELD, "Field", ACRE_WIDTH, ACRE_LENGTH, (ZONE_TCROP)},
		{ZONE_ONERHOUSE, "One Room House", 8, 8, (ZONE_TBED | ZONE_TCOOKING)},
		{ZONE_STORAGE, "Storage", 6, 6, (ZONE_TNONE)},
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

int NextZoneId() {return ++g_ZoneId;}
