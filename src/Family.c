/*
 * Author: David Brotz
 * File: Family.c
 */

#include "Family.h"

#include "Person.h"
#include "Herald.h"
#include "Crop.h"
#include "Good.h"
#include "Population.h"
#include "BigGuy.h"
#include "Location.h"
#include "sys/Event.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LuaCore.h"
#include "sys/Log.h"
#include "sys/RBTree.h"
#include "AI/LuaLib.h"
#include "AI/Setup.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <lua/lauxlib.h>

static struct Array* g_FirstNames = NULL;

void Family_Init(struct Array* _Array) {
	g_FirstNames = _Array;
}

void Family_Quit() {
	DestroyArray(g_FirstNames);
}

struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize, struct Settlement* _Location) {
	struct Family* _Family = (struct Family*) malloc(sizeof(struct Family));
	int i;

	if(_ChildrenSize >= CHILDREN_SIZE) {
		DestroyFamily(_Family);
		return NULL;
	}

	_Family->Name = _Name;
	_Family->Id = NextId();
	memset(_Family->People, 0, sizeof(struct Person*) * (FAMILY_PEOPLESZ));
	_Family->People[HUSBAND] = _Husband;
	_Husband->Family = _Family;
	_Family->People[WIFE] = _Wife;
	_Wife->Family = _Family;//If _Female is last of its family the family's name will be a memory leak.
	_Family->NumChildren = 0;
	for(i = 0; i < _ChildrenSize; ++i)
		_Family->People[CHILDREN + i] = _Children[i];
	_Family->Fields = CreateArray(0);
	_Family->Buildings = CreateArray(2);
	_Family->Goods = CreateArray(16);
	_Family->Animals = CreateArray(0);
	_Family->HomeLoc = _Location;
	return _Family;
}

struct Family* CreateRandFamily(const char* _Name, int _Size, struct Constraint** _AgeGroups, struct Constraint** _BabyAvg, int _X, int _Y, struct Settlement* _Location) {
	struct Family* _Family = NULL;

	if(_Size > FAMILY_PEOPLESZ)
		return NULL;

	if(_Size >= 2) {
		struct Person* _Husband = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(_AgeGroups[TEENAGER]->Min, _AgeGroups[ADULT]->Max), EMALE, 1500, _X, _Y, _Location);
		struct Person* _Wife = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(_AgeGroups[TEENAGER]->Min, _AgeGroups[ADULT]->Max), EFEMALE, 1500, _X, _Y, _Location);
		_Family = CreateFamily(_Name, _Husband, _Wife, NULL, 0, _Location);
		_Size -= 2;

		while(_Size-- > 0) {
			int _Child = CHILDREN + _Family->NumChildren;

			_Family->People[_Child] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(0, DaysToDate(DateToDays(_Wife->Age) - DateToDays(_AgeGroups[TEENAGER]->Min))), Random(1, 2), 1500, _X, _Y, _Location);
			_Family->People[_Child]->Family = _Family;
			++_Family->NumChildren;
		}
	}
	SettlementPlaceFamily(_Location, _Family, &_X, &_Y);
	return _Family;
}

void DestroyFamily(struct Family* _Family) {
	int _Max = _Family->NumChildren + 1;
	int i;
	struct Array* _Array = _Family->Goods;

	for(i = 0; i < _Array->Size; ++i) {
		DestroyGood(_Array->Table[i]);
	}
	while(_Max >= 0) {
		if(_Family->People[_Max] != NULL) {
			DestroyPerson(_Family->People[_Max]);
			_Family->People[_Max] = NULL;
		}
		--_Max;
	}
	DestroyArray(_Family->Fields);
	DestroyArray(_Family->Buildings);
	DestroyArray(_Family->Goods);
	DestroyArray(_Family->Animals);
	free(_Family);
}

struct Food* FamilyMakeFood(struct Family* _Family) {
	int _Size;
	int i;
	int j;
	struct InputReq** _Foods = GoodBuildList(_Family->Goods, &_Size, EFOOD | ESEED | EINGREDIENT);
	struct Array* _GoodsArray = NULL;
	struct FoodBase* _Food = NULL;
	struct Food* _FamFood = NULL;
	struct Good* _Good = NULL;

	if(_Foods == NULL)
		return NULL;

	for(i = 0; i < _Size; ++i) {
		_Food = ((struct FoodBase*)_Foods[i]->Req);
		for(j = 0; j < _Food->IGSize; ++j) {
			_Good = LinearSearch(_Food->InputGoods[j], _Family->Goods->Table, _Family->Goods->Size, (int(*)(const void*, const void*))InputReqGoodCmp);
			_Good->Quantity -= _Foods[i]->Quantity * _Food->InputGoods[j]->Quantity;
			_GoodsArray = _Family->Goods;
			if((_FamFood = LinearSearch(_Food, _GoodsArray->Table, _GoodsArray->Size, GoodCmp)) == NULL) {
				_FamFood =  CreateFood(_Food, _Family->People[1]->X, _Family->People[1]->Y);
				ArrayInsert_S(_GoodsArray, _FamFood);
			}
			_FamFood->Quantity += _Foods[i]->Quantity;
			goto end;
		}
		DestroyInputReq(_Foods[i]);
	}
	if(i == 0)
		Log(ELOG_WARNING, "Day %i: %i made no food in PAIMakeFood.", DateToDays(g_GameWorld.Date), _Family->Id);
	end:
	free(_Foods);
	return _FamFood;
}

void FamilyWorkField(struct Family* _Family) {
	struct Field* _Field = NULL;
	struct Array* _Array = NULL;
	int i = 0;
	int j = 0;

	for(i = 0; i < _Family->Fields->Size; ++i) {
		_Field = ((struct Field*)_Family->Fields->Table[i]);
		if(_Field == NULL)
			return;
		if(_Field->Status == ENONE) {
			SelectCrops(_Family, _Family->Fields);
		} else if(_Field->Status == EFALLOW && MONTH(g_GameWorld.Date) >= 1 && MONTH(g_GameWorld.Date) <= 2) {
			_Array = _Family->Goods;
			for(j = 0; j < _Array->Size; ++j) {
				if(strcmp(((struct Good*)_Array->Table[j])->Base->Name, _Field->Crop->Name) == 0) {
					FieldPlant(_Field, _Array->Table[j]);
					break;
				}
			}
		} else if(_Field->Status > EFALLOW) {
			if(_Field->Status == EHARVESTING)
				FieldHarvest((struct Field*)_Field, _Family->Goods);
			else
				FieldWork(_Field, ActorWorkMult((struct Actor*)_Family->People[0]));
		}
	}
}

int FamilyThink(struct Family* _Family) {
	int i = 0;
	int _PersonHungry = 0;
	struct Food* _Food = NULL;

	FamilyWorkField(_Family);
	for(i = 0; i < _Family->Fields->Size; ++i) {
		if(_Family == g_GameWorld.Player->Person->Family)
			FieldUpdate((struct Field*)_Family->Fields->Table[i]);
		else
		FieldUpdate((struct Field*)_Family->Fields->Table[i]);
	}
	if((_Food = FamilyMakeFood(_Family)) == NULL)
		goto hungry_person;
	for(i = 0; _Family->People[i] != NULL; ++i) {
		PersonThink(_Family->People[i]);
		if(_Food->Quantity > 0) {
			_Family->People[i]->Nutrition += _Food->Base->Nutrition;
			--_Food->Quantity;
		} else
			_PersonHungry = 1;
	}
	if(_PersonHungry != 0) {
		hungry_person:
		EventPush(CreateEventStarvingFamily(_Family));
	}
	return 1;
}

int FamilySize(const struct Family* _Family) {
	int _Size = 0;
	int i;

	for(i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(_Family->People[i] == NULL)
			continue;
		++_Size;
	}
	return _Size;
}

void Marry(struct Person* _Male, struct Person* _Female) {
	assert(_Male->Gender == EMALE && _Female->Gender == EFEMALE);
	CreateFamily(_Male->Family->Name, _Male, _Female, NULL, 0, _Male->Family->HomeLoc);
}

void FamilyAddGoods(struct Family* _Family, lua_State* _State, struct FamilyType** _FamilyTypes, int _X, int _Y, struct Settlement* _Location) {
	int i;
	int j;
	int _FamType = Random(0, 9999);
	int _Quantity = 0;
	const char* _Name = NULL;
	const struct Population* _Population = NULL;
	const struct LuaBehavior* _Behavior = NULL;
	struct LuaBehavior _Cmp;
	char* _Error = NULL;
	void* _Obj = NULL;

	for(i = 0; _FamilyTypes[i] != NULL; ++i) {
		if(_FamilyTypes[i]->Percent * 10000 > _FamType) {
			Log(ELOG_INFO, "Creating Family type: %s", _FamilyTypes[i]->LuaFunc);
			++g_Log.Indents;
			lua_getglobal(_State, _FamilyTypes[i]->LuaFunc);
			lua_pushinteger(_State, FamilySize(_Family));
			lua_pushlightuserdata(_State, _Location);
			if(LuaCallFunc(_State, 2, 1, 0) == 0) {
				--g_Log.Indents;
				return;
			}
			lua_getfield(_State, -1, "Goods");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				luaL_checktype(_State, -1, LUA_TLIGHTUSERDATA);
				_Obj = lua_touserdata(_State, -1);
				((struct Good*)_Obj)->X = _X;
				((struct Good*)_Obj)->Y = _Y;
				ArrayInsertSort_S(_Family->Goods, _Obj, GoodCmp);
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Field");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				ArrayInsert_S(_Family->Fields, CreateField(_X, _Y, NULL, lua_tointeger(_State, -1), _Family));
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Buildings");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if((_Obj = (struct Building*) LuaToObject(_State, -1, "Building")) != NULL)
					ArrayInsertSort_S(_Family->Buildings, _Obj, ObjCmp);
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Animals");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				lua_pushnil(_State);
				if(lua_next(_State, -2) == 0) {
					_Error = "Animals";
					goto LuaError;
				}
				AddString(_State, -1, &_Name);
				lua_pop(_State, 1);
				if(lua_next(_State, -2) == 0) {
					_Error = "Animals";
					goto LuaError;
				}
				AddInteger(_State, -1, &_Quantity);
				if((_Population = HashSearch(&g_Populations, _Name)) == NULL) {
					Log(ELOG_WARNING, "Cannot find Population %s.", _Name);
					lua_pop(_State, 3);
					continue;
				}
				for(j = 0; j < _Quantity; ++j)
					ArrayInsertSort_S(_Family->Animals, CreateAnimal(_Population, Random(0, _Population->Ages[AGE_DEATH]->Max), 1500, _X, _Y), AnimalCmp);
				lua_pop(_State, 3);
			}
			lua_pop(_State, 1);
			lua_getfield(_State, -1, "AI");
			if(lua_type(_State, -1) != LUA_TFUNCTION)
				luaL_error(_State, "Lua function expected, got %s", lua_typename(_State, lua_type(_State, -1)));
			for(j = 0; j < _Family->NumChildren + 2; ++j) {
				if(_Family->People[j] == NULL)
					continue;
				lua_pushvalue(_State, -1);
				lua_pushlightuserdata(_State, _Family->People[j]);
				LuaPushPerson(_State, -1);
				lua_remove(_State, -2);
				if(LuaCallFunc(_State, 1, 1, 0) == 0) {
					--g_Log.Indents;
					return;
				}
				if(lua_isstring(_State, -1) == 0)
					luaL_error(_State, "string expected, got %s.", lua_typename(_State, lua_type(_State, -1)));
				_Cmp.Name = lua_tostring(_State, -1);
				if((_Behavior = BinarySearch(&_Cmp, g_BhvList.Table, g_BhvList.Size, LuaBhvCmp)) == NULL) {
					--g_Log.Indents;
					Log(ELOG_WARNING, "%s is not a behavior", _Cmp.Name);
					return;
				}
				_Family->People[j]->Behavior = _Behavior->Behavior;
				lua_pop(_State, 1);
			}
			lua_pop(_State, 2);
			--g_Log.Indents;
			break;
		}
		_FamType -= _FamilyTypes[i]->Percent * 10000;
	}
	SelectCrops(_Family, _Family->Fields);
	return;
	LuaError:
	--g_Log.Indents;
	luaL_error(_State, "In function %s the %s table does not contain a valid element.", _FamilyTypes[i]->LuaFunc, _Error);
}

struct Good* FamilyTakeGood(struct Family* _Family, int _Index) {
	struct Good* _Good = _Family->Goods->Table[_Index];

	if(_Good->Quantity > 1) {
		--_Good->Quantity;
		_Good = g_GoodCopy[_Good->Base->Category](_Good);
	} else {
		_Family->Goods->Table[_Index] = _Family->Goods->Table[_Family->Goods->Size];
		--_Family->Goods->Size;
	}
	return _Good;
}

int FamilyNutReq(struct Family* _Family) {
	int _Nutrition = 0;
	int i;

	for(i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(_Family->People[i] == NULL)
			continue;
		if(DateToDays(_Family->People[i]->Age) > ADULT_AGE)
			_Nutrition += NUTRITION_REQ;
		else
			_Nutrition += NUTRITON_CHLDREQ;
	}
	return _Nutrition;
}
