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
extern const struct LuaEnumReg g_LuaSettlementEnums[];

int LuaArmyGetLeader(lua_State* _State);
int LuaArmyGetSize(lua_State* _State);

int LuaCrisisGetDefender(lua_State* State);
int LuaCrisisGetOffender(lua_State* State);
int LuaCrisisType(lua_State* State);
int LuaBGGetPerson(lua_State* _State);
int LuaBGGetCombat(lua_State* _State);
int LuaBGGetStrength(lua_State* _State);
int LuaBGGetToughness(lua_State* _State);
int LuaBGGetAgility(lua_State* _State);
int LuaBGGetWit(lua_State* _State);
int LuaBGGetCharisma(lua_State* _State);
int LuaBGGetIntelligence(lua_State* _State);
int LuaBGOpposedChallange(lua_State* _State);
int LuaBGGetAgent(lua_State* _State);
int LuaBGGetRelation(lua_State* _State);
int LuaBGRelationItr(lua_State* _State);
int LuaBGAddOpinion(lua_State* _State);
int LuaBGSetAction(lua_State* _State);
int LuaBGGetSettlement(lua_State* _State);
int LuaBGGetFamily(lua_State* _State);
int LuaBGGetName(lua_State* _State);
int LuaBGKill(lua_State* _State);
int LuaBGPopularity(lua_State* _State);
int LuaBGChangePopularity(lua_State* _State);
int LuaBGGlory(lua_State* _State);
int LuaBGChangeGlory(lua_State* _State);
int LuaBGSuccessMargin(lua_State* _State);
int LuaBGPlotsAgainst(lua_State* _State);
int LuaBGHasTrait(lua_State* _State);
int LuaBGMurder(lua_State* _State);
int LuaBGGetFaction(lua_State* State);
int LuaBGCrisis(lua_State* State);
int LuaBGTraitList(lua_State* _State);
//int LuaBGRecruit(lua_State* _State);
//int LuaBGIsRecruiting(lua_State* _State);
//int LuaBGRetinueSize(lua_State* _State);
//int LuaBGRetinueTable(lua_State* _State);

int LuaBGOpinionAction(lua_State* _State);
int LuaBGOpinionRelation(lua_State* _State);

int LuaBGRelationGetOpinion(lua_State* _State);
int LuaRelationChangeOpinion(lua_State* State);
int LuaBGRelationGetRelationList(lua_State* _State);
int LuaBGRelationBigGuy(lua_State* _State);

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
int LuaBGBulletinPost(lua_State* _State);
int LuaSettlementGetBulletins(lua_State* _State);
int LuaSettlementCountAdults(lua_State* _State);
int LuaSettlementGetFreeWarriors(lua_State* _State);
int LuaSettlementGetMaxWarriors(lua_State* _State);
int LuaSettlementMaleAdults(lua_State* _State);
int LuaSettlementFemaleAdults(lua_State* _State);
int LuaSettlementGetCombat(lua_State* _State);
int LuaSettlementGetStrength(lua_State* _State);
int LuaSettlementGetToughness(lua_State* _State);
int LuaSettlementGetAgility(lua_State* _State);
int LuaSettlementGetWit(lua_State* _State);
int LuaSettlementGetCharisma(lua_State* _State);
int LuaSettlementGetIntelligence(lua_State* _State);
int LuaSettlementGetHappiness(lua_State* _State);
int LuaSettlementSetHappiness(lua_State* _State);
int LuaSettlementGetFine(lua_State* State);

int LuaBulletinNext(lua_State* _State);
int LuaBulletinPrev(lua_State* _State);
int LuaBulletinNextItr(lua_State* _State);
int LuaBulletinPrevItr(lua_State* _State);
int LuaBulletinGetOwner(lua_State* _State);
int LuaBulletinGetDaysRemaining(lua_State* _State);
int LuaBulletinGetName(lua_State* _State);
int LuaBulletinGetMission(lua_State* _State);

int LuaPlotActionDescribe(lua_State* _State);
int LuaPlotActionGetType(lua_State* _State);

int LuaPlotCreate(lua_State* _State);
int LuaPlotJoin(lua_State* _State);
int LuaPlotInPlot(lua_State* _State);
int LuaPlotPlotters(lua_State* _State);
int LuaPlotDefenders(lua_State* _State);
int LuaPlotTypeStr(lua_State* _State);
int LuaPlotLeader(lua_State* _State);
int LuaPlotTarget(lua_State* _State);
int LuaPlotGetScore(lua_State* _State);
int LuaPlotAddAction(lua_State* _State);
int LuaPlotGetThreat(lua_State* _State);
int LuaPlotPrevMonthActions(lua_State* _State);
int LuaPlotCurrMonthActions(lua_State* _State);
int LuaPlotHasStarted(lua_State* _State);
int LuaPlotStart(lua_State* _State);

int LuaPolicyOptionName(lua_State* _State);
int LuaPolicyOptionDescription(lua_State* State);

int LuaPolicyName(lua_State* _State);
int LuaPolicyDescription(lua_State* State);
int LuaPolicyCategory(lua_State* _State);
int LuaPolicyOptions(lua_State* _State);

int LuaPolicyOptionName(lua_State* _State);

int LuaRetinueLeader(lua_State* _State);
int LuaRetinueWarriors(lua_State* _State);

int LuaBGTraitName(lua_State* _State);
#endif
