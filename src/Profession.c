/*
 * File: Profession.c 
 * Author: David Brotz
 */
#include "Profession.h"

#include "Herald.h"

#include "sys/Array.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"
#include "sys/LuaCore.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <malloc.h>

struct Profession* CreateProfession(const char* _Name, const struct GoodBase** _CraftedGoods, int _Payment) {
	struct Profession* _Profession = NULL;
	int _GoodSz = 0;

	_GoodSz = ArrayLen(_CraftedGoods);
	_Profession = (struct Profession*) malloc(sizeof(struct Profession));
	_Profession->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy((char*)_Profession->Name, _Name);
	_Profession->CraftedGoods = calloc(_GoodSz + 1, sizeof(const struct GoodBase*));
	for(int i = 0; i < _GoodSz; ++i) {
		_Profession->CraftedGoods[i] = _CraftedGoods[i];
	}
	_Profession->Payment = _Payment;
	_Profession->CraftedGoods[_GoodSz] = NULL;
	return _Profession;
}

void DestroyProfession(struct Profession* _Profession) {
	free((char*)_Profession->Name);
	free(_Profession->CraftedGoods);
	free(_Profession);
}

struct Profession* LoadProfession(lua_State* _State, int _Index) {
	struct LinkedList _List = {0, NULL, NULL};
	const struct GoodBase* _Good = NULL;
	int _GoodSize = 0;
	const struct GoodBase** _GoodList = NULL;
	const char* _Name = NULL;
	struct Profession* _Profession = NULL;
	int _Payment = 0;

	_Index = lua_absindex(_State, _Index);
	if(lua_type(_State, _Index) != LUA_TTABLE)
		return NULL;
	lua_pushstring(_State, "Name");
	lua_rawget(_State, _Index);
	if(lua_type(_State, -1) != LUA_TSTRING) {
		Log(ELOG_WARNING, "Profession's name is not a string.");
		return NULL;
	}
	_Name = lua_tostring(_State, -1);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Goods");
	lua_rawget(_State, _Index);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		Log(ELOG_WARNING, "Profession %s's Good table is not a table.", _Name);
		return NULL;
	}
	//Iterating through goods.
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_type(_State, -1) != LUA_TSTRING) {
			Log(ELOG_WARNING, "Profession %s's good table contains a non-string element.", _Name);
			lua_pop(_State, 1);
			continue;
		}
		if((_Good = HashSearch(&g_Goods, lua_tostring(_State, -1))) == NULL) {
			Log(ELOG_WARNING, "Profession %s's good table contains a non good %s.", _Name, lua_tostring(_State, -1));
			lua_pop(_State, 1);
			continue;
		}
		LnkLstPushBack(&_List, (struct GoodBase*) _Good);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	_GoodSize = _List.Size;
	_GoodList = alloca(sizeof(const struct GoodBase*) * (_GoodSize + 1));
	for(int i = 0; i < _GoodSize; ++i) {
		_GoodList[i] = _List.Front->Data;
		LnkLstPopFront(&_List);
	}
	_GoodList[_GoodSize] = NULL;
	_Profession = CreateProfession(_Name, _GoodList,  _Payment);
	return _Profession;
}

const struct GoodBase** LoadInKind(lua_State* _State, int _Index) {
	int _Size = 0;
	const char* _Key = NULL;
	struct LinkedList _List = {0, NULL, NULL};
	const struct GoodBase* _Good = NULL;
	const struct GoodBase** _GoodList = NULL;

	_Index = lua_absindex(_State, _Index);
	if(lua_type(_State, _Index) != LUA_TTABLE) {
		return NULL;
	}
	lua_pushnil(_State);
	while(lua_next(_State, _Index) != 0) {
		if(lua_type(_State, -1) != LUA_TSTRING) {
			goto loop_end;
		}
		_Key = lua_tostring(_State, -1);
		if((_Good = HashSearch(&g_Goods, _Key)) == NULL) {
			goto loop_end;
		} else {
			LnkLstPushBack(&_List, (struct GoodBase*) _Good);
		}
		loop_end:
		lua_pop(_State, 1);
	}
	_Size = _List.Size;
	_GoodList = calloc(_Size + 1, sizeof(const struct GoodBase*));
	for(int i = 0; i < _Size; ++i) {
		_GoodList[i] = _List.Front->Data;
		LnkLstPopFront(&_List);
	}
	_GoodList[_Size] = NULL;
	return _GoodList;
}

int ProfessionPaymentType(const char* _Str) {
	if(strcmp(_Str, "All") == 0) {
		return PAYMENT_MONEY | PAYMENT_INKIND | PAYMENT_SERVICE;			
	} else if(strcmp(_Str, "Money") == 0) {
		return PAYMENT_MONEY;
	} else if(strcmp(_Str, "InKind") == 0) {
		return PAYMENT_INKIND;
	} else if(strcmp(_Str, "Service") == 0) {
		return PAYMENT_SERVICE;
	} else if(strcmp(_Str, "NoService") == 0) {
		return PAYMENT_MONEY | PAYMENT_INKIND;
	}
	return 0;
}
