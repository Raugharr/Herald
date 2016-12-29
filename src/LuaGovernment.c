/**
 * Author: David Brotz
 * File: LuaGovernemnt.c
 */

#include "LuaGovernment.h"

#include "World.h"
#include "Faction.h"
#include "Government.h"
#include "Policy.h"

#include "sys/LuaCore.h"
#include "sys/Log.h"

#include <lua/lauxlib.h>
#include <lua/lualib.h>

static const luaL_Reg g_LuaFuncsGovernment[] = {
	{"Structure", LuaGovernmentStructure},
	{"Type", LuaGovernmentType},
	{"Rule", LuaGovernmentRule},
	{"GetLeader", LuaGovernmentGetLeader},
	{"GetJudge", LuaGovernmentGetJudge},
	{"GetMarshall", LuaGovernmentGetMarshall},
	{"GetSteward", LuaGovernmentGetSteward},
	{"HasPolicy", LuaGovernmentHasPolicy},
	{"GetPolicyCategory", LuaGovernmentGetPolicyCategory},
	{"GetTaxRate", LuaGovernmentGetTaxRate},
	{"PolicyApproval", LuaGovernmentPolicyGetPolicyApproval},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsFaction[] = {
	{"GetName", LuaFactionGetName},
	{"GetLeader", LuaFactionGetLeader},
	{"GetPower", LuaFactionGetPower},
	{"GetPowerGain", LuaFactionGetPowerGain},
	{"ListGoals", LuaFactionListGoals},
	{"SetGoal", LuaFactionSetGoal},
	{"GetMembers", LuaFactionGetMembers},
	{"SetBet", LuaFactionSetBet},
	{"GetBet", LuaFactionGetBet},
	{"Active", LuaFactionActive},
	{"VoteFor", LuaFactionVoteFor},
	{"CanPassGoal", LuaFactionCanPassGoal},
	{"InfluencePolicy", LuaFactionPolicyInfluence},
	{"SetPolicyInfluence", LuaFactionSetPolicyInfluence},
	{"InfluenceCost", LuaFactionInfluenceCost},
	{"GetCastePower", LuaFactionGetCastePower},
	{"GetCasteWeight", LuaFactionGetCasteWeight},
	{NULL, NULL}
};

const struct LuaObjectReg g_LuaGovernmentObjects[] = {
	{LOBJ_GOVERNMENT, "Government", LUA_REFNIL, g_LuaFuncsGovernment},
	{LOBJ_FACTION, "Faction", LUA_REFNIL, g_LuaFuncsFaction},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

int LuaGovernmentStructure(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);

	lua_pushstring(State, GovernmentTypeToStr(Government->GovType, GOVTYPE_MASK));
	return 1;
}

int LuaGovernmentType(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);

	lua_pushstring(State, GovernmentTypeToStr(Government->GovType, GOVSTCT_MASK));
	return 1;
}

int LuaGovernmentRule(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);

	lua_pushstring(State, GovernmentTypeToStr(Government->GovType, GOVRULE_MASK));
	return 1;
}

int LuaGovernmentGetLeader(lua_State* State) {
	struct Government* Gov = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);

	LuaCtor(State, Gov->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaGovernmentGetJudge(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);
	
	LuaCtor(State, Government->Appointments.Judge, LOBJ_BIGGUY);
	return 1;
}

int LuaGovernmentGetMarshall(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);
	
	LuaCtor(State, Government->Appointments.Marshall, LOBJ_BIGGUY);
	return 1;
}

int LuaGovernmentGetSteward(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);
	
	LuaCtor(State, Government->Appointments.Steward, LOBJ_BIGGUY);
	return 1;
}

int LuaGovernmentHasPolicy(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);
	struct Policy* Policy = LuaCheckClass(State, 2, LOBJ_POLICY);

	lua_pushboolean(State, GovernmentHasPolicy(Government, Policy));
	return 1;
}

int LuaGovernmentGetPolicyCategory(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);
	struct Policy* Policy = LuaCheckClass(State, 2, LOBJ_POLICY);
	struct ActivePolicy* ActPol = NULL;
	int Category = luaL_checkinteger(State, 3) - 1;

	if(Category < 0 || Category >= POLICY_SUBSZ) 
		return luaL_error(State, "Invalid input %d for category.", Category);
	for(struct LnkLst_Node* Itr = Government->PolicyList.Front; Itr != NULL; Itr = Itr->Next) {
		ActPol = Itr->Data;
		if(ActPol->Policy != Policy)
			continue;
		lua_pushinteger(State, ActPol->OptionSel[Category] + 1);
		return 1;	
	}
	lua_pushinteger(State, -1);
	return 1;
}

int LuaGovernmentGetTaxRate(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);

	lua_pushinteger(State, Government->TaxRate);
	return 1;
}

int LuaGovernmentPolicyGetPolicyApproval(lua_State* State) {
	struct Government* Government = LuaCheckClass(State, 1, LOBJ_GOVERNMENT);
	struct Policy* Policy = LuaCheckClass(State, 2, LOBJ_POLICY);

	lua_pushinteger(State, Government->PolicyPop[Policy->Id]);
	return 1;
}

int8_t LuaFactionGetIdeology(lua_State* State, int Idx) {
	int8_t Return = 0;

	lua_pushstring(State, "Ideology");
	lua_rawget(State, Idx);
	if(lua_type(State, -1) != LUA_TNUMBER)
		Return = luaL_checkinteger(State, Idx + 1);
	else
		Return = lua_tointeger(State, -1);
	lua_pop(State, 1);
	return Return;
}

int LuaFactionGetName(lua_State* State) {
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushstring(State, g_FactionNames[Ideology]);
	return 1;
}

int LuaFactionGetLeader(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	LuaCtor(State, Faction->Leader[Ideology], LOBJ_BIGGUY);
	return 1;
}

int LuaFactionGetPower(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushinteger(State, Faction->Power[Ideology]);
	return 1;
}

int LuaFactionGetPowerGain(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushinteger(State, Faction->PowerGain[Ideology]);
	return 1;
}

int LuaFactionListGoals(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);
	lua_createtable(State, FACTION_GSIZE, 0);
	
	for(int i = 0; i < FACTION_GSIZE; ++i) {
		if(FactionValGoal(Faction, Ideology, i) == false)
			continue;
		lua_pushstring(State, g_FactionGoalNames[i]);
		lua_rawseti(State, 2, i + 1);
	}
	return 1;
}

int LuaFactionSetGoal(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	FactionSetGoal(Faction, Ideology, luaL_checkinteger(State, 2));
	return 0;
}

int LuaFactionGetMembers(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushinteger(State, Faction->Mob[Ideology].Size + Faction->Bosses[Ideology].Size + 1);
	return 1;
}

int LuaFactionSetBet(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);
	int Bet = luaL_checkinteger(State, 2);

	FactionBet(Faction, Ideology, Bet);
	return 0;
}

int LuaFactionGetBet(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);
	
	lua_pushinteger(State, Faction->FactionBet[Ideology]);
	return 1;
}

int LuaFactionActive(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);

	lua_createtable(State, FACTION_IDSIZE, 0);
	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		if(FactionIsActive(Faction, i) == true) {
			lua_pushinteger(State, i);
			lua_rawseti(State, 2, i + 1);	
		}
	}
	return 1;
}

int LuaFactionVoteFor(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushboolean(State, (Faction->DidOppose[Ideology] == false));
	return 1;
}

int LuaFactionCanPassGoal(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushboolean(State, Faction->LastGoal[Ideology] == 0 || DaysBetween(Faction->LastGoal[Ideology], g_GameWorld.Date) < YEAR_DAYS);
	return 1;
}

int LuaFactionPolicyInfluence(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushinteger(State, Faction->PolicyInfluence[Ideology]);
	return 1;
}

int LuaFactionSetPolicyInfluence(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	struct Policy* Policy = LuaCheckClass(State, 2, LOBJ_POLICY);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	Faction->PolicyInfluence[Ideology] = Policy->Id;
	return 0;
}

int LuaFactionInfluenceCost(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);

	lua_pushinteger(State, FactionInfluenceCost(Faction, Ideology));
	return 1;

};

int LuaFactionGetCastePower(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);

	lua_createtable(State, CASTE_SIZE, 0);
	for(int i = 0; i < CASTE_SIZE; ++i) {
		lua_pushinteger(State, Faction->CastePower[i]);
		lua_rawseti(State, 2, i + 1);
	}
	return 1;
}

int LuaFactionGetCasteWeight(lua_State* State) {
	struct Faction* Faction = LuaCheckClass(State, 1, LOBJ_FACTION);
	int8_t Ideology = LuaFactionGetIdeology(State, 1);
	uint32_t CastePercent[CASTE_SIZE] = {0};//How many people of this caste will choose Ideology.
	uint32_t CasteMax[CASTE_SIZE] = {0};//Sum of CastePercent for every Ideology.

	lua_createtable(State, CASTE_SIZE, 0);
	for(int i = 0; i < CASTE_SIZE; ++i) {
		CastePercent[i] += Faction->FactionWeight[FactionCasteIdx(Ideology, i)];
		for(int j = 0; j < FACTION_IDSIZE; ++j) {
			if(FactionIsActive(Faction, j) == false)
				continue;
			CasteMax[i] += Faction->FactionWeight[FactionCasteIdx(j, i)];
		}
		lua_pushinteger(State, CastePercent[i] * 100 / CasteMax[i]);
		lua_rawseti(State, 2, i + 1);
	}
	return 1;
}