/*
 * File: Building.c
 * Author: David Brotz
 */

#include "Building.h"

#include "Herald.h"
#include "sys/Random.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person) {
	struct Construction* _Construct = (struct Construction*) malloc(sizeof(struct Construction));
	int _BuildTime = ConstructionTime(_Building, _Building->Width, _Building->Length);
	int _Percent = _BuildTime / 10;

	_Construct->Prev = NULL;
	_Construct->Next = NULL;
	_Construct->Type = ATT_CONSTRUCTION;
	_Construct->DaysLeft = _BuildTime - _Percent + Random(0, _Percent * 2);
	_Construct->Building = _Building;
	_Construct->Worker = _Person;
	return _Construct;
}

struct Construction* CopyConstruct(struct Construction* _Construct) {
	struct Construction* _NewConstruct = (struct Construction*) malloc(sizeof(struct Construction));

	_NewConstruct->Type = 0;
	_NewConstruct->DaysLeft = _Construct->DaysLeft;
	_NewConstruct->Building = _Construct->Building;
	_NewConstruct->Worker = _Construct->Worker;
	return _NewConstruct;
}

void DestroyConstruct(struct Construction* _Construct) {
	free(_Construct);
}

int ConstructUpdate(struct Construction* _Construct) {
	if(_Construct->DaysLeft <= 0)
		return 1;
	return 0;
}

int ConstructionTime(const struct Building* _Building, int _Width, int _Height) {
	int _Area = _Width * _Height;

	return (_Building->Walls->BuildCost * _Area ) + (_Building->Floor->BuildCost * _Area) + (_Building->Roof->BuildCost * _Area);
}

struct Building* CreateBuilding(int _ResType) {
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	_Building->Id = NextId();
	_Building->ResidentType = _ResType;
	return _Building;
}

struct Building* CopyBuilding(const struct Building* _Building) {
	struct Building* _NewBuilding = (struct Building*) malloc(sizeof(struct Building));
	struct InputReq** _OutputGoods = _Building->OutputGoods;
	struct InputReq** _BuildMats = _Building->BuildMats;
	struct InputReq** _NewOutGoods = calloc(ArrayLen(_OutputGoods), sizeof(struct InputReq*));
	struct InputReq** _NewBuildMats = calloc(ArrayLen(_BuildMats), sizeof(struct InputReq*));
	int i;

	_NewBuilding->Id = NextId();
	_NewBuilding->ResidentType = _Building->ResidentType;
	for(i = 0; _OutputGoods[i] != NULL; ++i) {
		_NewOutGoods[i] = (struct InputReq*) malloc(sizeof(struct InputReq));
		((struct InputReq*)_NewOutGoods[i])->Req = ((struct InputReq*)_OutputGoods[i])->Req;
		((struct InputReq*)_NewOutGoods[i])->Quantity = ((struct InputReq*)_OutputGoods[i])->Quantity;
	}
	for(i = 0; _BuildMats[i] != NULL; ++i) {
		_NewBuildMats[i] = malloc(sizeof(struct InputReq));
		((struct InputReq*)_NewBuildMats[i])->Req = ((struct InputReq*)_BuildMats[i])->Req;
		((struct InputReq*)_NewBuildMats[i])->Quantity = ((struct InputReq*)_BuildMats[i])->Quantity;
	}

	return _NewBuilding;
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
			_Return = AddString(_State, -1, &_Type);
		else if(strcmp(_Key, "GoodsPerFoot") == 0)
			_Return = AddNumber(_State, -1, &_GPF);
		else if(strcmp(_Key, "Cost") == 0)
			_Return = AddInteger(_State, -1, &_Cost);
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
	_Mat->Id = NextId();
	_Mat->Type = _TypeId;
	_Mat->BuildCost = _Cost;
	_Mat->MatCost = _GPF;
	return _Mat;
}

struct LnkLst_Node* BuildingLoad(lua_State* _State, int _Index) {
	struct GoodBase* _Good = NULL;
	struct LnkLst_Node* _Node = NULL;
	struct LnkLst_Node* _Prev = NULL;
	struct LnkLst_Node* _First = NULL;
	const char* _Temp = NULL;

	lua_pushnil(_State);
	if(lua_next(_State, -2) != 0) {
		if(AddString(_State, -1, &_Temp) != 0) {
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
