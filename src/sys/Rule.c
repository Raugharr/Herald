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
#include <assert.h>

RuleFunc g_RuleFuncLookup[] = {
		RuleTrue,
		(RuleFunc) RuleBoolean,
		(RuleFunc) RuleGreaterThan,
		(RuleFunc) RuleLessThan,
		(RuleFunc) RuleLuaCall,
		(RuleFunc) RulePrimitive,
		(RuleFunc) RuleIfThenElse,
		RuleTrue,
		(RuleFunc) RuleBlock,
		(RuleFunc) RuleLuaObject,
		(RuleFunc) RuleNegate,
		(RuleFunc) RuleCond
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

void PrimitiveToStr(const struct Primitive* Prim, char* restrict  Buffer, uint32_t BufSz) {
	switch(Prim->Type) {
		case PRIM_FLOAT:
			snprintf(Buffer, BufSz, "%f", Prim->Value.Float);
			break;
		case PRIM_INTEGER:
		case PRIM_BOOLEAN:
			snprintf(Buffer, BufSz, "%i", Prim->Value.Int);
			break;
		case PRIM_PTR:
			snprintf(Buffer, BufSz, "%p", Prim->Value.Ptr);
			break;
		case PRIM_STRING:
			snprintf(Buffer, BufSz, "%s", Prim->Value.String);
			break;
	}
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

void PrimitiveSetInt(struct Primitive* _Primitive, int _Int) {
	_Primitive->Value.Int = _Int;
	_Primitive->Type = PRIM_INTEGER;
}

void PrimitiveSetFloat(struct Primitive* _Primitive, float _Float) {
	_Primitive->Value.Float = _Float;
	_Primitive->Type = PRIM_FLOAT;
}

void PrimitiveSetPtr(struct Primitive* _Primitive, void* _Ptr, uint16_t Class) {
	_Primitive->Value.Ptr = _Ptr;
	_Primitive->Type = PRIM_PTR;
	_Primitive->Class = Class;
}

void PrimitiveSetStr(struct Primitive* _Primitive, const char* _Str) {
	_Primitive->Value.String = calloc(strlen(_Str) + 1, sizeof(char));
	strcpy(_Primitive->Value.String, _Str);
	_Primitive->Type = PRIM_STRING;
}

int RuleCmp(const void* _One, const void* _Two) {
	return RuleEval(((struct Rule*)_One)) - RuleEval(((struct Rule*)_Two));
}

struct RulePrimitive* CreateRulePrimitive(struct Primitive* _Primitive) {
	struct RulePrimitive* _Rule = (struct RulePrimitive*) malloc(sizeof(struct RulePrimitive));

	_Rule->Type = RULE_PRIMITIVE;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRulePrimitive;
	_Rule->Value.Type = -1;
	_Rule->Value = *_Primitive;
	return _Rule;
}

void DestroyRulePrimitive(struct RulePrimitive* _Rule) {
	free(_Rule);
}

struct RuleDecorator* CreateRuleDecorator(int _Type, struct RuleLuaCall* _RuleDec) {
	struct RuleDecorator* _Rule = (struct RuleDecorator*) malloc(sizeof(struct RuleDecorator));

	assert(_Type == RULE_NEGATE);
	_Rule->Type = _Type;
	_Rule->Destroy = (void(*)(struct Rule*))DestroyRuleDecorator;
	_Rule->Rule = _RuleDec;
	return _Rule;
}

void DestroyRuleDecorator(struct RuleDecorator* _Rule) {
	_Rule->Rule->Destroy((struct Rule*) _Rule->Rule);
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

struct RuleCond* CreateRuleCond(int _Conditions) {
	struct RuleCond* _Rule = (struct RuleCond*) malloc(sizeof(struct RuleCond));

	_Rule->Type = RULE_COND;
	_Rule->Destroy = (RuleDestroy) DestroyRuleCond;
	_Rule->Conditions = calloc(_Conditions, sizeof(struct RuleBoolean*));
	_Rule->Actions = calloc(_Conditions, sizeof(struct Rule*));
	_Rule->ListSz = _Conditions;
	return _Rule;
}

void DestroyRuleCond(struct RuleCond* _Rule) {
	for(int i = 0; i < _Rule->ListSz; ++i) {
		_Rule->Conditions[i]->Destroy((struct Rule*) _Rule->Conditions[i]);
		_Rule->Actions[i]->Destroy(_Rule->Actions[i]);
	}
	free(_Rule->Conditions);
	free(_Rule->Actions);
	free(_Rule);
}

struct RuleLuaObj* CreateRuleLuaObj(void* _Object, const char* _Class) {
	struct RuleLuaObj* _Rule = (struct RuleLuaObj*) malloc(sizeof(struct RuleLuaObj));

	_Rule->Destroy = (void (*)(struct Rule*))DestroyRuleLuaObj;
	_Rule->Type = RULE_LUAOBJ;
	_Rule->Object = _Object;
	_Rule->Class = calloc(strlen(_Class) + 1, sizeof(char));
	strcpy((char*) _Rule->Class, _Class);
	return _Rule;
}

void DestroyRuleLuaObj(struct RuleLuaObj* _Obj) {
	free((char*) _Obj->Class);
	free(_Obj);
}

int RuleTrue(const struct Rule* _Rule, lua_State* _State) {
	return 1;
}

int RuleFalse(const struct Rule* _Rule, lua_State* _State) {
	return 0;
}

int RuleGreaterThan(const struct RuleComparator* _Rule, lua_State* _State) {
	return (RuleEval(_Rule->Left) > RuleEval(_Rule->Right));
}

int RuleLessThan(const struct RuleComparator* _Rule, lua_State* _State) {
	return (RuleEval(_Rule->Left) < RuleEval(_Rule->Right));
}

int RuleLuaCall(const struct RuleLuaCall* _Rule, lua_State* _State) {
	int _Len = 0;
	int _Table = 0;
	struct RuleLuaCall* _RuleArg = NULL;

	lua_rawgeti(_State, LUA_REGISTRYINDEX, _Rule->TblRef);
	_Table = lua_absindex(_State, -1);
	_Len = lua_rawlen(_State, _Table);
	if(_Len == 0) {
		return luaL_error(_State, "LuaCall: table has no function to call.");
	}
	//Unwrap all elements in order the first is a function and the follwing elements are its arguments.
	for(int i = 1; i <= _Len; ++i) {
		lua_rawgeti(_State, _Table, i);
		if(lua_type(_State, -1) == LUA_TTABLE && (_RuleArg = LuaTestClass(_State, -1, LOBJ_RULE)) != NULL) {
			if(_RuleArg->Type == RULE_LUACALL) {
				if(RuleLuaCall(_RuleArg, _State) == 0)
					return 0;
			} else {
				if(LuaRuleEval(((struct Rule*)_RuleArg), _State) == 0)
					return 0;
			}
			lua_remove(_State, -2);
		}
	}
	if(LuaCallFunc(_State, _Len - 1, 1, 0) == 0) //Len - 1 args because the element in the table is the function.
		return 0;
	lua_remove(_State, _Table);//pop _Rule->TblRef.
	return 1;
}

int RulePrimitive(const struct RulePrimitive* _Primitive, lua_State* _State) {
	if(_Primitive->Value.Type == PRIM_INTEGER)
		return _Primitive->Value.Value.Int;
	return 0;
}

int RuleIfThenElse(const struct RuleIfThenElse* _Rule, lua_State* _State) {
	if(RuleEval((struct Rule*) _Rule->Comparator) != 0)
		return RuleEval(_Rule->OnTrue);
	return RuleEval(_Rule->OnFalse);
}

int RuleBoolean(const struct RuleBoolean* _Rule, lua_State* _State) {
	return _Rule->Boolean != 0;
}

int RuleBlock(const struct RuleBlock* _Block, lua_State* _State) {
	for(int i = 0; i < _Block->ListSz; ++i)
		RuleEval(_Block->RuleList[i]);
	return 0;
}

int RuleLuaObject(const struct RuleLuaObj* _Obj, lua_State* _State) {
	return 1;
}

int RuleNegate(const struct RuleDecorator* _Rule, lua_State* _State) {
	LuaRuleEval(((struct Rule*) _Rule->Rule), _State);
	if(lua_type(_State, LUA_TNUMBER)) {
		lua_pushinteger(_State, -lua_tointeger(_State, -1));
		lua_remove(_State, -2);
	}
	return 1;
}

int RuleCond(const struct RuleCond* _Obj, lua_State* _State) {
	for(int i = 0; i < _Obj->ListSz; ++i) {
		if(LuaRuleEval((struct Rule*) _Obj->Conditions[i], _State) != 0) {
			LuaRuleEval(_Obj->Actions[i], _State);
			return 1;	
		}
	}
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
