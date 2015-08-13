/*
 * File: Setup.c
 * Author: David Brotz
 */

#include "Setup.h"

#include "BehaviorTree.h"
#include "LuaLib.h"
#include "AIHelper.h"
#include "goap.h"
#include "Utility.h"
#include "../BigGuy.h"
#include "../Actor.h"
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

#include "../sys/LinkedList.h"
#include "../sys/HashTable.h"
#include "../sys/Log.h"
#include "../sys/Stack.h"
#include "../sys/LuaCore.h"
#include "../sys/Math.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Array g_BhvList;
struct LuaBhvAction g_BhvActions[] = {
	{"BuildHouse", PAIBuildHouse},
	{"CanFarm", PAICanFarm},
	{"ConstructBuilding", PAIConstructBuild},
	{"FeedAnimals", PAIFeedAnimals},
	{"HasAnimals", PAIHasAnimals},
	{"HasField", PAIHasField},
	{"HasHouse", PAIHasHouse},
	{"HasPlow", PAIHasPlow},
	{"HasReap", PAIHasReap},
	{"HasShelter", PAIHasShelter},
	{"IsMale", PAIIsMale},
	{"MakeFood", PAIMakeFood},
	{"MakeGood", PAIMakeGood},
	{"Nothing", BHVNothing},
	{"WorkField", PAIWorkField},
	{NULL, NULL}
};

static struct AgentUtility g_BigGuyPlanner;

int g_BhvActionsSz = 0;

int LuaBaCmp(const void* _One, const void* _Two) {
	return strcmp(((struct LuaBhvAction*)_One)->Name, ((struct LuaBhvAction*)_Two)->Name);
}

int PopulationInputReqCmp(const void* _One, const void* _Two) {
	return ((struct Population*)_One)->Id - ((struct Population*)((struct InputReq*)_Two)->Req)->Id;
}


int PAIHasField(struct Person* _Person, struct HashTable* _Table) {
	return _Person->Family->Fields->Table != NULL;
}

int PAIHasHouse(struct Person* _Person, struct HashTable* _Table) {
	int i;
	struct Array* _Array = _Person->Family->Buildings;
	void** _PerTbl = _Array->Table;
	struct HashNode* _Search = NULL;

	for(i = 0; i < _Array->Size; ++i)
		if((((struct Building*)_PerTbl[i])->ResidentType & ERES_HUMAN) == ERES_HUMAN)
			return 1;
	if((_Search = HashSearchNode(_Table, AI_MAKEGOOD)) != NULL) {
		free(_Search->Pair);
		_Search->Pair = AI_HOUSE;
	} else
		HashInsert(_Table, AI_MAKEGOOD, AI_HOUSE);
	return 0;
}

int PAIWorkField(struct Person* _Person, struct HashTable* _Table) {
	return 1;
}

int PAIBuildHouse(struct Person* _Person, struct HashTable* _Table) {
	/*struct Construction* _House = NULL;

	if((_House = ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_CONSTRUCTION)) == NULL) {
		ATimerInsert(&g_ATimer, CreateConstruct(NULL, _Person));
	} else {
		--_House->DaysLeft;
	}*/
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
	struct HashNode* _Search = NULL;
	int i;

	for(i = 0; i < _Goods->Size; ++i) {
		_Good = ((struct Good*)_Goods->Table[i])->Base;
		if(_Good->Category == ETOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_PLOW)
				return 1;
	}
	if((_Search = HashSearchNode(_Table, AI_MAKEGOOD)) != NULL)
		_Search->Pair = AI_PLOW;
	else
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
		_OwnedGood = CreateGood(_Good, _Person->Pos.x, _Person->Pos.y);
		_OwnedGood->Quantity = 1;
	} else {
		++_OwnedGood->Quantity;
	}
	return 1;
}

int PAIHasReap(struct Person* _Person, struct HashTable* _Table) {
	struct Array* _Goods = _Person->Family->Goods;
	struct GoodBase* _Good = NULL;
	struct HashNode* _Search = NULL;
	int i;

	for(i = 0; i < _Goods->Size; ++i) {
		_Good = (struct GoodBase*)_Goods->Table[i];
		if(_Good->Category == ETOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_REAP)
				return 1;
	}
	if((_Search = HashSearchNode(_Table, AI_MAKEGOOD)) != NULL)
		_Search->Pair = AI_REAP;
	else
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
	int j;
	int k;
	int _AnSize = 0;
	int _TotalNut = 0;
	struct Family* _Family = _Person->Family;
	struct StackNode _Stack;
	struct InputReq** _AnimalCt = AnimalTypeCount(_Family->Animals, &_AnSize);
	struct InputReq* _Req = NULL;
	struct Food* _Food = NULL;
	struct AnimalDep* _Dep = NULL;

	if(_AnSize == 0)
		return 1;

	_Stack.Prev = NULL;
	_Stack.Data = NULL;
	for(i = 0; i < g_AnFoodDep->Size; ++i) {
		for(j = 0; ((struct AnimalDep*)g_AnFoodDep->Table[i])->Animals->Size; ++j) {
			_Dep = ((struct AnimalDep*)g_AnFoodDep->Table[i]);
			if((_Req = BinarySearch(_Dep->Animals->Table[j], _AnimalCt, _AnSize, PopulationInputReqCmp)) == NULL)
				continue;
			_TotalNut += _Req->Quantity * ((struct Population*)_Req->Req)->Nutrition;
			if(_Food->Quantity >= _TotalNut) {
				//TODO: We can do better than this.
				for(k = 0; k < _Family->Animals->Size; ++k) {
					if(PopulationCmp(_Family->Animals->Table[k], _Req->Req) == 0)
						ActorFeed(_Family->Animals->Table[k], NUTRITION_LOSS);
				}
				_Food->Quantity -= _TotalNut;
			} else {
				struct StackNode _Top;

				_Top.Prev = &_Stack;
				_Top.Data = _Dep;
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
	free(_AnimalCt);
	return 1;
}

int PAIEat(struct Person* _Person, struct HashTable* _Table) {
	/*struct Family* _Family = _Person->Family;
	struct Food* _CloseFood = NULL;
	int _BestDist = INT_MAX;
	int _Distance = 0;
	int i = 0;

	for(i = 0; i < _Family->Goods->Size; ++i) {
		if(((struct Good*)_Family->Goods->Table[i])->Base->Category == EFOOD) {
			if((_Distance = Distance(_Person->X, _Person->Y, ((struct Food*)_Family->Goods->Table[i])->X, ((struct Food*)_Family->Goods->Table[i])->Y)) < _BestDist) {
				_BestDist = _Distance;
				_CloseFood = (struct Food*)_Family->Goods->Table[i];
			}
		}
	}
	ActorAddJob(ACTORJOB_EAT, (struct Actor*)_Person, (struct Object*)_CloseFood, NULL);*/
	return 1;
}

int PAIMakeFood(struct Person* _Person, struct HashTable* _Table) {
	int _Size;
	int i;
	int j;
	struct Family* _Family = _Person->Family;
	struct InputReq** _Foods = GoodBuildList(_Family->Goods, &_Size, EFOOD | ESEED | EINGREDIENT);
	struct Array* _GoodsArray = NULL;
	struct FoodBase* _Food = NULL;
	struct Food* _FamFood = NULL;
	struct Good* _Good = NULL;
	
	if(_Foods == NULL)
		return 1;

	for(i = 0; i < _Size; ++i) {
		_Food = ((struct FoodBase*)_Foods[i]->Req);
		for(j = 0; j < _Food->IGSize; ++j) {
			_Good = LinearSearch(_Food->InputGoods[j], _Family->Goods->Table, _Family->Goods->Size, (int(*)(const void*, const void*))InputReqGoodCmp);
			_Good->Quantity -= _Foods[i]->Quantity * _Food->InputGoods[j]->Quantity;
			_GoodsArray = _Family->Goods;
			if((_FamFood = LinearSearch(_Food, _GoodsArray->Table, _GoodsArray->Size, GoodCmp)) == NULL) {
				_FamFood =  CreateFood(_Food, _Person->Pos.x, _Person->Pos.y);
				ArrayInsert_S(_GoodsArray, _FamFood);
			}
			_FamFood->Quantity += _Foods[i]->Quantity;
		}
		DestroyInputReq(_Foods[i]);
	}
	if(i == 0)
		Log(ELOG_WARNING, "Day %i: %i made no food in PAIMakeFood.", DateToDays(g_GameWorld.Date), _Person->Id);
	free(_Foods);
	return 1;
}

int PAIIsMale(struct Person* _Person, struct HashTable* _Table) {
	if(_Person->Gender == EMALE)
		return 1;
	return 0;
}

int BHVNothing(struct Person* _Person, struct HashTable* _Table) {
	return 1;
}

int LuaActionLen(const struct LuaBhvAction* _Action) {
	int i = 0;

	while(_Action[i].Name != NULL)
		++i;
	return i;
}

const struct AgentUtility* GetBGPlanner() {
	return &g_BigGuyPlanner;
}

int BGImproveRelations(const void* _Data, const void* _Extra) {
	return 120;
}

int BGImproveRelationsAction(struct BigGuy* _Guy) {
	struct BigGuyRelation* _Relation = NULL;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct BigGuy* _Person = NULL;
	int _Rand = Random(0, _Settlement->BigGuys.Size - 1);
	int _Ct = 0;

	if(_Guy->Action.ActionFunc == NULL) {
		while(_Itr != NULL) {
			_Person = (struct BigGuy*)_Itr->Data;
			if(_Person != _Guy) {
				 if(_Ct >= _Rand)
					 break;
				++_Ct;
			}
			_Itr = _Itr->Next;
		}
		BigGuySetAction(_Guy, BGACT_IMRPOVEREL, _Person);
		return 0;
	}
	_Relation = BigGuyGetRelation((struct BigGuy*)_Guy->Action.Data, _Guy);
	if(_Relation->Modifier >= BIGGUY_LIKEMIN) {
		BigGuySetAction(_Guy, BGACT_NONE, NULL);
		return 1;
	}
	return 0;
}

int UtilityMakeFriends(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	int _Friends = 0;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	const struct BigGuy* _Person = NULL;
	const struct BigGuyRelation* _Relation = NULL;

	*_Min = 0;
	*_Max = _Settlement->BigGuys.Size - 1;
	while(_Itr != NULL) {
		_Person = (struct BigGuy*)_Itr->Data;
		if(_Person == _Guy)
			goto end_loop;
		if((_Relation = BigGuyGetRelation(_Guy, _Person)) != NULL && _Relation->Relation >= BGREL_LIKE)
			++_Friends;
		end_loop:
		_Itr = _Itr->Next;
	}
	WorldStateSetAtom(_State, BGBYTE_IMPROVINGRELATION, 1);
	return _Friends;
}

int BGRaiseFyrd(const void* _Data, const void* _Extra) {
	return 1;
}

int BGRaiseFyrdAction(struct BigGuy* _Guy) {
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct ArmyGoal _Goal;
	struct LinkedList _List = {0, NULL, NULL};

	WorldSettlementsInRadius(&g_GameWorld, &_Settlement->FirstPart->Pos, 20, &_List);
	if(_List.Size <= 0)
		goto end;
	ArmyGoalRaid(&_Goal, (struct Settlement*)&_List.Back->Data);
	SettlementRaiseFyrd(_Settlement, &_Goal);
	end:
	LnkLstClear(&_List);
	return 1;
}

int UtilityRaiseFyrdFood(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	int _MaxNutrition = _Settlement->NumPeople * NUTRITION_REQ / 12 * 3; //HOw much nutrition we need for 3 months.
	int _Nutrition = SettlementGetNutrition(_Settlement);

	if(_Nutrition >= _MaxNutrition)
		return 0;
	*_Min = 0;
	*_Max = _MaxNutrition;
	WorldStateSetAtom(_State, BGBYTE_FYRDRAISED, 1);
	return _Nutrition;
}

int BGChallangeLeader(const void* _Data, const void* _Extra) {
	return 1;
}

int BGChallangeLeaderAction(struct BigGuy* _Guy) {
	struct Government* _Government = FamilyGetSettlement(_Guy->Person->Family)->Government;
	struct BigGuy* _Leader = _Government->Leader;

	if(_Guy->Stats.Warfare > _Leader->Stats.Warfare)
		GovernmentSetLeader(_Government, _Guy);
	return 1;
}

int UtilityChallangeLeader(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	struct BigGuy* _Leader =  FamilyGetSettlement(_Guy->Person->Family)->Government->Leader;
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Leader);
	int _Utility = 0;

	*_Min = 0;
	*_Max = 255;
	if(_Guy->Stats.Warfare < _Leader->Stats.Warfare || (_Relation != NULL && _Relation->Relation == BGREL_LOVE))
		return 0;
	_Utility = _Utility + ((_Guy->Stats.Warfare - _Leader->Stats.Warfare) * 5);
	if(_Relation != NULL)
		_Utility = _Utility + ((-_Relation->Modifier) * 2);
	return (_Utility >= 255) ? (255) : (_Utility);
}

void BGSetup() {
	struct GOAPPlanner* _Planner = AUtilityGetGoap(&g_BigGuyPlanner);

	GoapClear(_Planner);
	AUtilityClear(&g_BigGuyPlanner);

	for(int i = 0; i < BGBYTE_SIZE; ++i)
		GoapAddAtom(_Planner, g_BGStateStr[i]);
	GoapAddPostcond(_Planner, "Improve Relations", "ImproveRelations", 1, WSOP_ADD);
	GoapSetActionCost(_Planner, "Improve Relations", BGImproveRelations);
	GoapSetAction(_Planner, "Improve Relations", (int(*)(void*))BGImproveRelationsAction);
	AUtilityAdd(&g_BigGuyPlanner, "MakeFriends", (int(*)(const void*, int*, int*, struct WorldState*))UtilityMakeFriends, (UTILITY_INVERSE | UTILITY_LINEAR));

	GoapAddPostcond(_Planner, "Challenge Leader", "IsLeader", 1, WSOP_SET);
	GoapSetActionCost(_Planner, "Challenge Leader", BGChallangeLeader);
	GoapSetAction(_Planner, "Challenge Leader", (int(*)(void*))BGChallangeLeaderAction);
	AUtilityAdd(&g_BigGuyPlanner, "Challenge Leader", (int(*)(const void*, int*, int*, struct WorldState*))UtilityChallangeLeader, UTILITY_LINEAR);

	GoapAddPrecond(_Planner, "Raid", "FyrdRaised", 0, WSOP_EQUAL);
	GoapAddPostcond(_Planner, "Raid", "FyrdRaised", 1, WSOP_EQUAL);
	GoapAddPostcond(_Planner, "Raid", "Prestige", 2, WSOP_ADD);
	GoapSetActionCost(_Planner, "Raid", BGRaiseFyrd);
	GoapSetAction(_Planner, "Raid", (int(*)(void*))BGRaiseFyrdAction);
	AUtilityAdd(&g_BigGuyPlanner, "Raid", (int(*)(const void*, int*, int*, struct WorldState*))UtilityRaiseFyrdFood, (UTILITY_INVERSE | UTILITY_QUADRATIC));
}

void AIInit(lua_State* _State) {
	int i = 0;
	int _Size = 0;
	const char* _Str = NULL;
	struct LuaBehavior* _Bhv = NULL;

	g_BhvActionsSz = LuaActionLen(g_BhvActions);
	luaL_newlibtable(_State, g_LuaAIFuncs);
	luaL_setfuncs(_State, g_LuaAIFuncs, 0);
	lua_setglobal(_State, "AI");
	if(LuaLoadFile(_State, "ai.lua", NULL) != LUA_OK) {
		exit(1);
	}
	lua_getglobal(_State, "AI");
	lua_pushstring(_State, "Init");
	lua_rawget(_State, -2);
	if(LuaCallFunc(_State, 0, 1, 0) == 0) {
		exit(1);
	}

	_Size = lua_rawlen(_State, -1);
	g_BhvList.Size = 0;
	g_BhvList.Table = calloc(_Size, sizeof(struct Behavior*));
	g_BhvList.TblSize = _Size;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_istable(_State, -1)) {
			luaL_error(_State, "Element #%d of AI.Init should be a table.", i);
			goto BhvListEnd;
		}
		lua_pushnil(_State);
		if(lua_next(_State, -2) != 0) {
			if(!lua_isstring(_State, -1)) {
				luaL_error(_State, "First element of table #%d in AI.Init should be a string.", i);
				goto BhvListEnd;
			}
			_Str = lua_tostring(_State, -1);
			if(BinarySearch(_Str, g_BhvList.Table, g_BhvList.Size, luaStrLuaBhvCmp) != 0) {
				luaL_error(_State, "Element #%d in AI.Init name is already used.", i);
				goto BhvListEnd;
			}
		}
		lua_pop(_State, 1);
		if(lua_next(_State, -2) != 0) {
			if(!lua_islightuserdata(_State, -1)) {
				luaL_error(_State, "Second element of table #%d in AI.Init should be a behavior.", i);
				goto BhvListEnd;
			}
			_Bhv = (struct LuaBehavior*) malloc(sizeof(struct LuaBehavior));
			_Bhv->Name = strcpy(calloc(strlen(_Str) + 1, sizeof(char)), _Str);
			_Bhv->Behavior = lua_touserdata(_State, -1);
		}
		if(_Bhv == NULL) {
			luaL_error(_State, "Cannot add NULL behavior to AI.Init.");
			goto BhvListEnd;
		}
		ArrayInsertSort(&g_BhvList, _Bhv, LuaBhvCmp);
		BhvListEnd:
		++i;
		lua_pop(_State, 3);
		_Bhv = NULL;
	}
	lua_pop(_State, 2);
	GoapInit();
	BGSetup();
}

void AIQuit() {
	int i;

	for(i = 0; i < g_BhvList.Size; ++i) {
		DestroyBehavior(((struct LuaBehavior*)g_BhvList.Table[i])->Behavior);
		free(((struct LuaBehavior*)g_BhvList.Table[i])->Name);
		free(g_BhvList.Table[i]);
	}
	g_BhvList.Size = 0;
	GoapQuit();
}
