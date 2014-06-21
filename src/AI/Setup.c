/*
 * File: Setup.c
 * Author: David Brotz
 */

#include "Setup.h"

#include "BehaviorTree.h"
#include "../Building.h"
#include "../Good.h"
#include "../Crop.h"
#include "../Person.h"
#include "../Family.h"
#include "../sys/Array.h"
#include "../Herald.h"

#include <string.h>

struct Behavior* g_AIMan = NULL;
struct Behavior* g_AIWoman = NULL;
struct Behavior* g_AIChild = NULL;

int PAIHasField(struct Person* _Person, void* _Data) {
	return _Person->Family->Field != NULL;
}

int PAIHasHouse(struct Person* _Person, void* _Data) {
	int i;
	struct Array* _Array = _Person->Family->Buildings;
	void** _Table = _Array->Table;

	for(i = 0; i < _Array->Size; ++i)
		if((((struct Building*)_Table[i])->ResidentType & ERES_HUMAN) == ERES_HUMAN)
			return 1;
	return 0;
}

int PAIWorkField(struct Person* _Person, void* _Data) {
	struct Family* _Family = _Person->Family;
	struct Field* _Field = _Family->Field;
	struct Array* _Array = NULL;
	int i;

	if(_Field == NULL)
		return 0;
	if(_Field->Status == EFALLOW) {
		_Array = _Family->Goods;
		for(i = 0; i < _Array->Size; ++i) {
			if(strcmp(((struct Good*)_Array->Table[i])->Name, _Field->Crop->Name) == 0) {
				FieldPlant(_Field, _Array->Table[i]);
				break;
			}
		}
	} else if(_Field->Status == EHARVESTING) {
		struct Good* _CropSeed = NULL;
		struct Good* _Good = NULL;

		if((_CropSeed = HashSearch(&g_Goods, _Field->Crop->Name)) == 0)
			return 0;
		_Good = CopyGood(_CropSeed);
		if(_Good == NULL)
			ArrayInsert_S(_Family->Goods, _Good);
		FieldHarvest(_Field, _Good);
	} else {
		FieldWork(_Field, PersonWorkMult(_Person));
	}
	return 1;
}

int PAIBuildHouse(struct Person* _Person, void* _Data) {
	struct Construction* _House = NULL;

	if((_House = ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_CONSTRUCTION)) == NULL) {
		ATimerInsert(&g_ATimer, CreateConstruct(HashSearch(&g_Buildings, "Cottage"), _Person));
	} else {
		--_House->DaysLeft;
	}
	return 1;
}


int BHVNothing(struct Person* _Person, void* _Data) {
	return 1;
}

void AIInit() {
	g_AIMan = CreateBHVComp(NULL, BHV_SEQUENCE, 3);
	g_AIWoman = CreateBHVNode(NULL, PersonUpdate);
	g_AIChild = CreateBHVNode(NULL, PersonUpdate);
	struct Behavior* _BhvHouse = NULL;

	CreateBHVNode(g_AIMan, PAIWorkField);

	_BhvHouse = CreateBHVComp(g_AIMan, BHV_SEQUENCE, 2);
	CreateBHVD(_BhvHouse, BHV_DNOT, PAIHasHouse);
	CreateBHVNode(_BhvHouse, PAIBuildHouse);

	CreateBHVNode(g_AIMan, PersonUpdate);
}
void AIQuit() {
	DestroyBehavior(g_AIMan);
	DestroyBehavior(g_AIWoman);
	DestroyBehavior(g_AIChild);
}
