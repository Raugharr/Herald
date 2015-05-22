/*
 * File: LuaSettlement.c
 * Author: David Brotz
 */

#include "LuaSettlement.h"

#include "Government.h"
#include "BigGuy.h"
#include "Herald.h"
#include "Location.h"

#include "sys/LuaCore.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <malloc.h>

lua_State* g_LuaState = NULL;

static luaL_Reg g_LuaFuncsReformPassing[] = {
		{"GetVotes", LuaReformPassingGetVotes},
		{"GetMaxVotes", LuaReformPassingGetMaxVotes},
		{NULL, NULL}
};

static luaL_Reg g_LuaFuncsReform[] = {
		{"GetName", LuaReformGetName},
		{NULL, NULL},
};

static const luaL_Reg g_LuaFuncsGovernment[] = {
		{"PossibleReforms", LuaGovernmentPossibleReforms},
		{"Structure", LuaGovernmentStructure},
		{"Type", LuaGovernmentType},
		{"Rule", LuaGovernmentRule},
		{"PassReform", LuaGovernmentPassReform},
		{"GetReform", LuaGovernmentGetReform},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuy[] = {
		{"GetPerson", LuaBGGetPerson},
		{"SetAuthority", LuaBGSetAuthority},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsSettlement[] = {
		{"GetLeader", LuaSettlementGetLeader},
		{NULL, NULL}
};

const struct LuaObjectReg g_LuaSettlementObjects[] = {
		{"ReformPassing", NULL, g_LuaFuncsReformPassing},
		{"Reform", NULL, g_LuaFuncsReform},
		{"Government", NULL, g_LuaFuncsGovernment},
		{"BigGuy", NULL, g_LuaFuncsBigGuy},
		{"Settlement", NULL, g_LuaFuncsSettlement},
		{"BuildMat", NULL, NULL},
		{NULL, NULL, NULL}
};

int LuaBGGetPerson(lua_State* _State) {
	struct BigGuy* _BG = LuaCheckClass(_State, 1, "BigGuy");

	LuaCtor(_State, "Person", _BG->Person);
	return 1;
}

int LuaBGSetAuthority(lua_State* _State) {
	struct BigGuy* _BG = LuaCheckClass(_State, 1, "BigGuy");
	int _Authority = luaL_checkinteger(_State, 2);

	_BG->Authority += _Authority;
	return 0;
}

int LuaGovernmentPossibleReforms(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");

	LuaCtor(_State, "LinkedList", &_Government->PossibleReforms);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "Reform");
	lua_rawset(_State, -3);
	return 1;
}

int LuaGovernmentStructure(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");

	lua_pushstring(_State, GovernmentTypeToStr(_Government->GovType, GOVTYPE_MASK));
	return 1;
}

int LuaGovernmentType(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");

	lua_pushstring(_State, GovernmentTypeToStr(_Government->GovType, GOVSTCT_MASK));
	return 1;
}

int LuaGovernmentRule(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");

	lua_pushstring(_State, GovernmentTypeToStr(_Government->GovType, GOVRULE_MASK));
	return 1;
}

int LuaGovernmentPassReform(lua_State* _State) {
	struct Government* _Gov = LuaCheckClass(_State, 1, "Government");
	struct Reform* _Reform = LuaCheckClass(_State, 2, "Reform");

	GovernmentPassReform(_Gov, _Reform);
	return 0;
}

int LuaGovernmentGetReform(lua_State* _State) {
	struct Government* _Gov = LuaCheckClass(_State, 1, "Government");

	if(_Gov->Reform != NULL) {
		LuaCtor(_State, "ReformPassing", _Gov->Reform);
	} else {
		lua_pushnil(_State);
	}
	return 1;
}

int LuaReformPassingGetVotes(lua_State* _State) {
	struct ReformPassing* _Reform = LuaCheckClass(_State, 1, "ReformPassing");

	lua_pushinteger(_State, _Reform->VotesFor);
	return 1;
}

int LuaReformPassingGetMaxVotes(lua_State* _State) {
	struct ReformPassing* _Reform = LuaCheckClass(_State, 1, "ReformPassing");

	lua_pushinteger(_State, _Reform->MaxVotes);
	return 1;
}

int LuaReformGetName(lua_State* _State) {
	struct Reform* _Reform = LuaCheckClass(_State, 1, "Reform");

	lua_pushstring(_State, _Reform->Name);
	return 1;
}

int LuaSettlementGetLeader(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	LuaCtor(_State, "BigGuy", _Settlement->Government->Leader);
	return 1;
}
