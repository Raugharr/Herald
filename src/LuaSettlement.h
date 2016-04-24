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
int LuaBGOpposedChallanged(lua_State* _State);
int LuaBGGetAgent(lua_State* _State);
int LuaBGGetRelation(lua_State* _State);
int LuaBGSetOpinion(lua_State* _State);
int LuaBGSetAction(lua_State* _State);
int LuaBGImproveRelationTarget(lua_State* _State);
int LuaBGGetSettlement(lua_State* _State);
int LuaBGGetFamily(lua_State* _State);
int LuaBGGetName(lua_State* _State);
int LuaBGKill(lua_State* _State);

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
int LuaSettlementGetNutrition(lua_State* _State);
int LuaSettlementYearlyNutrition(lua_State* _State);
int LuaSettlementCountAcres(lua_State* _State);
int LuaSettlementExpectedYield(lua_State* _State);
int LuaSettlementYearlyDeaths(lua_State* _State);
int LuaSettlementYearlyBirths(lua_State* _State);
int LuaSettlementBulitinPost(lua_State* _State);
int LuaSettlementGetBulitins(lua_State* _State);

int LuaBulitinNext(lua_State* _State);
int LuaBulitinPrev(lua_State* _State);
int LuaBulitinNextItr(lua_State* _State);
int LuaBulitinPrevItr(lua_State* _State);
int LuaBulitinGetOwner(lua_State* _State);
int LuaBulitinGetDaysRemaining(lua_State* _State);
int LuaBulitinGetName(lua_State* _State);
int LuaBulitinGetMission(lua_State* _State);

#endif
