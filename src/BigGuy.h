/*
 * File: BigGuy.h
 * Author: David Brotz
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

struct Person;
struct Mission;

struct BigGuy {
	struct Person* Person;
	int InMeeting;
	int Authority;
};

struct BigGuy* CreateBigGuy(struct Person* _Person);
void DestroyBigGuy(struct BigGuy* _BigGuy);

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyIdCmp(const struct BigGuy* _BigGuy, const int* _Two);
int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission);

#endif
