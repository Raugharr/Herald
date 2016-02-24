#ifndef __BULITIN_H
#define __BULITIN_H

#include "sys/LinkedList.h"

typedef struct lua_State lua_State;
struct Mission;
struct BigGuy;

enum {
	BULPRI_LOW,
	BULPRI_MEDIUM,
	BULPRI_HIGH
};

struct BulitinItem {
	IMPLICIT_LINKEDLIST(struct BulitinItem);
	const struct Mission* SuccMission;
	const struct Mission* FailMission;
	struct BigGuy* Owner;
	int Priority; //How important this item is to the owner.
	int DaysLeft; //Time until builitin mission cannot be solved.
};

const char* BulitinItemGetName(struct BulitinItem* _Item);
void BulitinItemOnClick(struct BulitinItem* _Item, lua_State* _State, struct BigGuy* _Selector);

struct BulitinItem* CreateBulitinItem(const struct Mission* _SuccMission, const struct Mission* _FailMission,
	 struct BigGuy* _Owner, int _DaysLeft, int _Priority);
void DestroyBulitinItem(struct BulitinItem* _Item);
void BulitinThink(struct BulitinItem* _Item);
#endif
