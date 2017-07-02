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
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsCrisis[] = {
	{"GetDefender", LuaCrisisGetDefender},
	{"GetOffender", LuaCrisisGetOffender},
	{"Type", LuaCrisisType},
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
	{"GetLeader", LuaSettlementGetLeader},
	{"GetGovernment", LuaSettlementGetGovernment},
	//{"RaiseArmy", LuaSettlementRaiseArmy},
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
	{"GetHappiness", LuaSettlementGetHappiness},
	{"SetHappiness", LuaSettlementSetHappiness},
	{"GetFine", LuaSettlementGetFine},
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

int LuaArmyGetLeader(lua_State* _State) {
	struct Army* _Army = LuaCheckClass(_State, 1, LOBJ_ARMY);

	LuaCtor(_State, _Army->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaArmyGetSize(lua_State* _State) {
	struct Army* _Army = LuaCheckClass(_State, 1, LOBJ_ARMY);

	lua_pushinteger(_State, ArmyGetSize(_Army));
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

int LuaCrisisSelectOption(lua_State* State) {
	struct Crisis* Crisis = LuaCheckClass(State, 1, LOBJ_CRISIS);
	int Option = luaL_checkinteger(State, 2);

	CrisisProcess(Crisis, Option);
	return 0;
}

int LuaBGGetPerson(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	LuaCtor(_State, _Guy->Person, LOBJ_PERSON);
	return 1;
}

int LuaBGGetCombat(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_COMBAT]);
	return 1;
}

int LuaBGGetStrength(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_STRENGTH]);
	return 1;
}

int LuaBGGetToughness(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_TOUGHNESS]);
	return 1;
}

int LuaBGGetAgility(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_AGILITY]);
	return 1;
}

int LuaBGGetWit(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_WIT]);
	return 1;
}

int LuaBGGetCharisma(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_CHARISMA]);
	return 1;
}

int LuaBGGetIntelligence(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_INTELLIGENCE]);
	return 1;
}

int LuaBGOpposedChallange(lua_State* _State) {
	struct BigGuy* _One = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct BigGuy* _Two = LuaCheckClass(_State, 2, LOBJ_BIGGUY);

	if(_One == NULL)
		return luaL_error(_State, "Agument #1 is not a BigGuy.");
	if(_Two == NULL)
		return luaL_error(_State, "Agument #2 is not a BigGuy.");
	lua_pushinteger(_State, BigGuyOpposedCheck(_One, _Two, luaL_checkinteger(_State, 3)));
	return 1;
}

int LuaBGGetAgent(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct Agent* _Agent = RBSearch(&g_GameWorld.Agents, _Guy);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	if(_Agent == NULL) {
		lua_pushnil(_State);
		goto end;
	}
	LuaCtor(_State, _Agent, LOBJ_AGENT);
	end:
	return 1;
}

int LuaBGGetRelation(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct BigGuy* _Target = LuaCheckClass(_State, 2, LOBJ_BIGGUY);
	struct Relation* _Relation = GetRelation(_Guy->Relations, _Target);

	if(_Relation == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, _Relation, LOBJ_RELATION);
	return 1;
}

int LuaBGRelItrNext(lua_State* _State) {
	struct Relation* _Relation = LuaCheckClass(_State, lua_upvalueindex(1), LOBJ_RELATION);

	if(_Relation->Next == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, _Relation->Next, LOBJ_RELATION);
	lua_pushvalue(_State, -1);
	lua_replace(_State, lua_upvalueindex(1));
	return 1;
}

int LuaBGRelationItr(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	if(_Guy->Relations == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, _Guy->Relations, LOBJ_RELATION); 
	lua_pushcclosure(_State, LuaBGRelItrNext, 1);
	return 1;
}

int LuaBGAddOpinion(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct BigGuy* _Target = LuaCheckClass(_State, 2, LOBJ_BIGGUY);
	int _Action = luaL_checkinteger(_State, 3);
	int _Mod = luaL_checkinteger(_State, 4);
	int _Length = luaL_optinteger(_State, 5, OPNLEN_MEDIUM);
	int _Strength = luaL_optinteger(_State, 6, OPINION_AVERAGE);

	AddOpinion(_Guy, _Target, _Action, _Mod, _Length, _Strength, &_Guy->Relations);
	return 0;
}

int LuaBGSetAction(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	int _Action = luaL_checkint(_State, 2);
	struct BigGuy* _Target = NULL;

/*	luaL_checktype(_State, 3, LUA_TTABLE);
	lua_pushstring(_State, "__self");
	lua_rawget(_State, 3);
	if(lua_isnil(_State, 3) == 1)
		return luaL_error(_State, "BigGuy:SetAction's 3rd argument is not an object");
	*/
	_Target = LuaCheckClass(_State, 3, LOBJ_BIGGUY);
	BigGuySetAction(_Guy, _Action, _Target, NULL);
	return 0;
}

int LuaBGGetSettlement(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	LuaCtor(_State, FamilyGetSettlement(_Guy->Person->Family), LOBJ_SETTLEMENT);
	return 1;
}

int LuaBGGetFamily(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	LuaCtor(_State, _Guy->Person->Family, LOBJ_FAMILY);
	return 1;
}

int LuaBGGetName(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	lua_pushstring(_State, _Guy->Person->Name);
	return 1;
}

int LuaBGKill(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	PersonDeath(_Guy->Person);
//	DestroyBigGuy(_Guy);
	return 0;
}

int LuaBGPopularity(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	lua_pushinteger(_State, BigGuyPopularity(_Guy));
	return 1; 
}

int LuaBGChangePopularity(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	int _Change = luaL_checkinteger(_State, 2);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	_Guy->Popularity += _Change;
	return 0;
}

int LuaBGGlory(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	lua_pushinteger(_State, _Guy->Glory);
	return 1; 
}

int LuaBGChangeGlory(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	int _Change = luaL_checkinteger(_State, 2);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	_Guy->Glory+= _Change;
	return 0;
}

int LuaBGSuccessMargin(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	int _Skill = luaL_checkinteger(_State, 2);
	int _ReqSkill = 0;

	if(lua_gettop(_State) > 2)
		_ReqSkill = luaL_checkinteger(_State, 3);
	else
		_ReqSkill = SKILLCHECK_DEFAULT;
	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	lua_pushinteger(_State, BigGuySuccessMargin(_Guy, _Skill, _ReqSkill));	
	return 1;
}

int LuaBGPlotsAgainst(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	CreateLuaLnkLstItr(_State, &_Guy->PlotsAgainst, LOBJ_PLOT);
	return 1;
}

int LuaBGHasTrait(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	const char* _TraitName = NULL;
	const struct Trait* _Trait = NULL;
	
	switch(lua_type(_State, 2)) {
		case LUA_TSTRING:
			_TraitName = luaL_checkstring(_State, 2);
			_Trait = HashSearch(&g_Traits, _TraitName);
			if(_Trait == NULL)
				goto error;
			lua_pushboolean(_State, HasTrait(_Guy, _Trait));
			break;
		case LUA_TTABLE:
			lua_pushboolean(_State, HasTrait(_Guy, LuaCheckClass(_State, 2, LOBJ_TRAIT)));
			break;
		default:
			error:
			return luaL_error(_State, "Arg #2 is neither a trait or a trait name.");
	}

	return 1;
}

int LuaBGTraitItrNext(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, lua_upvalueindex(1), LOBJ_BIGGUY);
	int _Idx = lua_tointeger(_State, lua_upvalueindex(2));

	if(_Guy == NULL || _Idx >= _Guy->TraitCt) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, _Guy->Traits[_Idx], LOBJ_BIGGUYTRAIT);
	lua_pushinteger(_State, _Idx + 1);
	lua_replace(_State, lua_upvalueindex(2));
	return 1;
}

int LuaBGTraitList(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, LOBJ_BIGGUY);
	if(_Guy->Traits== NULL) {
		lua_pushnil(_State);
		return 1;
	}
	lua_pushvalue(_State, 1);
	lua_pushinteger(_State, 0);
	lua_pushcclosure(_State, LuaBGTraitItrNext, 2);
	return 1;
}

int LuaBGMurder(lua_State* _State) {
	struct BigGuy* _Murderer = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct BigGuy* _Target = LuaCheckClass(_State, 2, LOBJ_BIGGUY);

	PersonDeath(_Target->Person);	
	PushEvent(EVENT_MURDERED, _Target, _Murderer);
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

int LuaBGBulletinPost(lua_State* _State) {
	struct BulletinItem* _Item = NULL;
	struct Mission* _Mission = NULL;
	struct Mission* _MissionFail = NULL;
	struct BigGuy* _Poster = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	const char* _MissionStr = luaL_checkstring(_State, 2);
	const char* _MissionFailStr = luaL_checkstring(_State, 3);
	int _DaysLeft = luaL_checkint(_State, 4);
	int _Priority = luaL_checkint(_State, 5);
	
	if((_Mission = MissionStrToId(_MissionStr)) == NULL) {
		luaL_error(_State, "%s is not a mission name.", _MissionStr);
	}
	if((_MissionFail = MissionStrToId(_MissionFailStr)) == NULL) {
		luaL_error(_State, "%s is not a mission name.", _MissionFailStr);
	}
	_Item = CreateBulletinItem(_Mission, NULL, _Poster, _DaysLeft, _Priority);		 
	ArrayInsert_S(&_Poster->Person->Family->HomeLoc->Bulletin, _Item);
	return 0;
}

/*int LuaBGRecruit(lua_State* _State) {
	struct BigGuy* _Leader = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct Settlement* _Home = PersonHome(_Leader->Person);

	for(struct Retinue* _Retinue = _Home->Retinues; _Retinue != NULL; _Retinue = _Retinue->Next) {
		if(_Retinue->Leader != _Leader)
			continue;
		_Retinue->IsRecruiting = (_Retinue->IsRecruiting ^ 1);
		return 0;
	}
	struct Retinue* _Retinue = SettlementAddRetinue(_Home, _Leader);

	_Retinue->IsRecruiting = (_Retinue->IsRecruiting ^ 1);
	return 0;
}

int LuaBGIsRecruiting(lua_State* _State) {
	struct BigGuy* _Leader = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct Settlement* _Home = PersonHome(_Leader->Person);

	for(struct Retinue* _Retinue = _Home->Retinues; _Retinue != NULL; _Retinue = _Retinue->Next) {
		if(_Retinue->Leader != _Leader)
			continue;
		lua_pushboolean(_State, _Retinue->IsRecruiting);
		return 1;
	}
	lua_pushboolean(_State, 0);
	return 1;
}

int LuaBGRetinueSize(lua_State* _State) {
	struct BigGuy* _Leader = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct Retinue* _GuyRetinue = IntSearch(&g_GameWorld.PersonRetinue, _Leader->Person->Id);
	struct Settlement* _Home = PersonHome(_Leader->Person);

	_Leader = _GuyRetinue->Leader;
	for(struct Retinue* _Retinue = _Home->Retinues; _Retinue != NULL; _Retinue = _Retinue->Next) {
		if(_Retinue->Leader != _Leader)
			continue;
		lua_pushinteger(_State, _Retinue->Warriors.Size);
		return 1;
	}
	lua_pushinteger(_State, 0);
	return 1;
}*/

/*int LuaBGRetinueTable(lua_State* _State) {
	struct BigGuy* _Leader = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct Settlement* _Home = PersonHome(_Leader->Person);
	struct Retinue* _Retinue = _Home->Retinues;

	for(; _Retinue != NULL; _Retinue = _Retinue->Next) {
		if(_Retinue->Leader != _Leader)
			continue;
		goto found_retinue;
	}
	lua_createtable(_State, 0, 0);
	return 1;
	found_retinue:
	lua_createtable(_State, _Retinue->Warriors.Size, 0);
	for(int i = 0; i < _Retinue->Warriors.Size; ++i) {
		LuaCtor(_State, _Retinue->Warriors.Table[i], LOBJ_PERSON);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
} */

int LuaBGOpinionAction(lua_State* _State) {
	struct Opinion* _Opinion = LuaCheckClass(_State, 1, LOBJ_BIGGUYOPINION);

	lua_pushstring(_State, g_OpinionActions[_Opinion->Action]);
	return 1;
}

int LuaBGOpinionRelation(lua_State* _State) {
	struct Opinion* _Opinion = LuaCheckClass(_State, 1, LOBJ_BIGGUYOPINION);

	lua_pushinteger(_State, _Opinion->RelMod);
	return 1;

}

int LuaBGRelationGetOpinion(lua_State* _State) {
	struct Relation* _Relation = LuaCheckClass(_State, 1, LOBJ_RELATION);

	if(_Relation == NULL)
		return LuaClassError(_State, 1, LOBJ_RELATION);
	lua_pushinteger(_State, _Relation->Modifier);
	return 1;
}

int LuaRelationChangeOpinion(lua_State* State) {
	struct Relation* Relation = LuaCheckClass(State, 1, LOBJ_RELATION);
	
	ChangeRelation(Relation, luaL_checkint(State, 2), luaL_checkint(State, 3), luaL_checkint(State, 4), luaL_checkint(State, 5));
	return 0;
}

int LuaBGRelationGetRelationList(lua_State* _State) {
	struct Relation* _Relation = LuaCheckClass(_State, 1, LOBJ_RELATION);
	int _Idx = 1;

	lua_createtable(_State, 6, 0);	
	for(struct Opinion* _Opinion = _Relation->Opinions; _Opinion != NULL; _Opinion = _Opinion->Next, ++_Idx) {
		LuaCtor(_State, _Opinion, LOBJ_BIGGUYOPINION);
		lua_rawseti(_State, -2, _Idx);
	}
	return 1;
}

int LuaBGRelationBigGuy(lua_State* _State) {
	struct Relation* _Relation = LuaCheckClass(_State, 1, LOBJ_RELATION);

	if(_Relation == NULL)
		return LuaClassError(_State, 1, LOBJ_RELATION);
	LuaCtor(_State, _Relation->Target, LOBJ_BIGGUY);
	return 1;
}

int LuaSettlementGetLeader(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	LuaCtor(_State, _Settlement->Government->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaSettlementGetGovernment(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	LuaCtor(_State, _Settlement->Government, LOBJ_GOVERNMENT);
	return 1;
}

//Used for the function below as a placeholder to decide which settlement to raid.
/*#include "World.h"

int LuaSettlementRaiseArmy(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);
	struct ArmyGoal _Goal;
	struct Army* _Army = NULL;
	struct Settlement* _Raid = NULL;

	if(g_GameWorld.Settlements.Front->Data == _Settlement)
		_Raid = g_GameWorld.Settlements.Back->Data;
	else
		_Raid = g_GameWorld.Settlements.Front->Data;
	_Army = CreateArmy(_Settlement, _Settlement->Government->Leader, ArmyGoalRaid(&_Goal, _Raid));
	LuaCtor(_State, _Army, LOBJ_ARMY);
	return 1;
}*/

int LuaSettlementGetPopulation(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->NumPeople);
	return 1;
}

int LuaSettlementCountWarriors(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, SettlementCountWarriors(_Settlement));
	return 1;
}

int LuaSettlementGetBigGuys(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	CreateLuaLnkLstItr(_State, &_Settlement->BigGuys, LOBJ_BIGGUY);
	return 1;
}

int LuaSettlementGetNutrition(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, SettlementGetNutrition(_Settlement));
	return 1;
}

int LuaSettlementYearlyNutrition(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(_State, SettlementYearlyNutrition(_Settlement));
	return 1;
}

int LuaSettlementCountAcres(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(_State, SettlementCountAcres(_Settlement));
	return 1;
}

int LuaSettlementExpectedYield(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(_State, SettlementExpectedYield(_Settlement));
	return 1;
}

int LuaSettlementYearlyDeaths(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(_State, _Settlement->YearDeaths);
	return 1;
}

int LuaSettlementYearlyBirths(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(_State, _Settlement->YearBirths);
	return 1;
}

int LuaSettlementGetBulletins(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	CreateLuaArrayItr(_State, &_Settlement->Bulletin, LOBJ_BULLETIN);
	return 1;
}

int LuaSettlementCountAdults(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, LOBJ_SETTLEMENT);
	lua_pushinteger(_State, SettlementAdultPop(_Settlement));
	return 1;
}

int LuaSettlementGetFreeWarriors(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->FreeWarriors.Size);
	return 1;
}

int LuaSettlementGetMaxWarriors(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->MaxWarriors);
	return 1;
}

int LuaSettlementMaleAdults(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);
	
	lua_pushinteger(_State, _Settlement->AdultMen);
	return 1;	
}

int LuaSettlementFemaleAdults(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);
	
	lua_pushinteger(_State, _Settlement->AdultWomen);
	return 1;	
}

int LuaSettlementGetCombat(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_COMBAT]);
	return 1;
}

int LuaSettlementGetStrength(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_STRENGTH]);
	return 1;
}

int LuaSettlementGetToughness(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_TOUGHNESS]);
	return 1;
}

int LuaSettlementGetAgility(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_AGILITY]);
	return 1;
}

int LuaSettlementGetWit(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_WIT]);
	return 1;
}

int LuaSettlementGetCharisma(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_CHARISMA]);
	return 1;
}

int LuaSettlementGetIntelligence(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, LOBJ_SETTLEMENT);

	lua_pushinteger(_State, _Settlement->Stats[BGSKILL_INTELLIGENCE]);
	return 1;
}

int LuaSettlementGetHappiness(lua_State* State) {
	/*struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	int Caste = luaL_checkinteger(State, 2);

	if(Caste < CASTE_THRALL || Caste > CASTE_NOBLE)
		return luaL_error(State, "Caste is invalid.");
	lua_pushinteger(State, Settlement->Happiness[Caste]);*/
	return 1;
}

int LuaSettlementSetHappiness(lua_State* State) {
/*	struct Settlement Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	int Caste = luaL_checkinteger(State, 2);
	int Amt = luaL_checkinteger(State, 3);

	if(Caste < CASTE_THRALL || Caste > CASTE_NOBLE)
		return luaL_error(State, "Caste is invalid.");
	if(Settlement->Happiness[Caste] + Amt > 255) 
		Settlement->Happiness[Caste] = 255;
	else if(Settlement->Happiness[Caste] + Amt < 0) 
		Settlement->Happiness[Caste] = 0;
	else 
		Settlement->Happiness[Caste] += Amt;*/
	return 0;
}

int LuaSettlementGetFine(lua_State* State) {
	struct Settlement* Settlement = LuaCheckClass(State, 1, LOBJ_SETTLEMENT);
	int Type = luaL_checkinteger(State, 2);
	int Level = luaL_checkinteger(State, 3);
	struct Person* Target = LuaCheckClass(State, 4, LOBJ_PERSON);

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

int LuaBulletinGetOwner(lua_State* _State) {
	struct BulletinItem* _Item = LuaCheckClass(_State, 1, LOBJ_BULLETIN);

	LuaConstCtor(_State, _Item->Owner, LOBJ_BIGGUY);
	return 1;
}

int LuaBulletinGetDaysRemaining(lua_State* _State) {
	struct BulletinItem* _Item = LuaCheckClass(_State, 1, LOBJ_BULLETIN);
	
	lua_pushinteger(_State, _Item->DaysLeft);
	return 1;
}

int LuaBulletinGetName(lua_State* _State) {
	struct BulletinItem* _Item = LuaCheckClass(_State, 1, LOBJ_BULLETIN);

	lua_pushstring(_State, BulletinItemGetName(_Item));
	return 1;
}

int LuaBulletinGetMission(lua_State* _State) {
	struct BulletinItem* _Item = LuaCheckClass(_State, 1, LOBJ_BULLETIN);

	lua_pushinteger(_State, _Item->SuccMission->Id);
	return 1;
}

int LuaPlotActionDescribe(lua_State* _State) {
	struct PlotAction* _Action = LuaCheckClass(_State, 1, LOBJ_PLOTACTION);
	char* _Buffer = FrameAlloc(1024);
	
	PlotActionEventStr(_Action, &_Buffer, 1024);
	lua_pushstring(_State, _Buffer);
	return 1;
}

int LuaPlotActionGetType(lua_State* _State) {
	struct PlotAction* _Action = LuaCheckClass(_State, 1, LOBJ_PLOTACTION);

	lua_pushinteger(_State, _Action->Type);
	return 1;
}

int LuaPlotJoin(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);
	struct BigGuy* _Guy = LuaCheckClass(_State, 2, LOBJ_BIGGUY);
	int _Side = luaL_checkinteger(_State, 3);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, LOBJ_PLOT);
	if(_Guy == NULL)
		return LuaClassError(_State, 2, LOBJ_BIGGUY);
	if(_Side != PLOT_ATTACKERS && _Side != PLOT_DEFENDERS)
		return luaL_error(_State, "Plot.Join uses an invalid number for Side.");
	PlotJoin(_Plot, _Side, _Guy);
	return 0;
}

int LuaPlotInPlot(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);
	struct BigGuy* _Guy = LuaCheckClass(_State, 2, LOBJ_BIGGUY);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, LOBJ_PLOT);
	if(_Guy == NULL)
		return LuaClassError(_State, 2, LOBJ_BIGGUY);
	lua_pushboolean(_State, IsInPlot(_Plot, _Guy));
	return 1;
}

int LuaPlotPlotters(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, LOBJ_PLOT);
	CreateLuaLnkLstItr(_State, &_Plot->Side[PLOT_ATTACKERS], LOBJ_BIGGUY);
	return 1;
}

int LuaPlotDefenders(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, LOBJ_PLOT);
	CreateLuaLnkLstItr(_State, &_Plot->Side[PLOT_DEFENDERS], LOBJ_BIGGUY);
	return 1;
}

int LuaPlotTypeStr(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	lua_pushstring(_State, PlotTypeStr(_Plot));
	return 1;
}

int LuaPlotLeader(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, LOBJ_PLOT);
	LuaCtor(_State, _Plot->Side[PLOT_ATTACKERS].Front->Data, LOBJ_BIGGUY);
	return 1;
}
	
int LuaPlotTarget(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, LOBJ_PLOT);
	LuaCtor(_State, _Plot->Side[PLOT_DEFENDERS].Front->Data, LOBJ_BIGGUY);
	return 1;
}

int LuaPlotGetScore(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	lua_pushinteger(_State, _Plot->WarScore);
	return 1;
}

int LuaPlotCreate(lua_State* _State) {
	struct Plot* _Plot = NULL;
	struct BigGuy* _Leader = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct BigGuy* _Target = LuaCheckClass(_State, 2, LOBJ_BIGGUY);
	int _Type = luaL_checkinteger(_State, 3);
	void* _Data = NULL;
	struct ActivePolicy* _PolAct = NULL; 
	int _PolicyCat = 0;

	if(_Leader == NULL)
		return luaL_error(_State, "Cannot create plot: Leader is invalid.");
	if(_Leader == _Target)
		return luaL_error(_State, "Cannot create plot: target and plotter are the same.");
	if(BigGuyHasPlot(_Leader) != 0)
		return luaL_error(_State, "Cannot create plot: Leader already has a plot.");
	if(IsPlotTypeValid(_Type) == 0)
		return luaL_error(_State, "Cannot create plot: Invalid plot type.");
	switch(_Type) {
		case PLOT_REMOVEPOLICY:
		case PLOT_PASSPOLICY:
			_Data = LuaCheckClass(_State, 4, LOBJ_POLICY);
			break;
		case PLOT_CHANGEPOLICY:
			_PolAct = malloc(sizeof(struct ActivePolicy));

			lua_rawgeti(_State, 4, 1);
			_PolAct->Policy = LuaCheckClass(_State, -1, LOBJ_POLICY);
			lua_rawgeti(_State, 4, 2);
			_PolicyCat = lua_tointeger(_State, -1) - 1;
			if(ValidPolicyCategory(_PolicyCat) == 0)
				return luaL_error(_State, "Invalid policy category %d.", _PolicyCat);
			lua_rawgeti(_State, 4, 3);
			_PolAct->OptionSel = POLICYACT_IGNORE;
			_PolAct->OptionSel = lua_tointeger(_State, -1) - 1;
			_Data = _PolAct;
		break;

	}
	_Plot = CreatePlot(_Type, _Data, _Leader, _Target);
	//_Plot->WarScore = 9;
	LuaCtor(_State, _Plot, LOBJ_PLOT);
	return 1;
}

int LuaPlotAddAction(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);
	int _Type = luaL_checkinteger(_State, 2);
	struct BigGuy* _Actor = LuaCheckClass(_State, 3, LOBJ_BIGGUY);
	struct BigGuy* _Target = LuaCheckClass(_State, 4, LOBJ_BIGGUY);

	PlotAddAction(_Plot, _Type, _Actor, _Target);
	return 0;
}

int LuaPlotGetThreat(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	lua_pushinteger(_State, _Plot->Threat[PLOT_ATTACKERS]);
	lua_pushinteger(_State, _Plot->Threat[PLOT_DEFENDERS]);
	return 2;
}

int LuaPlotPrevMonthActions(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);
	int i = 0;

	lua_createtable(_State, 0, 6);
	for(const struct PlotAction* _Action = PlotPrevActList(_Plot); _Action != NULL; _Action = _Action->Next) {
		LuaCtor(_State, (struct PlotAction*) _Action, LOBJ_PLOTACTION);
		lua_rawseti(_State, -2, ++i);
	}
	//CreateLuaLnkLstItr(_State, PlotPrevActList(_Plot), LOBJ_PLOTACTION);
	return 1;
}

int LuaPlotCurrMonthActions(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);
	int i = 0;

	lua_createtable(_State, 0, 6);
	for(const struct PlotAction* _Action = PlotPrevActList(_Plot); _Action != NULL; _Action = _Action->Next) {
		LuaCtor(_State, (struct PlotAction*) _Action, LOBJ_PLOTACTION);
		lua_rawseti(_State, -2, ++i);
	}
	//CreateLuaLnkLstItr(_State, PlotCurrActList(_Plot), LOBJ_PLOTACTION);
	return 1;
}
int LuaPlotHasStarted(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);

	lua_pushboolean(_State, _Plot->HasStarted);
	return 1;
}

int LuaPlotStart(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, LOBJ_PLOT);
	int _Start = 0;

	luaL_checktype(_State, 2, LUA_TBOOLEAN);
	_Start = lua_toboolean(_State, _Start);
	_Plot->HasStarted = _Start;
	return 0;
}

int LuaPolicyOptionName(lua_State* _State) {
	struct PolicyOption* _Opt = LuaCheckClass(_State, 1, LOBJ_POLICYOPTION);

	lua_pushstring(_State, _Opt->Name);
	return 1;
}

int LuaPolicyOptionDescription(lua_State* State) {
	struct PolicyOption* Opt = LuaCheckClass(State, 1, LOBJ_POLICYOPTION);

	lua_pushstring(State, Opt->Desc);
	return 1;
}

int LuaPolicyName(lua_State* _State) {
	struct Policy* _Policy = LuaCheckClass(_State, 1, LOBJ_POLICY);

	lua_pushstring(_State, _Policy->Name);
	return 1;
}

int LuaPolicyDescription(lua_State* State) {
	struct Policy* Policy = LuaCheckClass(State, 1, LOBJ_POLICY);

	lua_pushstring(State, Policy->Description);
	return 1;
}

int LuaPolicyCategory(lua_State* _State) {
	struct Policy* _Policy = LuaCheckClass(_State, 1, LOBJ_POLICY);

	lua_pushinteger(_State, _Policy->Category);
	return 1;
}

//FIXME: This should be done on initialization for each Policy.
int LuaPolicyOptions(lua_State* _State) {
	struct Policy* _Policy = LuaCheckClass(_State, 1, LOBJ_POLICY);
	int _Ct = 1;

	lua_createtable(_State, POLICY_SUBSZ, 0);
	for(int i = 0; i < _Policy->OptionsSz; ++i, ++_Ct) {
		LuaCtor(_State, (void*)&_Policy->Options[i], LOBJ_POLICYOPTION);
		lua_rawseti(_State, -2, _Ct);
		/*if(i >= (_Policy->Options.Size[_Idx] + _Last) - 1) {
			_Last += _Policy->Options.Size[_Idx];
			++_Idx;
			lua_rawseti(_State, -2, _Idx);
			if(i + 1 < _Policy->OptionsSz) {
				lua_createtable(_State, _Policy->Options.Size[_Idx], 1);
				lua_pushstring(_State, "Name");
				lua_pushstring(_State, _Policy->Options.Name[_Idx]);
				lua_rawset(_State, -3);
			}
			_Ct = 0;
		}*/
	}
	return 1;	
}

int LuaRetinueLeader(lua_State* _State) {
	struct Retinue* _Retinue = LuaCheckClass(_State, 1, LOBJ_RETINUE);

	LuaCtor(_State, _Retinue->Leader, LOBJ_BIGGUY);
	return 1;
}

int LuaRetinueWarriors(lua_State* _State) {
	struct Retinue* _Retinue = LuaCheckClass(_State, 1, LOBJ_RETINUE);
	
	LuaCtorArray(_State, &_Retinue->Warriors, LOBJ_PERSON);	
	return 1;
}

int LuaBGTraitName(lua_State* _State) {
	struct Trait* _Trait = LuaCheckClass(_State, 1, LOBJ_BIGGUYTRAIT);

	lua_pushstring(_State, _Trait->Name);
	return 1;
}
