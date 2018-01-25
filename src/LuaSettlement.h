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

int LuaArmyGetLeader(lua_State* State);
int LuaArmyGetSize(lua_State* State);
int LuaArmyGetWarbands(lua_State* State);

int LuaCrisisGetDefender(lua_State* State);
int LuaCrisisGetOffender(lua_State* State);
int LuaCrisisType(lua_State* State);

int LuaMarketGood(lua_State* State);
int LuaMarketQuantity(lua_State* State);
int LuaMarketPrice(lua_State* State);

int LuaBGGetPerson(lua_State* State);
int LuaBGGetCombat(lua_State* State);
int LuaBGGetStrength(lua_State* State);
int LuaBGGetToughness(lua_State* State);
int LuaBGGetAgility(lua_State* State);
int LuaBGGetWit(lua_State* State);
int LuaBGGetCharisma(lua_State* State);
int LuaBGGetIntelligence(lua_State* State);
int LuaBGOpposedChallange(lua_State* State);
int LuaBGGetAgent(lua_State* State);
int LuaBGGetRelation(lua_State* State);
int LuaBGRelationItr(lua_State* State);
int LuaBGAddOpinion(lua_State* State);
int LuaBGSetAction(lua_State* State);
int LuaBGGetSettlement(lua_State* State);
int LuaBGGetFamily(lua_State* State);
int LuaBGGetName(lua_State* State);
int LuaBGKill(lua_State* State);
int LuaBGPopularity(lua_State* State);
int LuaBGChangePopularity(lua_State* State);
int LuaBGGlory(lua_State* State);
int LuaBGChangeGlory(lua_State* State);
int LuaBGSuccessMargin(lua_State* State);
int LuaBGPlotsAgainst(lua_State* State);
int LuaBGHasTrait(lua_State* State);
int LuaBGMurder(lua_State* State);
int LuaBGGetFaction(lua_State* State);
int LuaBGCrisis(lua_State* State);
int LuaBGTraitList(lua_State* State);
//int LuaBGRecruit(lua_State* State);
//int LuaBGIsRecruiting(lua_State* State);
//int LuaBGRetinueSize(lua_State* State);
//int LuaBGRetinueTable(lua_State* State);

int LuaBGOpinionAction(lua_State* State);
int LuaBGOpinionRelation(lua_State* State);

int LuaBGRelationGetOpinion(lua_State* State);
int LuaRelationChangeOpinion(lua_State* State);
int LuaBGRelationGetRelationList(lua_State* State);
int LuaBGRelationBigGuy(lua_State* State);

int LuaSettlementGetName(lua_State* State);
int LuaSettlementGetLeader(lua_State* State);
int LuaSettlementGetGovernment(lua_State* State);
int LuaSettlementRaiseArmy(lua_State* State);
int LuaSettlementGetPopulation(lua_State* State);
int LuaSettlementCountWarriors(lua_State* State);
int LuaSettlementGetBigGuys(lua_State* State);
int LuaSettlementGetNutrition(lua_State* State);
int LuaSettlementYearlyNutrition(lua_State* State);
int LuaSettlementCountAcres(lua_State* State);
int LuaSettlementExpectedYield(lua_State* State);
int LuaSettlementYearlyDeaths(lua_State* State);
int LuaSettlementYearlyBirths(lua_State* State);
int LuaBGBulletinPost(lua_State* State);
int LuaSettlementGetBulletins(lua_State* State);
int LuaSettlementCountAdults(lua_State* State);
int LuaSettlementGetFreeWarriors(lua_State* State);
int LuaSettlementGetMaxWarriors(lua_State* State);
int LuaSettlementMaleAdults(lua_State* State);
int LuaSettlementFemaleAdults(lua_State* State);
int LuaSettlementGetCombat(lua_State* State);
int LuaSettlementGetStrength(lua_State* State);
int LuaSettlementGetToughness(lua_State* State);
int LuaSettlementGetAgility(lua_State* State);
int LuaSettlementGetWit(lua_State* State);
int LuaSettlementGetCharisma(lua_State* State);
int LuaSettlementGetIntelligence(lua_State* State);
int LuaSettlementGetFine(lua_State* State);
int LuaSettlementGetType(lua_State* State);
int LuaSettlementSelling(lua_State* State);
int LuaSettlementCasteCount(lua_State* State);
int LuaSettlementProfessionCount(lua_State* State);

int LuaBulletinNext(lua_State* State);
int LuaBulletinPrev(lua_State* State);
int LuaBulletinNextItr(lua_State* State);
int LuaBulletinPrevItr(lua_State* State);
int LuaBulletinGetOwner(lua_State* State);
int LuaBulletinGetDaysRemaining(lua_State* State);
int LuaBulletinGetName(lua_State* State);
int LuaBulletinGetMission(lua_State* State);

int LuaPlotActionDescribe(lua_State* State);
int LuaPlotActionGetType(lua_State* State);

int LuaPlotCreate(lua_State* State);
int LuaPlotJoin(lua_State* State);
int LuaPlotInPlot(lua_State* State);
int LuaPlotPlotters(lua_State* State);
int LuaPlotDefenders(lua_State* State);
int LuaPlotTypeStr(lua_State* State);
int LuaPlotLeader(lua_State* State);
int LuaPlotTarget(lua_State* State);
int LuaPlotGetScore(lua_State* State);
int LuaPlotAddAction(lua_State* State);
int LuaPlotGetThreat(lua_State* State);
int LuaPlotPrevMonthActions(lua_State* State);
int LuaPlotCurrMonthActions(lua_State* State);
int LuaPlotHasStarted(lua_State* State);
int LuaPlotStart(lua_State* State);

int LuaPolicyOptionName(lua_State* State);
int LuaPolicyOptionDescription(lua_State* State);

int LuaPolicyName(lua_State* State);
int LuaPolicyDescription(lua_State* State);
int LuaPolicyCategory(lua_State* State);
int LuaPolicyOptions(lua_State* State);

int LuaPolicyOptionName(lua_State* State);

int LuaRetinueLeader(lua_State* State);
int LuaRetinueWarriors(lua_State* State);
int LuaRetinueBranch(lua_State* State);
int LuaRetinueTroopCount(lua_State* State);

int LuaBGTraitName(lua_State* State);
#endif
