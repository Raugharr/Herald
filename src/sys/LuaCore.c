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
	{"eq", LuaObjectIsEqual},
	{"Equal", LuaObjectIsEqual},
	{"Self", LuaObjectSelf},
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

/*static const luaL_Reg g_LuaFuncsMath[] = {
	{"RandomVar", LuaMathRandomVar},
	{"Probability", LuaMathProbability},
	{NULL, NULL}
};*/

static const luaL_Reg g_LuaFuncsDate[] = {
	{"Years", LuaDateYears},
	{"Months", LuaDateMonths},
	{"Days", LuaDateDays},
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
	{LOBJ_DATE, "Date", LUA_REFNIL, g_LuaFuncsDate},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

int LuaCallFuncError(lua_State* State) {
	lua_Debug Debug;

	if(lua_isstring(State, -1) == 0)
		return 0;
	if(lua_getstack(State, 0, &Debug) == 0)
		return 1;
	lua_getinfo(State, "S", &Debug);
	lua_getglobal(State, "debug");
	lua_getfield(State, -1, "traceback");
	lua_call(State, 0, 1);
	lua_pushvalue(State, -3);
	lua_concat(State, 2);
	return 1;
}

static inline int LuaItrGetClass(lua_State* State, int Idx) {
	lua_pushstring(State, "__classtype");
	lua_rawget(State, Idx);
	if(lua_type(State, -1) != LUA_TNUMBER) {
		return luaL_error(State, "classtype is not an integer.");
	}
	return lua_tointeger(State, -1);
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
	RegisterLuaEnums(g_LuaState, g_LuaGovernmentEnums);
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
void RegisterLuaObjects(lua_State* State, const struct LuaObjectReg* Objects) {
	int i = 0;

	while(Objects[i].Class != LUA_REFNIL) {
		if(LuaRegisterObject(State, Objects[i].Name, Objects[i].Class, Objects[i].BaseClass, Objects[i].Funcs) == 0)
			return (void) luaL_error(State, "Loading Lua functions has failed loading class %s.", Objects[i].Class);
		++i;
	}
}

int LuaRegisterObject(lua_State* State, const char* Name, int Class, int BaseClass, const luaL_Reg* Funcs) {
	int Ref = 0;
	
	if(g_LuaClassTable[Class] != LUA_REFNIL)
		return luaL_error(State, "Loading Lua class #%d failed. (Class id is already defined.)", Class);
	if(Class < 0 || Class > 0xFF)
		return luaL_error(State, "Loading Lua class #%d failed. (Class id is invalid.)", Class);
	if((BaseClass < 0 && BaseClass != LUA_REFNIL) || BaseClass > 0xFF)
		return luaL_error(State, "Loading Lua class #%d failed. (Base Class id #%d is invalid.)", Class, BaseClass);
	lua_newtable(State);
	lua_pushvalue(State, -1);
	Ref = luaL_ref(State, LUA_REGISTRYINDEX);
	if(Ref == LUA_REFNIL) {
		lua_pop(State, 1);
		return LUA_REFNIL;
	}
	lua_pushliteral(State, "__index");
	lua_pushcfunction(State, LuaClassIndex);
	lua_rawset(State, -3);
	lua_pushinteger(State, LUA_OCLASS);
	lua_pushinteger(State, Ref);
	lua_rawset(State, -3);

	lua_pushinteger(State, LUA_OCLASS);
	lua_pushinteger(State, Class);
	lua_rawset(State, -3);
	//Give them the base class Object. This doesn't need to be done for a class with a base class as their base class with be an object.
	if(BaseClass != LUA_REFNIL) {
		lua_pushliteral(State, "__baseclass");
		lua_rawgeti(State, LUA_REGISTRYINDEX, g_LuaClassTable[BaseClass]);
		if(lua_type(State, -1) != LUA_TTABLE) {
			luaL_error(State, "Loading Lua class #%d failed. (Base class #%d is not defined.)", Class, BaseClass);
			lua_pop(State, 3);
			return LUA_REFNIL;
		}
		lua_rawset(State, -3);
	}
	lua_pushliteral(State, "__newindex");
	lua_pushnil(State);
	lua_rawset(State, -3);
	if(Funcs != NULL)
		luaL_setfuncs(State, Funcs, 0);
	//if(luaL_newmetatable(State, Name) == 0)
	//	return luaL_error(State, "Loading Lua class #%d failed. (Name %s is already taken.)", Name);
	lua_rawgeti(State, LUA_REGISTRYINDEX, Ref);
	lua_setfield(State, LUA_REGISTRYINDEX, Name);
	//lua_setmetatable(State, -2);
	//lua_pop(State, 1);
	g_LuaClassTable[Class] = Ref;
	lua_pop(State, 1);
	return 1;
}

void LuaRegisterFunctions(lua_State* State, const luaL_Reg* Funcs) {
	for(int i = 0; (Funcs[i].name != NULL && Funcs[i].func != NULL); ++i)
		lua_register(State, Funcs[i].name, Funcs[i].func);
}

void CreateLuaLnkLstItr(lua_State* State, struct LinkedList* List, int Class) {
	LuaCtor(State, (void*)List, LOBJ_LINKEDLIST);
	lua_pushstring(State, "__classtype");
	lua_pushinteger(State, Class);
	lua_rawset(State, -3);
}

//QUESTION: Should this be inlined or made a macro.
void CreateLuaArrayItr(lua_State* State, struct Array* Array, int Class) {
	LuaCtor(State, Array, LOBJ_ARRAYITERATOR);
	lua_pushstring(State, "__classtype");
	lua_pushinteger(State, Class);
	lua_rawset(State, -3);

	lua_pushstring(State, "__ipairs");
	lua_pushcfunction(State, LuaArrayItrNext);
	lua_rawset(State, -3);
}

void LuaArrayClassToTable(lua_State* State, const void** Table, int TableSz, int Class) {
	lua_createtable(State, TableSz, 0);
	for(int i = 0; i < TableSz; ++i) {
		LuaCtor(State, (void*)Table[i], Class);
		lua_rawseti(State, -2, i + 1);
	}
}

void LuaInitClass(lua_State* State, void* Ptr, int Class) {
	if(Class < 0 || Class > 0xFF) {
		Log(ELOG_ERROR, "LuaInitClass: Class argument got %i but requires [0:255].", Class);
		return;
	}
	Assert(g_LuaClassTable[Class] != LUA_REFNIL);
	lua_rawgeti(State, LUA_REGISTRYINDEX, g_LuaClassTable[Class]);
	if(lua_type(State, -1) == LUA_TTABLE) {
		lua_setmetatable(State, -2);
		lua_pushinteger(State, LUA_OSELF);
		lua_pushlightuserdata(State, Ptr);
		lua_rawset(State, -3);
		return;
	}
	lua_pop(State, 2);
	Log(ELOG_WARNING, "Lua class %i does not exist", Class);
	lua_pushnil(State);
}

const char* LuaObjectClass(lua_State* State, int Arg) {
	const char* Name = NULL;

	if(lua_type(State, Arg) != LUA_TTABLE)
		return NULL;
	if(lua_getmetatable(State, Arg) == 0)
		return NULL;
	lua_pushinteger(State, LUA_OCLASS);
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TNUMBER) {
		lua_pop(State, 2);
		return NULL;
	}
	Name = lua_tostring(State, -1);
	lua_pop(State, 2);
	return Name;	
}

void LuaCtorArray(lua_State* State, struct Array* Array, int Class) {
	LuaCtor(State, Array, LOBJ_ARRAY);
	lua_pushstring(State, "__classtype");
	lua_pushinteger(State, Class);
	lua_rawset(State, -3);
}

int LuaArrayCreate(lua_State* State) {
	int Size = luaL_checkinteger(State, 1);
	struct Array* Array = lua_newuserdata(State, sizeof(struct Array));

	CtorArray(Array, Size);
	LuaCtor(State, Array, LOBJ_ARRAY);
	return 1;
}

int LuaArrayCreateItr(lua_State* State) {
	struct Array* Array = LuaCheckClass(State, 1, LOBJ_ARRAY);
	int Class = LuaItrGetClass(State, 1);

	CreateLuaArrayItr(State, Array, Class);
	return 1;
}

int LuaArrayGetSize(lua_State* State) {
	struct Array* Array = LuaCheckClass(State, 1, LOBJ_ARRAY);

	lua_pushinteger(State, Array->Size);
	return 1;
}

int LuaArrayDestroy(lua_State* State) {
	struct Array* Array = LuaCheckClass(State, 1, LOBJ_ARRAY);
		
	DestroyArray(Array);
	return 0;
}

int LuaArrayItr_Aux(lua_State* State) {
	struct Array* Array = LuaCheckClass(State, lua_upvalueindex(1), LOBJ_ARRAYITERATOR);
	int Index = lua_tointeger(State, lua_upvalueindex(2));
	int Change = lua_tointeger(State, lua_upvalueindex(3));
	int Class = LUA_REFNIL;

	for(int i = Index; i < Array->Size; i += Change) {
		if(Array->Table[Index] != NULL)
			break;
		Index += Change;
	}

	if(Index < 0 || Index >= Array->Size) {
		lua_pushnil(State);
		lua_pushvalue(State, 1);
		lua_pushnil(State);
		return 3;
	}
	Class = LuaItrGetClass(State, lua_upvalueindex(1));
	/*lua_pushvalue(State, lua_upvalueindex(1));
	lua_pushstring(State, "__classtype");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TNUMBER)
		luaL_error(State, "Iterator does not have a classtype.");

	Class = lua_tointeger(State, -1);
	*/
	LuaCtor(State, Array->Table[Index], Class);
	lua_pushinteger(State, Index + Change);
	lua_replace(State, lua_upvalueindex(2));
	return 1;
}

int LuaArrayItrNext(lua_State* State) {
	LuaTestClass(State, 1, LOBJ_ARRAYITERATOR);
	lua_pushinteger(State, 0);
	lua_pushinteger(State, 1);
	lua_pushcclosure(State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaArrayItrPrev(lua_State* State) {
	LuaTestClass(State, 1, LOBJ_ARRAYITERATOR);
	lua_pushinteger(State, 0);
	lua_pushinteger(State, -1);
	lua_pushcclosure(State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaLnkLstNodeIterate(lua_State* State) {
	struct LnkLst_Node* Itr = LuaCheckClass(State, lua_upvalueindex(1), LOBJ_LINKEDLISTNODE);
	int Foward = lua_tointeger(State, lua_upvalueindex(2));
	int ItrClass = LUA_REFNIL;

	if(Itr == NULL)
		return 0;
	ItrClass = LuaItrGetClass(State, lua_upvalueindex(1));
	/*lua_pushstring(State, "__classtype");
	lua_rawget(State, lua_upvalueindex(1));
	if(lua_isnumber(State, -1) == 0) {
		lua_pushnil(State);
		return 1;
	}
	ItrClass = lua_tointeger(State, -1);
	*/
	lua_pop(State, 1);
	LuaCtor(State, Itr->Data, ItrClass);
	if(Foward != 0)
		Itr = Itr->Next;
	else
		Itr = Itr->Prev;
	lua_pushvalue(State, lua_upvalueindex(1));
	lua_pushinteger(State, LUA_OSELF);
	lua_pushlightuserdata(State, Itr);
	lua_rawset(State, -3);
	lua_pop(State, 1);
	return 1;
}

static inline void LuaCreateLnkLstNode(lua_State* State, struct LnkLst_Node* Node) {
	LuaCtor(State, Node, LOBJ_LINKEDLISTNODE);
	lua_pushstring(State, "__classtype");
	lua_pushstring(State, "__classtype");
	lua_rawget(State, 1);
	lua_rawset(State, -3);
}

int LuaLnkLstFront(lua_State* State) {
	struct LinkedList* List = LuaCheckClass(State, 1, LOBJ_LINKEDLIST);

	LuaCreateLnkLstNode(State, List->Front);
	lua_pushinteger(State, 1);
	lua_pushcclosure(State, LuaLnkLstNodeIterate, 2);
	return 1;
}

int LuaLnkLstBack(lua_State* State) {
	struct LinkedList* List = LuaCheckClass(State, 1, LOBJ_LINKEDLIST);

	LuaCreateLnkLstNode(State, List->Back);
	return 1;
}

int LuaLnkLstSize(lua_State* State) {
	struct LinkedList* List = LuaCheckClass(State, 1, LOBJ_LINKEDLIST);

	lua_pushinteger(State, List->Size);
	return 1;
}

int LuaLnkLstNodeNext(lua_State* State) {
	struct LnkLst_Node* Node = LuaCheckClass(State, 1, LOBJ_LINKEDLISTNODE);

	LuaCreateLnkLstNode(State, Node->Next);
	return 1;
}

int LuaLnkLstNodePrev(lua_State* State) {
	struct LnkLst_Node* Node = LuaCheckClass(State, 1, LOBJ_LINKEDLISTNODE);

	LuaCreateLnkLstNode(State, Node->Prev);
	return 1;
}

int LuaLnkLstNodeItr(lua_State* State) {
	struct LnkLst_Node* Node = LuaCheckClass(State, 1, LOBJ_LINKEDLISTNODE);
	int  Class = LUA_REFNIL;

	Class = LuaItrGetClass(State, 1);
	/*lua_pushstring(State, "__classtype");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TNUMBER) {
		luaL_error(State, "classtype is not defined.");
		lua_pushnil(State);
		return 1;
	}
	Class = lua_tointeger(State, -1);
	*/
	LuaCtor(State, Node->Data, Class);
	return 1;
}

int LuaArrayItr(lua_State* State) {
	lua_pushlightuserdata(State, LuaCheckClass(State, 1, LOBJ_ITERATOR));
	return 1;
}

void* LuaToObject(lua_State* State, int Index, int Class) {
	void* Obj = NULL;

	if((Obj = LuaTestClass(State, Index, Class)) == NULL)
		return LuaCheckClass(State, Index, Class);
	return Obj;
}

int LuaConstraint(lua_State* State) {
	lua_pushlightuserdata(State, CreateConstrntLst(NULL, luaL_checkint(State, 1),  luaL_checkint(State, 2),  luaL_checkint(State, 3)));
	return 1;
}

int LuaObjectIsEqual(lua_State* State) {
	void* Obj1 = LuaToClass(State, 1);
	void* Obj2 = LuaToClass(State, 2);

	lua_pushboolean(State, Obj1 == Obj2);
	return 1;
}

int LuaObjectSelf(lua_State* State) {
	if(lua_gettop(State) < 1) {
		lua_pushnil(State);
		return 1;
	}
	lua_rawgeti(State, 1, LUA_OSELF); 
	return 1;
}

int LuaObjectGetClassName(lua_State* State) {
	lua_pushstring(State, LuaObjectClass(State, 1));
	return 1;
}

int LuaConstraintBnds(lua_State* State) {
	luaL_checktype(State, -1, LUA_TTABLE);

	int Size = lua_rawlen(State, -1);
	int CurrMin = -1;
	int CurrMax = -1;
	int i = 0;
	struct Constraint** Constrnt = (struct Constraint**) malloc(sizeof(struct Constraint) * (Size));

	lua_pushnil(State);
	if(lua_next(State, -2) != 0) {
		CurrMin = luaL_checkint(State, -1);
		lua_pop(State, 1);
		if(lua_next(State, -2) != 0) {
			CurrMax = luaL_checkint(State, -1);
			Constrnt[i++] = CreateConstraint(CurrMin, CurrMax);
		} else
			goto error;
		lua_pop(State, 1);
	} else
		goto error;
	while(lua_next(State, -2) != 0) {
		CurrMin = CurrMax + 1;
		CurrMax = luaL_checkint(State, -1);
		Constrnt[i++] = CreateConstraint(CurrMin, CurrMax);
		lua_pop(State, 1);
	}
	Constrnt[Size - 1] = NULL;
	lua_pushlightuserdata(State, Constrnt);
	return 1;
	error:
	luaL_error(State, "Cannot create constraint.");
	free(Constrnt);
	return 0;
}

void ConstraintBndToLua(lua_State* State, struct Constraint** Constraints) {
	int i = 0;

	lua_newtable(State);
	while(*Constraints != NULL) {
		ConstraintToLua(State, *Constraints);
		lua_rawseti(State, -2, i++);
		Constraints += sizeof(struct Constraint*);
	}
}

void LuaAddEnum(lua_State* State, int Table, const struct LuaEnum* Enum) {
	Table = lua_absindex(State, Table);
	for(int i = 0; Enum[i].Key != NULL; ++i) {
		lua_pushstring(State, Enum[i].Key);
		lua_pushinteger(State, Enum[i].Value);
		lua_rawset(State, Table);
	}
}

void RegisterLuaEnums(lua_State* State, const struct LuaEnumReg* Reg) {
	for(int i = 0; Reg[i].Name != NULL; ++i) {
		if(Reg[i].SubTable == NULL) {
			lua_newtable(State);
			LuaAddEnum(State, -1, Reg[i].Enum);
			lua_setglobal(State, Reg[i].Name);
		} else {
			lua_getglobal(State, Reg[i].Name);
			if(lua_type(State, -1) != LUA_TTABLE) {
				lua_pop(State, 1);
				lua_createtable(State, 3, 0);
				lua_pushvalue(State, -1);
				lua_setglobal(State, Reg[i].Name);
				//Log(ELOG_ERROR, "Cannot load lua enumeration %s", Reg[i].Name);
				//continue;
			}
			lua_createtable(State, 3, 0);
			LuaAddEnum(State, -1, Reg[i].Enum);
			lua_pushstring(State, Reg[i].SubTable);
			lua_insert(State, -2);
			lua_rawset(State, -3);
		}
	}
}

int LuaYears(lua_State* State) {
	lua_pushinteger(State, luaL_checkinteger(State, 1) * YEAR_DAYS);
	return 1;
}

int LuaMonth(lua_State* State) {
	const char* Type = NULL;

	Type = luaL_checkstring(State, 1);
	luaL_checkinteger(State, 2);
	if(!strcmp(Type, "Years"))
		lua_pushinteger(State, TO_YEARS(lua_tointeger(State, 2)));
	else if(!strcmp(Type, "Days"))
		lua_pushinteger(State, TO_DAYS(lua_tointeger(State, 2)));
	else {
		return luaL_argerror(State, 2, "Must be either \"Years\" or \"Days\".");

	}
	return 1;
}

int LuaPrintDate(lua_State* State) {
	DATE Date = *(DATE*)LuaCheckClass(State, 1, LOBJ_DATE);
	DATE Days = DAY(Date);
	DATE Months = MONTH(Date);
	DATE Years = YEAR(Date);

	if(Months > 11)
		Months = 11;
	lua_pushfstring(State, "%s %d, %d", g_ShortMonths[Months], Days, Years);
	return 1;
}

int LuaPrintYears(lua_State* State) {
	DATE Date = *(DATE*)LuaCheckClass(State, 1, LOBJ_DATE);
	DATE Years = YEAR(Date);

	lua_pushfstring(State, "%d Years", Years);
	return 1;
}

int LuaHook(lua_State* State) {
	const char* Name = NULL;

	Name = luaL_checkstring(State, 1);
	if(!strcmp(Name, "Age")) {
		lua_pushlightuserdata(State, NULL/*CreateEventTime(NULL, luaL_checkinteger(State, 2))*/);
	} else {
		luaL_error(State, "Must be a valid hook type.");
		return 0;
	}
	return 1;
}

int LuaRandom(lua_State* State) {
	Random(luaL_checkinteger(State, 1), luaL_checkinteger(State, 2));
	return 1;
}

int LuaNull(lua_State* State) {
	if(lua_type(State, 1) != LUA_TTABLE) {
		lua_pushboolean(State, 1);
		return 1;
	}
	lua_pushinteger(State, LUA_OSELF);
	lua_rawget(State, 1);
	if(lua_touserdata(State, -1) == NULL)
		lua_pushboolean(State, true);
	else
		lua_pushboolean(State, false);
	return 1;
}

int LuaLoadFile(lua_State* State, const char* File, const char* Environment) {
	int Error = luaL_loadfile(State, File);

	if(Error != 0)
		goto error;
	if(Environment != NULL) {
		LuaGetEnv(State, Environment);
		lua_setupvalue(State, -2, 1);
	}
	//lua_pushcfunction(State, LuaCallFuncError);
	if((Error = lua_pcall(State, 0, LUA_MULTRET, 0)) != 0)
		goto error;
	const char* Test = lua_getupvalue(State, -1, 1);
	if(Test != NULL)
		lua_pop(State, 1);
	//lua_pop(State, 1);
	return LUA_OK;

	error:
	switch(Error) {
		case LUA_ERRFILE:
			Log(ELOG_ERROR, "Cannot load file: %s", File);
			lua_settop(State, 0);
			return Error;
		case LUA_ERRRUN:
			Log(ELOG_ERROR, "Cannot run file: %s", lua_tostring(State, -1));
			lua_settop(State, 0);
			return Error;
		default:
			Log(ELOG_ERROR, "%s", lua_tostring(State, -1));
			lua_settop(State, 0);
			return Error;
	}
	return LUA_ERRERR;
}

int LuaCallFunc(lua_State* State, int Args, int Results, int ErrFunc) {
	//TODO: If in debug mode the stack should be checked to ensure its balanced.
	int Error = 0;

	if(Args + 1 > lua_gettop(State)) {
		StackTrace();
	}
	lua_pushcfunction(State, LuaCallFuncError);
	lua_insert(State, 1);
	Error = lua_pcall(State, Args, Results, 1);
	lua_remove(State, 1);
	if(Error != 0)
		goto error;
	return 1;

	error:
	SDL_assert(0 == 1 || lua_tostring(State, -1));
	Log(ELOG_ERROR, "%s", lua_tostring(State, -1));
	return 0;
}

int LuaLoadList(lua_State* State, const char* File, const char* Global, void*(*Callback)(lua_State*, int), void(*Insert)(struct LinkedList*, void*), struct LinkedList* Return) {
	void* CallRet = NULL;

	if(LuaLoadFile(State, File, NULL) != LUA_OK)
		return 0;
	lua_getglobal(State, Global);
	if(!lua_istable(State, -1))
		return 0;
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(!lua_istable(State, -1)) {
			Log(ELOG_WARNING, "Warning: index is not a table.");
			lua_pop(State, 1);
			continue;
		}
		if((CallRet = Callback(State, -1)) != NULL) {
			Insert(Return, CallRet);
		} else {
			Log(ELOG_WARNING, "Failed to load data from file %s", File);
		}
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
	return 1;
}

int LuaGetInteger(lua_State* State, int Index, int* Number) {
	if(lua_isnumber(State, Index)) {
		*Number = lua_tointeger(State, Index);
		return 1;
	}
	*Number = 0;
	//Log(ELOG_ERROR, "Lua index %i is not an integer", Index);
	return 0;
}

int LuaGetString(lua_State* State, int Index, const char** String) {
	if(lua_isstring(State, Index)) {
		*String = lua_tostring(State, Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not a string", Index);
	return 0;
}

int LuaGetNumber(lua_State* State, int Index, double* Number) {
	if(lua_isnumber(State, Index)) {
		*Number = lua_tonumber(State, Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not is not a number", Index);
	return 0;
}

int LuaGetUData(lua_State* State, int Index, void** Data) {
	if(lua_islightuserdata(State, Index)) {
		*Data = lua_touserdata(State, Index);
		return 1;
	}
	//Log(ELOG_ERROR, "Lua index %i is not user data", Index);
	return 0;
}

int LuaGetFunction(lua_State* State, int Index, lua_CFunction* Function) {
	if(lua_iscfunction(State, Index)) {
		*Function = lua_tocfunction(State, Index);
		return 1;
	}
	//Log(ELOG_ERROR, "metafield is not a c function.");
	return 0;
}

int LuaRawString(lua_State* State, int Index, const char* Field, const char** Str) {
	lua_pushstring(State, Field);
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TSTRING) {	
		lua_pop(State, 1);
		return 0;
	}
	*Str = lua_tostring(State, -1);
	lua_pop(State, 1);
	return 1;
}

void LuaPrintTable(lua_State* State, int Index) {
	struct Primitive Key;
	struct Primitive Value;

	Index = lua_absindex(State, Index);
	lua_pushnil(State);
	printf("Table:");
	while(lua_next(State, Index) != 0) {
		LuaToPrimitive(State, -2, &Key);
		LuaToPrimitive(State, -1, &Value);
		PrimitivePrint(&Key);
		PrimitivePrint(&Value);
		lua_pop(State, 2);
	}
}

void LuaStackToTable(lua_State* State, int* Table) {
	int Top = lua_gettop(State);

	for(int i = 0; i < Top; ++i)
		Table[i] = lua_type(State, i);
}

void LuaCopyTable(lua_State* State, int Index) {
	Index = lua_absindex(State, Index);

	if(lua_type(State, Index) != LUA_TTABLE)
		return;
	if(lua_type(State, -1) != LUA_TTABLE)
		return;
	lua_pushnil(State);
	while(lua_next(State, Index) != 0) {
		lua_pushvalue(State, -2);
		lua_pushvalue(State, -2);
		lua_rawset(State, -5);
		lua_pop(State, 1);
	}
	lua_remove(State, Index);
}

void* LuaToClass(lua_State* State, int Index) {
	void* Pointer = NULL;
	int Pos = lua_absindex(State, Index);

	if((Pointer = lua_touserdata(State, Pos)) == NULL) {
		if(lua_type(State, Pos) == LUA_TNIL)
			return NULL;
		if(lua_type(State, Pos) != LUA_TTABLE)
			luaL_error(State, "LuaToClass: index is not a class (expected table got %s).", lua_typename(State, lua_type(State, Pos)));
		lua_pushinteger(State, LUA_OSELF);
		lua_rawget(State, Pos);
		Pointer = lua_touserdata(State, -1);
		lua_pop(State, 1);
	}
	return Pointer;
}

void* LuaTestClass(lua_State* State, int Index, int Class) {
	if(lua_getmetatable(State, Index) == 0)
		 luaL_error(State, LUA_TYPERROR(State, Index, Class, "LuaTestClass"));
	lua_pushinteger(State, LUA_OCLASS);
	lua_rawget(State, -2);
	if(lua_isnumber(State, -1) == 1 || Class == lua_tointeger(State, -1)) {
		lua_pop(State, 2);
		return NULL;
	}
	lua_pop(State, 2);
	return LuaToClass(State, Index);
}

void* LuaCheckClass(lua_State* State, int Index, int  Class) {
	int Pop = 4;

	Index = lua_absindex(State, Index);
	lua_rawgeti(State, LUA_REGISTRYINDEX, g_LuaClassTable[Class]);
	if(lua_getmetatable(State, Index) == 0) {
		lua_pop(State, 1);
		return NULL;
	}
	if(lua_rawequal(State, -1, -2)) {
		lua_pop(State, 2);
		goto found;
	}
	lua_pop(State, 2);
	lua_pushvalue(State, Index);
	if(lua_getmetatable(State, -1) == 0) {
		lua_pop(State, 1);
		goto end;
	}
	top:
	lua_pushliteral(State, "__baseclass");
	lua_rawget(State, -2);
	if(lua_type(State, -1) == LUA_TTABLE) {
		lua_rawgeti(State, LUA_REGISTRYINDEX, g_LuaClassTable[Class]);
	if(!lua_rawequal(State, -1, -2)) {
			lua_copy(State, -2, -4);
			lua_pop(State, 3);
			Pop = 3;
			goto top;
		}
		lua_pop(State, Pop);
		goto found;
	}
	end:
	lua_pop(State, 1);

	lua_pushinteger(State, LUA_OCLASS);
	lua_rawget(State, 1);
	luaL_error(State, LUA_TYPERROR(State, 1, lua_tostring(State, -1), "LuaCheckClass"));
	return 0;
	found:;
	void* Object = LuaToClass(State, Index);

	Assert(Object != NULL);
	return Object;
}

const char* LuaBaseClass(lua_State* State, int Index) {
	const char* BaseClass = NULL;

	if(lua_getmetatable(State, Index) == 0)
		 luaL_error(State, LUA_TYPERROR(State, Index, "Table", "LuaTestClass"));
	while(1) {
		lua_pushinteger(State, LUA_OCLASS);
		lua_rawget(State, -2);
		if(lua_isstring(State, -1) == 0) {
			lua_pop(State, 2);
			return NULL;
		}
		BaseClass = lua_tostring(State, -1);
		lua_pop(State, 1);
		lua_pushstring(State, "__baseclass");
		lua_rawget(State, -2);
		if(lua_type(State, -1) != LUA_TTABLE) {
			lua_pop(State, 1);
			break;
		}
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
	return BaseClass;
}

int LuaIntPair(lua_State* State, int Index, int* One, int* Two) {
	int Top = lua_gettop(State);

	lua_pushnil(State);
	if(lua_next(State, Index - 1) == 0)
		goto fail;
	if(LuaGetInteger(State, -1, One) == 0)
		goto fail;
	lua_pop(State, 1);
	if(lua_next(State, Index - 1) == 0)
		goto fail;
	if(LuaGetInteger(State, -1, Two) == 0)
		goto fail;
	lua_pop(State, 2);
	return 1;
	fail:
	lua_settop(State, Top);
	return 0;
}

int LuaKeyValue(lua_State* State, int Index, const char** Value, int* Pair) {
	int Top = lua_gettop(State);

	lua_pushnil(State);
	if(lua_next(State, Index - 1) == 0)
		goto fail;
	if(LuaGetString(State, -1, Value) == -1)
		goto fail;
	lua_pop(State, 1);
	if(lua_next(State, Index - 1) == 0)
		goto fail;
	if(LuaGetInteger(State, -1, Pair) == -1)
		goto fail;
	lua_pop(State, 2);
	return 1;
	fail:
	lua_settop(State, Top);
	return 0;
}

int LuaRuleLuaCall(lua_State* State) {
	int Args = lua_gettop(State);
	struct RuleLuaCall* Rule = NULL;

	SDL_assert(Args > 0);
	luaL_checktype(State, 1, LUA_TFUNCTION);
	lua_createtable(State, Args, 0);
	for(int i = 0; i < Args; ++i) {
		lua_pushvalue(State, i + 1);
		lua_rawseti(State, -2, i + 1);
	}
	SDL_assert(lua_type(State, -1) == LUA_TTABLE);
	Rule = CreateRuleLuaCall(State, luaL_ref(State, LUA_REGISTRYINDEX));
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleGreaterThan(lua_State* State) {
	struct RuleComparator* Rule = NULL;
	struct Rule* Left = NULL;
	struct Rule* Right = NULL;

	Left = LuaValueToRule(State, 1);
	Right = LuaValueToRule(State, 2);
	Rule = CreateRuleComparator(RULE_GREATERTHAN, Left, Right);
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleLessThan(lua_State* State) {
	struct RuleComparator* Rule = NULL;
	struct Rule* Left = NULL;
	struct Rule* Right = NULL;

	Left = LuaValueToRule(State, 1);
	Right = LuaValueToRule(State, 2);
	Rule = CreateRuleComparator(RULE_LESSTHAN, Left, Right);
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleTrue(lua_State* State) {
	struct Rule* Rule = (struct Rule*) CreateRuleBoolean(1);
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleFalse(lua_State* State) {
	struct Rule* Rule = (struct Rule*) CreateRuleBoolean(0);
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleIfThenElse(lua_State* State) {
	struct RuleComparator* Comparator = LuaCheckClass(State, 1, LOBJ_RULE);
	struct Rule* OnTrue = LuaCheckClass(State, 2, LOBJ_RULE);
	struct Rule* OnFalse = LuaCheckClass(State, 3, LOBJ_RULE);
	struct Rule* Rule = NULL;

	switch(Comparator->Type) {
	case RULE_BOOLEAN:
	case RULE_GREATERTHAN:
	case RULE_LESSTHAN:
		break;
	default:
		return luaL_error(State, "First argument must be a rule comparator.");
	}
	Rule = (struct Rule*) CreateRuleIfThenElse(Comparator, OnTrue, OnFalse);
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleEventFired(lua_State* State) {
	int i = 0;
	struct Rule* Rule = NULL;
	const char* String = luaL_checkstring(State, 1);

	for(i = 0; g_EventNames[i] != NULL; ++i) {
		if(strcmp(g_EventNames[i], String) == 0) {
			Rule = (struct Rule*) CreateRuleEvent(i);
			LuaCtor(State, Rule, LOBJ_RULE);
			return 1;
		}
	}
	luaL_error(State, "%s is not an event name.", String);
	return 0;
}

int LuaRuleBlock(lua_State* State) {
	struct RuleBlock* Rule =  NULL;
	int Args = lua_gettop(State);

	if(Args == 0)
		return luaL_error(State, "LuaRuleBlock must have at least one argument.");
	for(int i = 0; i < Args; ++i) {
		if(LuaCheckClass(State, i + 1, LOBJ_RULE) == NULL)
			return luaL_error(State, "LuaRuleBlock's %i argument is not a Rule.", i + 1);
	}
	Rule = CreateRuleBlock(Args);
	for(int i = 0; i < Args; ++i)
		Rule->RuleList[i] = LuaCheckClass(State, i + 1, LOBJ_RULE);
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleCond(lua_State* State) {
	struct RuleCond* Rule = NULL;
	int Args = lua_gettop(State);

	if((Args & 1) == 1) {
		return luaL_error(State, "Cond requires an even amount of arguments.");
	}
	Rule = CreateRuleCond(Args / 2);
	for(int i = 1, Ct = 0; i <= Args; i += 2, ++Ct) {
		Rule->Conditions[Ct] = LuaCheckClass(State, i, LOBJ_RULE);
		Rule->Actions[Ct] = LuaCheckClass(State, i + 1, LOBJ_RULE); 
	}
	LuaCtor(State, Rule, LOBJ_RULE);
	return 1;
}

int LuaRuleNegate(lua_State* State) {
	struct RuleLuaCall* Rule = LuaCheckClass(State, 1, LOBJ_RULE);

	LuaCtor(State, ((struct Rule*) CreateRuleDecorator(RULE_NEGATE, Rule)), LOBJ_RULE); 
	return 1;
}

int LuaMissionGetName(lua_State* State) {
	struct Mission* Mission = LuaCheckClass(State, 1, LOBJ_MISSION);

	lua_pushstring(State, Mission->Name);
	return 1;
}

int LuaMissionGetDesc(lua_State* State) {
	struct Mission* Mission = LuaCheckClass(State, 1, LOBJ_MISSION);

	lua_pushstring(State, Mission->Description);
	return 1;
}

int LuaMissionGetOptions(lua_State* State) {
	struct Mission* Mission = LuaCheckClass(State, 1, LOBJ_MISSION);

	lua_createtable(State, 0, Mission->OptionCt);
	for(int i = 0; i < Mission->OptionCt; ++i) {
		LuaCtor(State, &Mission->Options[i], LOBJ_MISSIONOPTION);
		lua_rawseti(State, -2, i + 1);
	}
	return 1;
}

int LuaMissionChooseOption(lua_State* State) {
	struct Mission* Mission = LuaCheckClass(State, 1, LOBJ_MISSION);
	struct MissionFrame* Data = LuaCheckClass(State, 2, LOBJ_MISSIONFRAME);
	int Option = luaL_checkinteger(State, 3);

	MissionCheckOption(State, Mission, Data, Option);
	return 0;
}

int LuaDateYears(lua_State* State) {
	DATE Date = *(DATE*)LuaCheckClass(State, 1, LOBJ_DATE);

	lua_pushinteger(State, YEAR(Date));
	return 1;
}

int LuaDateMonths(lua_State* State) {
	DATE Date = *(DATE*)LuaCheckClass(State, 1, LOBJ_DATE);

	lua_pushinteger(State, MONTH(Date));
	return 1;
}

int LuaDateDays(lua_State* State) {
	DATE Date = *(DATE*)LuaCheckClass(State, 1, LOBJ_DATE);

	lua_pushinteger(State, DAY(Date));
	return 1;
}

struct Rule* LuaValueToRule(lua_State* State, int Index) {
	struct RulePrimitive* Rule = NULL;

	if(lua_type(State, Index) == LUA_TTABLE) {
		if((Rule = LuaToObject(State, Index, LOBJ_RULE)) == NULL) {
			luaL_error(State, LUA_TYPERROR(State, Index, LOBJ_RULE, "LuaRuleGreaterThan"));
			return NULL;
		}
	} else {
		struct Primitive* Prim = CreatePrimitive();
		LuaToPrimitive(State, Index, Prim);
		Rule = CreateRulePrimitive(Prim);
	}
	return (struct Rule*) Rule;
}

void PrimitiveLuaPush(lua_State* State, struct Primitive* Primitive) {
	switch(Primitive->Type) {
	case PRIM_FLOAT:
		lua_pushnumber(State, Primitive->Value.Float);
		break;
	case PRIM_INTEGER:
		lua_pushinteger(State, Primitive->Value.Int);
		break;
	case PRIM_BOOLEAN:
		lua_pushboolean(State, Primitive->Value.Int);
		break;
	case PRIM_PTR:
		if(Primitive->Class == LOBJ_NONE) {
			lua_pushlightuserdata(State, Primitive->Value.Ptr);
		} else {
			LuaCtor(State, Primitive->Value.Ptr, Primitive->Class);
		}
		break;
	case PRIM_STRING:
		lua_pushstring(State, Primitive->Value.String);
		break;
	}
}

void LuaToPrimitive(lua_State* State, int Index, struct Primitive* Primitive) {
	void* Obj = NULL;
	int Class = LOBJ_NONE;

	switch(lua_type(State, Index)) {
		case LUA_TBOOLEAN:
			Primitive->Type = PRIM_BOOLEAN;
			Primitive->Value.Int = lua_toboolean(State, Index);
			break;
		case LUA_TSTRING:
			Primitive->Type = PRIM_STRING;
			Primitive->Value.String = calloc(strlen(lua_tostring(State, Index)) + 1, sizeof(char));
			strcpy(Primitive->Value.String, lua_tostring(State, Index));
			break;
		case LUA_TNUMBER:
			Primitive->Type = PRIM_INTEGER;
			Primitive->Value.Int = lua_tointeger(State, Index);
			break;
		case LUA_TTABLE:
			if((Obj = LuaIsObject(State, Index, &Class)) == NULL) {
				Primitive->Type = PRIM_STRING;
				Primitive->Value.String = calloc(sizeof("Table"), sizeof(char));
				strcpy(Primitive->Value.String, "Table");
			} else {
				Primitive->Type = PRIM_PTR;
				Primitive->Value.Ptr = Obj;
				Primitive->Class = Class;
			}
			break;
		case LUA_TFUNCTION:
			Primitive->Type = PRIM_STRING;
			Primitive->Value.String = calloc(sizeof("Function"), sizeof(char));
			strcpy(Primitive->Value.String, "Function");
			break;
	}
}

void LuaSetEnv(lua_State* State, const char* Env) {
	lua_getglobal(State, "Environments");
	lua_pushstring(State, Env);
	lua_pushvalue(State, -3);
	lua_rawset(State, -3);
	lua_pop(State, 1);
}

void LuaGetEnv(lua_State* State, const char* Env) {
	lua_getglobal(State, "Environments");
	lua_pushstring(State, Env);
	lua_rawget(State, -2);
	lua_remove(State, -2);
}

int LuaClassIndex(lua_State* State) {
	lua_getmetatable(State, 1);
	while(1) {
		lua_pushvalue(State, 2);
		//Search metatable for the key.
		lua_rawget(State, 3);
		if(lua_type(State, -1) != LUA_TNIL)
			break;
		//Key not found check base class.
		lua_pop(State, 1);
		lua_pushstring(State, "__baseclass");
		lua_rawget(State, 3);
		//No base class check LUA_BASECLASS metatable.
		if(lua_type(State, -1) == LUA_TNIL) {
			lua_rawgeti(State, LUA_REGISTRYINDEX, g_LuaClassTable[LUA_BASECLASS]);
			lua_pushvalue(State, 2);
			lua_rawget(State, -2);
			if(lua_type(State, -1) == LUA_TNIL) {
				lua_pushnil(State);
				return 1;
			}
			break;
		}
		lua_replace(State, 3);
	}
	return 1;
}

int LuaClassError(lua_State* State, int Arg, int Class) {
	const char* ArgType = NULL;

	Arg = lua_absindex(State, Arg);
	ArgType = LuaObjectClass(State, Arg);
	if(ArgType == NULL)
		ArgType = lua_typename(State, lua_type(State, Arg));
	else {
		lua_pushinteger(State, LUA_OSELF);
		lua_rawget(State, Arg);	
		if(lua_touserdata(State, -1) == NULL)
			return luaL_error(State, "Error argument #%d is of type \"%d\" but is NULL.", Arg, Class);
	}
	return luaL_error(State, "Error argument #%d is of type \"%s\" but expected \"%d\".", Arg, ArgType, Class); 
}

int LuaMathRandomVar(lua_State* State) {
	double Var = luaL_checknumber(State, 1);

	lua_pushnumber(State, Var * NormalDistribution(Var, .5, 0) + Var);
	return 1;
}

int LuaMathProbability(lua_State* State) {
	double Prob = luaL_checknumber(State, 1);
	int Max = luaL_checkinteger(State, 2);

	lua_pushnumber(State, Prob * Max);
	return 1;
}

int LuaInputMousePos(lua_State* State) {
	SDL_Point Pos;

	GetMousePos(&Pos);
	lua_pushinteger(State, Pos.x);
	lua_pushinteger(State, Pos.y);
	return 2;
}

void* LuaIsObject(lua_State* State, int Index, int* Class) {
	void* Self = NULL;

	Index = lua_absindex(State, Index);
	if(Class != NULL) *Class = LOBJ_NONE;
	if(lua_type(State, Index) != LUA_TTABLE) {
		return NULL;
	}
	lua_getmetatable(State, Index);
	lua_pushinteger(State, LUA_OCLASS);
	lua_rawget(State, -2);
	if(lua_isnumber(State, -1) == 0) return NULL;
	if(Class != NULL) *Class = lua_tointeger(State, -1);
	lua_pop(State, 2);

	lua_pushinteger(State, LUA_OSELF);
	lua_rawget(State, Index);
	Self = lua_touserdata(State, -1);
	lua_pop(State, 1);
	return Self;
}
