#ifndef __BULITIN_H
#define __BULITIN_H

#include "sys/LinkedList.h"

#include <stdint.h>

typedef struct lua_State lua_State;
struct Mission;
struct BigGuy;

enum {
	BULPRI_LOW,
	BULPRI_MEDIUM,
	BULPRI_HIGH
};

struct BulletinItem {
	const struct Mission* SuccMission;
	const struct Mission* FailMission;
	struct BigGuy* Owner;
	uint16_t DaysLeft; //Time until builitin mission cannot be solved.
	uint8_t Priority; //How important this item is to the owner.
};

const char* BulletinItemGetName(struct BulletinItem* _Item);
void BulletinItemOnClick(struct BulletinItem* _Item, lua_State* _State, struct BigGuy* _Selector);

struct BulletinItem* CreateBulletinItem(const struct Mission* _SuccMission, const struct Mission* _FailMission,
	 struct BigGuy* _Owner, int _DaysLeft, int _Priority);
void DestroyBulletinItem(struct BulletinItem* _Item);
#endif
