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

struct Profession* CreateProfession(const char* _Name, double _Markup, const struct GoodBase** _CraftedGoods, const struct GoodBase** _InKind, int _Payment) {
	struct Profession* _Profession = NULL;
	int _GoodSz = 0;

	_GoodSz = ArrayLen(_CraftedGoods);
	_Profession = (struct Profession*) malloc(sizeof(struct Profession));
	_Profession->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy((char*)_Profession->Name, _Name);
	_Profession->Markup = _Markup;
	_Profession->CraftedGoods = calloc(_GoodSz + 1, sizeof(const struct GoodBase*));
	if((_Payment & PAYMENT_INKIND) == PAYMENT_INKIND) {
		int _InKindSz = ArrayLen(_InKind);

		_Profession->InKind = calloc(_GoodSz, sizeof(const struct GoodBase*));
		for(int i = 0; i < _InKindSz; ++i) {
			_Profession->InKind[i] = _InKind[i];
		}
		_Profession->InKind[_InKindSz] = NULL;
	}
	for(int i = 0; i < _GoodSz; ++i) {
		_Profession->CraftedGoods[i] = _CraftedGoods[i];
	}
	_Profession->Payment = _Payment;
	_Profession->CraftedGoods[_GoodSz] = NULL;
	_Profession->Behavior = NULL;
	_Profession->SecondaryJob = NULL;
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
	const struct GoodBase** _InKind = NULL;
	const char* _Name = NULL;
	struct Behavior* _Behavior = NULL;
	struct Profession* _Profession = NULL;
	double _Markup = 0.0;
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

	lua_pushstring(_State, "Behavior");
	lua_rawget(_State, _Index);
	if((_Behavior = LuaCheckClass(_State, -1, "Behavior")) == NULL) {
		Log(ELOG_WARNING, "Profession %s's Behavior paramater is not a behavior.", _Name);
	}
	lua_pop(_State, 1);

	lua_pushstring(_State, "Markup");
	lua_rawget(_State, _Index);
	if(lua_type(_State, -1) != LUA_TNUMBER) {
		Log(ELOG_WARNING, "Profession %s's Markup paramater is not a number.", _Name);
		return NULL;
	}
	_Markup = lua_tonumber(_State, -1);
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
	lua_pushstring(_State, "InKind");
	lua_rawget(_State, _Index);
	if(lua_type(_State, -1) == LUA_TTABLE)
		_InKind = LoadInKind(_State, -1);
	lua_pop(_State, 1);

	lua_pop(_State, 1);
	_GoodSize = _List.Size;
	_GoodList = alloca(sizeof(const struct GoodBase*) * (_GoodSize + 1));
	for(int i = 0; i < _GoodSize; ++i) {
		_GoodList[i] = _List.Front->Data;
		LnkLstPopFront(&_List);
	}
	_GoodList[_GoodSize] = NULL;
	_Profession = CreateProfession(_Name, _Markup, _GoodList, _InKind,  _Payment);
	_Profession->Behavior = _Behavior;
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
			const struct LinkedList* _CatList = NULL;

			if((_CatList = GoodGetCategory(_Key)) != NULL)
				LnkLstMerge(&_List, _CatList);
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
