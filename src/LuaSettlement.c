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
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsBigGuyRelation[] = {
	{"GetOpinion", LuaBGRelationGetOpinion},
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

static const luaL_Reg g_LuaFuncsPlot[] = {
	{"Join", LuaPlotJoin},
	{"InPlot", LuaPlotInPlot},
	{"Plotters", LuaPlotPlotters},
	{"Defenders", LuaPlotDefenders},
	{"Leader", LuaPlotLeader},
	{"Target", LuaPlotTarget},
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
	{"Bulitin", NULL, g_LuaFuncsBulitin},
	{"Plot", NULL, g_LuaFuncsPlot},
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

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_ADMINISTRATION]);
	return 1;
}

int LuaBGGetIntrigue(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_INTRIGUE]);
	return 1;
}

int LuaBGGetStrategy(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_STRATEGY]);
	return 1;
}

int LuaBGGetWarfare(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_WARFARE]);
	return 1;
}

int LuaBGGetTactics(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_TACTICS]);
	return 1;
}

int LuaBGGetCharisma(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_CHARISMA]);
	return 1;
}

int LuaBGGetPiety(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, "BigGuy");

	lua_pushinteger(_State, _Guy->Stats[BGSKILL_PIETY]);
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

int LuaBGRelationGetOpinion(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, 1, "BigGuyRelation");

	if(_Relation == NULL)
		return LuaClassError(_State, 1, "BigGuyRelation");
	lua_pushinteger(_State, _Relation->Modifier);
	return 1;
}

int LuaBGRelationBigGuy(lua_State* _State) {
	struct BigGuyRelation* _Relation = LuaCheckClass(_State, 1, "BigGuyRelation");

	if(_Relation == NULL)
		return LuaClassError(_State, 1, "BigGuyRelation");
	LuaCtor(_State, "BigGuy", _Relation->Person);
	return 1;
}

int LuaGovernmentPossibleReforms(lua_State* _State) {
	struct Government* _Government = LuaCheckClass(_State, 1, "Government");

	CreateLuaLnkLstItr(_State, &_Government->PossibleReforms, "Reform");
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

	LuaCtor(_State, "LinkedList", &_Settlement->BigGuys);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "BigGuy");
	lua_rawset(_State, -3);
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
