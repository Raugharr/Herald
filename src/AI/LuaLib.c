/*
 * File: LuaLib.c
 * Author: David Brotz
 */

#include "LuaLib.h"

#include "BehaviorTree.h"
#include "Behaviors.h"
#include "Setup.h"

#include "../sys/LuaCore.h"
#include "../sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

static const luaL_Reg g_LuaAIFuncs[] = {
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
	{LOBJ_BEHAVIOR, "Behavior", LUA_REFNIL, g_LuaBehaviorFuncs},
	{LOBJ_BHVCOMPOSITE, "BhvComposite", LOBJ_BEHAVIOR, NULL},
	{LOBJ_BHVDECORATOR, "BhvDecorator", LOBJ_BEHAVIOR, NULL},
	{LOBJ_BHVNODE, "BhvNode", LOBJ_BEHAVIOR, NULL},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
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
	LuaCtor(_State, _Decorator, "BhvDecorator");
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
			if((_Child = LuaCheckClass(_State, -1, LOBJ_BEHAVIOR)) == NULL) {
				lua_pop(_State, 1);
				continue;
			}
			_Bhv->Children[_ChildSz++] = _Child;
			lua_pop(_State, 1);
		}
		LuaCtor(_State, _Bhv, LOBJ_BEHAVIOR);
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
	LuaCtor(_State, _Behavior, LOBJ_BHVNODE);
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


