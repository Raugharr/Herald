/*
 * File: Building.c
 * Author: David Brotz
 */

#include "Building.h"

#include "Herald.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/LinkedList.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Building* CreateBuilding(const char* _Name, int _Width, int _Length, int _ResType) {
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	_Building->Id = NextId();
	_Building->Width = _Width;
	_Building->Length = _Length;
	_Building->ResidentType = _ResType;
	_Building->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	strcpy(_Building->Name, _Name);
	return _Building;
}

struct Building* CopyBuilding(const struct Building* _Building) {
	struct Building* _NewBuilding = (struct Building*) malloc(sizeof(struct Building));
	struct Array* _OutputGoods = _Building->OutputGoods;
	struct Array* _BuildMats = _Building->BuildMats;
	struct Array* _NewOutGoods = CreateArray(_OutputGoods->Size);
	struct Array* _NewBuildMats = CreateArray(_BuildMats->Size);
	int i;

	_NewBuilding->Id = NextId();
	_NewBuilding->Width = _Building->Width;
	_NewBuilding->Length = _Building->Length;
	_NewBuilding->ResidentType = _Building->ResidentType;
	_NewBuilding->Name = (char*) malloc(sizeof(char) * strlen(_Building->Name) + 1);
	for(i = 0; i < _OutputGoods->Size; ++i) {
		_NewOutGoods->Table[i] = (struct InputReq*) malloc(sizeof(struct InputReq));
		((struct InputReq*)_NewOutGoods->Table[i])->Req = ((struct InputReq*)_OutputGoods->Table[i])->Req;
		((struct InputReq*)_NewOutGoods->Table[i])->Quantity = ((struct InputReq*)_OutputGoods->Table[i])->Quantity;
	}
	for(i = 0; i < _BuildMats->Size; ++i) {
		_NewBuildMats->Table[i] = malloc(sizeof(struct InputReq));
		((struct InputReq*)_NewBuildMats->Table[i])->Req = ((struct InputReq*)_BuildMats->Table[i])->Req;
		((struct InputReq*)_NewBuildMats->Table[i])->Quantity = ((struct InputReq*)_BuildMats->Table[i])->Quantity;
	}

	strcpy(_NewBuilding->Name, _Building->Name);
	return _NewBuilding;
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
	const char* _Key = NULL;
	const char* _Name = NULL;
	const char* _Temp = NULL;
	int _IntTemp = 0;
	int _Width = 0;
	int _Length = 0;
	int _ResType = 0;
	int _Return = 0;
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
		}
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

	_Building = CreateBuilding(_Name, _Width, _Length, _ResType);
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
