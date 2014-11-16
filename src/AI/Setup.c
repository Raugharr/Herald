/*
 * File: Setup.c
 * Author: David Brotz
 */

#include "Setup.h"

#include "BehaviorTree.h"
#include "LuaLib.h"
#include "../Population.h"
#include "../Building.h"
#include "../Good.h"
#include "../Crop.h"
#include "../Person.h"
#include "../Family.h"
#include "../sys/Array.h"
#include "../Herald.h"
#include "../sys/HashTable.h"
#include "../sys/Log.h"
#include "../sys/Stack.h"
#include "../sys/LuaHelper.h"

#include <string.h>
#include <stdlib.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Array g_BhvList;

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
	struct Family* _Family = _Person->Family;
	struct Field* _Field = NULL;;
	struct Array* _Array = NULL;
	int i;
	int j;
	
	for(i = 0; i < _Family->Fields->Size; ++i) {
		_Field = ((struct Field*)_Family->Fields->Table[i]);
		if(_Field == NULL)
			return 0;
		if(_Field->Status == EFALLOW) {
			_Array = _Family->Goods;
			for(j = 0; i < _Array->Size; ++j) {
				if(strcmp(((struct GoodBase*)_Array->Table[j])->Name, _Field->Crop->Name) == 0) {
					FieldPlant(_Field, _Array->Table[j]);
					break;
				}
			}
		} else if(_Field->Status == EHARVESTING) {
			struct GoodBase* _CropSeed = NULL;
			struct Good* _Good = NULL;

			if((_CropSeed = HashSearch(&g_Goods, _Field->Crop->Name)) == 0)
				return 0;
			_Good->Base = CopyGoodBase(_CropSeed);
			if(_Good == NULL) {
				ArrayInsertSort(_Family->Goods, _Good, GoodCmp);
			}
			FieldHarvest(_Field, _Good);
		} else {
			FieldWork(_Field, PersonWorkMult(_Person));
		}
	}
	return 1;
}

int PAIBuildHouse(struct Person* _Person, struct HashTable* _Table) {
	struct Construction* _House = NULL;

	if((_House = ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_CONSTRUCTION)) == NULL) {
		ATimerInsert(&g_ATimer, CreateConstruct(NULL, _Person));
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
	struct HashNode* _Search = NULL;
	int i;

	for(i = 0; i < _Goods->Size; ++i) {
		_Good = ((struct Good*)_Goods->Table[i])->Base;
		if(_Good->Category == ETOOL)
			if(((struct ToolBase*)_Good)->Function == ETOOL_PLOW)
				return 1;
	}
	if((_Search = HashSearchNode(_Table, AI_MAKEGOOD)) != NULL) {
		free(_Search->Pair);
		_Search->Pair = AI_PLOW;
	} else
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
		_OwnedGood = CreateGood(_Good, _Person->X, _Person->Y);
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
	if((_Search = HashSearchNode(_Table, AI_MAKEGOOD)) != NULL) {
		free(_Search->Pair);
		_Search->Pair = AI_REAP;
	} else
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
						AnimalFeed(_Family->Animals->Table[k]);
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
	int _Size;
	int i;
	int j;
	int k;
	struct Family* _Family = _Person->Family;
	struct InputReq** _Foods = GoodBuildList(_Family->Goods, &_Size, EFOOD | ESEED | EINGREDIENT);
	struct Array* _GoodsArray = NULL;
	struct FoodBase* _Food = NULL;
	struct Food* _FamFood = NULL;
	struct Good* _Good = NULL;
	
	if(_Foods == NULL)
		return 1;

	//TODO: Food should have to use a building to be created.
	for(i = 0; i < _Size; ++i) {
		_Food = ((struct FoodBase*)_Foods[i]->Req);
		for(j = 0; j < _Food->IGSize; ++j) {
			_Good = BinarySearch(_Food->InputGoods[j], _Family->Goods->Table, _Family->Goods->Size, (int(*)(const void*, const void*))InputReqGoodCmp);
			_Good->Quantity -= _Foods[i]->Quantity * _Food->InputGoods[j]->Quantity;
			_GoodsArray = _Family->Goods;
			for(k = 0; k < _GoodsArray->Size; ++k) {
				if((_FamFood = ((struct Food*)_GoodsArray->Table[k]))->Base == _Food) {
					_FamFood->Quantity += _Foods[i]->Quantity;
					goto found_good;
				}
			}
			_FamFood =  CreateFood(_Food, _Person->X, _Person->Y);
			_FamFood->Quantity = _Foods[i]->Quantity;
			ArrayInsert_S(_GoodsArray, _FamFood);
			found_good:
			continue;
		}
		DestroyInputReq(_Foods[i]);
	}
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

void AIInit(lua_State* _State) {
	int i = 0;
	int _Size = 0;
	const char* _Str = NULL;
	struct LuaBehavior* _Bhv = NULL;

	g_BhvActionsSz = LuaActionLen(g_BhvActions);
	//qsort(g_BhvActions, g_BhvActionsSz, sizeof(struct LuaBhvAction), LuaBaCmp);
	luaL_newlibtable(_State, g_LuaAIFuncs);
	luaL_setfuncs(_State, g_LuaAIFuncs, 0);
	lua_setglobal(_State, "AI");
	if(LuaLoadFile(_State, "ai.lua") != LUA_OK) {
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
}

void AIQuit() {
	int i;

	for(i = 0; i < g_BhvList.Size; ++i) {
		DestroyBehavior(((struct LuaBehavior*)g_BhvList.Table[i])->Behavior);
		free(((struct LuaBehavior*)g_BhvList.Table[i])->Name);
		free(g_BhvList.Table[i]);
	}
	g_BhvList.Size = 0;
}
