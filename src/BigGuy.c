/*
 * File: BigGuy.c
 * Author: David Brotz
 */

#include "BigGuy.h"

#include "Person.h"
#include "World.h"

#include "sys/RBTree.h"

#include <stdlib.h>

struct BigGuy* CreateBigGuy(struct Person* _Person) {
	struct BigGuy* _BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	_BigGuy->Person = _Person;
	_BigGuy->State = 0;
	_BigGuy->Authority = 0;
	RBInsert(&g_BigGuys, _BigGuy);
	RBInsert(&g_BigGuyState, _BigGuy);
	return _BigGuy;
}

void DestroyBigGuy(struct BigGuy* _BigGuy) {
	RBDelete(&g_BigGuys, _BigGuy);
	RBDelete(&g_BigGuyState, _BigGuy);
	free(_BigGuy);
}

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->Person->Id - _Two->Person->Id;
}

int BigGuyIdCmp(const struct BigGuy* _BigGuy, const int* _Two) {
	return _BigGuy->Person->Id - (*_Two);
}

int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->State - _Two->State;
}

int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission) {
	return 0;
}
