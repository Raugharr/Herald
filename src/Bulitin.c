#include "Bulitin.h"

#include "Mission.h"
#include "BigGuy.h"
#include "Person.h"
#include "Family.h"
#include "Location.h"

#include <stdlib.h>

const char* BulitinItemGetName(struct BulitinItem* _Item) {
	return _Item->SuccMission->Name;
}

void BulitinItemOnClick(struct BulitinItem* _Item, lua_State* _State, struct BigGuy* _Selector) {
	MissionCall(_State, _Item->SuccMission, _Selector, _Item->Owner);
}

struct BulitinItem* CreateBulitinItem(const struct Mission* _SuccMission, const struct Mission* _FailMission,
	 struct BigGuy* _Owner, int _DaysLeft, int _Priority) {
	struct BulitinItem* _Item = (struct BulitinItem*) malloc(sizeof(struct BulitinItem));

	_Item->SuccMission = _SuccMission;
	_Item->FailMission = _FailMission;
	_Item->Owner = _Owner;
	_Item->Priority = _Priority;
	_Item->DaysLeft = _DaysLeft;
	return _Item;
}

void DestroyBulitinItem(struct BulitinItem* _Item) {
	free(_Item);
}

void BulitinThink(struct BulitinItem* _Item) {
	--_Item->DaysLeft;
	if(_Item->DaysLeft <= 0) {
		ILL_DESTROY(_Item->Owner->Person->Family->HomeLoc->Bulitin, _Item);
		DestroyBulitinItem(_Item);
	}
}
