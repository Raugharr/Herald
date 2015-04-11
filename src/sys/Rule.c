/*
 * File: Rule.c
 * Author: David Brotz
 */

#include "Rule.h"

#include "LuaHelper.h"
#include "Log.h"

#include <string.h>
#include <stdlib.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <malloc.h>

int(*g_RuleFuncLookup[])(const struct Rule*) = {
		RuleTrue,
		RuleTrue,
		(int(*)(const struct Rule*))RuleGreaterThan,
		(int(*)(const struct Rule*))RuleLessThan,
		(int(*)(const struct Rule*))RuleLuaCall
};

struct Primitive* CreatePrimitive() {
	return (struct Primitive*) malloc(sizeof(struct Primitive));
}

void DestroyPrimitive(struct Primitive* _Primitive) {
	if(_Primitive->Type == PRIM_STRING)
		free(_Primitive->Value.String);
	free(_Primitive);
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

struct RuleBoolean* CreateRuleBoolean(struct Rule* _Boolean) {
	struct RuleBoolean* _Rule = (struct RuleBoolean*) malloc(sizeof(struct RuleBoolean));

	_Rule->Type = RULE_BOOLEAN;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleBoolean;
	_Rule->Boolean = _Boolean;
	return _Rule;
}

void DestroyRuleBoolean(struct RuleBoolean* _Rule) {
	_Rule->Boolean->Destroy(_Rule->Boolean);
	free(_Rule);
}

struct RuleLuaCall* CreateRuleLuaCall(lua_State* _State, const char* _FuncName, struct Primitive** _Arguments) {
	struct RuleLuaCall* _Rule = (struct RuleLuaCall*) malloc(sizeof(struct RuleLuaCall));

	_Rule->Type = RULE_LUACALL;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleLuaCall;
	_Rule->State = _State;
	_Rule->FuncName = calloc(strlen(_FuncName) + 1, sizeof(char));
	_Rule->Arguments = _Arguments;
	return _Rule;
}

void DestroyRuleLuaCall(struct RuleLuaCall* _Rule) {
	free(_Rule->FuncName);
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

int LuaRuleLuaCall(lua_State* _State) {
	const char* _Func = NULL;
	int _Args = lua_gettop(_State);
	int i = 1;
	struct Primitive** _Primitive = NULL;
	struct RuleLuaCall* _Rule = NULL;

	if(_Args > 1)
		_Primitive = alloca(sizeof(struct Primitive*) * (_Args - 1));
	_Func = luaL_checkstring(_State, 1);
	for(i = 1; i < _Args; ++i)
		_Primitive[i - 1] = LuaToPrimitive(_State, i);
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

int LuaRuleIsTrue(lua_State* _State) {
	struct Rule* _Boolean = NULL;
	struct RuleBoolean* _Rule = NULL;

	if((_Boolean = (struct Rule*) LuaToObject(_State, 1, "Rule")) == NULL)
			luaL_error(_State, LUA_TYPERROR(_State, 1, "Rule", "LuaRuleIsTrue"));
	_Rule = CreateRuleBoolean(_Boolean);
	LuaCtor(_State, "Rule", _Rule);
	return 1;
}

struct Rule* LuaValueToRule(lua_State* _State, int _Index) {
	struct RulePrimitive* _Rule = NULL;

	if(lua_type(_State, 1) == LUA_TTABLE) {
		if((_Rule = LuaToObject(_State, 1, "Rule")) == NULL) {
			luaL_error(_State, LUA_TYPERROR(_State, 1, "Rule", "LuaRuleGreaterThan"));
			return NULL;
		}
	} else
		_Rule = CreateRulePrimitive(LuaToPrimitive(_State, 1));
	return _Rule;
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
	struct Primitive* _Itr = *_Rule->Arguments;

	lua_getglobal(_Rule->State, _Rule->FuncName);
	while(_Itr != NULL) {
		PrimitiveLuaPush(_Rule->State, _Itr);
		++i;
		_Itr += sizeof(struct Primitive*);
	}
	LuaCallFunc(_Rule->State, i, 1, 0);
	if(lua_isboolean(_Rule->State, -1) != 0)
		return lua_toboolean(_Rule->State, -1);
	return 0;
}
