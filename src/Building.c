/*
 * File: Building.c
 * Author: David Brotz
 */

#include "Building.h"

#include "Herald.h"
#include "Good.h"
#include "Person.h"
#include "Family.h"
#include "sys/Math.h"
#include "sys/Array.h"
#include "sys/LuaCore.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person) {
	struct Construction* _Construct = (struct Construction*) malloc(sizeof(struct Construction));
	int _BuildTime = ConstructionTime(_Building->Walls, _Building->Floor, _Building->Roof, BuildingArea(_Building));
	int _Percent = _BuildTime / 10;

	CreateObject((struct Object*)_Construct, OBJECT_CONSTRUCT, (void(*)(struct Object*))ConstructThink);
	_Construct->DaysLeft = _BuildTime - _Percent + Random(0, _Percent * 2);
	_Construct->Building = _Building;
	_Construct->Worker = _Person;
	return _Construct;
}

void DestroyConstruct(struct Construction* _Construct) {
	DestroyObject((struct Object*)_Construct);
	free(_Construct);
}

void ConstructThink(struct Construction* _Construct) {
	if(_Construct->DaysLeft <= 0)
		DestroyConstruct(_Construct);
}

int ConstructionTime(const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof, int _Area) {
	return (_Walls->BuildCost * _Area) + (_Floor->BuildCost * _Area) + (_Roof->BuildCost * _Area);
}

struct Building* CreateBuilding(int _ResType, const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof) {
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	CreateObject((struct Object*)_Building, OBJECT_BUILDING, NULL);
	_Building->Pos.x = 0;
	_Building->Pos.y = 0;
	_Building->ResidentType = _ResType;
	_Building->Walls = _Walls;
	_Building->Floor = _Floor;
	_Building->Roof = _Roof;
	return _Building;
}

void DestroyBuilding(struct Building* _Building) {
	int i;

	for(i = 0; _Building->OutputGoods[i] != NULL; ++i)
		free(_Building->OutputGoods[i]);
	free(_Building->OutputGoods[i]);
	for(i = 0; _Building->BuildMats[i] != NULL; ++i)
		free(_Building->BuildMats[i]);
	free(_Building->BuildMats[i]);
	free(_Building);
}

int BuildingArea(const struct Building* _Building) {
	return 0;
}

struct BuildMat* BuildingLoad_Aux(lua_State* _State, int _Index) {
	int _Return = 0;
	int _Cost = 0;
	int _TypeId = 0;
	double _GPF = 0;
	const char* _Key = NULL;
	const char* _Type = NULL;
	struct BuildMat* _Mat = NULL;

	lua_pushnil(_State);
	while(lua_next(_State, _Index - 1) != 0) {
		if(!lua_isstring(_State, -2)) {
			lua_pop(_State, 1);
			continue;
		}
		_Key = lua_tostring(_State, -2);
		if(strcmp(_Key, "Type") == 0)
			_Return = LuaGetString(_State, -1, &_Type);
		else if(strcmp(_Key, "GoodsPerFoot") == 0)
			_Return = LuaGetNumber(_State, -1, &_GPF);
		else if(strcmp(_Key, "Cost") == 0)
			_Return = LuaGetInteger(_State, -1, &_Cost);
		if(_Return <= 0)
			return NULL;
		_Return = 0;
		lua_pop(_State, 1);
	}
	if(strcmp(_Type, "Wall") == 0)
		_TypeId = BMAT_WALL;
	else if(strcmp(_Type, "Floor") == 0)
		_TypeId = BMAT_FLOOR;
	else if(strcmp(_Type, "Roof") == 0)
		_TypeId = BMAT_ROOF;
	else {
		Log(ELOG_WARNING, "BuiltMat table contains invalid type: %s", _Type);
		return NULL;
	}
	_Mat = (struct BuildMat*) malloc(sizeof(struct BuildMat));
	_Mat->Type = _TypeId;
	_Mat->BuildCost = _Cost;
	_Mat->MatCost = _GPF;
	return _Mat;
}

struct BuildMat* SelectBuildMat(const struct Array* _Goods, int _MatType) {
	struct BuildMat* _HighMat = NULL;
	struct HashItrCons* _Itr = HashCreateItrCons(&g_BuildMats);
	double _Ratio = 0;
	double _HighRatio = 0;
	int i;

	while(_Itr != NULL) {
		for(i = 0; i < _Goods->Size; ++i) {
			if(((struct BuildMat*)_Itr->Node->Pair)->Type != _MatType)
				continue;
			if(((struct Good*)_Goods->Table[i])->Base->Id == ((struct BuildMat*)_Itr->Node->Pair)->Good->Id) {
				_Ratio = ((struct Good*)_Goods->Table[i])->Quantity / ((struct BuildMat*)_Itr->Node->Pair)->MatCost;
				if(_Ratio > _HighRatio) {
					_HighRatio = _Ratio;
					_HighMat = ((struct BuildMat*)_Itr->Node->Pair);
				}
			}
		}
		_Itr = HashNextCons(&g_BuildMats, _Itr);
	}
	HashDeleteItrCons(_Itr);
	return _HighMat;
}

struct Building* BuildingPlan(const struct Person* _Person, int _Type, int _RoomCt) {
	struct Array* _Goods = _Person->Family->Goods;
	struct Building* _Building = NULL;
	int _ResType = 0;

	if(_Type == EBT_HOME && _RoomCt == 1)
		_ResType = (ERES_HUMAN | ERES_ANIMAL);
	_Building = CreateBuilding(_ResType, SelectBuildMat(_Goods, BMAT_WALL), SelectBuildMat(_Goods, BMAT_FLOOR), SelectBuildMat(_Goods, BMAT_ROOF));
	return _Building;
}

struct LnkLst_Node* BuildingLoad(lua_State* _State, int _Index) {
	struct GoodBase* _Good = NULL;
	struct LnkLst_Node* _Node = NULL;
	struct LnkLst_Node* _Prev = NULL;
	struct LnkLst_Node* _First = NULL;
	const char* _Temp = NULL;

	lua_pushnil(_State);
	if(lua_next(_State, -2) != 0) {
		if(LuaGetString(_State, -1, &_Temp) != 0) {
			if((_Good = HashSearch(&g_Goods, _Temp)) == NULL) {
				Log(ELOG_WARNING, "BuildMat table contains an invalid Object name: %s", _Temp);
				lua_pop(_State, 2);
				return NULL;
			}
		} else {
			Log(ELOG_WARNING, "BuildMat table field Object is not a string.");
			lua_pop(_State, 2);
			return NULL;
		}
	}
	lua_pop(_State, 1);
	if(lua_next(_State, -2) != 0) {
		lua_pushnil(_State);
		if(lua_next(_State, -2) != 0) {
			_First = (struct LnkLst_Node*) malloc(sizeof(struct LnkLst_Node));
			if((_First->Data = BuildingLoad_Aux(_State, -1)) == NULL) {
				free(_First);
				lua_pop(_State, 2);
				return NULL;
			}
			((struct BuildMat*)_First->Data)->Good = _Good;
			((struct BuildMat*)_First->Data)->Name = _Good->Name;
			lua_pop(_State, 1);
			_Prev = _First;
		}
		while(lua_next(_State, -2) != 0) {
			_Node = (struct LnkLst_Node*) malloc(sizeof(struct LnkLst_Node));
			_Prev->Next = _Node;
			if((_Node->Data = BuildingLoad_Aux(_State, -1)) == NULL) {
				free(_Node);
				_Prev->Next = NULL;
				lua_pop(_State, 1);
				continue;
			}
			((struct BuildMat*)_Node->Data)->Good = _Good;
			((struct BuildMat*)_Node->Data)->Name = _Good->Name;
			_Node->Next = NULL;
			_Prev = _Node;
			lua_pop(_State, 1);
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	if(_Node == NULL)
		_First->Next = NULL;
	else
		_Node->Next = NULL;
	return _First;
}

struct GoodBase* BuildMatToGoodBase(struct BuildMat* _Mat) {
	char _Name[64];
	char* _RealName = NULL;
	int _Size = 0;

	if(_Mat->Type == BMAT_WALL)
		_Size = snprintf(_Name, 64, "%s %s", _Mat->Good->Name, "Wall");
	else if(_Mat->Type == BMAT_FLOOR)
		_Size = snprintf(_Name, 64, "%s %s", _Mat->Good->Name, "Floor");
	else if(_Mat->Type == BMAT_ROOF)
		_Size = snprintf(_Name, 64, "%s %s", _Mat->Good->Name, "Roof");
	_RealName = calloc(_Size + 1, sizeof(char));
	strcpy(_RealName, _Name);
	return InitGoodBase((struct GoodBase*) malloc(sizeof(struct GoodBase)), _Name, GOOD_OTHER);
}
