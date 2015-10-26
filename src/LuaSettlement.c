/*
 * File: LuaSettlement.c
 * Author: David Brotz
 */

#include "LuaSettlement.h"

#include "Government.h"
#include "BigGuy.h"
#include "Herald.h"
#include "Location.h"
#include "Warband.h"
#include "ArmyGoal.h"
#include "World.h"

#include "sys/LuaCore.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <malloc.h>

static luaL_Reg g_LuaFuncsArmy[] = {
		{"GetLeader", LuaArmyGetLeader},
		{"GetSize", LuaArmyGetSize},
		{NULL, NULL}
};

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
		{"GetAuthority", LuaBGGetAuthority},
		{"SetAuthority", LuaBGSetAuthority},
		{"GetPrestige", LuaBGGetPrestige},
		{"SetPrestige", LuaBGSetPrestige},
		{"GetAdministration", LuaBGGetAdministration},
		{"GetIntrigue", LuaBGGetIntrigue},
		{"GetStrategy", LuaBGGetStrategy},
		{"GetWarfare", LuaBGGetWarfare},
		{"GetTactics", LuaBGGetTactics},
		{"GetCharisma", LuaBGGetCharisma},
		{"GetPiety", LuaBGGetPiety},
		{"GetIntellegence", LuaBGGetIntellegence},
		{"GetAgent", LuaBGGetAgent},
		{"GetRelation", LuaBGGetRelation},
		{"SetOpinion", LuaBGSetOpinion},
		{"SetAction", LuaBGSetAction},
		{"ImproveRelationTarget", LuaBGImproveRelationTarget},
		{"Kill", LuaBGKill},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuyRelation[] = {
		{"GetOpinion", LuaBGRelationGetOpinion},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsSettlement[] = {
		{"GetLeader", LuaSettlementGetLeader},
		{"GetGovernment", LuaSettlementGetGovernment},
		{"RaiseArmy", LuaSettlementRaiseArmy},
		{"GetPopulation", LuaSettlementGetPopulation},
		{"CountWarriors", LuaSettlementCountWarriors},
		{"GetBigGuys", LuaSettlementGetBigGuys},
		{NULL, NULL}
};

const struct LuaObjectReg g_LuaSettlementObjects[] = {
		{"Army", NULL, g_LuaFuncsArmy},
		{"ReformPassing", NULL, g_LuaFuncsReformPassing},
		{"Reform", NULL, g_LuaFuncsReform},
		{"Government", NULL, g_LuaFuncsGovernment},
		{"BigGuy", NULL, g_LuaFuncsBigGuy},
		{"BigGuyRelation", NULL, g_LuaFuncsBigGuyRelation},
		{"Settlement", NULL, g_LuaFuncsSettlement},
		{"BuildMat", NULL, NULL},
		{NULL, NULL, NULL}
};

int LuaArmyGetLeader(lua_State* _State) {
	struct Army* _Army = LuaCheckClass(_State, 1, "Army");

	LuaCtor(_State, "BigGuy", _Army->Leader);
	return 1;
}

int LuaArmyGetSize(lua_State* _State) {
	struct Army* _Army = LuaCheckClass(_State, 1, "Army");

	lua_pushinteger(_State, ArmyGetSize(_Army));
	return 1;
}

int LuaBGGetPerson(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	LuaCtor(_State, "Person", _Guy->Person);
	return 1;
}

int LuaBGGetAuthority(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Authority);
	return 1;
}

int LuaBGSetAuthority(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	int _Authority = luaL_checkinteger(_State, 2);

	_Guy->Authority += _Authority;
	return 0;
}

int LuaBGGetPrestige(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Prestige);
	return 1;
}

int LuaBGSetPrestige(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	int _Prestige = luaL_checkinteger(_State, 2);

	_Guy->Prestige += _Prestige;
	return 0;
}

int LuaBGGetAdministration(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Administration);
	return 1;
}

int LuaBGGetIntrigue(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Intrigue);
	return 1;
}

int LuaBGGetStrategy(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Strategy);
	return 1;
}

int LuaBGGetWarfare(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Warfare);
	return 1;
}

int LuaBGGetTactics(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Tactics);
	return 1;
}

int LuaBGGetCharisma(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Charisma);
	return 1;
}

int LuaBGGetPiety(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Piety);
	return 1;
}

int LuaBGGetIntellegence(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats.Intellegence);
	return 1;
}

int LuaBGGetAgent(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct Agent* _Agent = RBSearch(&g_GameWorld.Agents, _Guy);

	if(_Agent == NULL) {
		lua_pushnil(_State);
		goto end;
	}
	LuaCtor(_State, "Agent", _Agent);
	end:
	return 1;
}

int LuaBGGetRelation(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct BigGuy* _Target = LuaCheckClass(_State, 2, "BigGuy");
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Target);

	if(_Relation == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, "BigGuyRelation", _Relation);
	return 1;
}

int LuaBGSetOpinion(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct BigGuy* _Target = LuaCheckClass(_State, 2, "BigGuy");
	int _Action = luaL_checkinteger(_State, 3);
	int _Mod = luaL_checkinteger(_State, 4);
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Target);

	if(_Relation == NULL) {
		_Relation = CreateBigGuyRelation(_Guy, _Target);
		CreateBigGuyOpinion(_Relation, _Action, _Mod);
		BigGuyRelationUpdate(_Relation);
	} else {
		BigGuyAddRelation(_Guy, _Relation, _Action, _Mod);
	}
	return 0;
}

int LuaBGSetAction(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	int _Action = luaL_checkint(_State, 2);
	struct BigGuy* _Target = NULL;
	void* _Data;

	luaL_checktype(_State, 3, LUA_TTABLE);
	lua_pushstring(_State, "__self");
	lua_rawget(_State, 3);
	if(lua_isnil(_State, 3) == 1)
		return luaL_error(_State, "BigGuy:SetAction's 3rd argument is not an object");
	_Target = LuaCheckClass(_State, 3, "BigGuy");
	_Data = lua_touserdata(_State, 4);
	BigGuySetAction(_Guy, _Action, _Target, _Data);
	return 0;
}

int LuaBGImproveRelationTarget(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy->Action.Type == BGACT_IMRPOVEREL) {
		LuaCtor(_State, "BigGuy", _Guy->Action.Target);
	} else {
		return luaL_error(_State, "ImproveRelationTarget argument #1 is not improving relations.");
	}
	return 1;
}

int LuaBGKill(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	DestroyBigGuy(_Guy);
	return 0;
}

int LuaBGRelationGetOpinion(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, 1, "BigGuyRelation");

	lua_pushinteger(_State, _Relation->Modifier);
	return 1;
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

int LuaSettlementGetGovernment(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	LuaCtor(_State, "Government", _Settlement->Government);
	return 1;
}

#include "World.h"

int LuaSettlementRaiseArmy(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");
	struct ArmyGoal _Goal;
	struct Army* _Army = NULL;
	struct Settlement* _Raid = NULL;

	if(g_GameWorld.Settlements.Front->Data == _Settlement)
		_Raid = g_GameWorld.Settlements.Back->Data;
	else
		_Raid = g_GameWorld.Settlements.Front->Data;
	_Army = CreateArmy(_Settlement, (struct SDL_Point*)&_Settlement->Pos, _Settlement->Government, _Settlement->Government->Leader, ArmyGoalRaid(&_Goal, _Raid));
	LuaCtor(_State, "Army", _Army);
	return 1;
}

int LuaSettlementGetPopulation(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	lua_pushinteger(_State, _Settlement->NumPeople);
	return 1;
}

int LuaSettlementCountWarriors(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	lua_pushinteger(_State, SettlementCountWarriors(_Settlement));
	return 1;
}

int LuaSettlementGetBigGuys(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	LuaCtor(_State, "LinkedList", &_Settlement->BigGuys);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "BigGuy");
	lua_rawset(_State, -3);
	return 1;
}
