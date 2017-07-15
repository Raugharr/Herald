/*
 * File: Behaviors.c
 * Author: David Brotz
 */

#include "Behaviors.h"

#include "../sys/Array.h"
#include "../sys/LinkedList.h"
#include "../sys/HashTable.h"
#include "../sys/Log.h"
#include "../sys/Stack.h"
#include "../sys/LuaCore.h"
#include "../sys/Math.h"

#include "../BigGuy.h"
#include "../Population.h"
#include "../Building.h"
#include "../Good.h"
#include "../Crop.h"
#include "../Person.h"
#include "../Family.h"
#include "../sys/Array.h"
#include "../Herald.h"
#include "../Location.h"
#include "../ArmyGoal.h"
#include "../Government.h"
#include "../Warband.h"

#include <string.h>
#include <stdlib.h>

struct LuaBhvAction g_BhvActions[] = {
	{"BuildHouse", PAIBuildHouse, 0},
	{"CanFarm", PAICanFarm, 0},
	{"ConstructBuilding", PAIConstructBuild, 0},
	{"FeedAnimals", PAIFeedAnimals, 0},
	{"HasAnimals", PAIHasAnimals, 0},
	{"HasAnimal", PAIHasAnimal, 1},
	{"BuyAnimal", PAIBuyAnimal, 2},
	{"HasField", PAIHasField, 0},
	{"HasHouse", PAIHasHouse},
	{"HasPlow", PAIHasPlow, 0},
	{"HasReap", PAIHasReap, 0},
	{"HasShelter", PAIHasShelter, 0},
	{"MakeFood", PAIMakeFood, 0},
	{"MakeGood", PAIMakeGood, 2},
	{"PurchaseGood", BHVNothing, 2},
	{"BuyGood", PAIBuyGood, 2},
	{"HasGood", PAIHasGood, 1},
	{"Nothing", BHVNothing, 0},
	{"Season", BHVNothing, 1},
	{"Hunt", BHVNothing, 0},
	{"WorkFields", BHVNothing, 0},
	{"SlaughterAnimals", BHVNothing, 0},
	{NULL, NULL}
};

int g_BhvActionsSz = 0;

int SortBhvActions(const struct LuaBhvAction* _One, const struct LuaBhvAction* _Two) {
	return strcmp(_One->Name, _Two->Name);
}

int BehaviorsInit(lua_State* _State) {

	g_BhvActionsSz = LuaActionLen(g_BhvActions);
	InsertionSort(g_BhvActions, g_BhvActionsSz, (CompCallback) SortBhvActions, sizeof(struct LuaBhvAction));
	return 1;
}

int LuaBaCmp(const void* _One, const void* _Two) {
	return strcmp(((struct LuaBhvAction*)_One)->Name, ((struct LuaBhvAction*)_Two)->Name);
}

int PopulationInputReqCmp(const void* _One, const void* _Two) {
	return ((struct Population*)_One)->Id - ((struct Population*)((struct InputReq*)_Two)->Req)->Id;
}

int PAIHasField(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	return (_Family->Prof == PROF_FARMER) ? (_Family->Spec.Farmer.FieldCt > 0) : (0);
}

int PAIHasHouse(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	//for(int i = 0; i < _Family->BuildingCt; ++i)
	//	if((_Family->Buildings[i]->ResidentType & ERES_HUMAN) == ERES_HUMAN)
	//		return 1;
	return 0;
}

int PAIBuildHouse(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	return 1;
}

int PAICanFarm(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	struct Array* _Array = &_Family->Goods;
	struct GoodBase* _Good = NULL;
	int _Tools = 0;

	if(!PAIHasField(_Family, _Vars, _Args, _ArgSize))
		return 0;
	for(int i = 0; i < _Array->Size; ++i) {
		_Good = _Array->Table[i];
		if(_Good->Category == GOOD_TOOL)
			_Tools |= ((struct ToolBase*)_Good)->Function;
	}
	return ((_Tools & (ETOOL_PLOW | ETOOL_REAP)) == (ETOOL_PLOW | ETOOL_REAP)) ? (1) : (0);
}

int PAIHasPlow(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	struct Array* _Goods = &_Family->Goods;
	const struct GoodBase* _Good = NULL;

	for(int i = 0; i < _Goods->Size; ++i) {
		_Good = ((struct Good*)_Goods->Table[i])->Base;
		if(_Good->Category == GOOD_TOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_PLOW)
				return 1;
	};
	return 0;
}

int PAIMakeGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	const struct GoodBase* _Good = NULL;

	if(_Args[0].Type != PRIM_STRING && _Args[1]. Type != PRIM_INTEGER)
		return 0;
	_Good = HashSearch(&g_Goods, _Args[0].Value.String);
	GoodMake(_Good, _Args[1].Value.Int, &_Family->Goods);
	return 1;
}

int PAIHasReap(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	struct Array* _Goods = &_Family->Goods;
	struct GoodBase* _Good = NULL;

	for(int i = 0; i < _Goods->Size; ++i) {
		_Good = (struct GoodBase*)_Goods->Table[i];
		if(_Good->Category == GOOD_TOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_REAP)
				return 1;
	}
	return 0;
}

int PAIHasAnimals(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	if(_Family->Animals.Size > 0)
		return 1;
	return 0;
}

int PAIConstructBuild(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	return 1;
}

int PAIHasShelter(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
/*	int i;
	struct Array* _Array = &_Family->Buildings;
	void** _PerTbl = _Array->Table;

	for(i = 0; i < _Array->Size; ++i)
		if((((struct Building*)_PerTbl[i])->ResidentType & ERES_ANIMAL) == ERES_ANIMAL)
			return 1;*/
	return 0;
}

int PAIFeedAnimals(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	/*int i;
	int j;
	int k;
	int _AnSize = 0;
	int _TotalNut = 0;
	struct StackNode _Stack;
	struct InputReq** _AnimalCt = AnimalTypeCount(&_Family->Animals, &_AnSize);
	struct InputReq* _Req = NULL;
	struct Food* _Food = NULL;
	struct AnimalDep* _Dep = NULL;

	if(_AnSize == 0)
		return 1;

	_Stack.Prev = NULL;
	_Stack.Data = NULL;
	for(i = 0; i < g_GameWorld.AnFoodDeps->Size; ++i) {
		for(j = 0; ((struct AnimalDep*)g_GameWorld.AnFoodDeps->Table[i])->Animals->Size; ++j) {
			_Dep = ((struct AnimalDep*)g_GameWorld.AnFoodDeps->Table[i]);
			if((_Req = BinarySearch(_Dep->Animals->Table[j], _AnimalCt, _AnSize, PopulationInputReqCmp)) == NULL)
				continue;
			_TotalNut += _Req->Quantity * ((struct Population*)_Req->Req)->Nutrition;
			if(_Food->Quantity >= _TotalNut) {
				//TODO: We can do better than this.
				for(k = 0; k < _Family->Animals.Size; ++k) {
					if(PopulationCmp(_Family->Animals.Table[k], _Req->Req) == 0)
						ActorFeed(_Family->Animals.Table[k], ((struct Animal*)_Family->Animals.Table[k])->PopType->Nutrition);
				}
				_Food->Quantity -= _TotalNut;
			} else {
				//struct StackNode _Top;

				//_Top.Prev = &_Stack;
				//_Top.Data = _Dep;
			}
		}
	}
	//TODO: Finish below loop.
	while(_Stack.Prev != NULL) {
		if((_Req = BinarySearch(_Dep->Animals->Table[j], _AnimalCt, _AnSize, PopulationInputReqCmp)) == NULL)
			continue;
		_Stack = *_Stack.Prev;
	}
	for(i = 0; i < _AnSize; ++i)
		free(_AnimalCt[i]);
	free(_AnimalCt);*/
	return 1;
}

int PAIMakeFood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	FamilyMakeFood(_Family);
	return 1;
}

int PAIHasAnimal(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	const struct Population* _Pop = NULL;

	if(_Args[0].Type != PRIM_STRING)
		return 0;
	_Pop = HashSearch(&g_Populations, _Args[0].Value.String);
	for(int i = 0; i < _Family->Animals.Size; ++i) {
		if(((struct Animal*)_Family->Animals.Table[i])->PopType == _Pop)
			return 1;
	}
	return 0;
}

int PAIBuyAnimal(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	return 0;
}

int PAIHasGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	const struct GoodBase* _Base = NULL;

	if(_Args[0].Type != PRIM_STRING)
		return 0;
	_Base = HashSearch(&g_Goods, _Args[0].Value.String);
	for(int i = 0; i < _Family->Goods.Size; ++i) {
		if(((struct Good*)_Family->Goods.Table[i])->Base == _Base)
			return 1;
	}
	return 0;
}

int PAIBuyGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	const struct GoodBase* _Base = NULL;

	if(_Args[0].Type != PRIM_STRING || _Args[1].Type != PRIM_INTEGER)
		return 0;
	if((_Base = HashSearch(&g_Goods, _Args[0].Value.String)) == NULL) {
		Log(ELOG_WARNING, "BuyGood: %s is not a good.", _Args[0].Value.String);
		return 0;
	}
	GoodBuy(_Family, _Base, _Args[1].Value.Int);
	return 1;
}

int BHVNothing(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize) {
	return 1;
}

int LuaActionLen(const struct LuaBhvAction* _Action) {
	int i = 0;

	while(_Action[i].Name != NULL)
		++i;
	return i;
}
