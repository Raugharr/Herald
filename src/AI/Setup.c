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
#include "../sys/HashTable.h"
#include "../sys/Log.h"

#include <string.h>
#include <stdlib.h>

struct Behavior* g_AIMan = NULL;
struct Behavior* g_AIWoman = NULL;
struct Behavior* g_AIChild = NULL;

int PAIHasField(struct Person* _Person, struct HashTable* _Table) {
	return _Person->Family->Field != NULL;
}

int PAIHasHouse(struct Person* _Person, struct HashTable* _Table) {
	int i;
	struct Array* _Array = _Person->Family->Buildings;
	void** _PerTbl = _Array->Table;

	for(i = 0; i < _Array->Size; ++i)
		if((((struct Building*)_PerTbl[i])->ResidentType & ERES_HUMAN) == ERES_HUMAN)
			return 1;
	HashInsert(_Table, AI_MAKEGOOD, AI_HOUSE);
	return 0;
}

int PAIWorkField(struct Person* _Person, struct HashTable* _Table) {
	struct Family* _Family = _Person->Family;
	struct Field* _Field = _Family->Field;
	struct Array* _Array = NULL;
	int i;

	if(_Field == NULL)
		return 0;
	if(_Field->Status == EFALLOW) {
		_Array = _Family->Goods;
		for(i = 0; i < _Array->Size; ++i) {
			if(strcmp(((struct GoodBase*)_Array->Table[i])->Name, _Field->Crop->Name) == 0) {
				FieldPlant(_Field, _Array->Table[i]);
				break;
			}
		}
	} else if(_Field->Status == EHARVESTING) {
		struct GoodBase* _CropSeed = NULL;
		struct Good* _Good = NULL;

		if((_CropSeed = HashSearch(&g_Goods, _Field->Crop->Name)) == 0)
			return 0;
		_Good->Base = CopyGoodBase(_CropSeed);
		if(_Good == NULL)
			ArrayInsert_S(_Family->Goods, _Good);
		FieldHarvest(_Field, _Good);
	} else {
		FieldWork(_Field, PersonWorkMult(_Person));
	}
	return 1;
}

int PAIBuildHouse(struct Person* _Person, struct HashTable* _Table) {
	struct Construction* _House = NULL;

	if((_House = ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_CONSTRUCTION)) == NULL) {
		ATimerInsert(&g_ATimer, CreateConstruct(HashSearch(&g_Buildings, "Cottage"), _Person));
	} else {
		--_House->DaysLeft;
	}
	return 1;
}

int PAICanFarm(struct Person* _Person, struct HashTable* _Table) {
	struct Family* _Family = _Person->Family;
	struct Array* _Array = _Family->Goods;
	struct GoodBase* _Good = NULL;
	int i;
	int _Tools = 0;

	if(!PAIHasField(_Person, _Table))
		return 0;
	for(i = 0; i < _Array->Size; ++i) {
		_Good = _Array->Table[i];
		if(_Good->Category == ETOOL)
			_Tools |= ((struct ToolBase*)_Good)->Function;
	}
	return ((_Tools & (ETOOL_PLOW | ETOOL_REAP)) == (ETOOL_PLOW | ETOOL_REAP)) ? (1) : (0);
}

int PAIHasPlow(struct Person* _Person, struct HashTable* _Table) {
	struct Array* _Goods = _Person->Family->Goods;
	const struct GoodBase* _Good = NULL;
	int i;

	for(i = 0; i < _Goods->Size; ++i) {
		_Good = ((struct Good*)_Goods->Table[i])->Base;
		if(_Good->Category == ETOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_PLOW)
				return 1;
	}
	HashInsert(_Table, AI_MAKEGOOD, AI_PLOW);
	return 0;
}

int PAIMakeGood(struct Person* _Person, struct HashTable* _Table) {
	struct GoodBase* _Good = HashSearch(&g_Goods, HashSearch(_Table, AI_MAKEGOOD));
	struct Good* _OwnedGood = NULL;
	struct GoodDep* _GoodDep = NULL;
	struct Family* _Family = _Person->Family;
	int _Size;
	void** _GoodTbl = NULL;
	int i;

	if(_Good == NULL)
		return 0;
	_GoodDep = GoodDependencies(g_GoodDeps, _Good);
	_Size = _GoodDep->DepTbl->Size;
	_GoodTbl = _GoodDep->DepTbl->Table;
	struct Good* _GoodIndxs[_Size];
	for(i = 0; i < _Size; ++i)
		if((_GoodIndxs[i] =
				bsearch(_GoodTbl[i],
						_Family->Goods->Table,
						_Family->Goods->Size,
						sizeof(struct GoodDep*),
						(int(*)(const void*, const void*))IdISCallback)) == NULL
				|| _GoodIndxs[i]->Quantity < ((struct InputReq*)_GoodTbl[i])->Quantity)
			return 0;
	for(i = 0; i < _Size; ++i) {
		_GoodIndxs[i]->Quantity -= ((struct InputReq*)_GoodTbl[i])->Quantity;
	}
	if((_OwnedGood = bsearch(_Good, _Family->Goods->Table, _Family->Goods->Size, sizeof(struct Good*), (int(*)(const void*, const void*))IdISCallback)) == NULL) {
		_OwnedGood = CreateGood(_Good);
		_OwnedGood->Quantity = 1;
	} else {
		++_OwnedGood->Quantity;
	}
	return 1;
}

int PAIHasReap(struct Person* _Person, struct HashTable* _Table) {
	struct Array* _Goods = _Person->Family->Goods;
	struct GoodBase* _Good = NULL;
	int i;

	for(i = 0; i < _Goods->Size; ++i) {
		_Good = (struct GoodBase*)_Goods->Table[i];
		if(_Good->Category == ETOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_REAP)
				return 1;
	}
	HashInsert(_Table, AI_MAKEGOOD, AI_REAP);
	return 0;
}

int PAIHasAnimals(struct Person* _Person, struct HashTable* _Table) {
	if(_Person->Family->Animals->Size > 0)
		return 1;
	return 0;
}

int PAIConstructBuild(struct Person* _Person, struct HashTable* _Table) {
	return 1;
}

int PAIHasShelter(struct Person* _Person, struct HashTable* _Table) {
	int i;
	struct Array* _Array = _Person->Family->Buildings;
	void** _PerTbl = _Array->Table;

	for(i = 0; i < _Array->Size; ++i)
		if((((struct Building*)_PerTbl[i])->ResidentType & ERES_ANIMAL) == ERES_ANIMAL)
			return 1;
	HashInsert(_Table, AI_MAKEGOOD, AI_SHELTER);
	return 0;
}

int PAIFeedAnimals(struct Person* _Person, struct HashTable* _Table) {
	int i;
	int _Size = _Person->Family->Goods->Size;
	void** _Tbl = _Person->Family->Goods->Table;
	struct Animal* _Animal = NULL;

	return 1;
}

int PAIEat(struct Person* _Person, struct HashTable* _Table) {
	int _Size = _Person->Family->Goods->Size;
	struct Good** _Tbl = (struct Good**)_Person->Family->Goods->Table;
	struct Food* _Food;
	int _Nut = 0;
	int _NutReq = NUTRITION_LOSS;
	int i;
	
	for(i = 0; i < _Size; ++i) {
		_Food = (struct Food*)_Tbl[i];
		if(_Food->Base->Category != EFOOD)
			continue;
		while(_Nut < _NutReq && _Food->Quantity > 0) {
			if(_Food->Base->Nutrition > _NutReq) {
				int _Div = _NutReq - _Nut;
				
				if(_Div > _Food->Parts) {
					_Div = _Food->Parts;
					--_Food->Quantity;
					_Food->Parts = FOOD_MAXPARTS;
				} else
					_Food->Parts -= _Div;
				_Nut += _Food->Base->Nutrition / (FOOD_MAXPARTS - _Div + 1);
			} else {
				if(_Food->Parts != FOOD_MAXPARTS) {
					--_Food->Quantity;
					_Food->Parts = FOOD_MAXPARTS;
					_Nut += _Food->Base->Nutrition / (FOOD_MAXPARTS - _Food->Parts);
				}
			}
		}
	}
	if(_Nut == 0)
		Log(ELOG_WARNING, "%i has no food to eat.", _Person->Id);
	return 1;
}

int PAIMakeFood(struct Person* _Person, struct HashTable* _Table) {
	int _Size = 0;
	int i;
	int j;
	int _Ct;
	struct Family* _Family = _Person->Family;
	struct InputReq** _Foods = BuildList(_Family->Goods, &_Size, EFOOD);
	struct Food* _Food = NULL;
	struct Good** _Input = NULL;
	int _FamGoodSize = _Family->Goods->Size;
	struct Good** _Tbl = (struct Good**)_Family->Goods->Table;
	
	for(i = 0; i < _Size; ++i) {
		_Food = ((struct Good*)_Foods[i]);
		_Input = (struct Good**)calloc(_Food->Base->IGSize, sizeof(struct Good*));
		for(j = 0, _Ct = 0; j < _FamGoodSize; ++j) {
			if(_Tbl[j]->Base->Id == _Food->Base->Id)
				_Input[_Ct++] = _Tbl[j];
		}
		free(_Input);
		DestroyInputReq(_Foods[i]);
	}
	free(_Foods);
	return 1;
}

int BHVNothing(struct Person* _Person, struct HashTable* _Table) {
	return 1;
}

void AIInit() {
	g_AIMan = CreateBHVComp(BHV_SEQUENCE,
				CreateBHVComp(BHV_SEQUENCE,
						CreateBHVNode(PAIHasField),
						CreateBHVComp(BHV_SELECTOR,
								CreateBHVNode(PAIHasPlow),
								CreateBHVNode(PAIMakeGood),
								NULL),
						CreateBHVComp(BHV_SELECTOR,
								CreateBHVNode(PAIHasReap),
								CreateBHVNode(PAIMakeGood),
								NULL),
						CreateBHVNode(PAIWorkField),
						NULL),
				CreateBHVComp(BHV_SELECTOR,
						CreateBHVNode(PAIHasHouse),
						CreateBHVNode(PAIConstructBuild),
						NULL),
				CreateBHVComp(BHV_SELECTOR,
						CreateBHVNode(PAIHasAnimals),
						CreateBHVComp(BHV_SELECTOR,
									CreateBHVNode(PAIHasShelter),
									CreateBHVNode(PAIConstructBuild),
									NULL),
						CreateBHVNode(PAIFeedAnimals),
						NULL),
				CreateBHVNode(PAIEat),
				CreateBHVNode(PersonUpdate),
				NULL);
	g_AIWoman = CreateBHVComp(BHV_SEQUENCE,
					CreateBHVNode(PAIEat),
					CreateBHVNode(PersonUpdate),
					NULL);
	g_AIChild = CreateBHVComp(BHV_SEQUENCE,
					CreateBHVNode(PAIEat),
					CreateBHVNode(PersonUpdate),
					NULL);
}

void AIQuit() {
	DestroyBehavior(g_AIMan);
	DestroyBehavior(g_AIWoman);
	DestroyBehavior(g_AIChild);
}
