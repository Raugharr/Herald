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
#include "Math.h"
#include "ResourceManager.h"

#include "../LuaGovernment.h"
#include "../LuaSettlement.h"
#include "../LuaFamily.h"
#include "../Mission.h"
#include "../Date.h"
#include "../LuaWorld.h"

#include "../video/GuiLua.h"
#include "../video/VideoLua.h"

#include "../AI/LuaAI.h"

#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LUA_CLASSCT (255)

lua_State* g_LuaState = NULL;

const char* g_LuaGlobals[] = {
		"Environments",
		NULL
};

int g_LuaClassTable[LUA_CLASSCT] = {0};

static const luaL_Reg g_LuaObjectFuncs[] = {
	{"__eq", LuaObjectIsEqual},
	{"Equal", LuaObjectIsEqual},
	{"GetClassName", LuaObjectGetClassName},
	{"Null", LuaNull},
	{NULL, NULL}
};

static const luaL_Reg g_LuaCoreFuncs[] = {
	{"CreateConstraintBounds", LuaConstraintBnds},
	{"ToYears", LuaYears},
	{"ToMonth", LuaMonth},
	{"PrintDate", LuaPrintDate},
	{"PrintYears", LuaPrintYears},
	{"Hook", LuaHook},
	{"Random", LuaRandom},
	{"Null", LuaNull},
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
	{"IfThenElse", LuaRuleIfThenElse},
	{"EventFired", LuaRuleEventFired},
	{"Block", LuaRuleBlock},
	{"Cond", LuaRuleCond},
	{"Negate", LuaRuleNegate},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsIterator[] = {
	{"Itr", NULL},
	{"Next", NULL},
	{"Prev", NULL},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsList[] = {
	{"Itr", NULL},
	{"Next", NULL},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsArray[] = {
	{"Create", LuaArrayCreate},
	{"Itr", LuaArrayCreateItr},
	{"GetSize", LuaArrayGetSize},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsMission[] = {
	{"GetName", LuaMissionGetName},
	{"GetDescription", LuaMissionGetDesc},
	{"GetOptions", LuaMissionGetOptions},
	{"ChooseOption", LuaMissionChooseOption},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsMath[] = {
	{"RandomVar", LuaMathRandomVar},
	{"Probability", LuaMathProbability},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsInput[] = {
	{"MousePos", LuaInputMousePos},
	{NULL, NULL}
};

static const struct LuaObjectReg g_LuaCoreObjects[] = {
	{LOBJ_ITERATOR, "Iterator", LUA_REFNIL, g_LuaFuncsIterator},
	{LOBJ_LINKEDLISTNODE, "LinkedListNode", LOBJ_ITERATOR, g_LuaFuncsLinkedListNode},
	{LOBJ_LINKEDLIST, "LinkedList", LUA_REFNIL, g_LuaFuncsLinkedList},
	{LOBJ_LIST, "List", LUA_REFNIL, g_LuaFuncsList},
	{LOBJ_RULE, "Rule", LUA_REFNIL, g_LuaFuncsRule},
	{LOBJ_ARRAY, "Array", LUA_REFNIL, g_LuaFuncsArray},
	{LOBJ_ARRAYITERATOR, "ArrayIterator", LOBJ_ARRAY, g_LuaFuncsArrayIterator},
	{LOBJ_MISSION, "Mission", LUA_REFNIL, g_LuaFuncsMission},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

int LuaCallFuncError(lua_State* _State) {
	lua_Debug _Debug;

	if(lua_isstring(_State, -1) == 0)
		return 0;
	lua_getstack(_State, 0, &_Debug);
	lua_getinfo(_State, "S", &_Debug);
	lua_getglobal(_State, "debug");
	lua_getfield(_State, -1, "traceback");
	lua_call(_State, 0, 1);
	lua_pushvalue(_State, -3);
	lua_concat(_State, 2);
	return 1;
}

static inline int LuaItrGetClass(lua_State* _State, int _Idx) {
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, _Idx);
	if(lua_type(_State, -1) != LUA_TNUMBER) {
		return luaL_error(_State, "__classtype is not an integer.");
	}
	return lua_tointeger(_State, -1);
}

void InitLuaCore() {
	g_LuaState = luaL_newstate();
	for(int i = 0; i < LUA_CLASSCT; ++i)
		g_LuaClassTable[i] = LUA_REFNIL;
	for(int i = 0; g_LuaGlobals[i] != NULL; ++i) {
		lua_newtable(g_LuaState);
		lua_setglobal(g_LuaState, g_LuaGlobals[i]);
	}
	lua_atpanic(g_LuaState, LuaCallFuncError);
	luaL_openlibs(g_LuaState);
	LuaRegisterObject(g_LuaState, "Object", LUA_BASECLASS, LUA_REFNIL, g_LuaObjectFuncs);
	LuaRegisterFunctions(g_LuaState, g_LuaFuncsInput);
	RegisterLuaObjects(g_LuaState, g_LuaCoreObjects);
	LuaCreateLibrary(g_LuaState, g_LuaFuncsRule, "Rule");
	LuaRegisterFunctions(g_LuaState, g_LuaCoreFuncs);
	atexit(LogCloseFile);
}

int InitLuaSystem() {
	InitLuaCore();
	InitLuaFamily(g_LuaState);
	RegisterLuaObjects(g_LuaState, g_LuaSettlementObjects);
	RegisterLuaObjects(g_LuaState, g_LuaGovernmentObjects);
	RegisterLuaEnums(g_LuaState, g_LuaSettlementEnums);
	RegisterLuaEnums(g_LuaState, g_LuaGuiEnums);
	lua_getglobal(g_LuaState, "Plot");
	luaL_getmetatable(g_LuaState, "Plot");
	lua_setmetatable(g_LuaState, -2);
	lua_pop(g_LuaState, 1);
	//LuaSettlementObjects(g_LuaState);
	RegisterLuaObjects(g_LuaState, g_LuaAIObjects);

	InitMissionLua(g_LuaState);
	InitVideoLua(g_LuaState);
	lua_newtable(g_LuaState);
	lua_pushstring(g_LuaState, "__index");
	lua_pushvalue(g_LuaState, LUA_REGISTRYINDEX);
	lua_pushstring(g_LuaState, "Animation");
	lua_rawget(g_LuaState, -2);
	lua_pop(g_LuaState, 1);
	lua_rawset(g_LuaState, -3);
	lua_setmetatable(g_LuaState, -2);
	LuaSetEnv(g_LuaState, "Animation");
	lua_pop(g_LuaState, 1);

	LuaWorldInit(g_LuaState);
	Log(ELOG_INFO, "Loading Missions");
	++g_Log.Indents;
	LoadAllMissions(g_LuaState, &g_MissionEngine);
	--g_Log.Indents;
	return 1;
}

void QuitLuaSystem() {
	lua_close(g_LuaState);
}

/*
 * TODO: Should allow for default member functions.
 */
void RegisterLuaObjects(lua_State* _State, const struct LuaObjectReg* _Objects) {
	int i = 0;

	while(_Objects[i].Class != LUA_REFNIL) {
		if(LuaRegisterObject(_State, _Objects[i].Name, _Objects[i].Class, _Objects[i].BaseClass, _Objects[i].Funcs) == 0)
			return (void) luaL_error(_State, "Loading Lua functions has failed loading class %s.", _Objects[i].Class);
		++i;
	}
}

int LuaRegisterObject(lua_State* _State, const char* _Name, int _Class, int _BaseClass, const luaL_Reg* _Funcs) {
	int _Ref = 0;
	
	if(g_LuaClassTable[_Class] != LUA_REFNIL)
		return luaL_error(_State, "Loading Lua class #%d failed. (Class id is already defined.)", _Class);
	if(_Class < 0 || _Class > 0xFF)
		return luaL_error(_State, "Loading Lua class #%d failed. (Class id is invalid.)", _Class);
	if((_BaseClass < 0 && _BaseClass != LUA_REFNIL) || _BaseClass > 0xFF)
		return luaL_error(_State, "Loading Lua class #%d failed. (Base Class id #%d is invalid.)", _Class, _BaseClass);
	lua_newtable(_State);
	lua_pushvalue(_State, -1);
	_Ref = luaL_ref(_State, LUA_REGISTRYINDEX);
	if(_Ref == LUA_REFNIL) {
		lua_pop(_State, 1);
		return LUA_REFNIL;
	}
	lua_pushliteral(_State, "__index");
	lua_pushcfunction(_State, LuaClassIndex);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "__class");
	lua_pushinteger(_State, _Ref);
	lua_rawset(_State, -3);
	//Give them the base class Object. This doesn't need to be done for a class with a base class as their base class with be an object.
	if(_BaseClass != LUA_REFNIL) {
		lua_pushliteral(_State, "__baseclass");
		lua_rawgeti(_State, LUA_REGISTRYINDEX, g_LuaClassTable[_BaseClass]);
		if(lua_type(_State, -1) != LUA_TTABLE) {
			luaL_error(_State, "Loading Lua class #%d failed. (Base class #%d is not defined.)", _Class, _BaseClass);
			lua_pop(_State, 3);
			return LUA_REFNIL;
		}
		lua_rawset(_State, -3);
	}
	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	if(_Funcs != NULL)
		luaL_setfuncs(_State, _Funcs, 0);
	//if(luaL_newmetatable(_State, _Name) == 0)
	//	return luaL_error(_State, "Loading Lua class #%d failed. (Name %s is already taken.)", _Name);
	lua_rawgeti(_State, LUA_REGISTRYINDEX, _Ref);
	lua_setfield(_State, LUA_REGISTRYINDEX, _Name);
	//lua_setmetatable(_State, -2);
	//lua_pop(_State, 1);
	g_LuaClassTable[_Class] = _Ref;
	return 1;
}

void LuaRegisterFunctions(lua_State* _State, const luaL_Reg* _Funcs) {
	for(int i = 0; (_Funcs[i].name != NULL && _Funcs[i].func != NULL); ++i)
		lua_register(_State, _Funcs[i].name, _Funcs[i].func);
}

void CreateLuaLnkLstItr(lua_State* _State, struct LinkedList* _List, int _Class) {
	LuaCtor(_State, (void*)_List, LOBJ_LINKEDLIST);
	lua_pushstring(_State, "__classtype");
	lua_pushinteger(_State, _Class);
	lua_rawset(_State, -3);
}

//QUESTION: Should this be inlined or made a macro.
void CreateLuaArrayItr(lua_State* _State, struct Array* _Array, int _Class) {
	LuaCtor(_State, _Array, LOBJ_ARRAYITERATOR);
	lua_pushstring(_State, "__classtype");
	lua_pushinteger(_State, _Class);
	lua_rawset(_State, -3);
}

void LuaArrayClassToTable(lua_State* _State, const void** _Table, int _TableSz, int _Class) {
	lua_createtable(_State, _TableSz, 0);
	for(int i = 0; i < _TableSz; ++i) {
		LuaCtor(_State, (void*)_Table[i], _Class);
		lua_rawseti(_State, -2, i + 1);
	}
}

void LuaInitClass(lua_State* _State, void* _Ptr, int _Class) {
	if(_Class < 0 || _Class > 0xFF) {
		Log(ELOG_ERROR, "LuaInitClass: _Class argument got %i but requires [0:255].", _Class);
		return;
	}
	lua_rawgeti(_State, LUA_REGISTRYINDEX, g_LuaClassTable[_Class]);
	if(lua_type(_State, -1) == LUA_TTABLE) {
		lua_setmetatable(_State, -2);
		lua_pushstring(_State, "__self");
		lua_pushlightuserdata(_State, _Ptr);
		lua_rawset(_State, -3);
		return;
	}
	lua_pop(_State, 2);
	Log(ELOG_WARNING, "Lua class %i does not exist", _Class);
	lua_pushnil(_State);
}

const char* LuaObjectClass(lua_State* _State, int _Arg) {
	const char* _Name = NULL;

	if(lua_type(_State, _Arg) != LUA_TTABLE)
		return NULL;
	if(lua_getmetatable(_State, _Arg) == 0)
		return NULL;
	lua_pushstring(_State, "__class");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING) {
		lua_pop(_State, 2);
		return NULL;
	}
	_Name = lua_tostring(_State, -1);
	lua_pop(_State, 2);
	return _Name;	
}

void LuaCtorArray(lua_State* _State, struct Array* _Array, int _Class) {
	LuaCtor(_State, _Array, LOBJ_ARRAY);
	lua_pushstring(_State, "__classtype");
	lua_pushinteger(_State, _Class);
	lua_rawset(_State, -3);
}

int LuaArrayCreate(lua_State* _State) {
	int _Size = luaL_checkinteger(_State, 1);
	struct Array* _Array = CreateArray(_Size);

	LuaCtor(_State, _Array, LOBJ_ARRAY);
	return 1;
}

int LuaArrayCreateItr(lua_State* _State) {
	struct Array* _Array = LuaCheckClass(_State, 1, LOBJ_ARRAY);
	int _Class = LuaItrGetClass(_State, 1);

	CreateLuaArrayItr(_State, _Array, _Class);
	return 1;
}

int LuaArrayGetSize(lua_State* _State) {
	struct Array* _Array = LuaCheckClass(_State, 1, LOBJ_ARRAY);

	lua_pushinteger(_State, _Array->Size);
	return 1;
}

int LuaArrayItr_Aux(lua_State* _State) {
	struct Array* _Array = LuaCheckClass(_State, lua_upvalueindex(1), LOBJ_ARRAYITERATOR);
	int _Index = lua_tointeger(_State, lua_upvalueindex(2));
	int _Change = lua_tointeger(_State, lua_upvalueindex(3));
	int _Class = LUA_REFNIL;

	for(int i = _Index; i < _Array->Size; i += _Change) {
		if(_Array->Table[_Index] != NULL)
			break;
		_Index += _Change;
	}

	if(_Index < 0 || _Index >= _Array->Size) {
		lua_pushnil(_State);
		lua_pushvalue(_State, 1);
		lua_pushnil(_State);
		return 3;
	}
	_Class = LuaItrGetClass(_State, lua_upvalueindex(1));
	/*lua_pushvalue(_State, lua_upvalueindex(1));
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TNUMBER)
		luaL_error(_State, "Iterator does not have a __classtype.");

	_Class = lua_tointeger(_State, -1);
	*/
	LuaCtor(_State, _Array->Table[_Index], _Class);
	lua_pushinteger(_State, _Index + _Change);
	lua_replace(_State, lua_upvalueindex(2));
	return 1;
}

int LuaArrayItrNext(lua_State* _State) {
	LuaTestClass(_State, 1, LOBJ_ARRAYITERATOR);
	lua_pushinteger(_State, 0);
	lua_pushinteger(_State, 1);
	lua_pushcclosure(_State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaArrayItrPrev(lua_State* _State) {
	LuaTestClass(_State, 1, LOBJ_ARRAYITERATOR);
	lua_pushinteger(_State, 0);
	lua_pushinteger(_State, -1);
	lua_pushcclosure(_State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaLnkLstNodeIterate(lua_State* _State) {
	struct LnkLst_Node* _Itr = LuaCheckClass(_State, lua_upvalueindex(1), LOBJ_LINKEDLISTNODE);
	int _Foward = lua_tointeger(_State, lua_upvalueindex(2));
	int _ItrClass = LUA_REFNIL;

	if(_Itr == NULL)
		return 0;
	_ItrClass = LuaItrGetClass(_State, lua_upvalueindex(1));
	/*lua_pushstring(_State, "__classtype");
	lua_rawget(_State, lua_upvalueindex(1));
	if(lua_isnumber(_State, -1) == 0) {
		lua_pushnil(_State);
		return 1;
	}
	_ItrClass = lua_tointeger(_State, -1);
	*/
	lua_pop(_State, 1);
	LuaCtor(_State, _Itr->Data, _ItrClass);
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

static inline void LuaCreateLnkLstNode(lua_State* _State, struct LnkLst_Node* _Node) {
	LuaCtor(_State, _Node, LOBJ_LINKEDLISTNODE);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, 1);
	lua_rawset(_State, -3);
}

int LuaLnkLstFront(lua_State* _State) {
	struct LinkedList* _List = LuaCheckClass(_State, 1, LOBJ_LINKEDLIST);

	LuaCreateLnkLstNode(_State, _List->Front);
	lua_pushinteger(_State, 1);
	lua_pushcclosure(_State, LuaLnkLstNodeIterate, 2);
	return 1;
}

int LuaLnkLstBack(lua_State* _State) {
	struct LinkedList* _List = LuaCheckClass(_State, 1, LOBJ_LINKEDLIST);

	LuaCreateLnkLstNode(_State, _List->Back);
	return 1;
}

int LuaLnkLstSize(lua_State* _State) {
	struct LinkedList* _List = LuaCheckClass(_State, 1, LOBJ_LINKEDLIST);

	lua_pushinteger(_State, _List->Size);
	return 1;
}

int LuaLnkLstNodeNext(lua_State* _State) {
	struct LnkLst_Node* _Node = LuaCheckClass(_State, 1, LOBJ_LINKEDLISTNODE);

	LuaCreateLnkLstNode(_State, _Node->Next);
	return 1;
}

int LuaLnkLstNodePrev(lua_State* _State) {
	struct LnkLst_Node* _Node = LuaCheckClass(_State, 1, LOBJ_LINKEDLISTNODE);

	LuaCreateLnkLstNode(_State, _Node->Prev);
	return 1;
}

int LuaLnkLstNodeItr(lua_State* _State) {
	struct LnkLst_Node* _Node = LuaCheckClass(_State, 1, LOBJ_LINKEDLISTNODE);
	int  _Class = LUA_REFNIL;

	_Class = LuaItrGetClass(_State, 1);
	/*lua_pushstring(_State, "__classtype");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TNUMBER) {
		luaL_error(_State, "__classtype is not defined.");
		lua_pushnil(_State);
		return 1;
	}
	_Class = lua_tointeger(_State, -1);
	*/
	LuaCtor(_State, _Node->Data, _Class);
	return 1;
}

int LuaArrayItr(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, LOBJ_ITERATOR));
	return 1;
}

void* LuaToObject(lua_State* _State, int _Index, int _Class) {
	void* _Obj = NULL;

	if((_Obj = LuaTestClass(_State, _Index, _Class)) == NULL)
		return LuaCheckClass(_State, _Index, _Class);
	return _Obj;
}

int LuaConstraint(lua_State* _State) {
	lua_pushlightuserdata(_State, CreateConstrntLst(NULL, luaL_checkint(_State, 1),  luaL_checkint(_State, 2),  luaL_checkint(_State, 3)));
	return 1;
}

int LuaObjectIsEqual(lua_State* _State) {
	void* _Obj1 = LuaToClass(_State, 1);
	void* _Obj2 = LuaToClass(_State, 2);

	lua_pushboolean(_State, _Obj1 == _Obj2);
	return 1;
}

int LuaObjectGetClassName(lua_State* _State) {
	LuaObjectClass(_State, 1);
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

void LuaAddEnum(lua_State* _State, int _Table, const struct LuaEnum* _Enum) {
	_Table = lua_absindex(_State, _Table);
	for(int i = 0; _Enum[i].Key != NULL; ++i) {
		lua_pushstring(_State, _Enum[i].Key);
		lua_pushinteger(_State, _Enum[i].Value);
		lua_rawset(_State, _Table);
	}
}

void RegisterLuaEnums(lua_State* _State, const struct LuaEnumReg* _Reg) {
	for(int i = 0; _Reg[i].Name != NULL; ++i) {
		if(_Reg[i].SubTable == NULL) {
			lua_newtable(_State);
			LuaAddEnum(_State, -1, _Reg[i].Enum);
			lua_setglobal(_State, _Reg[i].Name);
		} else {
			lua_getglobal(_State, _Reg[i].Name);
			if(lua_type(_State, -1) != LUA_TTABLE) {
				lua_pop(_State, 1);
				lua_createtable(_State, 3, 0);
				lua_pushvalue(_State, -1);
				lua_setglobal(_State, _Reg[i].Name);
				//Log(ELOG_ERROR, "Cannot load lua enumeration %s", _Reg[i].Name);
				//continue;
			}
			lua_createtable(_State, 3, 0);
			LuaAddEnum(_State, -1, _Reg[i].Enum);
			lua_pushstring(_State, _Reg[i].SubTable);
			lua_insert(_State, -2);
			lua_rawset(_State, -3);
		}
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
		lua_pushlightuserdata(_State, NULL/*CreateEventTime(NULL, luaL_checkinteger(_State, 2))*/);
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

int LuaNull(lua_State* _State) {
	if(lua_type(_State, 1) != LUA_TTABLE) {
		lua_pushboolean(_State, 1);
		return 1;
	}
	lua_pushstring(_State, "__self");
	lua_rawget(_State, 1);
	if(lua_touserdata(_State, -1) == NULL)
		lua_pushboolean(_State, true);
	else
		lua_pushboolean(_State, false);
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
	//lua_pushcfunction(_State, LuaCallFuncError);
	if((_Error = lua_pcall(_State, 0, LUA_MULTRET, 0)) != 0)
		goto error;
	const char* _Test = lua_getupvalue(_State, -1, 1);
	if(_Test != NULL)
		lua_pop(_State, 1);
	//lua_pop(_State, 1);
	return LUA_OK;

	error:
	switch(_Error) {
		case LUA_ERRFILE:
			Log(ELOG_ERROR, "Cannot load file: %s", _File);
			lua_settop(_State, 0);
			return _Error;
		case LUA_ERRRUN:
			Log(ELOG_ERROR, "Cannot run file: %s", lua_tostring(_State, -1));
			lua_settop(_State, 0);
			return _Error;
		default:
			Log(ELOG_ERROR, "%s", lua_tostring(_State, -1));
			lua_settop(_State, 0);
			return _Error;
	}
	return LUA_ERRERR;
}

int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc) {
	//TODO: If in debug mode the stack should be checked to ensure its balanced.
	int _Error = 0;

	lua_pushcfunction(_State, LuaCallFuncError);
	lua_insert(_State, 1);
	_Error = lua_pcall(_State, _Args, _Results, 1);
	lua_remove(_State, 1);
	if(_Error != 0)
		goto error;
	return 1;

	error:
	SDL_assert(0 == 1 || lua_tostring(_State, -1));
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
		if((_CallRet = _Callback(_State, -1)) != NULL) {
			_Insert(_Return, _CallRet);
		} else {
			Log(ELOG_WARNING, "Failed to load data from file %s", _File);
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	return 1;
}

int LuaGetInteger(lua_State* _State, int _Index, int* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tointeger(_State, _Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not an integer", _Index);
	return 0;
}

int LuaGetString(lua_State* _State, int _Index, const char** _String) {
	if(lua_isstring(_State, _Index)) {
		*_String = lua_tostring(_State, _Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not a string", _Index);
	return 0;
}

int LuaGetNumber(lua_State* _State, int _Index, double* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tonumber(_State, _Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not is not a number", _Index);
	return 0;
}

int LuaGetUData(lua_State* _State, int _Index, void** _Data) {
	if(lua_islightuserdata(_State, _Index)) {
		*_Data = lua_touserdata(_State, _Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not user data", _Index);
	return 0;
}

int LuaGetFunction(lua_State* _State, int _Index, lua_CFunction* _Function) {
	if(lua_iscfunction(_State, _Index)) {
		*_Function = lua_tocfunction(_State, _Index);
		return 1;
	}
	//Log(ELOG_ERROR, "metafield is not a c function.");
	return 0;
}

int LuaRawString(lua_State* _State, int _Index, const char* _Field, const char** _Str) {
	lua_pushstring(_State, _Field);
	lua_rawget(_State, _Index);
	if(lua_type(_State, -1) != LUA_TSTRING) {	
		lua_pop(_State, 1);
		return 0;
	}
	*_Str = lua_tostring(_State, -1);
	lua_pop(_State, 1);
	return 1;
}

void LuaPrintTable(lua_State* _State, int _Index) {
	struct Primitive _Key;
	struct Primitive _Value;

	_Index = lua_absindex(_State, _Index);
	lua_pushnil(_State);
	Log(ELOG_INFO, "Table:");
	while(lua_next(_State, _Index) != 0) {
		LuaToPrimitive(_State, -2, &_Key);
		LuaToPrimitive(_State, -1, &_Value);
		PrimitivePrint(&_Key);
		PrimitivePrint(&_Value);
		lua_pop(_State, 1);
	}
}

void LuaStackToTable(lua_State* _State, int* _Table) {
	int _Top = lua_gettop(_State);

	for(int i = 0; i < _Top; ++i)
		_Table[i] = lua_type(_State, i);
}

void LuaCopyTable(lua_State* _State, int _Index) {
	_Index = lua_absindex(_State, _Index);

	if(lua_type(_State, _Index) != LUA_TTABLE)
		return;
	if(lua_type(_State, -1) != LUA_TTABLE)
		return;
	lua_pushnil(_State);
	while(lua_next(_State, _Index) != 0) {
		lua_pushvalue(_State, -2);
		lua_pushvalue(_State, -2);
		lua_rawset(_State, -5);
		lua_pop(_State, 1);
	}
	lua_remove(_State, _Index);
}

void* LuaToClass(lua_State* _State, int _Index) {
	void* _Pointer = NULL;
	int _Pos = lua_absindex(_State, _Index);

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

void* LuaTestClass(lua_State* _State, int _Index, int _Class) {
	if(lua_getmetatable(_State, _Index) == 0)
		 luaL_error(_State, LUA_TYPERROR(_State, _Index, _Class, "LuaTestClass"));
	lua_pushstring(_State, "__class");
	lua_rawget(_State, -2);
	if(lua_isnumber(_State, -1) == 1 || _Class == lua_tointeger(_State, -1)) {
		lua_pop(_State, 2);
		return NULL;
	}
	lua_pop(_State, 2);
	return LuaToClass(_State, _Index);
}

void* LuaCheckClass(lua_State* _State, int _Index, int  _Class) {
	int _Pop = 4;

	_Index = lua_absindex(_State, _Index);
	lua_rawgeti(_State, LUA_REGISTRYINDEX, g_LuaClassTable[_Class]);
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
		lua_rawgeti(_State, LUA_REGISTRYINDEX, g_LuaClassTable[_Class]);
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

const char* LuaBaseClass(lua_State* _State, int _Index) {
	const char* _BaseClass = NULL;

	if(lua_getmetatable(_State, _Index) == 0)
		 luaL_error(_State, LUA_TYPERROR(_State, _Index, "Table", "LuaTestClass"));
	while(1) {
		lua_pushstring(_State, "__class");
		lua_rawget(_State, -2);
		if(lua_isstring(_State, -1) == 0) {
			lua_pop(_State, 2);
			return NULL;
		}
		_BaseClass = lua_tostring(_State, -1);
		lua_pop(_State, 1);
		lua_pushstring(_State, "__baseclass");
		lua_rawget(_State, -2);
		if(lua_type(_State, -1) != LUA_TTABLE) {
			lua_pop(_State, 1);
			break;
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	return _BaseClass;
}

int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two) {
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(LuaGetInteger(_State, -1, _One) == 0)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(LuaGetInteger(_State, -1, _Two) == 0)
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
	if(LuaGetString(_State, -1, _Value) == -1)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(LuaGetInteger(_State, -1, _Pair) == -1)
		goto fail;
	lua_pop(_State, 2);
	return 1;
	fail:
	lua_settop(_State, _Top);
	return 0;
}

int LuaRuleLuaCall(lua_State* _State) {
	int _Args = lua_gettop(_State);
	struct RuleLuaCall* _Rule = NULL;

	SDL_assert(_Args > 0);
	luaL_checktype(_State, 1, LUA_TFUNCTION);
	lua_createtable(_State, _Args, 0);
	for(int i = 0; i < _Args; ++i) {
		lua_pushvalue(_State, i + 1);
		lua_rawseti(_State, -2, i + 1);
	}
	SDL_assert(lua_type(_State, -1) == LUA_TTABLE);
	_Rule = CreateRuleLuaCall(_State, luaL_ref(_State, LUA_REGISTRYINDEX));
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleGreaterThan(lua_State* _State) {
	struct RuleComparator* _Rule = NULL;
	struct Rule* _Left = NULL;
	struct Rule* _Right = NULL;

	_Left = LuaValueToRule(_State, 1);
	_Right = LuaValueToRule(_State, 2);
	_Rule = CreateRuleComparator(RULE_GREATERTHAN, _Left, _Right);
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleLessThan(lua_State* _State) {
	struct RuleComparator* _Rule = NULL;
	struct Rule* _Left = NULL;
	struct Rule* _Right = NULL;

	_Left = LuaValueToRule(_State, 1);
	_Right = LuaValueToRule(_State, 2);
	_Rule = CreateRuleComparator(RULE_LESSTHAN, _Left, _Right);
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleTrue(lua_State* _State) {
	struct Rule* _Rule = (struct Rule*) CreateRuleBoolean(1);
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleFalse(lua_State* _State) {
	struct Rule* _Rule = (struct Rule*) CreateRuleBoolean(0);
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleIfThenElse(lua_State* _State) {
	struct RuleComparator* _Comparator = LuaCheckClass(_State, 1, LOBJ_RULE);
	struct Rule* _OnTrue = LuaCheckClass(_State, 2, LOBJ_RULE);
	struct Rule* _OnFalse = LuaCheckClass(_State, 3, LOBJ_RULE);
	struct Rule* _Rule = NULL;

	switch(_Comparator->Type) {
	case RULE_BOOLEAN:
	case RULE_GREATERTHAN:
	case RULE_LESSTHAN:
		break;
	default:
		return luaL_error(_State, "First argument must be a rule comparator.");
	}
	_Rule = (struct Rule*) CreateRuleIfThenElse(_Comparator, _OnTrue, _OnFalse);
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleEventFired(lua_State* _State) {
	int i = 0;
	struct Rule* _Rule = NULL;
	const char* _String = luaL_checkstring(_State, 1);

	for(i = 0; g_EventNames[i] != NULL; ++i) {
		if(strcmp(g_EventNames[i], _String) == 0) {
			_Rule = (struct Rule*) CreateRuleEvent(i);
			LuaCtor(_State, _Rule, LOBJ_RULE);
			return 1;
		}
	}
	luaL_error(_State, "%s is not an event name.", _String);
	return 0;
}

int LuaRuleBlock(lua_State* _State) {
	struct RuleBlock* _Rule =  NULL;
	int _Args = lua_gettop(_State);

	if(_Args == 0)
		return luaL_error(_State, "LuaRuleBlock must have at least one argument.");
	for(int i = 0; i < _Args; ++i) {
		if(LuaCheckClass(_State, i + 1, LOBJ_RULE) == NULL)
			return luaL_error(_State, "LuaRuleBlock's %i argument is not a Rule.", i + 1);
	}
	_Rule = CreateRuleBlock(_Args);
	for(int i = 0; i < _Args; ++i)
		_Rule->RuleList[i] = LuaCheckClass(_State, i + 1, LOBJ_RULE);
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleCond(lua_State* _State) {
	struct RuleCond* _Rule = NULL;
	int _Args = lua_gettop(_State);

	if((_Args & 1) == 1) {
		return luaL_error(_State, "Cond requires an even amount of arguments.");
	}
	_Rule = CreateRuleCond(_Args / 2);
	for(int i = 1, _Ct = 0; i <= _Args; i += 2, ++_Ct) {
		_Rule->Conditions[_Ct] = LuaCheckClass(_State, i, LOBJ_RULE);
		_Rule->Actions[_Ct] = LuaCheckClass(_State, i + 1, LOBJ_RULE); 
	}
	LuaCtor(_State, _Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleNegate(lua_State* _State) {
	struct RuleLuaCall* _Rule = LuaCheckClass(_State, 1, LOBJ_RULE);

	LuaCtor(_State, ((struct Rule*) CreateRuleDecorator(RULE_NEGATE, _Rule)), LOBJ_RULE); 
	return 1;
}

int LuaMissionGetName(lua_State* _State) {
	struct Mission* _Mission = LuaCheckClass(_State, 1, LOBJ_MISSION);

	lua_pushstring(_State, _Mission->Name);
	return 1;
}

int LuaMissionGetDesc(lua_State* _State) {
	struct Mission* _Mission = LuaCheckClass(_State, 1, LOBJ_MISSION);

	lua_pushstring(_State, _Mission->Description);
	return 1;
}

int LuaMissionGetOptions(lua_State* _State) {
	struct Mission* _Mission = LuaCheckClass(_State, 1, LOBJ_MISSION);

	lua_createtable(_State, 0, _Mission->OptionCt);
	for(int i = 0; i < _Mission->OptionCt; ++i) {
		LuaCtor(_State, &_Mission->Options[i], LOBJ_MISSIONOPTION);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
}

int LuaMissionChooseOption(lua_State* _State) {
	struct Mission* _Mission = LuaCheckClass(_State, 1, LOBJ_MISSION);
	struct MissionFrame* _Data = LuaCheckClass(_State, 2, LOBJ_MISSIONFRAME);
	int _Option = luaL_checkinteger(_State, 3);

	MissionCheckOption(_State, _Mission, _Data, _Option);
	return 0;
}

struct Rule* LuaValueToRule(lua_State* _State, int _Index) {
	struct RulePrimitive* _Rule = NULL;

	if(lua_type(_State, _Index) == LUA_TTABLE) {
		if((_Rule = LuaToObject(_State, _Index, LOBJ_RULE)) == NULL) {
			luaL_error(_State, LUA_TYPERROR(_State, _Index, LOBJ_RULE, "LuaRuleGreaterThan"));
			return NULL;
		}
	} else {
		struct Primitive* _Prim = CreatePrimitive();
		LuaToPrimitive(_State, _Index, _Prim);
		_Rule = CreateRulePrimitive(_Prim);
	}
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

void LuaToPrimitive(lua_State* _State, int _Index, struct Primitive* _Primitive) {
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
		case LUA_TTABLE:
			_Primitive->Type = PRIM_STRING;
			_Primitive->Value.String = calloc(sizeof("Table"), sizeof(char));
			strcpy(_Primitive->Value.String, "Table");
			break;
		case LUA_TFUNCTION:
			_Primitive->Type = PRIM_STRING;
			_Primitive->Value.String = calloc(sizeof("Function"), sizeof(char));
			strcpy(_Primitive->Value.String, "Function");
			break;
	}
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

int LuaClassIndex(lua_State* _State) {
	lua_getmetatable(_State, 1);
	while(1) {
		lua_pushvalue(_State, 2);
		//Search metatable for the key.
		lua_rawget(_State, 3);
		if(lua_type(_State, -1) != LUA_TNIL)
			break;
		//Key not found check base class.
		lua_pop(_State, 1);
		lua_pushstring(_State, "__baseclass");
		lua_rawget(_State, 3);
		//No base class check LUA_BASECLASS metatable.
		if(lua_type(_State, -1) == LUA_TNIL) {
			lua_rawgeti(_State, LUA_REGISTRYINDEX, g_LuaClassTable[LUA_BASECLASS]);
			lua_pushvalue(_State, 2);
			lua_rawget(_State, -2);
			if(lua_type(_State, -1) == LUA_TNIL) {
				lua_pushnil(_State);
				return 1;
			}
			break;
		}
		lua_replace(_State, 3);
	}
	return 1;
}

int LuaClassError(lua_State* _State, int _Arg, int  _Class) {
	const char* _ArgType = NULL;

	_Arg = lua_absindex(_State, _Arg);
	_ArgType = LuaObjectClass(_State, _Arg);
	if(_ArgType == NULL)
		_ArgType = lua_typename(_State, lua_type(_State, _Arg));
	else {
		lua_pushstring(_State, "__self");
		lua_rawget(_State, _Arg);	
		if(lua_touserdata(_State, -1) == NULL)
			return luaL_error(_State, "Error argument #%d is of type \"%d\" but is NULL.", _Arg, _Class);
	}
	return luaL_error(_State, "Error argument #%d is of type \"%s\" but expected \"%d\".", _Arg, _ArgType, _Class); 
}

int LuaMathRandomVar(lua_State* _State) {
	double _Var = luaL_checknumber(_State, 1);

	lua_pushnumber(_State, _Var * NormalDistribution(_Var, .5, 0) + _Var);
	return 1;
}

int LuaMathProbability(lua_State* _State) {
	double _Prob = luaL_checknumber(_State, 1);
	int _Max = luaL_checkinteger(_State, 2);

	lua_pushnumber(_State, _Prob * _Max);
	return 1;
}

int LuaInputMousePos(lua_State* _State) {
	SDL_Point _Pos;

	GetMousePos(&_Pos);
	lua_pushinteger(_State, _Pos.x);
	lua_pushinteger(_State, _Pos.y);
	return 2;
}
