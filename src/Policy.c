/**
 * Author: David Brotz
 * File: Policy.h
 */

#include "Policy.h"

#include "World.h"

#include "sys/Log.h"
#include "sys/LuaCore.h"

#include <stdlib.h>
#include <string.h>

#include <lua/lauxlib.h>
#include <lua/lualib.h>

static uint16_t g_PolicyId = 0;

enum {
	POL_NONE,
	POL_SLVLGN,
	POL_SLVRSN,
	POL_SLVRGHT,
	POL_SLVPROP,
	POL_SLVTRT,
	POL_SLVRESTRIC
};

struct PolicyFunc g_PolicyFuncs[] = {
	{"SlaveLength", POL_SLVLGN},
	{"SlaveReason", POL_SLVRSN},
	{"SlaveRights", POL_SLVRGHT},
	{"SlaveProperty", POL_SLVPROP},
	{"SlaveTreatment", POL_SLVTRT},
	{"SlaveRestriction", POL_SLVRESTRIC},
	{NULL, POL_NONE}
};
uint8_t g_PolicyFuncSz = 0;

uint8_t CountPolicyFuncs(struct PolicyFunc* Funcs) {
	uint8_t i = 0;

	while(Funcs[i].Name != NULL) ++i;
	return i;
}

void CtorPolicy(struct Policy* _Policy, const char* _Name, const char* _Description, int _Category) {
	_Policy->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Policy->Description = calloc(strlen(_Description) + 1, sizeof(char));
	_Policy->Category = _Category;
	_Policy->OptionsSz = 0;
	for(int i = 0; i < POLICY_MAXOPTIONS; ++i) {
		for(int j = 0; j < CASTE_SIZE; ++j) {
			_Policy->Options[i].CastePreference[j] = POLPREF_NETURAL;
		}
	}
	strcpy((char*)_Policy->Name, _Name);
	strcpy((char*)_Policy->Description, _Description);
	_Policy->Id = g_PolicyId;
}

void DestroyPolicy(struct Policy* _Policy) {
	free((char*) _Policy->Name);
	free((char*) _Policy->Description);
}

void PolicyAddOption(struct Policy* _Policy, int _Row, const char* _Name, const char* _Desc, PolicyOptFunc _CallFunc, PolicyOptUtility _Utility) {
	struct PolicyOption* _Opt = &_Policy->Options[_Policy->OptionsSz];

	Assert(_Row >= 0 || _Row < POLICY_SUBSZ || _Policy->OptionsSz < POLICY_MAXOPTIONS);
	++_Policy->OptionsSz;
	_Opt->Name = calloc(sizeof(char), strlen(_Name) + 1);
	_Opt->Desc = calloc(sizeof(char), strlen(_Desc) + 1);
	_Opt->Utility = _Utility;
	strcpy((char*) _Opt->Name, _Name);
	strcpy((char*) _Opt->Desc, _Desc);
}

void PolicyAddCategory(struct Policy* _Policy, const char* _Name) {
	int _Category = 0;

	_Policy->Options[_Category].Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy((char*)_Policy->Options[_Category].Name, _Name);
}

const struct PolicyOption* PolicyRow(const struct Policy* _Policy, int _Col) {
	Assert(_Col >= 0 && _Col < POLICY_MAXOPTIONS);
	return &_Policy->Options[_Col];
}

void DestroyPolicyOption(struct PolicyOption* _Opt) {
	free((char*) _Opt->Name);
}

const struct PolicyOption* PolicyChange(const struct ActivePolicy* _Policy) {
		if(_Policy->OptionSel != POLICYACT_IGNORE) {
			return PolicyRow(_Policy->Policy, _Policy->OptionSel);
		}
	return NULL;
}

int LuaPolicyLoad(lua_State* State) {
	struct Policy* Policy = NULL;
	const char* PolicyName = NULL;
	const char* Name = NULL;
	const char* Desc = NULL;
	int Category = 0;
	int OptCt = 0;
	int KeyValSz = 0;
	struct PolicyKV KeyVal[POL_MAXACT];

#ifndef DEBUG
	int _LuaTop = lua_gettop(State);
#endif
	lua_pushstring(State, "Name");
	lua_rawget(State, 1);
	if(LuaGetString(State, -1, &PolicyName) == 0) {
		return luaL_error(State, "Policy Name parameter is not a string.");
	}
	lua_pushstring(State, "Desc");
	lua_rawget(State, 1);
	if(LuaGetString(State, -1, &Desc) == 0) {
		return luaL_error(State, "Policy Desc parameter is not a string.");
	}
	lua_pushstring(State, "Category");
	lua_rawget(State, 1);
	if(LuaGetInteger(State, -1, &Category) == 0) {
		return luaL_error(State, "Policy Category parameter is not a string.");
	}

	lua_pop(State, 3);
	lua_pushstring(State, "Options");
	lua_rawget(State, 1);
	if(!lua_istable(State, -1)) {
		free(Policy);
		return luaL_error(State, "Policy Desc parameter is not a string.");
	}
	Policy = malloc(sizeof(struct Policy));
	CtorPolicy(Policy, PolicyName, Desc, Category);
	PolicyAddCategory(Policy, "Foo");
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_istable(State, -1) == 0)
			return luaL_error(State, "Policy %s, option is not a table.", PolicyName);
		lua_pushstring(State, "Name");
		lua_rawget(State, -2);
		if(LuaGetString(State, -1, &Name) == 0) {
			free(Policy);
			return luaL_error(State, "Policy %s, option Name parameter is not a string.", PolicyName);
		}
		lua_pop(State, 1);
		lua_pushstring(State, "Desc");
		lua_rawget(State, -2);
		if(LuaGetString(State, -1, &Desc) == 0) {
			free(Policy);
			return luaL_error(State, "Policy %s, option Desc parameter is not a string.", PolicyName);
		}
		lua_pop(State, 1);
		lua_pushstring(State, "Action");
		lua_rawget(State, -2);
		if(lua_istable(State, -1) == 0) {
			free(Policy);
			return luaL_error(State, "Policy %s, option Action paramater is not a table.", PolicyName);	
		}
		lua_pushnil(State);
		while(lua_next(State, -2) != 0) {
			const char* Key = NULL;
			int Val = 0;
			uint8_t Low = 0;
			uint8_t High = g_PolicyFuncSz;
			uint8_t Index = g_PolicyFuncSz / 2;
			int Result = 0;

			if(LuaGetString(State, -2, &Key) == 0) {
				free(Policy);
				return luaL_error(State, "Policy %s, action table contains non-string key.", PolicyName);
			}
			//TODO: Remove this and replace with a function that will do a binary search on g_PolicyFuncs and return the index of the string contained in Key .
			while(Low <= High) {
				Index = (Low + High) / 2;
				Result = strcmp(Key, g_PolicyFuncs[Index].Name);
				if(Result < 0) {
					High = Index - 1;
				} else if(Result > 0) {
					Low = Index + 1;
				} else {
					goto found_index;
				}
			}
			free(Policy);
			return luaL_error(State, "Policy %s, action table contains invalid key %s", PolicyName, lua_tostring(State, -2));	
			found_index:
			if(LuaGetInteger(State, -1, &Val) == 0) {
				free(Policy);
				return luaL_error(State, "Policy %s, action table contains non-integer value for key %s", PolicyName, Key);
			}
			//TODO: Add check here to make sure KeyVal doesnt overflow
			KeyVal[KeyValSz].Key = Index;
			KeyVal[KeyValSz].Val = Val;

			++KeyValSz;
			lua_pop(State, 1);
		}
		lua_pop(State, 2);
		PolicyAddOption(Policy, 0, Name, Desc, NULL, NULL);
		for(int i = 0; i < KeyValSz; ++i) {
			Policy->Options[Policy->OptionsSz].Actions[i] = KeyVal[i];
		}
		KeyValSz = 0;
		//TODO: add KeyVals to PolicyOption.
		//TODO: Reset KeyValSz to 0 here
		++OptCt;
	}
	lua_pop(State, 1);
	Policy->Id = g_GameWorld.Policies.Size;
	ArrayInsert_S(&g_GameWorld.Policies, Policy); 
#ifndef DEBUG
	Assert(LuaTop == lua_gettop(State));
#endif
	return 0;
}

int PolicyFuncCmp(const void* One, const void* Two) {
	const struct PolicyFunc* FuncOne = One;
	const struct PolicyFunc* FuncTwo = Two;

	return strcmp(FuncOne->Name, FuncTwo->Name);
}
