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
#include "../Profession.h"
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

int SortBhvActions(const struct LuaBhvAction* One, const struct LuaBhvAction* Two) {
	return strcmp(One->Name, Two->Name);
}

int BehaviorsInit(lua_State* State) {

	g_BhvActionsSz = LuaActionLen(g_BhvActions);
	InsertionSort(g_BhvActions, g_BhvActionsSz, (CompCallback) SortBhvActions, sizeof(struct LuaBhvAction));
	return 1;
}

int LuaBaCmp(const void* One, const void* Two) {
	return strcmp(((struct LuaBhvAction*)One)->Name, ((struct LuaBhvAction*)Two)->Name);
}

int PopulationInputReqCmp(const void* One, const void* Two) {
	return ((struct Population*)One)->Id - ((struct Population*)((struct InputReq*)Two)->Req)->Id;
}

int PAIHasField(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	return (Family->Prof == PROF_FARMER) ? (Family->Farmer.FieldCt > 0) : (0);
}

int PAIHasHouse(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	//for(int i = 0; i < Family->BuildingCt; ++i)
	//	if((Family->Buildings[i]->ResidentType & ERES_HUMAN) == ERES_HUMAN)
	//		return 1;
	return 0;
}

int PAIBuildHouse(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	return 1;
}

int PAICanFarm(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	struct Array* Array = &Family->Goods;
	struct GoodBase* Good = NULL;
	int Tools = 0;

	if(!PAIHasField(Family, Vars, Args, ArgSize))
		return 0;
	for(int i = 0; i < Array->Size; ++i) {
		Good = Array->Table[i];
		if(Good->Category == GOOD_TOOL)
			Tools |= ((struct ToolBase*)Good)->Function;
	}
	return ((Tools & (ETOOL_PLOW | ETOOL_REAP)) == (ETOOL_PLOW | ETOOL_REAP)) ? (1) : (0);
}

int PAIHasPlow(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	struct Array* Goods = &Family->Goods;
	const struct GoodBase* Good = NULL;

	for(int i = 0; i < Goods->Size; ++i) {
		Good = ((struct Good*)Goods->Table[i])->Base;
		if(Good->Category == GOOD_TOOL)
			if(((struct ToolBase*)Good)->Function == ETOOL_PLOW)
				return 1;
	};
	return 0;
}

int PAIMakeGood(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	const struct GoodBase* Good = NULL;

	if(Args[0].Type != PRIM_STRING && Args[1]. Type != PRIM_INTEGER)
		return 0;
	Good = HashSearch(&g_Goods, Args[0].Value.String);
	GoodMake(Good, Args[1].Value.Int, &Family->Goods);
	return 1;
}

int PAIHasReap(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	struct Array* Goods = &Family->Goods;
	struct GoodBase* Good = NULL;

	for(int i = 0; i < Goods->Size; ++i) {
		Good = (struct GoodBase*)Goods->Table[i];
		if(Good->Category == GOOD_TOOL)
			if(((struct ToolBase*)Good)->Function == ETOOL_REAP)
				return 1;
	}
	return 0;
}

int PAIHasAnimals(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	if(Family->Animals.Size > 0)
		return 1;
	return 0;
}

int PAIConstructBuild(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	return 1;
}

int PAIHasShelter(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
/*	int i;
	struct Array* Array = &Family->Buildings;
	void** PerTbl = Array->Table;

	for(i = 0; i < Array->Size; ++i)
		if((((struct Building*)PerTbl[i])->ResidentType & ERES_ANIMAL) == ERES_ANIMAL)
			return 1;*/
	return 0;
}

int PAIFeedAnimals(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	/*int i;
	int j;
	int k;
	int AnSize = 0;
	int TotalNut = 0;
	struct StackNode Stack;
	struct InputReq** AnimalCt = AnimalTypeCount(&Family->Animals, &AnSize);
	struct InputReq* Req = NULL;
	struct Food* Food = NULL;
	struct AnimalDep* Dep = NULL;

	if(AnSize == 0)
		return 1;

	Stack.Prev = NULL;
	Stack.Data = NULL;
	for(i = 0; i < g_GameWorld.AnFoodDeps->Size; ++i) {
		for(j = 0; ((struct AnimalDep*)g_GameWorld.AnFoodDeps->Table[i])->Animals->Size; ++j) {
			Dep = ((struct AnimalDep*)g_GameWorld.AnFoodDeps->Table[i]);
			if((Req = BinarySearch(Dep->Animals->Table[j], AnimalCt, AnSize, PopulationInputReqCmp)) == NULL)
				continue;
			TotalNut += Req->Quantity * ((struct Population*)Req->Req)->Nutrition;
			if(Food->Quantity >= TotalNut) {
				//TODO: We can do better than this.
				for(k = 0; k < Family->Animals.Size; ++k) {
					if(PopulationCmp(Family->Animals.Table[k], Req->Req) == 0)
						ActorFeed(Family->Animals.Table[k], ((struct Animal*)Family->Animals.Table[k])->PopType->Nutrition);
				}
				Food->Quantity -= TotalNut;
			} else {
				//struct StackNode Top;

				//_Top.Prev = &Stack;
				//_Top.Data = Dep;
			}
		}
	}
	//TODO: Finish below loop.
	while(Stack.Prev != NULL) {
		if((Req = BinarySearch(Dep->Animals->Table[j], AnimalCt, AnSize, PopulationInputReqCmp)) == NULL)
			continue;
		Stack = *Stack.Prev;
	}
	for(i = 0; i < AnSize; ++i)
		free(AnimalCt[i]);
	free(AnimalCt);*/
	return 1;
}

int PAIMakeFood(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	FamilyMakeFood(Family);
	return 1;
}

int PAIHasAnimal(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	const struct Population* Pop = NULL;

	if(Args[0].Type != PRIM_STRING)
		return 0;
	Pop = HashSearch(&g_Populations, Args[0].Value.String);
	for(int i = 0; i < Family->Animals.Size; ++i) {
		if(((struct Animal*)Family->Animals.Table[i])->PopType == Pop)
			return 1;
	}
	return 0;
}

int PAIBuyAnimal(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	return 0;
}

int PAIHasGood(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	const struct GoodBase* Base = NULL;

	if(Args[0].Type != PRIM_STRING)
		return 0;
	Base = HashSearch(&g_Goods, Args[0].Value.String);
	for(int i = 0; i < Family->Goods.Size; ++i) {
		if(((struct Good*)Family->Goods.Table[i])->Base == Base)
			return 1;
	}
	return 0;
}

int PAIBuyGood(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	const struct GoodBase* Base = NULL;

	if(Args[0].Type != PRIM_STRING || Args[1].Type != PRIM_INTEGER)
		return 0;
	if((Base = HashSearch(&g_Goods, Args[0].Value.String)) == NULL) {
		Log(ELOG_WARNING, "BuyGood: %s is not a good.", Args[0].Value.String);
		return 0;
	}
	GoodBuy(Family, Base, Args[1].Value.Int);
	return 1;
}

int BHVNothing(struct Family* Family, struct HashTable* Vars, const struct Primitive* Args, int ArgSize) {
	return 1;
}

int LuaActionLen(const struct LuaBhvAction* Action) {
	int i = 0;

	while(Action[i].Name != NULL)
		++i;
	return i;
}
