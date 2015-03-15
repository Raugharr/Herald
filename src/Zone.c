/*
 * File: Zone.c
 * Author: David Brotz
 */

#include "Zone.h"

#include "Herald.h"

struct ZoneBase g_Zones[] = {
		{ZONE_FIELD, "Field", ACRE_WIDTH, ACRE_LENGTH, (ZONE_TCROP)},
		{ZONE_ONERHOUSE, "One Room House", 8, 8, (ZONE_TBED | ZONE_TCOOKING)},
		{ZONE_STORAGE, "Storage", 6, 6, (ZONE_TNONE)}
};
