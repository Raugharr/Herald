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

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Construction* CreateConstruct(struct Building* _Building, struct Person* _Person) {
	struct Construction* _Construct = (struct Construction*) malloc(sizeof(struct Construction));
	int _Percent = _Building->BuildTime / 10;

	_Construct->Prev = NULL;
	_Construct->Next = NULL;
	_Construct->Type = ATT_CONSTRUCTION;
	_Construct->DaysLeft = _Building->BuildTime - _Percent + Random(0, _Percent * 2);
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

struct Building* CreateBuilding(const char* _Name, int _Width, int _Length, int _ResType, int _BuildTime) {
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	_Building->Id = NextId();
	_Building->Width = _Width;
	_Building->Length = _Length;
	_Building->ResidentType = _ResType;
	_Building->BuildTime = _BuildTime;
	_Building->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	strcpy(_Building->Name, _Name);
	return _Building;
}

void DestroyBuilding(struct Building* _Building) {
	int i;

	for(i = 0; i < _Building->OutputGoods->Size; ++i)
		free(_Building->OutputGoods->Table[i]);
	for(i = 0; i < _Building->BuildMats->Size; ++i)
		free(_Building->BuildMats->Table[i]);
	free(_Building->Name);
	free(_Building);
}

int BuildingProduce(const struct Building* _Building, struct HashTable* _Hash) {
	return 0;
}

struct Building* BuildingLoad(lua_State* _State, int _Index) {
	int _IntTemp = 0;
	int _Width = 0;
	int _Length = 0;
	int _ResType = 0;
	int _Return = 0;
	int _BuildTime = 0;
	const char* _Key = NULL;
	const char* _Name = NULL;
	const char* _Temp = NULL;
	struct InputReq* _Req = NULL;
	struct Building* _Building = NULL;
	struct LinkedList* _OutputGoods = CreateLinkedList();
	struct LinkedList* _BuildMats = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);

	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else {
			lua_pop(_State, 1);
			continue;
		}
		if(!strcmp("Output", _Key)) {
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if(LuaKeyValue(_State, -1, &_Temp, &_IntTemp) == 0) {
					lua_pop(_State, 1);
					continue;
				}
				_Req = (struct InputReq*) malloc(sizeof(struct InputReq));
				if((_Req->Req = HashSearch(&g_Goods, _Temp)) == NULL)
					luaL_error(_State, "Cannot find good %s", _Temp);
				_Req->Quantity = _IntTemp;
				LnkLst_PushBack(_OutputGoods, _Req);
				lua_pop(_State, 1);
			}
		} else if (!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("Width", _Key))
			_Return = AddInteger(_State, -1, &_Width);
		else if(!strcmp("Length", _Key))
			_Return = AddInteger(_State, -1, &_Length);
		else if(!strcmp("Residents", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if(!strcmp(_Temp, "None"))
				_ResType = 0;
			else if(!strcmp(_Temp, "Animal"))
				_ResType = ERES_ANIMAL;
			else if(!strcmp(_Temp, "Human"))
				_ResType = ERES_HUMAN;
			else if(!strcmp(_Temp, "All"))
				_ResType = ERES_ANIMAL | ERES_HUMAN;
		} else if(!strcmp("BuildTime", _Key))
			_Return = AddInteger(_State, -1, &_BuildTime);
		else if(!strcmp("BuildMats", _Key)) {
			if(lua_istable(_State, -1) == 0) {
				printf("Input for good is not a table");
				goto end;
			}
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if(lua_isstring(_State, -2)) {
					struct InputReq* _Good = CreateInputReq();
					int _Return = AddInteger(_State, -1, &_Good->Quantity);
					if(_Return == -1) {
						_Building = NULL;
						goto end;
					}
					if((_Good->Req = HashSearch(&g_Goods, lua_tostring(_State, -2))) == 0) {
						_Building = NULL;
						goto end;
					}
					LnkLst_PushBack(_BuildMats, _Good);
				}
			}
		}
		lua_pop(_State, 1);
	}
	if(_Return < 0) {
		_Building = NULL;
		goto end;
	}

	_Building = CreateBuilding(_Name, _Width, _Length, _ResType, _BuildTime);
	_Building->OutputGoods = CreateArray(_OutputGoods->Size);
	_Itr = _OutputGoods->Front;
	while(_Itr != NULL) {
		ArrayInsert(_Building->OutputGoods, _Itr->Data);
		_Itr = _Itr->Next;
	}
	_Building->BuildMats = CreateArray(_BuildMats->Size);
	_Itr = _BuildMats->Front;
	while(_Itr != NULL) {
		ArrayInsert(_Building->BuildMats, _Itr->Data);
		_Itr = _Itr->Next;
	}
	end:
	DestroyLinkedList(_BuildMats);
	return _Building;
}
