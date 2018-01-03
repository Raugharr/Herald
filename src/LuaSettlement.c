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
#include "Family.h"
#include "Person.h"
#include "Mission.h"
#include "Bulletin.h"
#include "Plot.h"
#include "Policy.h"
#include "Trait.h"
#include "Relation.h"
#include "Crisis.h"
#include "Profession.h"
#include "Market.h"

#include "sys/LuaCore.h"
#include "sys/Log.h"
#include "sys/FrameAllocator.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <malloc.h>

static luaL_Reg g_LuaFuncsArmy[] = {
	{"GetLeader", LuaArmyGetLeader},
	{"GetSize", LuaArmyGetSize},
	{"GetWarbands", LuaArmyGetWarbands},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsCrisis[] = {
	{"GetDefender", LuaCrisisGetDefender},
	{"GetOffender", LuaCrisisGetOffender},
	{"Type", LuaCrisisType},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsMarket[] = {
	{"Good", LuaMarketGood},
	{"Quantity", LuaMarketQuantity},
	{"Price", LuaMarketPrice},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuy[] = {
	{"GetPerson", LuaBGGetPerson},
	{"GetCombat", LuaBGGetCombat},
	{"GetStrength", LuaBGGetStrength},
	{"GetToughness", LuaBGGetToughness},
	{"GetAgility", LuaBGGetAgility},
	{"GetWit", LuaBGGetWit},
	{"GetCharisma", LuaBGGetCharisma},
	{"GetIntelligence", LuaBGGetIntelligence},
	{"OpposedChallange", LuaBGOpposedChallange},
	{"GetAgent", LuaBGGetAgent},
	{"GetRelation", LuaBGGetRelation},
	{"RelationsItr", LuaBGRelationItr},
	{"AddOpinion", LuaBGAddOpinion},
	{"SetAction", LuaBGSetAction},
	{"GetSettlement", LuaBGGetSettlement},
	{"GetFamily", LuaBGGetFamily},
	{"GetName", LuaBGGetName},
	{"Kill", LuaBGKill},
	{"Popularity", LuaBGPopularity},
	{"ChangePopularity", LuaBGChangePopularity},
	{"Glory", LuaBGGlory},
	{"ChangeGlory", LuaBGChangeGlory},
	{"SuccessMargin", LuaBGSuccessMargin},
	{"PlotsAgainst", LuaBGPlotsAgainst},
	{"HasTrait", LuaBGHasTrait},
	{"TraitList", LuaBGTraitList},
	{"Murder", LuaBGMurder},
	{"GetFaction", LuaBGGetFaction},
	{"Crisis", LuaBGCrisis},
	{"BulletinPost", LuaBGBulletinPost},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsOpinion[] = {
	{"Action", LuaBGOpinionAction},
	{"Relation", LuaBGOpinionRelation},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsRelation[] = {
	{"GetOpinion", LuaBGRelationGetOpinion},
	{"ChanceOpinion", LuaRelationChangeOpinion},
	{"GetRelationList", LuaBGRelationGetRelationList},
	{"BigGuy", LuaBGRelationBigGuy},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsSettlement[] = {
	{"GetName", LuaSettlementGetName},
	{"GetLeader", LuaSettlementGetLeader},
	{"GetGovernment", LuaSettlementGetGovernment},
	{"GetPopulation", LuaSettlementGetPopulation},
	{"CountWarriors", LuaSettlementCountWarriors},
	{"GetBigGuys", LuaSettlementGetBigGuys},
	{"GetNutrition", LuaSettlementGetNutrition},
	{"GetYearlyNutrition", LuaSettlementYearlyNutrition},
	{"CountAcres", LuaSettlementCountAcres},
	{"ExpectedYield", LuaSettlementExpectedYield},
	{"YearlyDeaths", LuaSettlementYearlyDeaths},
	{"YearlyBirths", LuaSettlementYearlyBirths},
	{"GetBulletins", LuaSettlementGetBulletins},
	{"CountAdults", LuaSettlementCountAdults},
	{"GetFreeWarriors", LuaSettlementGetFreeWarriors},
	{"GetMaxWarriors", LuaSettlementGetMaxWarriors},
	{"MaleAdults", LuaSettlementMaleAdults},
	{"FemaleAdults", LuaSettlementFemaleAdults},
	{"GetCombat", LuaSettlementGetCombat},
	{"GetStrength", LuaSettlementGetStrength},
	{"GetToughness", LuaSettlementGetToughness},
	{"GetAgility", LuaSettlementGetAgility},
	{"GetWit", LuaSettlementGetWit},
	{"GetCharisma", LuaSettlementGetCharisma},
	{"GetIntelligence", LuaSettlementGetIntelligence},
	{"GetFine", LuaSettlementGetFine},
	{"GetType", LuaSettlementGetType},
	{"Buying", LuaSettlementBuying},
	{"Selling", LuaSettlementSelling},
	{"CasteCount", LuaSettlementCasteCount},
	{"ProfessionCount", LuaSettlementProfessionCount},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBulletin[] = {
	{"GetOwner", LuaBulletinGetOwner},
	{"DaysLeft", LuaBulletinGetDaysRemaining},
	{"GetName", LuaBulletinGetName},
	{"GetMission", LuaBulletinGetMission},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsPlotAction[] = {
	{"Describe", LuaPlotActionDescribe},
	{"GetType", LuaPlotActionGetType},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsPlot[] = {
	{"Create", LuaPlotCreate},
	{"Join", LuaPlotJoin},
	{"InPlot", LuaPlotInPlot},
	{"Plotters", LuaPlotPlotters},
	{"Defenders", LuaPlotDefenders},
	{"TypeStr", LuaPlotTypeStr},
	{"Leader", LuaPlotLeader},
	{"Target", LuaPlotTarget},
	{"GetScore", LuaPlotGetScore},
	{"AddAction", LuaPlotAddAction},
	{"GetThreat", LuaPlotGetThreat},
	{"PrevMonthActions", LuaPlotPrevMonthActions},
	{"CurrMonthActions", LuaPlotCurrMonthActions},
	{"HasStarted", LuaPlotHasStarted},
	{"Start", LuaPlotStart},
	{NULL, NULL}
};

static luaL_Reg g_LuaFuncsPolicy[] = {
	{"GetName", LuaPolicyName},
	{"GetDescription", LuaPolicyDescription},
	{"Category", LuaPolicyCategory},
	{"Options", LuaPolicyOptions},
	{NULL, NULL}
};

static luaL_Reg g_LuaFuncsPolicyOption[] = {
	{"GetName", LuaPolicyOptionName},
	{"GetDescription", LuaPolicyOptionDescription},
	{NULL, NULL}
};

static luaL_Reg g_LuaFuncsRetinue[] = {
	{"Leader", LuaRetinueLeader},
	{"Warriors", LuaRetinueWarriors},
	{"Branch", LuaRetinueBranch}, 
	{"TroopCount", LuaRetinueTroopCount},
	{NULL, NULL}
};

static luaL_Reg g_LuaFuncsBigGuyTrait[] = {
	{"GetName", LuaBGTraitName},
	{NULL, NULL}
};

const struct LuaObjectReg g_LuaSettlementObjects[] = {
	{LOBJ_ARMY, "Army", LUA_REFNIL, g_LuaFuncsArmy},
	{LOBJ_BIGGUY, "BigGuy", LUA_REFNIL, g_LuaFuncsBigGuy},
	{LOBJ_RELATION, "Relation", LUA_REFNIL, g_LuaFuncsRelation},
	{LOBJ_SETTLEMENT, "Settlement", LUA_REFNIL, g_LuaFuncsSettlement},
	{LOBJ_BUILDMAT, "BuildMat", LUA_REFNIL, NULL},
	{LOBJ_BULLETIN, "Bulletin", LUA_REFNIL, g_LuaFuncsBulletin},
	{LOBJ_PLOTACTION, "PlotAction", LUA_REFNIL, g_LuaFuncsPlotAction},
	{LOBJ_PLOT, "Plot", LUA_REFNIL, g_LuaFuncsPlot},
	{LOBJ_POLICY, "Policy", LUA_REFNIL, g_LuaFuncsPolicy},
	{LOBJ_POLICYOPTION, "PolicyOption", LUA_REFNIL, g_LuaFuncsPolicyOption},
	{LOBJ_BIGGUYOPINION, "Opinion", LUA_REFNIL, g_LuaFuncsOpinion},
	{LOBJ_RETINUE, "Retinue", LUA_REFNIL, g_LuaFuncsRetinue},
	{LOBJ_BIGGUYTRAIT, "Trait", LUA_REFNIL, g_LuaFuncsBigGuyTrait},
	{LOBJ_MARREQ, "MarReq", LUA_REFNIL, g_LuaFuncsMarket},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

const struct LuaEnum g_LuaPlotEnum[] = {
	{"Attack", PLOTACT_ATTACK},
	{"DoubleDamage", PLOTACT_DOUBLEDMG},
	{"DoubleAttack", PLOTACT_DOUBLEATTK},
	{"StopAttack", PLOTACT_STOPATTK},
	{"LowerStats", PLOTACT_LOWERSTAT},
	{NULL, 0}
};

const struct LuaEnum g_LuaPlotTypeEnum[] = {
	{"NewPolicy", PLOT_PASSPOLICY},
	{"ChangePolicy", PLOT_CHANGEPOLICY},
	{"RemovePolicy", PLOT_REMOVEPOLICY},
	{"ControlRetinue", PLOT_CONRETINUE},
	{NULL, 0}
};

const struct LuaEnum g_LuaPolicyEnum[] = {
	{"Economy", POLCAT_ECONOMY},
	{"Law", POLCAT_LAW},
	{"Military", POLCAT_MILITARY},
	{NULL, 0}
};

const struct LuaEnum g_LuaBigGuyActionEnum[] = {
	{"Befriend", BGACT_BEFRIEND},
	{"Sabotage", BGACT_SABREL},
	{"Steal", BGACT_STEAL},
	{"Duel", BGACT_DUEL},
	{"Murder", BGACT_MURDER},
	{"Slander", BGACT_SLANDER},
	{"Size", BGACT_SIZE},
	{NULL, 0}
};

const struct LuaEnum g_LuaRelOpnEnum[] = {
	{"Token", OPINION_TOKEN},
	{"Small", OPINION_SMALL},
	{"Average", OPINION_AVERAGE},
	{"Great", OPINION_GREAT},
	{NULL, 0}
};

const struct LuaEnum g_LuaRelLengthEnum[] = {
	{"Small", OPNLEN_SMALL},
	{"Medium", OPNLEN_MEDIUM},
	{"Large", OPNLEN_LARGE},
	{"Forever", OPNLEN_FOREVER},
	{NULL, 0}
};

const struct LuaEnum g_LuaRelActionEnum[] = {
	{"Theft", ACTTYPE_THEFT},
	{"General", ACTTYPE_GENERAL},
	{NULL, 0}
};

const struct LuaEnum g_LuaStatsEnum[] = {
	{"Combat", BGSKILL_COMBAT},
	{"Strength", BGSKILL_STRENGTH},
	{"Toughness", BGSKILL_TOUGHNESS},
	{"Agility", BGSKILL_AGILITY},
	{"Wit", BGSKILL_WIT},
	{"Charisma", BGSKILL_CHARISMA},
	{"Intelligence", BGSKILL_INTELLIGENCE},
	{"Min", STAT_MIN},
	{"Max", STAT_MAX},
	{NULL, 0}
};

const struct LuaEnum g_LuaCrisisEnum[] = {
	{"Murder", CRISIS_MURDER},
	{NULL, 0}
};

const struct LuaEnumReg g_LuaSettlementEnums[] = {
	{"Plot", NULL,  g_LuaPlotEnum},
	{"Plot", "Type", g_LuaPlotTypeEnum},
	{"Action", NULL, g_LuaBigGuyActionEnum},
	{"Relation", "Opinion", g_LuaRelOpnEnum},
	{"Relation", "Length", g_LuaRelLengthEnum},
	{"Relation", "Action", g_LuaRelActionEnum},
	{"Policy", NULL, g_LuaPolicyEnum},
	{"Stat", NULL, g_LuaStatsEnum},
	{"Crisis", NULL, g_LuaCrisisEnum},
	{NULL, NULL}
};

int LuaArmyGetLeader(lua_State* State) {
	struct Army* Army = LuaCheckClass(State, 1, LOBJ_ARMY);

	LuaCtor(State, Army->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaArmyGetSize(lua_State* State) {
	struct Army* Army = LuaCheckClass(State, 1, LOBJ_ARMY);

	lua_pushinteger(State, ArmyGetSize(Army));
	return 1;
}

int LuaArmyGetWarbands(lua_State* State) {
	struct Army* Army = LuaCheckClass(State, 1, LOBJ_ARMY);

	LuaCtorArray(State, &Army->Warbands, LOBJ_WARBAND);
	return 1;
}

int LuaCrisisGetDefender(lua_State* State) {
	struct Crisis* Crisis = LuaCheckClass(State, 1, LOBJ_CRISIS);

	LuaCtor(State, Crisis->Defender, LOBJ_BIGGUY);
	return 1;
}

int LuaCrisisGetOffender(lua_State* State) {
	struct Crisis* Crisis = LuaCheckClass(State, 1, LOBJ_CRISIS);

	LuaCtor(State, Crisis->Offender, LOBJ_BIGGUY);
	return 1;
}

int LuaCrisisType(lua_State* State) {
	struct Crisis* Crisis = LuaCheckClass(State, 1, LOBJ_CRISIS);

	lua_pushinteger(State, Crisis->Type);
	return 1;
}

int LuaMarketGood(lua_State* State) {
	struct MarReq* Req = LuaCheckClass(State, 1, LOBJ_MARREQ);

	lua_pushstring(State, Req->Base->Name);
	lua_remove(State, -2);
	LuaGoodBase(State);
	return 1;
}

int LuaMarketQuantity(lua_State* State) {
	struct MarReq* Req = LuaCheckClass(State, 1, LOBJ_MARREQ);

	lua_pushinteger(State, Req->Quantity);
	return 1;
}

int LuaMarketPrice(lua_State* State) {
	struct MarReq* Req = LuaCheckClass(State, 1, LOBJ_MARREQ);

	lua_pushinteger(State, Req->Price);
	return 1;
}

int LuaCrisisSelectOption(lua_State* State) {
	struct Crisis* Crisis = LuaCheckClass(State, 1, LOBJ_CRISIS);
	int Option = luaL_checkinteger(State, 2);

	CrisisProcess(Crisis, Option);
	return 0;
}

int LuaBGGetPerson(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	LuaCtor(State, Guy->Person, LOBJ_PERSON);
	return 1;
}

int LuaBGGetCombat(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_COMBAT]);
	return 1;
}

int LuaBGGetStrength(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_STRENGTH]);
	return 1;
}

int LuaBGGetToughness(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_TOUGHNESS]);
	return 1;
}

int LuaBGGetAgility(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_AGILITY]);
	return 1;
}

int LuaBGGetWit(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_WIT]);
	return 1;
}

int LuaBGGetCharisma(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_CHARISMA]);
	return 1;
}

int LuaBGGetIntelligence(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	lua_pushinteger(State, Guy->Stats[BGSKILL_INTELLIGENCE]);
	return 1;
}

int LuaBGOpposedChallange(lua_State* State) {
	struct BigGuy* One = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Two = LuaCheckClass(State, 2, LOBJ_BIGGUY);

	if(One == NULL)
		return luaL_error(State, "Agument #1 is not a BigGuy.");
	if(Two == NULL)
		return luaL_error(State, "Agument #2 is not a BigGuy.");
	lua_pushinteger(State, BigGuyOpposedCheck(One, Two, luaL_checkinteger(State, 3)));
	return 1;
}

int LuaBGGetAgent(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Agent* Agent = RBSearch(&g_GameWorld.Agents, Guy);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	if(Agent == NULL) {
		lua_pushnil(State);
		goto end;
	}
	LuaCtor(State, Agent, LOBJ_AGENT);
	end:
	return 1;
}

int LuaBGGetRelation(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Target = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	struct Relation* Relation = GetRelation(Guy->Relations, Target);

	if(Relation == NULL) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, Relation, LOBJ_RELATION);
	return 1;
}

int LuaBGRelItrNext(lua_State* State) {
	struct Relation* Relation = LuaCheckClass(State, lua_upvalueindex(1), LOBJ_RELATION);

	if(Relation->Next == NULL) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, Relation->Next, LOBJ_RELATION);
	lua_pushvalue(State, -1);
	lua_replace(State, lua_upvalueindex(1));
	return 1;
}

int LuaBGRelationItr(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	if(Guy->Relations == NULL) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, Guy->Relations, LOBJ_RELATION); 
	lua_pushcclosure(State, LuaBGRelItrNext, 1);
	return 1;
}

int LuaBGAddOpinion(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Target = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	int Action = luaL_checkinteger(State, 3);
	int Mod = luaL_checkinteger(State, 4);
	int Length = luaL_optinteger(State, 5, OPNLEN_MEDIUM);
	int Strength = luaL_optinteger(State, 6, OPINION_AVERAGE);

	AddOpinion(Guy, Target, Action, Mod, Length, Strength, &Guy->Relations);
	return 0;
}

int LuaBGSetAction(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	int Action = luaL_checkint(State, 2);
	struct BigGuy* Target = NULL;

/*	luaL_checktype(State, 3, LUA_TTABLE);
	lua_pushstring(State, "__self");
	lua_rawget(State, 3);
	if(lua_isnil(State, 3) == 1)
		return luaL_error(State, "BigGuy:SetAction's 3rd argument is not an object");
	*/
	Target = LuaCheckClass(State, 3, LOBJ_BIGGUY);
	BigGuySetAction(Guy, Action, Target, NULL);
	return 0;
}

int LuaBGGetSettlement(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	LuaCtor(State, FamilyGetSettlement(Guy->Person->Family), LOBJ_SETTLEMENT);
	return 1;
}

int LuaBGGetFamily(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	LuaCtor(State, Guy->Person->Family, LOBJ_FAMILY);
	return 1;
}

int LuaBGGetName(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	lua_pushstring(State, Guy->Person->Name);
	return 1;
}

int LuaBGKill(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	PersonDeath(Guy->Person);
//	DestroyBigGuy(Guy);
	return 0;
}

int LuaBGPopularity(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	lua_pushinteger(State, BigGuyPopularity(Guy));
	return 1; 
}

int LuaBGChangePopularity(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	int Change = luaL_checkinteger(State, 2);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	Guy->Popularity += Change;
	return 0;
}

int LuaBGGlory(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	lua_pushinteger(State, Guy->Glory);
	return 1; 
}

int LuaBGChangeGlory(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	int Change = luaL_checkinteger(State, 2);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	Guy->Glory+= Change;
	return 0;
}

int LuaBGSuccessMargin(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	int Skill = luaL_checkinteger(State, 2);
	int ReqSkill = 0;

	if(lua_gettop(State) > 2)
		ReqSkill = luaL_checkinteger(State, 3);
	else
		ReqSkill = SKILLCHECK_DEFAULT;
	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	lua_pushinteger(State, BigGuySuccessMargin(Guy, Skill, ReqSkill));	
	return 1;
}

int LuaBGPlotsAgainst(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	CreateLuaLnkLstItr(State, &Guy->PlotsAgainst, LOBJ_PLOT);
	return 1;
}

int LuaBGHasTrait(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	const char* TraitName = NULL;
	const struct Trait* Trait = NULL;
	
	switch(lua_type(State, 2)) {
		case LUA_TSTRING:
			TraitName = luaL_checkstring(State, 2);
			Trait = HashSearch(&g_Traits, TraitName);
			if(Trait == NULL)
				goto error;
			lua_pushboolean(State, HasTrait(Guy, Trait));
			break;
		case LUA_TTABLE:
			lua_pushboolean(State, HasTrait(Guy, LuaCheckClass(State, 2, LOBJ_TRAIT)));
			break;
		default:
			error:
			return luaL_error(State, "Arg #2 is neither a trait or a trait name.");
	}

	return 1;
}

int LuaBGTraitItrNext(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, lua_upvalueindex(1), LOBJ_BIGGUY);
	int Idx = lua_tointeger(State, lua_upvalueindex(2));

	if(Guy == NULL || Idx >= Guy->TraitCt) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, Guy->Traits[Idx], LOBJ_BIGGUYTRAIT);
	lua_pushinteger(State, Idx + 1);
	lua_replace(State, lua_upvalueindex(2));
	return 1;
}

int LuaBGTraitList(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);

	if(Guy == NULL)
		return LuaClassError(State, 1, LOBJ_BIGGUY);
	if(Guy->Traits == NULL) {
		lua_pushnil(State);
		return 1;
	}
	lua_pushvalue(State, 1);
	lua_pushinteger(State, 0);
	lua_pushcclosure(State, LuaBGTraitItrNext, 2);
	return 1;
}

int LuaBGMurder(lua_State* State) {
	struct BigGuy* Murderer = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Target = LuaCheckClass(State, 2, LOBJ_BIGGUY);

	PersonDeath(Target->Person);	
	PushEvent(EVENT_MURDERED, Target, Murderer);
	return 0;
}

int LuaBGGetFaction(lua_State* State) {
	struct BigGuy* BigGuy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Family* Family = BigGuy->Person->Family;

	if(Family->Faction == FACTION_IDNONE) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, &Family->HomeLoc->Factions, LOBJ_FACTION);
	lua_pushstring(State, "Ideology");
	lua_pushinteger(State, Family->Faction);
	lua_rawset(State, -3);
	return 1;
}

int LuaBGCrisis(lua_State* State) {
	struct BigGuy* Offender = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Defender  = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	int Type = luaL_checkinteger(State, 3);
	struct Settlement* Settlement = NULL;
	struct Crisis* Crisis = NULL;

	if(Type < 0 || Type >= CRISIS_SIZE) luaL_argerror(State, 3, "Invalid type.");
	Settlement = Defender->Person->Family->HomeLoc;
	Crisis = CreateCrisis(Offender->Person, Defender->Person, Type);
	ArrayInsert_S(&Settlement->Crisis, Crisis);
	return 0;
}

int LuaBGBulletinPost(lua_State* State) {
	struct BulletinItem* Item = NULL;
	struct Mission* Mission = NULL;
	struct Mission* MissionFail = NULL;
	struct BigGuy* Poster = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	const char* MissionStr = luaL_checkstring(State, 2);
	const char* MissionFailStr = luaL_checkstring(State, 3);
	int DaysLeft = luaL_checkint(State, 4);
	int Priority = luaL_checkint(State, 5);
	
	if((Mission = MissionStrToId(MissionStr)) == NULL) {
		luaL_error(State, "%s is not a mission name.", MissionStr);
	}
	if((MissionFail = MissionStrToId(MissionFailStr)) == NULL) {
		luaL_error(State, "%s is not a mission name.", MissionFailStr);
	}
	Item = CreateBulletinItem(Mission, NULL, Poster, DaysLeft, Priority);		 
	ArrayInsert_S(&Poster->Person->Family->HomeLoc->Bulletin, Item);
	return 0;
}

/*int LuaBGRecruit(lua_State* State) {
	struct BigGuy* Leader = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Settlement* Home = PersonHome(Leader->Person);

	for(struct Retinue* Retinue = Home->Retinues; Retinue != NULL; Retinue = Retinue->Next) {
		if(Retinue->Leader != Leader)
			continue;
		Retinue->IsRecruiting = (Retinue->IsRecruiting ^ 1);
		return 0;
	}
	struct Retinue* Retinue = SettlementAddRetinue(Home, Leader);

	Retinue->IsRecruiting = (Retinue->IsRecruiting ^ 1);
	return 0;
}

int LuaBGIsRecruiting(lua_State* State) {
	struct BigGuy* Leader = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Settlement* Home = PersonHome(Leader->Person);

	for(struct Retinue* Retinue = Home->Retinues; Retinue != NULL; Retinue = Retinue->Next) {
		if(Retinue->Leader != Leader)
			continue;
		lua_pushboolean(State, Retinue->IsRecruiting);
		return 1;
	}
	lua_pushboolean(State, 0);
	return 1;
}

int LuaBGRetinueSize(lua_State* State) {
	struct BigGuy* Leader = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Retinue* GuyRetinue = IntSearch(&g_GameWorld.PersonRetinue, Leader->Person->Id);
	struct Settlement* Home = PersonHome(Leader->Person);

	Leader = GuyRetinue->Leader;
	for(struct Retinue* Retinue = Home->Retinues; Retinue != NULL; Retinue = Retinue->Next) {
		if(Retinue->Leader != Leader)
			continue;
		lua_pushinteger(State, Retinue->Warriors.Size);
		return 1;
	}
	lua_pushinteger(State, 0);
	return 1;
}*/

/*int LuaBGRetinueTable(lua_State* State) {
	struct BigGuy* Leader = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Settlement* Home = PersonHome(Leader->Person);
	struct Retinue* Retinue = Home->Retinues;

	for(; Retinue != NULL; Retinue = Retinue->Next) {
		if(Retinue->Leader != Leader)
			continue;
		goto found_retinue;
	}
	lua_createtable(State, 0, 0);
	return 1;
	found_retinue:
	lua_createtable(State, Retinue->Warriors.Size, 0);
	for(int i = 0; i < Retinue->Warriors.Size; ++i) {
		LuaCtor(State, Retinue->Warriors.Table[i], LOBJ_PERSON);
		lua_rawseti(State, -2, i + 1);
	}
	return 1;
} */

int LuaBGOpinionAction(lua_State* State) {
	struct Opinion* Opinion = LuaCheckClass(State, 1, LOBJ_BIGGUYOPINION);

	lua_pushstring(State, g_OpinionActions[Opinion->Action]);
	return 1;
}

int LuaBGOpinionRelation(lua_State* State) {
	struct Opinion* Opinion = LuaCheckClass(State, 1, LOBJ_BIGGUYOPINION);

	lua_pushinteger(State, Opinion->RelMod);
	return 1;

}

int LuaBGRelationGetOpinion(lua_State* State) {
	struct Relation* Relation = LuaCheckClass(State, 1, LOBJ_RELATION);

	if(Relation == NULL)
		return LuaClassError(State, 1, LOBJ_RELATION);
	lua_pushinteger(State, Relation->Modifier);
	return 1;
}

int LuaRelationChangeOpinion(lua_State* State) {
	struct Relation* Relation = LuaCheckClass(State, 1, LOBJ_RELATION);
	
	ChangeRelation(Relation, luaL_checkint(State, 2), luaL_checkint(State, 3), luaL_checkint(State, 4), luaL_checkint(State, 5));
	return 0;
}

int LuaBGRelationGetRelationList(lua_State* State) {
	struct Relation* Relation = LuaCheckClass(State, 1, LOBJ_RELATION);
	int Idx = 1;

	lua_createtable(State, 6, 0);	
	for(struct Opinion* Opinion = Relation->Opinions; Opinion != NULL; Opinion = Opinion->Next, ++Idx) {
		LuaCtor(State, Opinion, LOBJ_BIGGUYOPINION);
		lua_rawseti(State, -2, Idx);
	}
	return 1;
}

int LuaBGRelationBigGuy(lua_State* State) {
	struct Relation* Relation = LuaCheckClass(State, 1, LOBJ_RELATION);

	if(Relation == NULL)
		return LuaClassError(State, 1, LOBJ_RELATION);
	LuaCtor(State, Relation->Target, LOBJ_BIGGUY);
	return 1;
}

int LuaSettlementGetName(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushstring(State, Settlement->Name);
	return 1;
}

int LuaSettlementGetLeader(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	LuaCtor(State, Settlement->Government->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaSettlementGetGovernment(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	LuaCtor(State, Settlement->Government, LOBJ_GOVERNMENT);
	return 1;
}

//Used for the function below as a placeholder to decide which settlement to raid.
/*#include "World.h"

int LuaSettlementRaiseArmy(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	struct ArmyGoal Goal;
	struct Army* Army = NULL;
	struct Settlement* Raid = NULL;

	if(g_GameWorld.Settlements.Front->Data == Settlement)
		Raid = g_GameWorld.Settlements.Back->Data;
	else
		Raid = g_GameWorld.Settlements.Front->Data;
	Army = CreateArmy(Settlement, Settlement->Government->Leader, ArmyGoalRaid(&Goal, Raid));
	LuaCtor(State, Army, LOBJ_ARMY);
	return 1;
}*/

int LuaSettlementGetPopulation(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->People.Size);
	return 1;
}

int LuaSettlementCountWarriors(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, SettlementCountWarriors(Settlement));
	return 1;
}

int LuaSettlementGetBigGuys(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	CreateLuaLnkLstItr(State, &Settlement->BigGuys, LOBJ_BIGGUY);
	return 1;
}

int LuaSettlementGetNutrition(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, SettlementGetNutrition(Settlement));
	return 1;
}

int LuaSettlementYearlyNutrition(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(State, SettlementYearlyNutrition(Settlement));
	return 1;
}

int LuaSettlementCountAcres(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(State, SettlementCountAcres(Settlement));
	return 1;
}

int LuaSettlementExpectedYield(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(State, SettlementExpectedYield(Settlement));
	return 1;
}

int LuaSettlementYearlyDeaths(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(State, Settlement->YearDeaths);
	return 1;
}

int LuaSettlementYearlyBirths(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(State, Settlement->YearBirths);
	return 1;
}

int LuaSettlementGetBulletins(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	CreateLuaArrayItr(State, &Settlement->Bulletin, LOBJ_BULLETIN);
	return 1;
}

int LuaSettlementCountAdults(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	if(Settlement == NULL)
		return LuaClassError(State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(State, SettlementAdultPop(Settlement));
	return 1;
}

int LuaSettlementGetFreeWarriors(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->FreeWarriors.Size);
	return 1;
}

int LuaSettlementGetMaxWarriors(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->MaxWarriors);
	return 1;
}

int LuaSettlementMaleAdults(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	
	lua_pushinteger(State, Settlement->AdultMen);
	return 1;	
}

int LuaSettlementFemaleAdults(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	
	lua_pushinteger(State, Settlement->AdultWomen);
	return 1;	
}

int LuaSettlementGetCombat(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_COMBAT]);
	return 1;
}

int LuaSettlementGetStrength(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_STRENGTH]);
	return 1;
}

int LuaSettlementGetToughness(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_TOUGHNESS]);
	return 1;
}

int LuaSettlementGetAgility(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_AGILITY]);
	return 1;
}

int LuaSettlementGetWit(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_WIT]);
	return 1;
}

int LuaSettlementGetCharisma(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_CHARISMA]);
	return 1;
}

int LuaSettlementGetIntelligence(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(State, Settlement->Stats[BGSKILL_INTELLIGENCE]);
	return 1;
}

int LuaSettlementGetFine(lua_State* State) {
	int Type = luaL_checkinteger(State, 2);
	int Level = luaL_checkinteger(State, 3);

	switch(Type) {
		case FINE_MURDER:
			switch(Level) {
				case FINE_LLIGHT:
					return 12 * 3 / 4;
				case FINE_LNORMAL:
					return 12;
				case FINE_LHEAVY:
					return 12 * 5 / 4;
			}
			break;
	}
	return 1;
}

int LuaSettlementGetType(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	lua_pushstring(State, SettlementString(SettlementType(Settlement)));
	return 1;
}

int LuaSettlementBuying(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	CreateLuaArrayItr(State, &Settlement->BuyReqs, LOBJ_MARREQ);
	return 1;
}

int LuaSettlementSelling(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);

	CreateLuaArrayItr(State, &Settlement->Market, LOBJ_MARREQ);
	return 1;
}

int LuaSettlementCasteCount(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	int CasteCount[CASTE_SIZE] = {0};

	lua_createtable(State, CASTE_SIZE, 0);
	SettlementCasteCount(Settlement, &CasteCount);
	for(int i = 0; i < CASTE_SIZE; ++i) {
		lua_pushinteger(State, CasteCount[i]);
		lua_rawseti(State, -2, i + 1);
	}
	return 1;
}

int LuaSettlementProfessionCount(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	int Count = ProfCount();
	uint16_t* ProfessionCount = alloca(sizeof(uint16_t) * Count);

	memset(ProfessionCount, 0, sizeof(uint16_t) * Count);
	lua_createtable(State, CASTE_SIZE, 0);
	SettlementProfessionCount(Settlement, &ProfessionCount);
	for(int i = 0; i < Count; ++i) {
		lua_pushinteger(State, ProfessionCount[i]);
		lua_rawseti(State, -2, i + 1);
	}
	return 1;
}

int LuaBulletinGetOwner(lua_State* State) {
	struct BulletinItem* Item = LuaCheckClass(State, 1, LOBJ_BULLETIN);

	LuaConstCtor(State, Item->Owner, LOBJ_BIGGUY);
	return 1;
}

int LuaBulletinGetDaysRemaining(lua_State* State) {
	struct BulletinItem* Item = LuaCheckClass(State, 1, LOBJ_BULLETIN);
	
	lua_pushinteger(State, Item->DaysLeft);
	return 1;
}

int LuaBulletinGetName(lua_State* State) {
	struct BulletinItem* Item = LuaCheckClass(State, 1, LOBJ_BULLETIN);

	lua_pushstring(State, BulletinItemGetName(Item));
	return 1;
}

int LuaBulletinGetMission(lua_State* State) {
	struct BulletinItem* Item = LuaCheckClass(State, 1, LOBJ_BULLETIN);

	lua_pushinteger(State, Item->SuccMission->Id);
	return 1;
}

int LuaPlotActionDescribe(lua_State* State) {
	struct PlotAction* Action = LuaCheckClass(State, 1, LOBJ_PLOTACTION);
	char* Buffer = FrameAlloc(1024);
	
	PlotActionEventStr(Action, &Buffer, 1024);
	lua_pushstring(State, Buffer);
	return 1;
}

int LuaPlotActionGetType(lua_State* State) {
	struct PlotAction* Action = LuaCheckClass(State, 1, LOBJ_PLOTACTION);

	lua_pushinteger(State, Action->Type);
	return 1;
}

int LuaPlotJoin(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);
	struct BigGuy* Guy = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	int Side = luaL_checkinteger(State, 3);

	if(Plot == NULL)
		return LuaClassError(State, 1, LOBJ_PLOT);
	if(Guy == NULL)
		return LuaClassError(State, 2, LOBJ_BIGGUY);
	if(Side != PLOT_ATTACKERS && Side != PLOT_DEFENDERS)
		return luaL_error(State, "Plot.Join uses an invalid number for Side.");
	PlotJoin(Plot, Side, Guy);
	return 0;
}

int LuaPlotInPlot(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);
	struct BigGuy* Guy = LuaCheckClass(State, 2, LOBJ_BIGGUY);

	if(Plot == NULL)
		return LuaClassError(State, 1, LOBJ_PLOT);
	if(Guy == NULL)
		return LuaClassError(State, 2, LOBJ_BIGGUY);
	lua_pushboolean(State, IsInPlot(Plot, Guy));
	return 1;
}

int LuaPlotPlotters(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	if(Plot == NULL)
		return LuaClassError(State, 1, LOBJ_PLOT);
	CreateLuaLnkLstItr(State, &Plot->Side[PLOT_ATTACKERS], LOBJ_BIGGUY);
	return 1;
}

int LuaPlotDefenders(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	if(Plot == NULL)
		return LuaClassError(State, 1, LOBJ_PLOT);
	CreateLuaLnkLstItr(State, &Plot->Side[PLOT_DEFENDERS], LOBJ_BIGGUY);
	return 1;
}

int LuaPlotTypeStr(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	lua_pushstring(State, PlotTypeStr(Plot));
	return 1;
}

int LuaPlotLeader(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	if(Plot == NULL)
		return LuaClassError(State, 1, LOBJ_PLOT);
	LuaCtor(State, Plot->Side[PLOT_ATTACKERS].Front->Data, LOBJ_BIGGUY);
	return 1;
}
	
int LuaPlotTarget(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	if(Plot == NULL)
		return LuaClassError(State, 1, LOBJ_PLOT);
	LuaCtor(State, Plot->Side[PLOT_DEFENDERS].Front->Data, LOBJ_BIGGUY);
	return 1;
}

int LuaPlotGetScore(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	lua_pushinteger(State, Plot->WarScore);
	return 1;
}

int LuaPlotCreate(lua_State* State) {
	struct Plot* Plot = NULL;
	struct BigGuy* Leader = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Target = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	int Type = luaL_checkinteger(State, 3);
	void* Data = NULL;
	struct ActivePolicy* PolAct = NULL; 
	int PolicyCat = 0;

	if(Leader == NULL)
		return luaL_error(State, "Cannot create plot: Leader is invalid.");
	if(Leader == Target)
		return luaL_error(State, "Cannot create plot: target and plotter are the same.");
	if(BigGuyHasPlot(Leader) != 0)
		return luaL_error(State, "Cannot create plot: Leader already has a plot.");
	if(IsPlotTypeValid(Type) == 0)
		return luaL_error(State, "Cannot create plot: Invalid plot type.");
	switch(Type) {
		case PLOT_REMOVEPOLICY:
		case PLOT_PASSPOLICY:
			Data = LuaCheckClass(State, 4, LOBJ_POLICY);
			break;
		case PLOT_CHANGEPOLICY:
			PolAct = malloc(sizeof(struct ActivePolicy));

			lua_rawgeti(State, 4, 1);
			PolAct->Policy = LuaCheckClass(State, -1, LOBJ_POLICY);
			lua_rawgeti(State, 4, 2);
			PolicyCat = lua_tointeger(State, -1) - 1;
			if(ValidPolicyCategory(PolicyCat) == 0)
				return luaL_error(State, "Invalid policy category %d.", PolicyCat);
			lua_rawgeti(State, 4, 3);
			PolAct->OptionSel = POLICYACT_IGNORE;
			PolAct->OptionSel = lua_tointeger(State, -1) - 1;
			Data = PolAct;
		break;

	}
	Plot = CreatePlot(Type, Data, Leader, Target);
	//_Plot->WarScore = 9;
	LuaCtor(State, Plot, LOBJ_PLOT);
	return 1;
}

int LuaPlotAddAction(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);
	int Type = luaL_checkinteger(State, 2);
	struct BigGuy* Actor = LuaCheckClass(State, 3, LOBJ_BIGGUY);
	struct BigGuy* Target = LuaCheckClass(State, 4, LOBJ_BIGGUY);

	PlotAddAction(Plot, Type, Actor, Target);
	return 0;
}

int LuaPlotGetThreat(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	lua_pushinteger(State, Plot->Threat[PLOT_ATTACKERS]);
	lua_pushinteger(State, Plot->Threat[PLOT_DEFENDERS]);
	return 2;
}

int LuaPlotPrevMonthActions(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);
	int i = 0;

	lua_createtable(State, 0, 6);
	for(const struct PlotAction* Action = PlotPrevActList(Plot); Action != NULL; Action = Action->Next) {
		LuaCtor(State, (struct PlotAction*) Action, LOBJ_PLOTACTION);
		lua_rawseti(State, -2, ++i);
	}
	//CreateLuaLnkLstItr(State, PlotPrevActList(Plot), LOBJ_PLOTACTION);
	return 1;
}

int LuaPlotCurrMonthActions(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);
	int i = 0;

	lua_createtable(State, 0, 6);
	for(const struct PlotAction* Action = PlotPrevActList(Plot); Action != NULL; Action = Action->Next) {
		LuaCtor(State, (struct PlotAction*) Action, LOBJ_PLOTACTION);
		lua_rawseti(State, -2, ++i);
	}
	//CreateLuaLnkLstItr(State, PlotCurrActList(Plot), LOBJ_PLOTACTION);
	return 1;
}
int LuaPlotHasStarted(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);

	lua_pushboolean(State, Plot->HasStarted);
	return 1;
}

int LuaPlotStart(lua_State* State) {
	struct Plot* Plot = LuaCheckClass(State, 1, LOBJ_PLOT);
	int Start = 0;

	luaL_checktype(State, 2, LUA_TBOOLEAN);
	Start = lua_toboolean(State, Start);
	Plot->HasStarted = Start;
	return 0;
}

int LuaPolicyOptionName(lua_State* State) {
	struct PolicyOption* Opt = LuaCheckClass(State, 1, LOBJ_POLICYOPTION);

	lua_pushstring(State, Opt->Name);
	return 1;
}

int LuaPolicyOptionDescription(lua_State* State) {
	struct PolicyOption* Opt = LuaCheckClass(State, 1, LOBJ_POLICYOPTION);

	lua_pushstring(State, Opt->Desc);
	return 1;
}

int LuaPolicyName(lua_State* State) {
	struct Policy* Policy = LuaCheckClass(State, 1, LOBJ_POLICY);

	lua_pushstring(State, Policy->Name);
	return 1;
}

int LuaPolicyDescription(lua_State* State) {
	struct Policy* Policy = LuaCheckClass(State, 1, LOBJ_POLICY);

	lua_pushstring(State, Policy->Description);
	return 1;
}

int LuaPolicyCategory(lua_State* State) {
	struct Policy* Policy = LuaCheckClass(State, 1, LOBJ_POLICY);

	lua_pushinteger(State, Policy->Category);
	return 1;
}

//FIXME: This should be done on initialization for each Policy.
int LuaPolicyOptions(lua_State* State) {
	struct Policy* Policy = LuaCheckClass(State, 1, LOBJ_POLICY);
	int Ct = 1;

	lua_createtable(State, POLICY_SUBSZ, 0);
	for(int i = 0; i < Policy->OptionsSz; ++i, ++Ct) {
		LuaCtor(State, (void*)&Policy->Options[i], LOBJ_POLICYOPTION);
		lua_rawseti(State, -2, Ct);
		/*if(i >= (Policy->Options.Size[Idx] + Last) - 1) {
			Last += Policy->Options.Size[Idx];
			++Idx;
			lua_rawseti(State, -2, Idx);
			if(i + 1 < Policy->OptionsSz) {
				lua_createtable(State, Policy->Options.Size[Idx], 1);
				lua_pushstring(State, "Name");
				lua_pushstring(State, Policy->Options.Name[Idx]);
				lua_rawset(State, -3);
			}
			Ct = 0;
		}*/
	}
	return 1;	
}

int LuaRetinueLeader(lua_State* State) {
	struct Retinue* Retinue = LuaCheckClass(State, 1, LOBJ_RETINUE);

	LuaCtor(State, Retinue->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaRetinueWarriors(lua_State* State) {
	struct Retinue* Retinue = LuaCheckClass(State, 1, LOBJ_RETINUE);
	
	LuaCtorArray(State, &Retinue->Warriors, LOBJ_PERSON);	
	return 1;
}

int LuaRetinueBranch(lua_State* State) {
	struct Retinue* Retinue = LuaCheckClass(State, 1, LOBJ_RETINUE);
	
	LuaCtorArray(State, &Retinue->Children, LOBJ_PERSON);
	return 1;
}

int LuaRetinueTroopCount(lua_State* State) {
	struct Retinue* Retinue = LuaCheckClass(State, 1, LOBJ_RETINUE);

	lua_pushinteger(State, Retinue->Warriors.Size + 1);//+ 1 to account for the leader of the retinue.
	return 1;
}

int LuaBGTraitName(lua_State* State) {
	struct Trait* Trait = LuaCheckClass(State, 1, LOBJ_BIGGUYTRAIT);

	lua_pushstring(State, Trait->Name);
	return 1;
}

