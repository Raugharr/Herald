/*
 * File: LuaCore.c
 * Author: David Brotz
 */

#include "LuaCore.h"

#include "Rule.h"
#include "Event.h"
#include "LinkedList.h"
#include "Log.h"
#include "Array.h"
#include "Constraint.h"
#include "Random.h"

#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <stdlib.h>
#include <string.h>

const char* g_LuaGlobals[] = {
		"Environments",
		NULL
};

static const luaL_Reg g_LuaCoreFuncs[] = {
		{"CreateConstraintBounds", LuaConstraintBnds},
		{"ToYears", LuaYears},
		{"ToMonth", LuaMonth},
		{"PrintDate", LuaPrintDate},
		{"PrintYears", LuaPrintYears},
		{"Hook", LuaHook},
		{"Random", LuaRandom},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsArrayIterator[] = {
		{"Itr", LuaArrayItr},
		{"Next", LuaArrayItrNext},
		{"Prev", LuaArrayItrPrev},
		{NULL, NULL}
};

static luaL_Reg g_LuaFuncsLinkedListNode[] = {
		{"Next", LuaLnkLstNodeNext},
		{"Prev", LuaLnkLstNodePrev},
		{"Itr", LuaLnkLstNodeItr},
		{NULL, NULL}
};

static luaL_Reg g_LuaFuncsLinkedList[] = {
		{"Front", LuaLnkLstFront},
		{"Back", LuaLnkLstBack},
		{"Size", LuaLnkLstSize},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsRule[] = {
		{"LuaCall", LuaRuleLuaCall},
		{"GreaterThan", LuaRuleGreaterThan},
		{"LessThan", LuaRuleLessThan},
		{"True", LuaRuleTrue},
		{"False", LuaRuleFalse},
		{"EventFired", LuaRuleEventFired},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsIterator[] = {
		{"Itr", NULL},
		{"Next", NULL},
		{"Prev", NULL},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsArray[] = {
		{"Create", LuaArrayCreate},
		{NULL, NULL}
};

static const struct LuaObjectReg g_LuaCoreObjects[] = {
		{"Iterator", NULL, g_LuaFuncsIterator},
		{"LinkedListNode", "Iterator", g_LuaFuncsLinkedListNode},
		{"LinkedList", NULL, g_LuaFuncsLinkedList},
		{"Rule", NULL, g_LuaFuncsRule},
		{"Array", NULL, g_LuaFuncsArray},
		{"ArrayIterator", "Iterator", g_LuaFuncsArrayIterator},
		{NULL, NULL, NULL}
};

void InitLuaCore() {
	//g_LuaState = luaL_newstate();
	for(int i = 0; g_LuaGlobals[i] != NULL; ++i) {
		lua_newtable(g_LuaState);
		lua_setglobal(g_LuaState, g_LuaGlobals[i]);
	}
	lua_atpanic(g_LuaState, LogLua);
	luaL_openlibs(g_LuaState);
	RegisterLuaObjects(g_LuaState, g_LuaCoreObjects);
	LuaRegisterFunctions(g_LuaState, g_LuaCoreFuncs);
	atexit(LogCloseFile);
}

void QuitLuaCore() {
	//lua_close(g_LuaState);
}

/*
 * TODO: Should allow for default member functions.
 */
void RegisterLuaObjects(lua_State* _State, const struct LuaObjectReg* _Objects) {
	int i = 0;

	while(_Objects[i].Name != NULL) {
		if(LuaRegisterObject(_State, _Objects[i].Name, _Objects[i].BaseClass, _Objects[i].Funcs) == 0)
			return (void) luaL_error(_State, "Loading Lua functions has failed loading class %s.", _Objects[i].Name);
		++i;
	}
}

int LuaRegisterObject(lua_State* _State, const char* _Name, const char* _BaseClass, const luaL_Reg* _Funcs) {
	if(luaL_newmetatable(_State, _Name) == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "__class");
	lua_pushstring(_State, _Name);
	lua_rawset(_State, -3);
	if(_BaseClass != NULL) {
		lua_pushliteral(_State, "__baseclass");
		lua_getglobal(_State, _BaseClass);
		if(lua_type(_State, -1) != LUA_TTABLE) {
			luaL_error(_State, "Loading Lua class %s failed. Base class %s is not a class.", _Name, _BaseClass);
			lua_pop(_State, 3);
			return 0;
		}
		lua_rawset(_State, -3);
	}
	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	if(_Funcs != NULL) {
		luaL_setfuncs(_State, _Funcs, 0);
	}
	lua_setglobal(_State, _Name);
	return 1;
}

void LuaRegisterFunctions(lua_State* _State, const luaL_Reg* _Funcs) {
	int i = 0;

	for(i = 0; (_Funcs[i].name != NULL && _Funcs[i].func != NULL); ++i)
		lua_register(_State, _Funcs[i].name, _Funcs[i].func);
}

int LuaArrayCreate(lua_State* _State) {
	int _Size = luaL_checkinteger(_State, 1);
	struct Array* _Array = CreateArray(_Size);

	lua_newtable(_State);
	lua_getglobal(_State, "Array");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Array);
	lua_rawset(_State, -3);
	return 1;
}

int LuaArrayItr_Aux(lua_State* _State) {
	struct Array* _Array = LuaCheckClass(_State, lua_upvalueindex(1), "ArrayIterator");
	int _Index = lua_tointeger(_State, lua_upvalueindex(2));
	int _Change = lua_tointeger(_State, lua_upvalueindex(3));
	int i;

	for(i = _Index; i < _Array->TblSize; i += _Change) {
		if(_Array->Table[_Index] != NULL)
			break;
		_Index += _Change;
	}

	if(_Index < 0 || _Index >= _Array->TblSize) {
		lua_pushnil(_State);
		lua_pushvalue(_State, 1);
		lua_pushnil(_State);
		return 3;
	}
	lua_pushvalue(_State, lua_upvalueindex(1));
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING)
		luaL_error(_State, "Iterator does not have a __classtype.");

	lua_newtable(_State);
	lua_getglobal(_State, lua_tostring(_State, -2));
	lua_setmetatable(_State, -2);
	lua_remove(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Array->Table[_Index]);
	lua_rawset(_State, -3);
	lua_pushinteger(_State, _Index + _Change);
	lua_replace(_State, lua_upvalueindex(2));
	return 1;
}

int LuaArrayItrNext(lua_State* _State) {
	LuaTestClass(_State, 1, "ArrayIterator");
	lua_pushinteger(_State, 0);
	lua_pushinteger(_State, 1);
	lua_pushcclosure(_State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaArrayItrPrev(lua_State* _State) {
	LuaTestClass(_State, 1, "ArrayIterator");
	lua_pushinteger(_State, 0);
	lua_pushinteger(_State, -1);
	lua_pushcclosure(_State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaLnkLstNodeIterate(lua_State* _State) {
	struct LnkLst_Node* _Itr = LuaCheckClass(_State, lua_upvalueindex(1), "LinkedListNode");
	int _Foward = lua_tointeger(_State, lua_upvalueindex(2));

	if(_Itr == NULL)
		return 0;
	LuaCtor(_State, "Reform", _Itr->Data);
	if(_Foward != 0)
		_Itr = _Itr->Next;
	else
		_Itr = _Itr->Prev;
	lua_pushvalue(_State, lua_upvalueindex(1));
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Itr);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
	return 1;
}

int LuaLnkLstFront(lua_State* _State) {
	struct LinkedList* _List = LuaCheckClass(_State, 1, "LinkedList");

	LuaCtor(_State, "LinkedListNode", _List->Front);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, 1);
	lua_rawset(_State, -3);
	lua_pushinteger(_State, 1);
	lua_pushcclosure(_State, LuaLnkLstNodeIterate, 2);
	return 1;
}

int LuaLnkLstBack(lua_State* _State) {
	struct LinkedList* _List = LuaCheckClass(_State, 1, "LinkedList");

	LuaCtor(_State, "LinkedListNode", _List->Back);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, 1);
	lua_rawset(_State, -3);
	return 1;
}

int LuaLnkLstSize(lua_State* _State) {
	struct LinkedList* _List = LuaCheckClass(_State, 1, "LinkedList");

	lua_pushinteger(_State, _List->Size);
	return 1;
}

int LuaLnkLstNodeNext(lua_State* _State) {
	struct LnkLst_Node* _Node = LuaCheckClass(_State, 1, "LinkedListNode");

	LuaCtor(_State, "LinkedListNode", _Node->Next);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, 1);
	lua_rawset(_State, -3);
	return 1;
}

int LuaLnkLstNodePrev(lua_State* _State) {
	struct LnkLst_Node* _Node = LuaCheckClass(_State, 1, "LinkedListNode");

	LuaCtor(_State, "LinkedListNode", _Node->Prev);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, 1);
	lua_rawset(_State, -3);
	return 1;
}

int LuaLnkLstNodeItr(lua_State* _State) {
	struct LnkLst_Node* _Node = LuaCheckClass(_State, 1, "LinkedListNode");
	const char* _Class = NULL;

	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING) {
		luaL_error(_State, "__classtype is not defined.");
		lua_pushnil(_State);
		return 1;
	}
	_Class = lua_tostring(_State, -1);
	LuaCtor(_State, _Class, _Node->Data);
	return 1;
}

int LuaArrayItr(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, "Iterator"));
	return 1;
}

void* LuaToObject(lua_State* _State, int _Index, const char* _Class) {
	void* _Obj = NULL;

	if((_Obj = LuaTestClass(_State, _Index, _Class)) == NULL)
		return LuaCheckClass(_State, _Index, _Class);
	return _Obj;
}

int LuaConstraint(lua_State* _State) {
	lua_pushlightuserdata(_State, CreateConstrntLst(NULL, luaL_checkint(_State, 1),  luaL_checkint(_State, 2),  luaL_checkint(_State, 3)));
	return 1;
}

int LuaConstraintBnds(lua_State* _State) {
	luaL_checktype(_State, -1, LUA_TTABLE);

	int _Size = lua_rawlen(_State, -1);
	int _CurrMin = -1;
	int _CurrMax = -1;
	int i = 0;
	struct Constraint** _Constrnt = (struct Constraint**) malloc(sizeof(struct Constraint) * (_Size));

	lua_pushnil(_State);
	if(lua_next(_State, -2) != 0) {
		_CurrMin = luaL_checkint(_State, -1);
		lua_pop(_State, 1);
		if(lua_next(_State, -2) != 0) {
			_CurrMax = luaL_checkint(_State, -1);
			_Constrnt[i++] = CreateConstraint(_CurrMin, _CurrMax);
		} else
			goto error;
		lua_pop(_State, 1);
	} else
		goto error;
	while(lua_next(_State, -2) != 0) {
		_CurrMin = _CurrMax + 1;
		_CurrMax = luaL_checkint(_State, -1);
		_Constrnt[i++] = CreateConstraint(_CurrMin, _CurrMax);
		lua_pop(_State, 1);
	}
	_Constrnt[_Size - 1] = NULL;
	lua_pushlightuserdata(_State, _Constrnt);
	return 1;
	error:
	luaL_error(_State, "Cannot create constraint.");
	free(_Constrnt);
	return 0;
}

void ConstraintBndToLua(lua_State* _State, struct Constraint** _Constraints) {
	int i = 0;

	lua_newtable(_State);
	while(*_Constraints != NULL) {
		ConstraintToLua(_State, *_Constraints);
		lua_rawseti(_State, -2, i++);
		_Constraints += sizeof(struct Constraint*);
	}
}

int LuaYears(lua_State* _State) {
	lua_pushinteger(_State, luaL_checkinteger(_State, 1) * YEAR_DAYS);
	return 1;
}

int LuaMonth(lua_State* _State) {
	const char* _Type = NULL;

	_Type = luaL_checkstring(_State, 1);
	luaL_checkinteger(_State, 2);
	if(!strcmp(_Type, "Years"))
		lua_pushinteger(_State, TO_YEARS(lua_tointeger(_State, 2)));
	else if(!strcmp(_Type, "Days"))
		lua_pushinteger(_State, TO_DAYS(lua_tointeger(_State, 2)));
	else {
		return luaL_argerror(_State, 2, "Must be either \"Years\" or \"Days\".");

	}
	return 1;
}

int LuaPrintDate(lua_State* _State) {
	DATE _Date = luaL_checkinteger(_State, 1);
	DATE _Days = DAY(_Date);
	DATE _Months = MONTH(_Date);
	DATE _Years = YEAR(_Date);

	if(_Months > 11)
		_Months = 11;
	lua_pushfstring(_State, "%s %d, %d", g_ShortMonths[_Months], _Days, _Years);
	return 1;
}

int LuaPrintYears(lua_State* _State) {
	DATE _Date = luaL_checkinteger(_State, 1);
	DATE _Years = YEAR(_Date);

	lua_pushfstring(_State, "%d Years", _Years);
	return 1;
}

int LuaHook(lua_State* _State) {
	const char* _Name = NULL;

	_Name = luaL_checkstring(_State, 1);
	if(!strcmp(_Name, "Age")) {
		lua_pushlightuserdata(_State, CreateEventTime(NULL, luaL_checkinteger(_State, 2)));
	} else {
		luaL_error(_State, "Must be a valid hook type.");
		return 0;
	}
	return 1;
}

int LuaRandom(lua_State* _State) {
	Random(luaL_checkinteger(_State, 1), luaL_checkinteger(_State, 2));
	return 1;
}

int LuaLoadFile(lua_State* _State, const char* _File, const char* _Environment) {
	int _Error = luaL_loadfile(_State, _File);

	if(_Error != 0)
		goto error;
	if(_Environment != NULL) {
		LuaGetEnv(_State, _Environment);
		lua_setupvalue(_State, -2, 1);
	}
	if((_Error = lua_pcall(_State, 0, LUA_MULTRET, 0)) != 0)
		goto error;
	const char* _Test = lua_getupvalue(_State, -1, 1);
	if(_Test != NULL)
		lua_pop(_State, 1);
	return LUA_OK;

	error:
	switch(_Error) {
		case LUA_ERRSYNTAX:
			Log(ELOG_ERROR, "%s", lua_tostring(_State, -1));
			return _Error;
		case LUA_ERRFILE:
			Log(ELOG_ERROR, "Cannot load file: %s", _File);
			return _Error;
		case LUA_ERRRUN:
			Log(ELOG_ERROR, "Cannot run file: %s", lua_tostring(_State, -1));
			return _Error;
	}
	return LUA_ERRERR;
}

int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc) {
	//TODO: If in debug mode the stack should be checked to ensure its balanced.
	int _Error = lua_pcall(_State, _Args, _Results, _ErrFunc);

	if(_Error != 0)
		goto error;
	return 1;

	error:
	Log(ELOG_ERROR, "%s", lua_tostring(_State, -1));
	return 0;
}

int LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return) {
	void* _CallRet = NULL;

	if(LuaLoadFile(_State, _File, NULL) != LUA_OK)
		return 0;
	lua_getglobal(_State, _Global);
	if(!lua_istable(_State, -1))
		return 0;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_istable(_State, -1)) {
			Log(ELOG_WARNING, "Warning: index is not a table.");
			lua_pop(_State, 1);
			continue;
		}
		if((_CallRet = _Callback(_State, -1)) != NULL)
			_Insert(_Return, _CallRet);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	return 1;
}

int AddInteger(lua_State* _State, int _Index, int* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tointeger(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a integer");
	return 0;
}

int AddString(lua_State* _State, int _Index, const char** _String) {
	if(lua_isstring(_State, _Index)) {
		*_String = lua_tostring(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a string");
	return 0;
}

int AddNumber(lua_State* _State, int _Index, double* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tonumber(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a number");
	return 0;
}

int LuaLudata(lua_State* _State, int _Index, void** _Data) {
	if(lua_islightuserdata(_State, _Index)) {
		*_Data = lua_touserdata(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not user data");
	return 0;
}

int LuaFunction(lua_State* _State, int _Index, lua_CFunction* _Function) {
	if(lua_iscfunction(_State, _Index)) {
		*_Function = lua_tocfunction(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a c function.");
	return 0;
}

void LuaStackToTable(lua_State* _State, int* _Table) {
	int _Top = lua_gettop(_State);
	int i;

	for(i = 0; i < _Top; ++i)
		_Table[i] = lua_type(_State, i);
}

void LuaCopyTable(lua_State* _State, int _Index) {
	_Index = LuaAbsPos(_State, _Index);

	if(lua_type(_State, _Index) != LUA_TTABLE)
		return;
	if(lua_type(_State, -1) != LUA_TTABLE)
		return;
	lua_pushvalue(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushvalue(_State, -2);
		lua_pushvalue(_State, -2);
		lua_rawset(_State, -6);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	lua_remove(_State, _Index);
}

void* LuaToClass(lua_State* _State, int _Index) {
	void* _Pointer = NULL;
	int _Pos = LuaAbsPos(_State, _Index);

	if((_Pointer = lua_touserdata(_State, _Pos)) == NULL) {
		if(lua_type(_State, _Pos) == LUA_TNIL)
			return NULL;
		if(lua_type(_State, _Pos) != LUA_TTABLE)
			luaL_error(_State, "LuaToClass: index is not a class (expected table got %s).", lua_typename(_State, lua_type(_State, _Pos)));
		lua_pushstring(_State, "__self");
		lua_rawget(_State, _Pos);
		_Pointer = lua_touserdata(_State, -1);
		lua_pop(_State, 1);
	}
	return _Pointer;
}

void* LuaTestClass(lua_State* _State, int _Index, const char* _Class) {
	if(lua_getmetatable(_State, _Index) == 0)
		 luaL_error(_State, LUA_TYPERROR(_State, 1, _Class, "LuaTestClass"));
	lua_pushstring(_State, "__class");
	lua_rawget(_State, -2);
	if(!lua_isstring(_State, -1) || strcmp(_Class, lua_tostring(_State, -1)) != 0) {
		lua_pop(_State, 2);
		return NULL;
	}
	lua_pop(_State, 2);
	return LuaToClass(_State, _Index);
}

void* LuaCheckClass(lua_State* _State, int _Index, const char* _Class) {
	int _Pop = 4;
	lua_getglobal(_State, _Class);
	if(lua_getmetatable(_State, _Index) == 0) {
		lua_pop(_State, 1);
		return NULL;
	}
	if(lua_rawequal(_State, -1, -2)) {
		lua_pop(_State, 2);
		return LuaToClass(_State, _Index);
	}
	lua_pop(_State, 2);
	lua_pushvalue(_State, _Index);
	if(lua_getmetatable(_State, -1) == 0) {
		lua_pop(_State, 1);
		goto end;
	}
	top:
	lua_pushliteral(_State, "__baseclass");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) == LUA_TTABLE) {
		lua_getglobal(_State, _Class);
	if(!lua_rawequal(_State, -1, -2)) {
			lua_copy(_State, -2, -4);
			lua_pop(_State, 3);
			_Pop = 3;
			goto top;
		}
		lua_pop(_State, _Pop);
		return LuaToClass(_State, _Index);
	}
	end:
	lua_pop(_State, 1);
	return (void*) luaL_error(_State, LUA_TYPERROR(_State, 1, _Class, "LuaCheckClass"));
}

int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two) {
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _One) == -1)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _Two) == -1)
		goto fail;
	lua_pop(_State, 2);
	return 1;
	fail:
	lua_settop(_State, _Top);
	return 0;
}

int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair) {
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddString(_State, -1, _Value) == -1)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _Pair) == -1)
		goto fail;
	lua_pop(_State, 2);
	return 1;
	fail:
	lua_settop(_State, _Top);
	return 0;
}

int LuaRuleLuaCall(lua_State* _State) {
	int i = 1;
	int _Args = lua_gettop(_State);
	struct RuleLuaCall* _Rule = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_createtable(_State, _Args, 0);
	for(i = 1; i <= _Args; ++i) {
		lua_pushvalue(_State, i);
		lua_rawseti(_State, -2, i);
	}
	_Rule = CreateRuleLuaCall(_State, luaL_ref(_State, LUA_REGISTRYINDEX));
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

void LuaSetEnv(lua_State* _State, const char* _Env) {
	lua_getglobal(_State, "Environments");
	lua_pushstring(_State, _Env);
	lua_pushvalue(_State, -3);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
}

void LuaGetEnv(lua_State* _State, const char* _Env) {
	lua_getglobal(_State, "Environments");
	lua_pushstring(_State, _Env);
	lua_rawget(_State, -2);
	lua_remove(_State, -2);
}
