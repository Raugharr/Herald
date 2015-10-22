/*
 * File: Rule.c
 * Author: David Brotz
 */

#include "Rule.h"

#include "LuaCore.h"
#include "Log.h"
#include "Event.h"

#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

int(*g_RuleFuncLookup[])(const struct Rule*) = {
		RuleTrue,
		(int(*)(const struct Rule*))RuleBoolean,
		(int(*)(const struct Rule*))RuleGreaterThan,
		(int(*)(const struct Rule*))RuleLessThan,
		(int(*)(const struct Rule*))RuleLuaCall,
		(int(*)(const struct Rule*))RulePrimitive,
		(int(*)(const struct Rule*))RuleIfThenElse,
		RuleTrue,
		(int(*)(const struct Rule*))RuleBlock
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

void PrimitivePrint(const struct Primitive* _Primitive) {
	switch(_Primitive->Type) {
		case PRIM_FLOAT:
			Log(ELOG_INFO, "%f", _Primitive->Value.Float);
			break;
		case PRIM_INTEGER:
		case PRIM_BOOLEAN:
			Log(ELOG_INFO, "%i", _Primitive->Value.Int);
			break;
		case PRIM_PTR:
			Log(ELOG_INFO, "%X", _Primitive->Value.Ptr);
			break;
		case PRIM_STRING:
			Log(ELOG_INFO, "%s", _Primitive->Value.String);
			break;
	}
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

struct RuleIfThenElse* CreateRuleIfThenElse(struct RuleComparator* _Comparator, struct Rule* _OnTrue, struct Rule* _OnFalse) {
	struct RuleIfThenElse* _Rule = (struct RuleIfThenElse*) malloc(sizeof(struct RuleIfThenElse));

	_Rule->Type = RULE_IFTHENELSE;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleIfThenElse;
	_Rule->Comparator = _Comparator;
	_Rule->OnFalse = _OnFalse;
	_Rule->OnTrue = _OnTrue;
	return _Rule;
}

void DestroyRuleIfThenElse(struct RuleIfThenElse* _Rule) {
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

struct RuleLuaCall* CreateRuleLuaCall(lua_State* _State, int _TableRef) {
	struct RuleLuaCall* _Rule = (struct RuleLuaCall*) malloc(sizeof(struct RuleLuaCall));

	_Rule->Type = RULE_LUACALL;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleLuaCall;
	_Rule->State = _State;
	_Rule->TblRef = _TableRef;
	return _Rule;
}

void DestroyRuleLuaCall(struct RuleLuaCall* _Rule) {
	luaL_unref(_Rule->State, LUA_REGISTRYINDEX, _Rule->TblRef);
	free(_Rule);
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

struct RuleBlock* CreateRuleBlock(int _Size) {
	struct RuleBlock* _Rule = (struct RuleBlock*) malloc(sizeof(struct RuleBlock));

	_Rule->Type = RULE_BLOCK;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleBlock;
	_Rule->RuleList = calloc(_Size, sizeof(struct Rule*));
	_Rule->ListSz = _Size;
	return _Rule;
}

void DestroyRuleBlock(struct RuleBlock* _Rule) {
	free(_Rule->RuleList);
	free(_Rule);
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
	int _Len = 0;
	int _Table = 0;
	struct RuleLuaCall* _RuleArg = NULL;

	lua_rawgeti(_Rule->State, LUA_REGISTRYINDEX, _Rule->TblRef);
	_Table = lua_absindex(_Rule->State, -1);
	_Len = lua_rawlen(_Rule->State, _Table);
	for(i = 1; i <= _Len; ++i) {
		lua_rawgeti(_Rule->State, _Table, i);
		if(lua_type(_Rule->State, -1) == LUA_TTABLE && (_RuleArg = LuaTestClass(_Rule->State, -1, "Rule")) != NULL && _Rule->Type == RULE_LUACALL) {
			RuleLuaCall(_RuleArg);
			lua_remove(_Rule->State, -2);
		}
	}
	LuaCallFunc(_Rule->State, _Len - 1, 1, 0);
	lua_remove(_Rule->State, _Table);//pop _Rule->TblRef.
	if(lua_isnumber(_Rule->State, -1) != 0) {
		return lua_tointeger(_Rule->State, -1);
	}
	return 0;
}

int RulePrimitive(const struct RulePrimitive* _Primitive) {
	if(_Primitive->Value.Type == PRIM_INTEGER)
		return _Primitive->Value.Value.Int;
	return 0;
}

int RuleIfThenElse(const struct RuleIfThenElse* _Rule) {
	if(RuleEval((struct Rule*) _Rule->Comparator) != 0)
		RuleEval(_Rule->OnTrue);
	else
		RuleEval(_Rule->OnFalse);
	return 0;
}

int RuleBoolean(const struct RuleBoolean* _Rule) {
	return _Rule->Boolean != 0;
}

int RuleBlock(const struct RuleBlock* _Block) {
	for(int i = 0; i < _Block->ListSz; ++i)
		RuleEval(_Block->RuleList[i]);
	return 0;
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
