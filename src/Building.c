/*
 * File: Building.c
 * Author: David Brotz
 */

#include "Building.h"

#include "Herald.h"
#include "sys/LuaHelper.h"
#include "sys/LinkedList.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Building* CreateBuilding(const char* _Name, struct Good* _Output, int _Tax, int _Throughput, int _SquareFeet) {
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	_Building->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Building->OutputGood = _Output;
	_Building->Tax = _Tax;
	_Building->Throughput = _Throughput;
	_Building->Size = _SquareFeet;
	strcpy(_Building->Name, _Name);
	return _Building;
}

struct Building* CopyBuilding(const struct Building* _Building, struct Good* _Good) {
	struct Building* _NewBuilding = (struct Building*) malloc(sizeof(struct Building));

	_NewBuilding->Name = (char*) malloc(sizeof(char) * strlen(_Building->Name) + 1);
	_NewBuilding->OutputGood = _Building->OutputGood;
	_NewBuilding->Tax = _Building->Tax;
	_NewBuilding->Throughput = _Building->Throughput;
	_NewBuilding->Size = _Building->Size;
	strcpy(_NewBuilding->Name, _Building->Name);
	return _NewBuilding;
}

void DestroyBuilding(struct Building* _Building) {
	struct LnkLst_Node* _Itr = _Building->BuildMats.Front;

	while(_Itr != NULL) {
		DestroyInputReq(_Itr->Data);
		_Itr = _Itr->Next;
	}

	free(_Building->Name);
	free(_Building);
}

int Building_Produce(const struct Building* _Building, struct HashTable* _Hash) {
	return 0;
}

struct Building* BuildingLoad(lua_State* _State, int _Index) {
	const char* _Key = NULL;
	char* _Name = NULL;
	char* _Temp = NULL;
	struct Good* _Output = NULL;
	int _Tax = 0;
	int _Throughput = 0;
	int _Return = 0;
	int _SquareFeet = 0;
	struct Building* _Building = NULL;
	struct LinkedList* _BuildMats = CreateLinkedList();
	struct LinkedList* _Animals = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);

	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("Output", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if((_Output = HashSearch(&g_Goods, _Temp)) == 0)
				luaL_error(_State, "cannot find the good %s", _Temp);
		} else if (!strcmp("Tax", _Key))
			_Return = AddInteger(_State, -1, &_Tax);
		else if (!strcmp("Throughput", _Key))
			_Return = AddInteger(_State, -1, &_Throughput);
		else if (!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("SquareFeet", _Key))
			_Return = AddInteger(_State, -1, &_SquareFeet);
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
		} else if(!strcmp("Requires", _Key)) {
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if(lua_isstring(_State, -2)) {
					struct InputReq* _Animal = CreateInputReq();
					struct Population* _Info = NULL;
					int _Return = AddInteger(_State, -1, &_Animal->Quantity);
					if(_Return == -1) {
						_Building = NULL;
						goto end;
					}
					if((_Info = HashSearch(&g_Populations, lua_tostring(_State, -2))) == 0) {
						_Building = NULL;
						goto end;
					}
					_Animal->Req = _Info;
					LnkLst_PushBack(_Animals, _Animal);
				}
			}
		}
		lua_pop(_State, 1);
	}
	if(_Return < 0) {
		_Building = NULL;
		goto end;
	}

	_Building = CreateBuilding(_Name, _Output, _Tax, _Throughput, _SquareFeet);
	_Itr = _BuildMats->Front;
	while(_Itr != NULL) {
		LnkLst_PushBack(&_Building->BuildMats, _Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Animals->Front;
	while(_Itr != NULL) {
		LnkLst_PushBack(&_Building->Animals, _Itr->Data);
		_Itr = _Itr->Next;
	}
	end:
	DestroyLinkedList(_BuildMats);
	DestroyLinkedList(_Animals);
	return _Building;
}
