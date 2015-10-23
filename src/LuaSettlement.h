/*
 * File: LuaSettlement.h
 * Author: David Brotz
 */

#ifndef __LUASETTLEMENT_H
#define __LUASETTLEMENT_H

#include "sys/LuaCore.h"

typedef struct lua_State lua_State;
typedef struct luaL_Reg luaL_Reg;
typedef int (*lua_CFunction) (lua_State *L);

extern const struct LuaObjectReg g_LuaSettlementObjects[];

int LuaArmyGetLeader(lua_State* _State);
int LuaArmyGetSize(lua_State* _State);

int LuaBGGetPerson(lua_State* _State);
int LuaBGGetAuthority(lua_State* _State);
int LuaBGSetAuthority(lua_State* _State);
int LuaBGGetPrestige(lua_State* _State);
int LuaBGSetPrestige(lua_State* _State);
int LuaBGGetAdministration(lua_State* _State);
int LuaBGGetIntrigue(lua_State* _State);
int LuaBGGetStrategy(lua_State* _State);
int LuaBGGetWarfare(lua_State* _State);
int LuaBGGetTactics(lua_State* _State);
int LuaBGGetCharisma(lua_State* _State);
int LuaBGGetPiety(lua_State* _State);
int LuaBGGetIntellegence(lua_State* _State);
int LuaBGGetAgent(lua_State* _State);
int LuaBGGetRelation(lua_State* _State);
int LuaBGSetOpinion(lua_State* _State);
int LuaBGSetAction(lua_State* _State);
int LuaBGImproveRelationTarget(lua_State* _State);

int LuaBGRelationGetOpinion(lua_State* _State);

int LuaGovernmentPossibleReforms(lua_State* _State);
int LuaGovernmentStructure(lua_State* _State);
int LuaGovernmentType(lua_State* _State);
int LuaGovernmentRule(lua_State* _State);
int LuaGovernmentPassReform(lua_State* _State);
int LuaGovernmentGetReform(lua_State* _State);

int LuaReformPassingGetVotes(lua_State* _State);
int LuaReformPassingGetMaxVotes(lua_State* _State);

int LuaReformGetName(lua_State* _State);

int LuaSettlementGetLeader(lua_State* _State);
int LuaSettlementGetGovernment(lua_State* _State);
int LuaSettlementRaiseArmy(lua_State* _State);
int LuaSettlementGetPopulation(lua_State* _State);
int LuaSettlementCountWarriors(lua_State* _State);
int LuaSettlementGetBigGuys(lua_State* _State);

#endif
