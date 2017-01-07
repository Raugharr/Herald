/**
 * Author: David Brotz
 * File: LuaGovernment.h
 */

#ifndef __LUAGOVERNMENT_H
#define __LUAGOVERNMENT_H

#include "sys/LuaCore.h"

extern const struct LuaObjectReg g_LuaGovernmentObjects[];
extern const struct LuaEnumReg g_LuaGovernmentEnums[];

typedef struct lua_State lua_State;
typedef struct luaL_Reg luaL_Reg;

int LuaGovernmentStructure(lua_State* State);
int LuaGovernmentType(lua_State* State);
int LuaGovernmentRule(lua_State* State);
int LuaGovernmentGetLeader(lua_State* State);
int LuaGovernmentGetJudge(lua_State* State);
int LuaGovernmentGetMarshall(lua_State* State);
int LuaGovernmentGetSteward(lua_State* State);
int LuaGovernmentHasPolicy(lua_State* State);
int LuaGovernmentGetPolicyCategory(lua_State* State);
int LuaGovernmentGetTaxRate(lua_State* State);
int LuaGovernmentPolicyGetPolicyApproval(lua_State* State);

int LuaFactionGetName(lua_State* State);
int LuaFactionGetLeader(lua_State* State);
int LuaFactionGetPower(lua_State* State);
int LuaFactionGetPowerGain(lua_State* State);
int LuaFactionListGoals(lua_State* State);
int LuaFactionSetGoal(lua_State* State);
int LuaFactionGetMembers(lua_State* State);
int LuaFactionSetBet(lua_State* State);
int LuaFactionGetBet(lua_State* State);
int LuaFactionActive(lua_State* State);
int LuaFactionVoteFor(lua_State* State);
int LuaFactionCanPassGoal(lua_State* State);
int LuaFactionPolicyInfluence(lua_State* State);
int LuaFactionSetPolicyInfluence(lua_State* State);
int LuaFactionInfluenceCost(lua_State* State);
int LuaFactionGetCastePower(lua_State* State);
int LuaFactionGetCasteWeight(lua_State* State);

#endif
