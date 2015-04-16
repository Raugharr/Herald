/*
 * File: Rule.c
 * Author: David Brotz
 */

#include "Rule.h"

#include "LuaHelper.h"
#include "Log.h"
#include "Event.h"

#include <string.h>
#include <stdlib.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <malloc.h>

int(*g_RuleFuncLookup[])(const struct Rule*) = {
		RuleTrue,
		(int(*)(const struct Rule*))RuleBoolean,
		(int(*)(const struct Rule*))RuleGreaterThan,
		(int(*)(const struct Rule*))RuleLessThan,
		(int(*)(const struct Rule*))RuleLuaCall,
		(int(*)(const struct Rule*))RulePrimitive
};

struct Primitive* CreatePrimitive() {
	return (struct Primitive*) malloc(sizeof(struct Primitive));
}

void DestroyPrimitive(struct Primitive* _Primitive) {
	if(_Primitive->Type == PRIM_STRING)
		free(_Primitive->Value.String);
	free(_Primitive);
}

int PrimitiveToBoolean(struct Primitive* _Primitive) {
	switch(_Primitive->Type) {
		case PRIM_FLOAT:
			return _Primitive->Value.Float != 0;
		case PRIM_INTEGER:
			return _Primitive->Value.Int != 0;
		case PRIM_BOOLEAN:
			return _Primitive->Value.Int != 0;
		case PRIM_PTR:
			return _Primitive->Value.Ptr != NULL;
		case PRIM_STRING:
			return _Primitive->Value.String != ((char*)0);
	}
	return 0;
}

struct Rule* CreateRule(int _Type, void(*_Destroy)(struct Rule*)) {
	struct Rule* _Rule = (struct Rule*) malloc(sizeof(struct Rule));

	_Rule->Type = _Type;
	_Rule->Destroy = _Destroy;
	return _Rule;
}

void DestroyRule(struct Rule* _Rule) {
	free(_Rule);
}

int RuleCmp(const void* _One, const void* _Two) {
	return RuleEval(((struct Rule*)_One)) - RuleEval(((struct Rule*)_Two));
}

struct RulePrimitive* CreateRulePrimitive(struct Primitive* _Primitive) {
	struct RulePrimitive* _Rule = (struct RulePrimitive*) malloc(sizeof(struct RulePrimitive));

	_Rule->Type = RULE_PRIMITIVE;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRulePrimitive;
	_Rule->Value = *_Primitive;
	return _Rule;
}

void DestroyRulePrimitive(struct RulePrimitive* _Rule) {
	free(_Rule);
}

struct RuleComparator* CreateRuleComparator(int _Type, struct Rule* _Left, struct Rule* _Right) {
	struct RuleComparator* _Rule = (struct RuleComparator*) malloc(sizeof(struct RuleComparator));

	_Rule->Type = _Type;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleComparator;
	_Rule->Left = _Left;
	_Rule->Right = _Right;
	return _Rule;
}

void DestroyRuleComparator(struct RuleComparator* _Rule) {
	_Rule->Left->Destroy(_Rule->Left);
	_Rule->Right->Destroy(_Rule->Right);
	free(_Rule);
}

struct RuleBoolean* CreateRuleBoolean(int _Boolean) {
	struct RuleBoolean* _Rule = (struct RuleBoolean*) malloc(sizeof(struct RuleBoolean));

	_Rule->Type = RULE_BOOLEAN;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleBoolean;
	_Rule->Boolean = _Boolean;
	return _Rule;
}

void DestroyRuleBoolean(struct RuleBoolean* _Rule) {
	free(_Rule);
}

struct RuleLuaCall* CreateRuleLuaCall(lua_State* _State, const char* _FuncName, struct Primitive** _Arguments) {
	struct RuleLuaCall* _Rule = (struct RuleLuaCall*) malloc(sizeof(struct RuleLuaCall));

	_Rule->Type = RULE_LUACALL;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleLuaCall;
	_Rule->State = _State;
	_Rule->FuncName = calloc(strlen(_FuncName) + 1, sizeof(char));
	_Rule->Arguments = _Arguments;
	strcpy(_Rule->FuncName, _FuncName);
	return _Rule;
}

void DestroyRuleLuaCall(struct RuleLuaCall* _Rule) {
	struct Primitive** _Itr = _Rule->Arguments;

	while((*_Itr) != NULL) {
		free(*_Itr);
		++_Itr;
	}
	free(_Rule->FuncName);
	free(_Rule->Arguments);
	free(_Rule);
}

void PrimitiveLuaPush(lua_State* _State, struct Primitive* _Primitive) {
	switch(_Primitive->Type) {
	case PRIM_FLOAT:
		lua_pushnumber(_State, _Primitive->Value.Float);
		break;
	case PRIM_INTEGER:
		lua_pushinteger(_State, _Primitive->Value.Int);
		break;
	case PRIM_BOOLEAN:
		lua_pushboolean(_State, _Primitive->Value.Int);
		break;
	case PRIM_PTR:
		lua_pushlightuserdata(_State, _Primitive->Value.Ptr);
		break;
	case PRIM_STRING:
		lua_pushstring(_State, _Primitive->Value.String);
		break;
	}
}

struct Primitive* LuaToPrimitive(lua_State* _State, int _Index) {
	struct Primitive* _Primitive = CreatePrimitive();

	switch(lua_type(_State, _Index)) {
		case LUA_TBOOLEAN:
			_Primitive->Type = PRIM_BOOLEAN;
			_Primitive->Value.Int = lua_toboolean(_State, _Index);
			break;
		case LUA_TSTRING:
			_Primitive->Type = PRIM_STRING;
			_Primitive->Value.String = calloc(strlen(lua_tostring(_State, _Index)) + 1, sizeof(char));
			strcpy(_Primitive->Value.String, lua_tostring(_State, _Index));
			break;
		case LUA_TNUMBER:
			_Primitive->Type = PRIM_INTEGER;
			_Primitive->Value.Int = lua_tointeger(_State, _Index);
			break;
	}
	return _Primitive;
}

struct RuleEvent* CreateRuleEvent(int _Event) {
	struct RuleEvent* _Rule = (struct RuleEvent*) malloc(sizeof(struct RuleEvent));

	_Rule->Type = RULE_EVENT;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleEvent;
	_Rule->Event = _Event;
	return _Rule;
}
void DestroyRuleEvent(struct RuleEvent* _Rule) {
	free(_Rule);
}

int LuaRuleLuaCall(lua_State* _State) {
	const char* _Func = NULL;
	int _Args = lua_gettop(_State);
	int i = 1;
	struct Primitive** _Primitive = NULL;
	struct RuleLuaCall* _Rule = NULL;

	if(_Args > 1)
		_Primitive = calloc(sizeof(struct Primitive), (_Args));
	_Func = luaL_checkstring(_State, 1);
	for(i = 1; i < _Args; ++i)
		_Primitive[i - 1] = LuaToPrimitive(_State, i + 1);
	_Primitive[i] = NULL;
	_Rule = CreateRuleLuaCall(_State, _Func, _Primitive);
	LuaCtor(_State, "Rule", _Rule);
	return 1;
}

int LuaRuleGreaterThan(lua_State* _State) {
	struct RuleComparator* _Rule = NULL;
	struct Rule* _Left = NULL;
	struct Rule* _Right = NULL;

	_Left = LuaValueToRule(_State, 1);
	_Right = LuaValueToRule(_State, 2);
	_Rule = CreateRuleComparator(RULE_GREATERTHAN, _Left, _Right);
	LuaCtor(_State, "Rule", _Rule);
	return 1;
}

int LuaRuleLessThan(lua_State* _State) {
	struct RuleComparator* _Rule = NULL;
	struct Rule* _Left = NULL;
	struct Rule* _Right = NULL;

	_Left = LuaValueToRule(_State, 1);
	_Right = LuaValueToRule(_State, 2);
	_Rule = CreateRuleComparator(RULE_LESSTHAN, _Left, _Right);
	LuaCtor(_State, "Rule", _Rule);
	return 1;
}

int LuaRuleTrue(lua_State* _State) {
	struct Rule* _Rule = (struct Rule*) CreateRuleBoolean(1);
	LuaCtor(_State, "Rule", _Rule);
	return 1;
}

int LuaRuleFalse(lua_State* _State) {
	struct Rule* _Rule = (struct Rule*) CreateRuleBoolean(0);
	LuaCtor(_State, "Rule", _Rule);
	return 1;
}

int LuaRuleEventFired(lua_State* _State) {
	int i = 0;
	struct Rule* _Rule = NULL;
	const char* _String = luaL_checkstring(_State, 1);

	for(i = 0; g_EventNames[i] != NULL; ++i) {
		if(strcmp(g_EventNames[i], _String) == 0) {
			_Rule = (struct Rule*) CreateRuleEvent(i);
			LuaCtor(_State, "Rule", _Rule);
			return 1;
		}
	}
	luaL_error(_State, "%s is not an event name.", _String);
	return 0;
}

struct Rule* LuaValueToRule(lua_State* _State, int _Index) {
	struct RulePrimitive* _Rule = NULL;

	if(lua_type(_State, _Index) == LUA_TTABLE) {
		if((_Rule = LuaToObject(_State, _Index, "Rule")) == NULL) {
			luaL_error(_State, LUA_TYPERROR(_State, _Index, "Rule", "LuaRuleGreaterThan"));
			return NULL;
		}
	} else
		_Rule = CreateRulePrimitive(LuaToPrimitive(_State, 1));
	return (struct Rule*) _Rule;
}

int RuleTrue(const struct Rule* _Rule) {
	return 1;
}

int RuleFalse(const struct Rule* _Rule) {
	return 0;
}

int RuleGreaterThan(const struct RuleComparator* _Rule) {
	return (RuleEval(_Rule->Left) > RuleEval(_Rule->Right));
}


int RuleLessThan(const struct RuleComparator* _Rule) {
	return (RuleEval(_Rule->Left) < RuleEval(_Rule->Right));
}

int RuleLuaCall(const struct RuleLuaCall* _Rule) {
	int i = 0;
	struct Primitive** _Itr = _Rule->Arguments;

	lua_getglobal(_Rule->State, _Rule->FuncName);
	if(lua_type(_Rule->State, -1) != LUA_TFUNCTION)
		return 0;
	while((*_Itr) != NULL) {
		PrimitiveLuaPush(_Rule->State, *_Itr);
		++i;
		++_Itr;
	}
	LuaCallFunc(_Rule->State, i, 1, 0);
	if(lua_isboolean(_Rule->State, -1) != 0)
		return lua_toboolean(_Rule->State, -1);
	else if(lua_isnumber(_Rule->State, -1) != 0)
		return lua_tointeger(_Rule->State, -1);
	return 0;
}

int RulePrimitive(const struct RulePrimitive* _Primitive) {
	if(_Primitive->Value.Type == PRIM_INTEGER)
		return _Primitive->Value.Value.Int;
	return 0;
}

int RuleBoolean(const struct RuleBoolean* _Rule) {
	return _Rule->Boolean != 0;
}

int RuleEventCompare(const struct Rule* _One, const struct Rule* _Two) {
	int _Diff = _One->Type - _Two->Type;

	if(_Diff != 0)
		return _Diff;
	if(_One->Type == RULE_EVENT)
		return ((struct RuleEvent*)_One)->Event - ((struct RuleEvent*)_Two)->Event;
	_Diff = RuleEventCompare(((struct RuleComparator*)_One)->Left, ((struct RuleComparator*)_Two)->Left);
	if(_Diff != 0)
		return _Diff;
	return RuleEventCompare(((struct RuleComparator*)_One)->Right, ((struct RuleComparator*)_Two)->Right);
}
