#include "Bulletin.h"

#include "Mission.h"
#include "BigGuy.h"
#include "Person.h"
#include "Family.h"
#include "Location.h"

#include <stdlib.h>

const char* BulletinItemGetName(struct BulletinItem* _Item) {
	return _Item->SuccMission->Name;
}

void BulletinItemOnClick(struct BulletinItem* _Item, lua_State* _State, struct BigGuy* _Selector) {
	MissionCall(_State, CreateMissionFrame(_Item->Owner, _Selector, _Item->SuccMission));
}

struct BulletinItem* CreateBulletinItem(const struct Mission* _SuccMission, const struct Mission* _FailMission,
	 struct BigGuy* _Owner, int _DaysLeft, int _Priority) {
	struct BulletinItem* _Item = NULL;

	if(_SuccMission == NULL || _Owner == NULL)
		return NULL;
	_Item = (struct BulletinItem*) malloc(sizeof(struct BulletinItem));
	_Item->SuccMission = _SuccMission;
	_Item->FailMission = _FailMission;
	_Item->Owner = _Owner;
	_Item->Priority = _Priority;
	_Item->DaysLeft = _DaysLeft;
	return _Item;
}

void DestroyBulletinItem(struct BulletinItem* _Item) {
	free(_Item);
}
