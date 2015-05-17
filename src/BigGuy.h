/*
 * File: BigGuy.h
 * Author: David Brotz
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

struct Person;
struct Mission;

enum {
	BGSTATE_PASSREFORM = (1 << 0),
	BGSTATE_ISLEADER = (1 << 1),
	BGSTATE_SIZE = 2
};

extern const char* g_BGStateStr[BGSTATE_SIZE];

struct BigGuyStats {
	int Charisma;
	int Leadership;
	int Combat;
};

struct BigGuy {
	struct Person* Person;
	int IsDirty;
	int State;
	int Authority;
};

struct BigGuy* CreateBigGuy(struct Person* _Person);
void DestroyBigGuy(struct BigGuy* _BigGuy);

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyIdCmp(const struct BigGuy* _BigGuy, const int* _Two);
int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission);

struct BigGuy* BigGuyLeaderType(struct Person* _Person);

#endif
