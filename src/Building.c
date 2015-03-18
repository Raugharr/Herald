/*
 * File: Building.c
 * Author: David Brotz
 */

#include "Building.h"

#include "Herald.h"
#include "Good.h"
#include "Person.h"
#include "Family.h"
#include "Zone.h"
#include "sys/Random.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define BUILD_WALL(_Start, _End, _X, _Y, _Itr _Mid)	\
	for((_Itr) = (_Start); (_Itr) < (_End); ++(_Itr)) { 									\
		if((_Itr) != (_Mid)) 															\	
			CreateObject(malloc(sizeof(struct Object)), OBJECT_WALL, (_X), (_Y), ObjNoThink);			\
	}

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person) {
	struct Construction* _Construct = (struct Construction*) malloc(sizeof(struct Construction));
	int _BuildTime = ConstructionTime(_Building->Walls, _Building->Floor, _Building->Roof, BuildingArea(_Building));
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

int ConstructionTime(const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof, int _Area) {
	return (_Walls->BuildCost * _Area) + (_Floor->BuildCost * _Area) + (_Roof->BuildCost * _Area);
}

struct Building* CreateBuilding(int _ResType, const struct BuildMat* _Walls, const struct BuildMat* _Floor, const struct BuildMat* _Roof, struct Zone** _Zones) {
	int i = 0;
	int x = 0;
	int y = 0;
	int _MidX = 0;
	int _MidY = 0;
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	CreateObject((struct Object*)_Building, OBJECT_BUILDING, 0, 0, ObjNoThink);
	_Building->ResidentType = _ResType;
	_Building->Walls = _Walls;
	_Building->Floor = _Floor;
	_Building->Roof = _Roof;
	/*while(_Zones[i] != NULL) {
		_Zones[i]->X = 0;
		_Zones[i]->Y = 0;
		++i;
	}*/
	_Building->Zones = calloc(i, sizeof(struct Zone*));
	for(i = 0; _Zones[i] != NULL; ++i) {
		_Building->Zones[i] = _Zones[i];
		_MidX = (_Zones[i]->X + (_Zones[i]->X + _Zones[i]->Width)) / 2;
	
		x = _Zones[i]->X;
		BUILD_WALL(_Zones[i]->Y - 1, _Zones[i]->Y + _Zones[i]->Length, x, y, y, _MidY);
		x = _Zones[i]->X + _Zones[i]->Length;
		BUILD_WALL(_Zones[i]->Y - 1, _Zones[i]->Y + _Zones[i]->Length, x, y, y, _MidY);
		y = _Zones[i]->Y - 1;
		BUILD_WALL(_Zones[i]->X, _Zones[i]->X + _Zones[i]->Width, x, y, x _MidX);
		y = _Zones[i]->Y + _Zones[i]->Length + 1;
		BUILD_WALL(_Zones[i]->X, _Zones[i]->X + _Zones[i]->Width, x, y, x, _MidX);
		for(x = _Zones[i]->X; x < _Zones[i]->X + _Zones[i]->Width; ++x) {
			for(y = _Zones[i]->Y; y < _Zones[i]->Y + _Zones[i]->Length; ++y) {
				CreateObject(malloc(sizeof(struct Object)), OBJECT_FLOOR, x, y, ObjNoThink);
			}
		}
	}
	_Zones[i] = NULL;
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
	for(i = 0; _Building->Zones[i] != NULL; ++i)
		DestroyZone(_Building->Zones[i]);
	free(_Building->Zones);
	free(_Building);
}

int BuildingArea(const struct Building* _Building) {
	int _Area = 0;
	int i = 0;

	for(i = 0; _Building->Zones[i] != NULL; ++i)
		_Area += _Building->Zones[i]->Width * _Building->Zones[i]->Length;
	return _Area;
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
	_Building = CreateBuilding(_ResType, SelectBuildMat(_Goods, BMAT_WALL), SelectBuildMat(_Goods, BMAT_FLOOR), SelectBuildMat(_Goods, BMAT_ROOF), NULL);
	return _Building;
}

void BuildingPlanSize(const struct Zone** _Zones, int* _Width, int* _Length) {
	int i = 0;

	while(_Zones[i] != NULL) {
		*_Width += _Zones[i]->Width + 2;
		*_Length += _Zones[i]->Length;
		++i;
	}
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
			((struct BuildMat*)_First->Data)->Good = _Good;
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
	char* _Name = alloca(sizeof(char) * 64);
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
	return InitGoodBase((struct GoodBase*) malloc(sizeof(struct GoodBase)), _Name, EOTHER);
}
