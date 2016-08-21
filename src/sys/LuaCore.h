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
struct Resource;
struct Array;

/**
 * TODO: Complete const functionality for classes, by creating a table for each metatable that
 * contains a list of all functions that can only be used if the object is constant and then change its
 * __index to look at that table.
 * TODO: Instead of using strings for object types instead use integers.
 */

#define LUA_BASECLASS "Object"
#define LuaStackAssert(_State, _Code) (int __LuaStack = lua_gettop(_State); (_Code) Assert(lua_gettop(_State) != __LuaStack))

//FIXME: The arguments of LuaCtor should match LuaCheckClass and thus be changed to _State, _Ptr, _Class
#define LuaCtor(_State, _Class, _Ptr)			\
	lua_createtable((_State), 0, 1);			\
	LuaInitClass((_State), (_Class), (_Ptr))

#define LuaAddEnumToTable(_State, _String, _Int)	\
	lua_pushstring(_State, _String);				\
	lua_pushinteger(_State, _Int);					\
	lua_rawset(_State, -3)

#define LuaConstCtor(_State, _Class, _Ptr)				\
	lua_createtable((_State), 0, 1);					\
	LuaInitClass((_State), (_Class), (void*) (_Ptr))	

#define LuaCreateLibrary(_State, _LuaReg, _Name)	\
	luaL_newlib((_State), (_LuaReg));				\
	lua_setglobal((_State), (_Name))

#define ConstraintToLua(_State, _Constraint)			\
	lua_createtable((_State), 0, 2);					\
	lua_pushstring((_State), "Min");					\
	lua_pushinteger((_State), (_Constraint)->Min);		\
	lua_rawset((_State), -3);							\
	lua_pushstring((_State), "Max");					\
	lua_pushinteger((_State), (_Constraint)->Max);		\
	lua_rawset((_State), -3)

/*
 * FIXME: Remove _Func and instead use luaL_where to retrieve where in the Lua code the error occurred instead of an arbitrary function name.
 */
#define LUA_TYPERROR(_State, _Arg, _Type, _Func) "bad argument %d to '%s' (%s expected got %s)", (_Arg), (_Func), (_Type), lua_typename((_State), lua_type((_State), (_Arg)))

extern lua_State* g_LuaState;

struct LuaObjectReg {
	const char* Name;
	const char* BaseClass;
	const luaL_Reg* Funcs;
};

struct LuaModuleReg {
	const char* Name;
	const luaL_Reg* Funcs;
};

struct LuaEnum {
	const char* Key;
	int Value;
};

struct LuaEnumReg {
	const char* Name;
	const char* SubTable;
	const struct LuaEnum* Enum;
};

void InitLuaCore();
void QuitLuaCore();

/**
 * Calls LuaRegisterObject on each element of _Objects.
 */
void RegisterLuaObjects(lua_State* _State, const struct LuaObjectReg* _Objects);
/**
 * Creates a class prototype that is named _Class that is a subclass of _BaseClass and contains the Lua functions declared in _Funcs.
 * If _BaseClass is NULL then the prototype will have no subclass.
 * If _Funcs is NULL no Lua functions will be loaded.
 */
int LuaRegisterObject(lua_State* _State, const char* _Class, const char* _BaseClass, const luaL_Reg* _Funcs);
/**
 * Registers all Lua functions in _Funcs to the global space of _State.
 */
void LuaRegisterFunctions(lua_State* _State, const luaL_Reg* _Funcs);
void CreateLuaLnkLstItr(lua_State* _State, const struct LinkedList* _List, const char* _Class);
void CreateLuaArrayItr(lua_State* _State, const struct Array* _Array, const char* _Class);
void LuaArrayClassToTable(lua_State* _State, const void** _Table, int _TableSz, const char* _Class);

/**
 * Sets the table at the top of the stack to have _Class as its metatable, and an element __self with _Ptr as its value.
 */
void LuaInitClass(lua_State* _State, const char* _Class, void* _Ptr);
const char* LuaObjectClass(lua_State* _State, int _Arg);

int LuaArrayCreate(lua_State* _State);
int LuaArrayItrNext(lua_State* _State);
int LuaArrayItrPrev(lua_State* _State);

int LuaLnkLstFront(lua_State* _State);
int LuaLnkLstBack(lua_State* _State);
int LuaLnkLstSize(lua_State* _State);
int LuaLnkLstNodeNext(lua_State* _State);
int LuaLnkLstNodePrev(lua_State* _State);
int LuaLnkLstNodeItr(lua_State* _State);

int LuaArrayItr(lua_State* _State);

/**
 * Returns the pointer contained by the table located at _Index.
 * Pointer is located at the table's __self variable only if the table's __class variable is equal to _Class.
 * If the table at _Index is not of type _Class LuaToObject returns NULL.
 */
void* LuaToObject(lua_State* _State, int _Index, const char* _Class);
/**
 * Takes three arguments from the stack and returns a light user data containing a struct Constraint**.
 * The three arguments are in order, min, max, and interval.
 */
int LuaConstraint(lua_State* _State);
int LuaObjectIsEqual(lua_State* _State);
int LuaObjectGetClassName(lua_State* _State);
/**
 * A Lua variant of CreateConstrntVaBnds.
 */
int LuaConstraintBnds(lua_State* _State);
void ConstraintBndToLua(lua_State* _State, struct Constraint** _Constraints);

/**
 * Function that adds a variable amount of integers to a Lua table.
 * \param _State The lua_State to use.
 * \param _Table the index of the table the variables will be added to.
 * \param _Enum array of LuaEnum that will use Key as the table Key and
 * value as the value of the table index. Passing NULL as a value of a
 * LuaEnum's Key will signal the end of the _Enum array.
 */
void LuaAddEnum(lua_State* _State, int _Table, const struct LuaEnum* _Enum); 
void RegisterLuaEnums(lua_State* _State, const struct LuaEnumReg* _Reg);

/**
 * \brief Converts years to days.
 */
int LuaYears(lua_State* _State);
int LuaMonth(lua_State* _State);
int LuaPrintDate(lua_State* _State);
int LuaPrintYears(lua_State* _State);
int LuaHook(lua_State* _State);
int LuaRandom(lua_State* _State);
int LuaNull(lua_State* _State);

int LuaLoadFile(lua_State* _State, const char* _File, const char* _Environment);
int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc);
//FIXME: _Callback's second parameter is not used.
int LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return);
int LuaGetInteger(lua_State* _State, int _Index, int* _Number);
int LuaGetString(lua_State* _State, int _Index, const char** _String);
int LuaGetNumber(lua_State* _State, int _Index, double* _Number);
int LuaGetUData(lua_State* _State, int _Index, void** _Data);
int LuaGetFunction(lua_State* _State, int _Index, lua_CFunction* _Function);

int LuaRawString(lua_State* _State, int _Index, const char* _Field, const char** _Str);

void LuaPrintTable(lua_State* _State, int _Index);
/**
 * _Table must be big enough to contain at least lua_gettop(_State) elements.
 */
void LuaStackToTable(lua_State* _State, int* _Table);
/**
 * Copies every element of the table at index _Index into the table at the top of the stack
 * The value at the top of the stack will be popped.
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
/**
 * Checks if the element located on the stack at _Index is of type _Class.
 * If the element is not the correct _Class LuaCheckClass will recursively
 * load the global table with the name of the element's __baseclass until
 * __baseclass is nil or the __baseclass is equal to _Class.
 * Will raise an error if the table at _Index is not of type _Class.
 */
void* LuaCheckClass(lua_State* _State, int _Index, const char* _Class);
/**
 * Returns the lowest base class for the object at _Index
 */
const char* LuaBaseClass(lua_State* _State, int _Index);
/*
 * These functions are for retrieving data from simple tables.
 */
int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

int LuaRuleLuaCall(lua_State* _State);
int LuaRuleGreaterThan(lua_State* _State);
int LuaRuleLessThan(lua_State* _State);
int LuaRuleTrue(lua_State* _State);
int LuaRuleFalse(lua_State* _State);
int LuaRuleIfThenElse(lua_State* _State);
int LuaRuleEventFired(lua_State* _State);
int LuaRuleBlock(lua_State* _State);
int LuaRuleCond(lua_State* _State);
int LuaRuleNegate(lua_State* _State);

int LuaMissionGetName(lua_State* _State);
int LuaMissionGetDesc(lua_State* _State);
int LuaMissionGetOptions(lua_State* _State);
int LuaMissionChooseOption(lua_State* _State);
int LuaMissionOptionGetName(lua_State* _State);
int LuaMissionOptionConditionSatisfied(lua_State* _State);

struct Rule* LuaValueToRule(lua_State* _State, int _Index);

void PrimitiveLuaPush(lua_State* _State, struct Primitive* _Primitive);
void LuaToPrimitive(lua_State* _State, int _Index, struct Primitive* _Primitive);

/*
 * Set the environment _Env to the table at the top of the stack.
 */
void LuaSetEnv(lua_State* _State, const char* _Env);
/*
 * Get the environment _Env and pushes it into the stack.
 */
void LuaGetEnv(lua_State* _State, const char* _Env);
//Index function for Lua classes.
int LuaClassIndex(lua_State* _State);

void InitMissionLua(lua_State* _State);
int LuaClassError(lua_State* _State, int _Arg, const char* _Class);

#endif
