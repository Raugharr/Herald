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

void CtorPolicy(struct Policy* _Policy, const char* _Name, const char* _Description, int _Category) {
	_Policy->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Policy->Description = calloc(strlen(_Description) + 1, sizeof(char));
	_Policy->Category = _Category;
	_Policy->OptionsSz = 0;
	for(int i = 0; i < POLICY_SUBSZ; ++i)
		_Policy->Options.Size[i] = POLICYCAT_UNUSED;
	for(int i = 0; i < POLICY_MAXOPTIONS; ++i) {
		for(int j = 0; j < CASTE_SIZE; ++j) {
			_Policy->Options.Options[i].CastePreference[j] = POLPREF_NETURAL;
		}
	}
	strcpy((char*)_Policy->Name, _Name);
	strcpy((char*)_Policy->Description, _Description);
	_Policy->Id = g_PolicyId;
}

void DestroyPolicy(struct Policy* _Policy) {
	for(int i = 0; i < POLICY_SUBSZ; ++i)
		free(_Policy->Options.Name[i]);
	free((char*) _Policy->Name);
	free((char*) _Policy->Description);
}

void PolicyAddOption(struct Policy* _Policy, int _Row, const char* _Name, const char* _Desc, PolicyOptFunc _CallFunc, PolicyOptUtility _Utility) {
	struct PolicyOption* _Opt = &_Policy->Options.Options[_Policy->OptionsSz];

	Assert(!(_Row < 0 || _Row > POLICY_SUBSZ || _Policy->Options.Size[_Row] == POLICYCAT_UNUSED));
	++_Policy->OptionsSz;
	++_Policy->Options.Size[_Row];
	_Opt->Name = calloc(sizeof(char), strlen(_Name) + 1);
	_Opt->Desc = calloc(sizeof(char), strlen(_Desc) + 1);
	_Opt->OnPass = _CallFunc;
	_Opt->OnRemove = NULL;
	_Opt->Utility = _Utility;
	strcpy((char*) _Opt->Name, _Name);
	strcpy((char*) _Opt->Desc, _Desc);
}

void PolicyAddCategory(struct Policy* _Policy, const char* _Name) {
	int _Category = 0;

	for(_Category = 0; _Category < POLICY_SUBSZ; ++_Category) {
		if(_Policy->Options.Size[_Category] == POLICYCAT_UNUSED) {
			_Policy->Options.Size[_Category] = 0;	
			break;
		}
	}
	_Policy->Options.Name[_Category] = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Policy->Options.Name[_Category], _Name);
}

const struct PolicyOption* PolicyRow(const struct Policy* _Policy, int _Row, int _Col) {
	int _Idx = 0;

	Assert(_Row >= 0 && _Row < POLICY_SUBSZ);
	Assert(_Col >= 0 && _Col < POLICY_MAXOPTIONS);
	if(_Row == 0) {
		if(_Col >= _Policy->Options.Size[0])
			return NULL;
		return &_Policy->Options.Options[_Col];
	}
	for(int i = 1; i < POLICY_SUBSZ; ++i) {
		_Idx += _Policy->Options.Size[i];
		if(i == _Row) {
			if(_Col >= _Policy->Options.Size[i])
				return NULL;
			return &_Policy->Options.Options[_Idx + _Col];
		}
	}
	return NULL;
}

void DestroyPolicyOption(struct PolicyOption* _Opt) {
	free((char*) _Opt->Name);
}

const struct PolicyOption* PolicyChange(const struct ActivePolicy* _Policy) {
	for(int i = 0; i < POLICY_SUBSZ; ++i) {
		if(_Policy->OptionSel[i] != POLICYACT_IGNORE) {
			return PolicyRow(_Policy->Policy, i, _Policy->OptionSel[i]);
		}
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
		if(LuaGetString(State, -1, &Name) == 0) {
			free(Policy);
			return luaL_error(State, "Policy %s, option Desc parameter is not a string.", PolicyName);
		}
		lua_pop(State, 2);
		PolicyAddOption(Policy, 0, Name, Desc, NULL, NULL);
		++OptCt;
	}
	lua_pop(State, 1);
	ArrayInsert_S(&g_GameWorld.Policies, Policy); 
	return 0;
}
