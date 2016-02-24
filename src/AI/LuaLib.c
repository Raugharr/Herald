/*
 * File: LuaLib.c
 * Author: David Brotz
 */

#include "LuaLib.h"

#include "BehaviorTree.h"
#include "Setup.h"
#include "../sys/LuaCore.h"
#include "../sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

static const luaL_Reg g_LuaAIFuncs[] = {
	{"AddBhvTree", LuaBhvTree},
	//{"Hook", LuaAISetHook},
	{NULL, NULL}
};

static const luaL_Reg g_LuaBehaviorFuncs[] = {
	{"Selector", LuaBhvSelector},
	{"Sequence", LuaBhvSequence},
	{"Inverter", LuaBhvInverter},
	{"Node", LuaBhvNode},
	{NULL, NULL}
};

static const struct LuaObjectReg g_LuaAIObjects[] = {
		{"Behavior", NULL, g_LuaBehaviorFuncs},
		{"BhvComposite", "Behavior", NULL},
		{"BhvDecorator", "Behavior", NULL},
		{"BhvNode", "Behavior", NULL},
		{NULL, NULL, NULL}
};

void LuaAILibInit(lua_State* _State) {
	luaL_newlib(_State, g_LuaAIFuncs);
	lua_setglobal(_State, "AI");
	RegisterLuaObjects(_State, g_LuaAIObjects);
	luaL_newlib(_State, g_LuaBehaviorFuncs);
	lua_setglobal(_State, "Behavior");
}

int LuaBhvCmp(const void* _One, const void* _Two) {
	return strcmp(((const struct LuaBehavior*)_One)->Name, ((const struct LuaBehavior*)_Two)->Name);
}

int luaStrLuaBhvCmp(const void* _One, const void* _Two) {
	return strcmp(((const char*)_One), ((const struct LuaBehavior*)_Two)->Name);
}

/*int LuaBhvCreateDecorator(lua_State* _State, BhvCallback _Callback) {
	struct Behavior* _Decorator = NULL;
	struct BehaviorNode* _Node = LuaCheckClass(_State, 1, "BhvNode");

	_Decorator = CreateBehaviorNode(NULL, _Node->Action, 0, _Callback);
	LuaCtor(_State, "BhvDecorator", _Decorator);
	return 1;
}*/

int LuaBhvCreateComposite(lua_State* _State, BhvCallback _Callback) {
	struct Behavior* _Bhv = NULL;
	int _Len = 0;
	int _ChildSz = 0;
	struct Behavior* _Child = NULL;

	luaL_checktype(_State, 1, LUA_TTABLE);
	_Len = lua_rawlen(_State, 1);
	_Bhv = CreateBehavior(NULL, NULL, _Len, _Callback);
		lua_pushnil(_State);
		while(lua_next(_State, -2) != 0) {
			if((_Child = LuaCheckClass(_State, -1, "Behavior")) == NULL) {
				lua_pop(_State, 1);
				continue;
			}
			_Bhv->Children[_ChildSz++] = _Child;
			lua_pop(_State, 1);
		}
		LuaCtor(_State, "Behavior", _Bhv);
		return 1;
}

struct BehaviorNode* LuaBhvCreateNode(lua_State* _State) {
	struct LuaBhvAction _Temp;
	struct LuaBhvAction* _Action = NULL;
	struct BehaviorNode* _Behavior = NULL;
	int _Args = lua_gettop(_State) - 1;

	_Temp.Name = luaL_checkstring(_State, 1);
	if((_Action = bsearch(&_Temp, g_BhvActions, g_BhvActionsSz, sizeof(struct LuaBhvAction), LuaBaCmp)) == NULL) {
		lua_pushfstring(_State, ERRMSG_BHV, _Temp.Name);
		return (struct BehaviorNode*) luaL_argerror(_State, 1, lua_tostring(_State, -1));
	}
	if(_Args != _Action->Arguments)
		return (struct BehaviorNode*) luaL_error(_State, "Behavior %s requires %d arguments but provided with %d.", _Temp.Name, _Action->Arguments, _Args);
	_Behavior = CreateBehaviorNode(_Action->Action, _Args);
	for(int i = 0; i < _Args; ++i)
		LuaToPrimitive(_State, 2 + i, &_Behavior->Arguments[i]);
	LuaCtor(_State, "BhvNode", _Behavior);
	return _Behavior;
}

int LuaBhvSelector(lua_State* _State) {
	return LuaBhvCreateComposite(_State, BhvSelector);
}

int LuaBhvSequence(lua_State* _State) {
	return LuaBhvCreateComposite(_State, BhvSequence);
}

int LuaBhvInverter(lua_State* _State) {
	struct BehaviorNode* _Behavior = NULL;

	_Behavior = LuaBhvCreateNode(_State);
	_Behavior->Callback = BhvNot;
	return 1;
}

int LuaBhvNode(lua_State* _State) {
	return (LuaBhvCreateNode(_State) != NULL);
}

int LuaBhvTree(lua_State* _State) {
	const char* _Arg = NULL;
	struct Behavior* _Behavior = LuaCheckClass(_State, 1, "Behavior");

	_Arg = luaL_checkstring(_State, 2);
	if(BinarySearch(_Arg, g_BhvList.Table, g_BhvList.Size, LuaBhvCmp) == NULL) {
		struct LuaBehavior* _Bhv = (struct LuaBehavior*) malloc(sizeof(struct LuaBehavior));

		_Bhv->Name = calloc(strlen(_Arg) + 1, sizeof(char));
		_Bhv->Behavior = _Behavior;
		ArrayInsert_S(&g_BhvList, _Bhv);
		return 0;
	} else {
		lua_pushfstring(_State, "%s is already a behavior tree.", _Arg);
		return luaL_argerror(_State, 2, lua_tostring(_State, -1));
	}
	return 0;
}

/*int LuaAISetHook(lua_State* _State) {
	struct AIHook* _Hook = NULL;
	struct Behavior* _Behavior = NULL;
	int i = 0;
	int _Size = 0;

	if(!lua_islightuserdata(_State, 2)) {
		luaL_error(_State, "arg #2 must be a hook.");
		return 0;
	}
	if(!lua_istable(_State, 3)) {
		luaL_error(_State, "arg #3 must be a table.");
		return 0;
	}
	if(!lua_istable(_State, 4)) {
		luaL_error(_State, "arg #4 must be a table.");
		return 0;
	}
	if(lua_rawlen(_State, 3) != lua_rawlen(_State, 4))
		luaL_error(_State, "Argument 3 and 4 do not have the same length.");
	_Size = lua_rawlen(_State, 3);
	_Hook = (struct AIHook*) malloc(sizeof(struct AIHook));
	_Hook->Bhv = (struct Behavior*) LuaCheckClass(_State, 1, "Behavior");
	_Hook->Event = (struct Event*) lua_touserdata(_State, 2);
	_Hook->Condition = CreateBehavior(NULL, NULL, _Size, BhvSelector);
	_Hook->BhvPaths = calloc(_Size + 1, sizeof(struct Behavior*));
	lua_pushvalue(_State, 3);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if((_Behavior = LuaCheckClass(_State, -1, "Behavior")) == 0) {
			luaL_error(_State, "Argument #3 has an element which is not a behavior.");
			goto argthree;
		}
		_Hook->Condition->Children[i++] = _Behavior;
		argthree:
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	i = 0;
	lua_pushvalue(_State, -1);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_islightuserdata(_State, -1) == 0) {
			luaL_error(_State, "Argument #4 has an element which is not a behavior tree.");
			goto argfour;
		}
		_Hook->BhvPaths[i++] = lua_touserdata(_State, -1);
		argfour:
		lua_pop(_State, 1);
	}
	_Hook->BhvPaths[i] = NULL;
	lua_pop(_State, 1);
	return 0;
}*/
