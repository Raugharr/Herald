/*
 * File: Zone.h
 * Author: David Brotz
 */
#ifndef ZONE_H
#define ZONE_H

enum {
	ZONE_FIELD,
	ZONE_ONERHOUSE,
	ZONE_STORAGE,
};

enum {
	ZONE_TNONE = (1 << 0),
	ZONE_TCROP = (1 << 1),
	ZONE_TANYGOOD = (1 << 2),
	ZONE_TBED = (1 << 3),
	ZONE_TCOOKING = (1 << 4)
};

struct ZoneBase {
	int Id;
	const char* Name;
	int MinWidth;
	int MinLength;
	int Requirements;
};

struct Zone {
	int Id;
	int Type;
	int X;
	int Y;
	int Width;
	int Length;
	struct ZoneBase* Base;
	struct Actor* Owner;
};

extern struct ZoneBase g_Zones[];

#endif
