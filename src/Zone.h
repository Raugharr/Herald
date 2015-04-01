/*
 * File: Zone.h
 * Author: David Brotz
 */
#ifndef ZONE_H
#define ZONE_H

struct LinkedList;
struct Zone;

enum {
	ZONE_FIELD,
	ZONE_ONERHOUSE,
	ZONE_STORAGE,
};

struct ZoneBase {
	int Type;
	const char* Name;
	int MinWidth;
	int MinLength;
	int (*Requirements)(const struct Zone*, const struct LinkedList*);
};

struct Zone {
	int Id;
	int X;
	int Y;
	int Width;
	int Length;
	struct ZoneBase* Base;
	struct Actor* Owner;
};

extern struct ZoneBase g_Zones[];
extern int g_ZoneCt;

struct Zone* CreateZone(struct ZoneBase* _Base, int _Width, int _Length);
void DestroyZone(struct ZoneBase* _Zone);
int NextZoneId();

#endif
