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
#include "Bulitin.h"
#include "Plot.h"
#include "Policy.h"

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
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuy[] = {
	{"GetPerson", LuaBGGetPerson},
	{"GetAuthority", LuaBGGetAuthority},
	{"SetAuthority", LuaBGSetAuthority},
	{"GetPrestige", LuaBGGetPrestige},
	{"SetPrestige", LuaBGSetPrestige},
	{"GetCombat", LuaBGGetCombat},
	{"GetStrength", LuaBGGetStrength},
	{"GetToughness", LuaBGGetToughness},
	{"GetAgility", LuaBGGetAgility},
	{"GetWit", LuaBGGetWit},
	{"GetCharisma", LuaBGGetCharisma},
	{"GetIntrigue", LuaBGGetIntrigue},
	{"GetIntellegence", LuaBGGetIntellegence},
	{"OpposedChallange", LuaBGOpposedChallanged},
	{"GetAgent", LuaBGGetAgent},
	{"GetRelation", LuaBGGetRelation},
	{"RelationsItr", LuaBGRelationItr},
	{"SetOpinion", LuaBGSetOpinion},
	{"SetAction", LuaBGSetAction},
	{"ImproveRelationTarget", LuaBGImproveRelationTarget},
	{"GetSettlement", LuaBGGetSettlement},
	{"GetFamily", LuaBGGetFamily},
	{"GetName", LuaBGGetName},
	{"Kill", LuaBGKill},
	{"Popularity", LuaBGPopularity},
	{"ChangePopularity", LuaBGChangePopularity},
	{"SuccessMargin", LuaBGSuccessMargin},
	{"PlotsAgainst", LuaBGPlotsAgainst},
	{"Recruit", LuaBGRecruit},
	{"IsRecruiting", LuaBGIsRecruiting},
	{"RetinueSize", LuaBGRetinueSize},
	{"GetRetinueTable", LuaBGRetinueTable},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuyOpinion[] = {
	{"Action", LuaBGOpinionAction},
	{"Relation", LuaBGOpinionRelation},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuyRelation[] = {
	{"GetOpinion", LuaBGRelationGetOpinion},
	{"GetRelationList", LuaBGRelationGetRelationList},
	{"BigGuy", LuaBGRelationBigGuy},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsSettlement[] = {
	{"GetLeader", LuaSettlementGetLeader},
	{"GetGovernment", LuaSettlementGetGovernment},
	{"RaiseArmy", LuaSettlementRaiseArmy},
	{"GetPopulation", LuaSettlementGetPopulation},
	{"CountWarriors", LuaSettlementCountWarriors},
	{"GetBigGuys", LuaSettlementGetBigGuys},
	{"GetNutrition", LuaSettlementGetNutrition},
	{"GetYearlyNutrition", LuaSettlementYearlyNutrition},
	{"CountAcres", LuaSettlementCountAcres},
	{"ExpectedYield", LuaSettlementExpectedYield},
	{"YearlyDeaths", LuaSettlementYearlyDeaths},
	{"YearlyBirths", LuaSettlementYearlyBirths},
	{"BulitinPost", LuaSettlementBulitinPost},
	{"GetBulitins", LuaSettlementGetBulitins},
	{"CountAdults", LuaSettlementCountAdults},
	{"GetFreeWarriors", LuaSettlementGetFreeWarriors},
	{"GetMaxWarriors", LuaSettlementGetMaxWarriors},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBulitin[] = {
	{"Next", LuaBulitinNext},
	{"Prev", LuaBulitinPrev},
	{"NextItr", LuaBulitinNextItr},
	{"PrevItr", LuaBulitinPrevItr},
	{"GetOwner", LuaBulitinGetOwner},
	{"DaysLeft", LuaBulitinGetDaysRemaining},
	{"GetName", LuaBulitinGetName},
	{"GetMission", LuaBulitinGetMission},
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
	{NULL, NULL}
};

static luaL_Reg g_LuaFuncsPolicy[] = {
	{"Name", LuaPolicyName},
	{"Category", LuaPolicyCategory},
	{"Options", LuaPolicyOptions},
	{NULL, NULL}
};

static luaL_Reg g_LuaFuncsPolicyOption[] = {
	{"Name", LuaPolicyOptionName},
	{NULL, NULL}
};

const struct LuaObjectReg g_LuaSettlementObjects[] = {
	{"Army", NULL, g_LuaFuncsArmy},
	{"Government", NULL, g_LuaFuncsGovernment},
	{"BigGuy", NULL, g_LuaFuncsBigGuy},
	{"BigGuyRelation", NULL, g_LuaFuncsBigGuyRelation},
	{"Settlement", NULL, g_LuaFuncsSettlement},
	{"BuildMat", NULL, NULL},
	{"Bulitin", NULL, g_LuaFuncsBulitin},
	{"PlotAction", NULL, g_LuaFuncsPlotAction},
	{"Plot", NULL, g_LuaFuncsPlot},
	{"Policy", NULL, g_LuaFuncsPolicy},
	{"PolicyOption", NULL, g_LuaFuncsPolicyOption},
	{"BigGuyOpinion", NULL, g_LuaFuncsBigGuyOpinion},
	{NULL, NULL}
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
	{NULL, 0}
};

const struct LuaEnum g_LuaPolicyEnum[] = {
	{"Economy", POLCAT_ECONOMY},
	{"Law", POLCAT_LAW},
	{"Military", POLCAT_MILITARY},
	{NULL, 0}
};

const struct LuaEnum g_LuaBigGuyActionEnum[] = {
	{"Influence", BGACT_IMRPOVEREL},
	{"StealCattle", BGACT_STEALCATTLE},
	{"Sabotage", BGACT_SABREL},
	{"Duel", BGACT_DUEL},
	{"Murder", BGACT_MURDER},
	{"Convince", BGACT_CONVINCE},
	{"PlotOverthrow", BGACT_PLOTOVERTHROW},
	{NULL, 0}
};

const struct LuaEnum g_LuaBigGuyRelationEnum[] = {
	{"Token", OPINION_TOKEN},
	{"Small", OPINION_SMALL},
	{"Average", OPINION_AVERAGE},
	{"Great", OPINION_GREAT},
	{NULL, 0}
};

const struct LuaEnumReg g_LuaSettlementEnums[] = {
	{"Plot", NULL,  g_LuaPlotEnum},
	{"Plot", "Type", g_LuaPlotTypeEnum},
	{"BigGuy", "Action", g_LuaBigGuyActionEnum},
	{"BigGuy", "Relation", g_LuaBigGuyRelationEnum},
	{"Policy", NULL, g_LuaPolicyEnum},
	{NULL, NULL}
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

int LuaBGGetCombat(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_COMBAT]);
	return 1;
}

int LuaBGGetStrength(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_STRENGTH]);
	return 1;
}

int LuaBGGetToughness(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_TOUGHNESS]);
	return 1;
}

int LuaBGGetAgility(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_AGILITY]);
	return 1;
}

int LuaBGGetWit(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_WIT]);
	return 1;
}

int LuaBGGetIntrigue(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_INTRIGUE]);
	return 1;
}

int LuaBGGetCharisma(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_CHARISMA]);
	return 1;
}

int LuaBGGetIntellegence(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_INTELLEGENCE]);
	return 1;
}

int LuaBGOpposedChallanged(lua_State* _State) {
	struct BigGuy* _One = LuaCheckClass(_State, 1, "BigGuy");
	struct BigGuy* _Two = LuaCheckClass(_State, 2, "BigGuy");

	if(_One == NULL)
		return luaL_error(_State, "Agument #1 is not a BigGuy.");
	if(_Two == NULL)
		return luaL_error(_State, "Agument #2 is not a BigGuy.");
	lua_pushinteger(_State, BigGuyOpposedCheck(_One, _Two, luaL_checkinteger(_State, 3)));
	return 1;
}

int LuaBGGetAgent(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct Agent* _Agent = RBSearch(&g_GameWorld.Agents, _Guy);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
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

int LuaBGRelItrNext(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, lua_upvalueindex(1), "BigGuyRelation");

	if(_Relation->Next == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, "BigGuyRelation", _Relation->Next);
	lua_pushvalue(_State, -1);
	lua_replace(_State, lua_upvalueindex(1));
	return 1;
}

int LuaBGRelationItr(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	if(_Guy->Relations == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, "BigGuyRelation", _Guy->Relations); 
	lua_pushcclosure(_State, LuaBGRelItrNext, 1);
	return 1;
}

int LuaBGSetOpinion(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct BigGuy* _Target = LuaCheckClass(_State, 2, "BigGuy");
	int _Action = luaL_checkinteger(_State, 3);
	int _Mod = luaL_checkinteger(_State, 4);
	int _Length = luaL_checkinteger(_State, 5);
	int _Strength = luaL_checkinteger(_State, 6);

	BigGuyAddOpinion(_Guy, _Target, _Action, _Mod, _Length, _Strength);
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

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	if(_Guy->Action.Type == BGACT_IMRPOVEREL) {
		LuaCtor(_State, "BigGuy", _Guy->Action.Target);
	} else {
		return luaL_error(_State, "ImproveRelationTarget argument #1 is not improving relations.");
	}
	return 1;
}

int LuaBGGetSettlement(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	LuaCtor(_State, "Settlement", FamilyGetSettlement(_Guy->Person->Family));
	return 1;
}

int LuaBGGetFamily(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	LuaCtor(_State, "Family", _Guy->Person->Family);
	return 1;
}

int LuaBGGetName(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	lua_pushstring(_State, _Guy->Person->Name);
	return 1;
}

int LuaBGKill(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	PersonDeath(_Guy->Person);
//	DestroyBigGuy(_Guy);
	return 0;
}

int LuaBGPopularity(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	lua_pushinteger(_State, BigGuyPopularity(_Guy));
	return 1; 
}

int LuaBGChangePopularity(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	int _Change = luaL_checkinteger(_State, 2);

	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	_Guy->Popularity += _Change;
	return 0;
}

int LuaBGSuccessMargin(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	int _Skill = luaL_checkinteger(_State, 2);
	int _ReqSkill = 0;

	if(lua_gettop(_State) > 2)
		_ReqSkill = luaL_checkinteger(_State, 3);
	else
		_ReqSkill = SKILLCHECK_DEFAULT;
	if(_Guy == NULL)
		return LuaClassError(_State, 1, "BigGuy");
	lua_pushinteger(_State, BigGuySuccessMargin(_Guy, _Skill, _ReqSkill));	
	return 1;
}

int LuaBGPlotsAgainst(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	CreateLuaLnkLstItr(_State, &_Guy->PlotsAgainst, "Plot");
	return 1;
}

int LuaBGRecruit(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct Retinue* _Retinue = NULL;
	struct Settlement* _Home = PersonHome(_Guy->Person);

	for(struct LnkLst_Node* _Itr = _Home->Retinues.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_Retinue = _Itr->Data;
		if(_Retinue->Leader != _Guy)
			continue;
		_Retinue->IsRecruiting = (_Retinue->IsRecruiting ^ 1);
		return 0;
	}
	_Retinue = CreateRetinue(_Guy);
	_Retinue->IsRecruiting = (_Retinue->IsRecruiting ^ 1);
	SettlementAddRetinue(_Home, _Retinue);
	return 0;
}

int LuaBGIsRecruiting(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct Retinue* _Retinue = NULL;
	struct Settlement* _Home = PersonHome(_Guy->Person);

	for(struct LnkLst_Node* _Itr = _Home->Retinues.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_Retinue = _Itr->Data;
		if(_Retinue->Leader != _Guy)
			continue;
		lua_pushboolean(_State, _Retinue->IsRecruiting);
		return 1;
	}
	lua_pushboolean(_State, 0);
	return 1;
}

int LuaBGRetinueSize(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct Retinue* _Retinue = NULL;
	struct Settlement* _Home = PersonHome(_Guy->Person);

	for(struct LnkLst_Node* _Itr = _Home->Retinues.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_Retinue = _Itr->Data;
		if(_Retinue->Leader != _Guy)
			continue;
		lua_pushinteger(_State, _Retinue->Warriors.Size);
		return 1;
	}
	lua_pushinteger(_State, 0);
	return 1;
}

int LuaBGRetinueTable(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");
	struct Retinue* _Retinue = NULL;
	struct Settlement* _Home = PersonHome(_Guy->Person);

	for(struct LnkLst_Node* _Itr = _Home->Retinues.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_Retinue = _Itr->Data;
		if(_Retinue->Leader != _Guy)
			continue;
		goto found_retinue;
	}
	lua_createtable(_State, 0, 0);
	return 1;
	found_retinue:
	lua_createtable(_State, _Retinue->Warriors.Size, 0);
	for(int i = 0; i < _Retinue->Warriors.Size; ++i) {
		LuaCtor(_State, "Person", _Retinue->Warriors.Table[i]);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
}

int LuaBGOpinionAction(lua_State* _State) {
	struct BigGuyOpinion* _Opinion = LuaCheckClass(_State, 1, "BigGuyOpinion");

	lua_pushstring(_State, g_BigGuyOpinionActions[_Opinion->Action]);
	return 1;
}

int LuaBGOpinionRelation(lua_State* _State) {
	struct BigGuyOpinion* _Opinion = LuaCheckClass(_State, 1, "BigGuyOpinion");

	lua_pushinteger(_State, _Opinion->RelMod);
	return 1;

}

int LuaBGRelationGetOpinion(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, 1, "BigGuyRelation");

	if(_Relation == NULL)
		return LuaClassError(_State, 1, "BigGuyRelation");
	lua_pushinteger(_State, _Relation->Modifier);
	return 1;
}

int LuaBGRelationGetRelationList(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, 1, "BigGuyRelation");
	int _Idx = 1;

	lua_createtable(_State, 6, 0);	
	for(struct BigGuyOpinion* _Opinion = _Relation->Opinions; _Opinion != NULL; _Opinion = _Opinion->Next, ++_Idx) {
		LuaCtor(_State, "BigGuyOpinion", _Opinion);
		lua_rawseti(_State, -2, _Idx);
	}
	return 1;
}

int LuaBGRelationBigGuy(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, 1, "BigGuyRelation");

	if(_Relation == NULL)
		return LuaClassError(_State, 1, "BigGuyRelation");
	LuaCtor(_State, "BigGuy", _Relation->Person);
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

int LuaGovernmentGetLeader(lua_State* _State) {
	struct Government* _Gov = LuaCheckClass(_State, 1, "Government");

	LuaCtor(_State, "BigGuy", _Gov->Leader);
	return 1;
}

int LuaGovernmentGetJudge(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");
	
	LuaCtor(_State, "BigGuy", _Government->Appointments.Judge);
	return 1;
}

int LuaGovernmentGetMarshall(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");
	
	LuaCtor(_State, "BigGuy", _Government->Appointments.Marshall);
	return 1;
}

int LuaGovernmentGetSteward(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");
	
	LuaCtor(_State, "BigGuy", _Government->Appointments.Steward);
	return 1;
}

int LuaGovernmentHasPolicy(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");
	struct Policy* _Policy = LuaCheckClass(_State, 2, "Policy");

	lua_pushboolean(_State, GovernmentHasPolicy(_Government, _Policy));
	return 1;
}

int LuaGovernmentGetPolicyCategory(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");
	struct Policy* _Policy = LuaCheckClass(_State, 2, "Policy");
	struct ActivePolicy* _ActPol = NULL;
	int _Category = luaL_checkinteger(_State, 3) - 1;

	if(_Category < 0 || _Category >= POLICY_SUBSZ) 
		return luaL_error(_State, "Invalid input %d for category.", _Category);
	for(struct LnkLst_Node* _Itr = _Government->PolicyList.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_ActPol = _Itr->Data;
		if(_ActPol->Policy != _Policy)
			continue;
		lua_pushinteger(_State, _ActPol->OptionSel[_Category] + 1);
		return 1;	
	}
	lua_pushinteger(_State, -1);
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

//Used for the function below as a placeholder to decide which settlement to raid.
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

	CreateLuaLnkLstItr(_State, &_Settlement->BigGuys, "BigGuy");
	return 1;
}

int LuaSettlementGetNutrition(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	lua_pushinteger(_State, SettlementGetNutrition(_Settlement));
	return 1;
}

int LuaSettlementYearlyNutrition(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	lua_pushinteger(_State, SettlementYearlyNutrition(_Settlement));
	return 1;
}

int LuaSettlementCountAcres(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	lua_pushinteger(_State, SettlementCountAcres(_Settlement));
	return 1;
}

int LuaSettlementExpectedYield(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	lua_pushinteger(_State, SettlementExpectedYield(_Settlement));
	return 1;
}

int LuaSettlementYearlyDeaths(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	lua_pushinteger(_State, _Settlement->YearDeaths);
	return 1;
}

int LuaSettlementYearlyBirths(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	lua_pushinteger(_State, _Settlement->YearBirths);
	return 1;
}

int LuaSettlementBulitinPost(lua_State* _State) {
	struct BulitinItem* _Item = NULL;
	struct Mission* _Mission = NULL;
	struct Mission* _MissionFail = NULL;
	const char* _MissionStr = luaL_checkstring(_State, 1);
	const char* _MissionFailStr = luaL_checkstring(_State, 2);
	int _DaysLeft = luaL_checkint(_State, 3);
	int _Priority = luaL_checkint(_State, 4);
	
	if((_Mission = StrToMission(_MissionStr)) == NULL) {
		luaL_error(_State, "%s is not a mission name.", _MissionStr);
	}
	if((_MissionFail = StrToMission(_MissionFailStr)) == NULL) {
		luaL_error(_State, "%s is not a mission name.", _MissionFailStr);
	}
	_Item = CreateBulitinItem(_Mission, NULL, MissionDataOwner(MissionDataTop()), _DaysLeft, _Priority);		 
	ILL_CREATE(MissionDataOwner(MissionDataTop())->Person->Family->HomeLoc->Bulitin, _Item);
	return 0;
}

int LuaSettlementGetBulitins(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	LuaCtor(_State, "Bulitin", _Settlement->Bulitin);
	return 1;
}

int LuaSettlementCountAdults(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	if(_Settlement == NULL)
		return LuaClassError(_State, 1, "Settlement");
	lua_pushinteger(_State, SettlementAdultPop(_Settlement));
	return 1;
}

int LuaSettlementGetFreeWarriors(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	lua_pushinteger(_State, _Settlement->FreeWarriors.Size);
	return 1;
}

int LuaSettlementGetMaxWarriors(lua_State* _State) {
	struct Settlement* _Settlement = LuaCheckClass(_State, 1, "Settlement");

	lua_pushinteger(_State, _Settlement->MaxWarriors);
	return 1;
}

int LuaBulitinNext(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, 1, "Bulitin");

	LuaCtor(_State, "Bulitin", _Item->Next);
	return 1;
}

int LuaBulitinPrev(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, 1, "Bulitin");

	LuaCtor(_State, "Bulitin", _Item->Prev);
	return 1;
}

int LuaBulitinNextItr_Aux(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, lua_upvalueindex(1), "Bulitin");

	if(_Item == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, "Bulitin", _Item);
	LuaCtor(_State, "Bulitin", _Item->Next);
	lua_replace(_State, lua_upvalueindex(1));
	return 1;
}

int LuaBulitinNextItr(lua_State* _State) {
	LuaCheckClass(_State, 1, "Bulitin");
	lua_pushcclosure(_State, LuaBulitinNextItr_Aux, 1);
	return 1;
}

int LuaBulitinPrevItr_Aux(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, lua_upvalueindex(1), "Bulitin");

	if(_Item == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, "Bulitin", _Item);
	LuaCtor(_State, "Bulitin", _Item->Prev);
	lua_replace(_State, lua_upvalueindex(1));
	return 1;
}

int LuaBulitinPrevItr(lua_State* _State) {
	lua_pushcfunction(_State, LuaBulitinPrev);
	return 1;
}

int LuaBulitinGetOwner(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, 1, "Bulitin");

	LuaConstCtor(_State, "BigGuy", _Item->Owner);
	return 1;
}

int LuaBulitinGetDaysRemaining(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, 1, "Bulitin");
	
	lua_pushinteger(_State, _Item->DaysLeft);
	return 1;
}

int LuaBulitinGetName(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, 1, "Bulitin");

	lua_pushstring(_State, BulitinItemGetName(_Item));
	return 1;
}

int LuaBulitinGetMission(lua_State* _State) {
	struct BulitinItem* _Item = LuaCheckClass(_State, 1, "Bulitin");

	lua_pushinteger(_State, _Item->SuccMission->Id);
	return 1;
}

int LuaPlotActionDescribe(lua_State* _State) {
	struct PlotAction* _Action = LuaCheckClass(_State, 1, "PlotAction");
	char* _Buffer = FrameAlloc(1024);
	
	PlotActionEventStr(_Action, &_Buffer, 1024);
	lua_pushstring(_State, _Buffer);
	return 1;
}

int LuaPlotActionGetType(lua_State* _State) {
	struct PlotAction* _Action = LuaCheckClass(_State, 1, "PlotAction");

	lua_pushinteger(_State, _Action->Type);
	return 1;
}

int LuaPlotJoin(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");
	struct BigGuy* _Guy = LuaCheckClass(_State, 2, "BigGuy");
	int _Side = luaL_checkinteger(_State, 3);

	if(_Plot == NULL)
		return LuaClassError(_State, 1, "Plot");
	if(_Guy == NULL)
		return LuaClassError(_State, 2, "BigGuy");
	if(_Side != PLOT_ATTACKERS && _Side != PLOT_DEFENDERS)
		return luaL_error(_State, "Plot.Join uses an invalid number for Side.");
	PlotJoin(_Plot, _Side, _Guy);
	return 0;
}

int LuaPlotInPlot(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");
	struct BigGuy* _Guy = LuaCheckClass(_State, 2, "BigGuy");

	if(_Plot == NULL)
		return LuaClassError(_State, 1, "Plot");
	if(_Guy == NULL)
		return LuaClassError(_State, 2, "BigGuy");
	lua_pushboolean(_State, IsInPlot(_Plot, _Guy));
	return 1;
}

int LuaPlotPlotters(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	if(_Plot == NULL)
		return LuaClassError(_State, 1, "Plot");
	CreateLuaLnkLstItr(_State, &_Plot->Side[PLOT_ATTACKERS], "BigGuy");
	return 1;
}

int LuaPlotDefenders(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	if(_Plot == NULL)
		return LuaClassError(_State, 1, "Plot");
	CreateLuaLnkLstItr(_State, &_Plot->Side[PLOT_DEFENDERS], "BigGuy");
	return 1;
}

int LuaPlotTypeStr(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	lua_pushstring(_State, PlotTypeStr(_Plot));
	return 1;
}

int LuaPlotLeader(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	if(_Plot == NULL)
		return LuaClassError(_State, 1, "Plot");
	LuaCtor(_State, _Plot->Side[PLOT_ATTACKERS].Front->Data, "BigGuy");
	return 1;
}
	
int LuaPlotTarget(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	if(_Plot == NULL)
		return LuaClassError(_State, 1, "Plot");
	LuaCtor(_State, _Plot->Side[PLOT_DEFENDERS].Front->Data, "BigGuy");
	return 1;
}

int LuaPlotGetScore(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	lua_pushinteger(_State, _Plot->WarScore);
	return 1;
}

int LuaPlotCreate(lua_State* _State) {
	struct Plot* _Plot = NULL;
	struct BigGuy* _Leader = LuaCheckClass(_State, 1, "BigGuy");
	struct BigGuy* _Target = LuaCheckClass(_State, 2, "BigGuy");
	int _Type = luaL_checkinteger(_State, 3);
	void* _Data = NULL;
	struct ActivePolicy* _PolAct = NULL; 
	int _PolicyCat = 0;

	if(_Leader == NULL)
		return luaL_error(_State, "Leader is invalid.");
	if(BigGuyHasPlot(_Leader) != 0)
		return luaL_error(_State, "Leader already has a plot.");
	if(IsPlotTypeValid(_Type) == 0)
		return luaL_error(_State, "Invalid plot type.");
	switch(_Type) {
		case PLOT_REMOVEPOLICY:
		case PLOT_PASSPOLICY:
			_Data = LuaCheckClass(_State, 4, "Policy");
			break;
		case PLOT_CHANGEPOLICY:
			_PolAct = malloc(sizeof(struct ActivePolicy));

			lua_rawgeti(_State, 4, 1);
			_PolAct->Policy = LuaCheckClass(_State, -1, "Policy");
			lua_rawgeti(_State, 4, 2);
			_PolicyCat = lua_tointeger(_State, -1) - 1;
			if(ValidPolicyCategory(_PolicyCat) == 0)
				return luaL_error(_State, "Invalid policy category %d.", _PolicyCat);
			lua_rawgeti(_State, 4, 3);
			for(int i = 0; i < POLICY_SUBSZ; ++i)
				_PolAct->OptionSel[i] = POLICYACT_IGNORE;
			_PolAct->OptionSel[_PolicyCat] = lua_tointeger(_State, -1) - 1;
			_Data = _PolAct;
		break;

	}
	_Plot = CreatePlot(_Type, _Data, _Leader, _Target);
	//_Plot->WarScore = 9;
	LuaCtor(_State, "Plot", _Plot);
	return 1;
}

int LuaPlotAddAction(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");
	int _Type = luaL_checkinteger(_State, 2);
	struct BigGuy* _Actor = LuaCheckClass(_State, 3, "BigGuy");
	struct BigGuy* _Target = LuaCheckClass(_State, 4, "BigGuy");

	PlotAddAction(_Plot, _Type, _Actor, _Target);
	return 0;
}

int LuaPlotGetThreat(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");

	lua_pushinteger(_State, PlotGetThreat(_Plot));
	return 1;
}

int LuaPlotPrevMonthActions(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");
	int i = 0;

	lua_createtable(_State, 0, 6);
	for(const struct PlotAction* _Action = PlotPrevActList(_Plot); _Action != NULL; _Action = _Action->Next) {
		LuaCtor(_State, "PlotAction", (struct PlotAction*) _Action);
		lua_rawseti(_State, -2, ++i);
	}
	//CreateLuaLnkLstItr(_State, PlotPrevActList(_Plot), "PlotAction");
	return 1;
}

int LuaPlotCurrMonthActions(lua_State* _State) {
	struct Plot* _Plot = LuaCheckClass(_State, 1, "Plot");
	int i = 0;

	lua_createtable(_State, 0, 6);
	for(const struct PlotAction* _Action = PlotPrevActList(_Plot); _Action != NULL; _Action = _Action->Next) {
		LuaCtor(_State, "PlotAction", (struct PlotAction*) _Action);
		lua_rawseti(_State, -2, ++i);
	}
	//CreateLuaLnkLstItr(_State, PlotCurrActList(_Plot), "PlotAction");
	return 1;
}

int LuaPolicyOptionName(lua_State* _State) {
	struct PolicyOption* _Opt = LuaCheckClass(_State, 1, "PolicyOption");

	lua_pushstring(_State, _Opt->Name);
	return 1;
}

int LuaPolicyName(lua_State* _State) {
	struct Policy* _Policy = LuaCheckClass(_State, 1, "Policy");

	lua_pushstring(_State, _Policy->Name);
	return 1;
}

int LuaPolicyCategory(lua_State* _State) {
	struct Policy* _Policy = LuaCheckClass(_State, 1, "Policy");

	lua_pushinteger(_State, _Policy->Category);
	return 1;
}

//FIXME: This should be done on initialization for each Policy.
int LuaPolicyOptions(lua_State* _State) {
	struct Policy* _Policy = LuaCheckClass(_State, 1, "Policy");
	int _Last = 0;
	int _Ct = 1;
	int _Idx = 0;

	lua_createtable(_State, POLICY_SUBSZ, 0);
	lua_createtable(_State, _Policy->Options.Size[0], 1);
	lua_pushstring(_State, "Name");
	lua_pushstring(_State, _Policy->Options.Name[0]);
	lua_rawset(_State, -3);
	for(int i = 0; i < _Policy->OptionsSz; ++i, ++_Ct) {
		LuaCtor(_State, "PolicyOption", (void*)&_Policy->Options.Options[i]);
		lua_rawseti(_State, -2, _Ct);
		if(i >= (_Policy->Options.Size[_Idx] + _Last) - 1) {
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
		}
	}
	//lua_rawseti(_State, -2, _Idx + 1);
	return 1;	
}
