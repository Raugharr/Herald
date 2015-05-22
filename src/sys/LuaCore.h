/*
 * File: LuaCore.h
 * Author: David Brotz
 */
#ifndef __LUACORE_H
#define __LUACORE_H

typedef struct lua_State lua_State;
typedef struct luaL_Reg luaL_Reg;
typedef int (*lua_CFunction) (lua_State *L);
struct Constraint;
struct LinkedList;
struct Rule;
struct Primitive;

#define LuaAbsPos(_State, _Index) ((_Index > 0) ? (_Index) : (((_Index < -(lua_gettop(_State))) ? (_Index) : lua_gettop(_State) + (_Index) + 1)))
#define LuaCtor(_State, _Class, _Ptr)			\
	lua_createtable((_State), 0, 1);			\
	lua_getglobal((_State), (_Class));			\
	if(lua_type((_State), -1) != LUA_TNIL) {	\
		lua_setmetatable((_State), -2);				\
		lua_pushstring((_State), "__self");			\
		lua_pushlightuserdata((_State), (_Ptr));	\
		lua_rawset((_State), -3);					\
	} else {										\
		lua_pop((_State), 1);						\
		Log(ELOG_WARNING, "Lua class %s does not exist", (_Class));	\
	}

#define ConstraintToLua(_State, _Constraint)			\
	lua_createtable((_State), 0, 2);					\
	lua_pushstring((_State), "Min");					\
	lua_pushinteger((_State), (_Constraint)->Min);		\
	lua_rawset((_State), -3);							\
	lua_pushstring((_State), "Max");					\
	lua_pushinteger((_State), (_Constraint)->Max);		\
	lua_rawset((_State), -3)

#define LUA_TYPERROR(_State, _Arg, _Type, _Func) "bad argument %d to '%s' (%s expected got %s)", (_Arg), (_Func), (_Type), lua_typename((_State), lua_type((_State), (_Arg)))

extern lua_State* g_LuaState;

struct LuaObjectReg {
	const char* Name;
	const char* BaseClass;
	const luaL_Reg* Funcs;
};

void InitLuaSystem();
void QuitLuaSystem();

void RegisterLuaObjects(lua_State* _State, const struct LuaObjectReg* _Objects);
int LuaRegisterObject(lua_State* _State, const char* _Name, const char* _BaseClass, const luaL_Reg* _Funcs);

int LuaArrayCreate(lua_State* _State);
int LuaArrayItrNext(lua_State* _State);
int LuaArrayItrPrev(lua_State* _State);

int LuaLnkLstFront(lua_State* _State);
int LuaLnkLstBack(lua_State* _State);
int LuaLnkLstSize(lua_State* _State);
int LuaLnkLstIterate(lua_State* _State);
int LuaLnkLstNodeNext(lua_State* _State);
int LuaLnkLstNodePrev(lua_State* _State);
int LuaLnkLstNodeItr(lua_State* _State);

int LuaArrayItr(lua_State* _State);

void* LuaToObject(lua_State* _State, int _Index, const char* _Name);
/**
 * Takes three arguments from the stack and returns a light user data containing a struct Constraint**.
 * The three arguments are in order, min, max, and interval.
 */
int LuaConstraint(lua_State* _State);
/**
 * A Lua variant of CreateConstrntVaBnds.
 */
int LuaConstraintBnds(lua_State* _State);
void ConstraintBndToLua(lua_State* _State, struct Constraint** _Constraints);

/**
 * Converts years to days.
 */
int LuaYears(lua_State* _State);
int LuaMonth(lua_State* _State);
int LuaPrintDate(lua_State* _State);
int LuaPrintYears(lua_State* _State);
int LuaHook(lua_State* _State);
int LuaRandom(lua_State* _State);

int LuaLoadFile(lua_State* _State, const char* _File);
int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc);
//FIXME: _Callback's second parameter is not used.
int LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return);
//TODO: Rename Add* to Lua*.
int AddInteger(lua_State* _State, int _Index, int* _Number);
int AddString(lua_State* _State, int _Index, const char** _String);
int AddNumber(lua_State* _State, int _Index, double* _Number);
int LuaLudata(lua_State* _State, int _Index, void** _Data);
int LuaFunction(lua_State* _State, int _Index, lua_CFunction* _Function);

/**
 * _Table must be big enough to contain at least lua_gettop(_State) elements.
 */
void LuaStackToTable(lua_State* _State, int* _Table);
/**
 * Copies every element of the table at _Index into the table at the top of the stack.
 * The table at _Index will be removed.
 */
void LuaCopyTable(lua_State* _State, int _Index);
/**
 * Returns a pointer to C data that is contained inside a Lua class.
 */
void* LuaToClass(lua_State* _State, int _Index);
/**
 * Checks if the element at _Index in the Lua stack is of type _Class.
 * Returns a pointer to it's C data or NULL if the element at _Index is not
 * of type _Class.
 */
void* LuaTestClass(lua_State* _State, int _Index, const char* _Class);
void* LuaCheckClass(lua_State* _State, int _Index, const char* _Class);
/*
 * These functions are for retrieving data from simple tables.
 */
int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

struct RuleLuaCall* CreateRuleLuaCall(lua_State* _State, int _TableRef);

int LuaRuleLuaCall(lua_State* _State);
int LuaRuleGreaterThan(lua_State* _State);
int LuaRuleLessThan(lua_State* _State);
int LuaRuleTrue(lua_State* _State);
int LuaRuleFalse(lua_State* _State);
int LuaRuleEventFired(lua_State* _State);

struct Rule* LuaValueToRule(lua_State* _State, int _Index);

void PrimitiveLuaPush(lua_State* _State, struct Primitive* _Primitive);
struct Primitive* LuaToPrimitive(lua_State* _State, int _Index);

#endif
