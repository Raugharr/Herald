/*
 * File: Setup.c
 * Author: David Brotz
 */

#include "Setup.h"

#include "BehaviorTree.h"
#include "Behaviors.h"
#include "LuaLib.h"
#include "goap.h"

#include "../Location.h"
#include "../Family.h"
#include "../BigGuy.h"
#include "../Person.h"
#include "../Government.h"
#include "../ArmyGoal.h"
#include "../Warband.h"

#include "../sys/LinkedList.h"
#include "../sys/HashTable.h"
#include "../sys/Log.h"
#include "../sys/Stack.h"
#include "../sys/LuaCore.h"
#include "../sys/Math.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct GOAPPlanner g_Goap;

int BGImproveRelations(const void* _Data, const void* _Extra) {
	return 120;
}

int BGImproveRelationsAction(struct BigGuy* _Guy) {
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct BigGuy* _Person = NULL;
	int _Rand = Random(0, _Settlement->BigGuys.Size - 1);
	int _Ct = 0;

	if(_Guy->ActionFunc == NULL) {
		while(_Itr != NULL) {
			_Person = (struct BigGuy*)_Itr->Data;
			if(_Person != _Guy) {
				 if(_Ct >= _Rand)
					 break;
				++_Ct;
			}
			_Itr = _Itr->Next;
		}
		BigGuySetAction(_Guy, BGACT_IMRPOVEREL, _Person, NULL);
		return 0;
	}
	//Commented out as BigGuySetAction will most likely be removed.
	/*_Relation = BigGuyGetRelation((struct BigGuy*)_Guy->Action.Target, _Guy);
	if(_Relation->Modifier >= BIGGUY_LIKEMIN) {
		BigGuySetAction(_Guy, BGACT_NONE, NULL, NULL);
		return 1;
	}*/
	return 0;
}

int UtilityMakeFriends(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	int _Friends = 0;
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	const struct BigGuy* _Person = NULL;
	const struct BigGuyRelation* _Relation = NULL;

	*_Min = 0;
	*_Max = _Settlement->BigGuys.Size - 1;
	while(_Itr != NULL) {
		_Person = (struct BigGuy*)_Itr->Data;
		if(_Person == _Guy)
			goto end_loop;
		if((_Relation = BigGuyGetRelation(_Guy, _Person)) != NULL && _Relation->Relation >= BGREL_LIKE)
			++_Friends;
		end_loop:
		_Itr = _Itr->Next;
	}
	WorldStateSetAtom(_State, BGBYTE_IMPROVINGRELATION, 1);
	return *_Max;
//	return _Friends;
}

int BGRaiseFyrd(const void* _Data, const void* _Extra) {
	return 1;
}

int BGRaiseFyrdAction(struct BigGuy* _Guy) {
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	struct ArmyGoal _Goal;
	struct LinkedList _List = {0, NULL, NULL};

	WorldSettlementsInRadius(&g_GameWorld, &_Settlement->Pos, 20, &_List);
	if(_List.Size <= 0)
		goto end;
	ArmyGoalRaid(&_Goal, (struct Settlement*)&_List.Back->Data);
	SettlementRaiseFyrd(_Settlement, &_Goal);
	end:
	LnkLstClear(&_List);
	return 1;
}

int UtilityRaiseFyrdFood(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	struct Settlement* _Settlement = FamilyGetSettlement(_Guy->Person->Family);
	int _MaxNutrition = _Settlement->NumPeople * NUTRITION_REQ / 4; //HOw much nutrition we need for 3 months.
	int _Nutrition = SettlementGetNutrition(_Settlement);

	if(_Nutrition >= _MaxNutrition)
		return 0;
	*_Min = 0;
	*_Max = _MaxNutrition;
	WorldStateSetAtom(_State, BGBYTE_FYRDRAISED, 1);
	return _Nutrition;
}

int BGChallangeLeader(const void* _Data, const void* _Extra) {
	return 1;
}

int BGChallangeLeaderAction(struct BigGuy* _Guy) {
	struct Government* _Government = FamilyGetSettlement(_Guy->Person->Family)->Government;
	struct BigGuy* _Leader = _Government->Leader;

	if(_Guy->Stats.Warfare > _Leader->Stats.Warfare)
		GovernmentSetLeader(_Government, _Guy);
	return 1;
}

int UtilityChallangeLeader(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	struct BigGuy* _Leader =  FamilyGetSettlement(_Guy->Person->Family)->Government->Leader;
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Leader);
	int _Utility = 0;

	*_Min = 0;
	*_Max = 255;
	if(_Guy->Stats.Warfare < _Leader->Stats.Warfare || (_Relation != NULL && _Relation->Relation == BGREL_LOVE))
		return 0;
	_Utility = _Utility + ((_Guy->Stats.Warfare - _Leader->Stats.Warfare) * 5);
	if(_Relation != NULL)
		_Utility = _Utility + ((-_Relation->Modifier) * 2);
	return (_Utility >= 255) ? (255) : (_Utility);
}

int BGSufficientFriends(const void* _Data, const void* _Extra) {
	return 1;
}

int BGSufficientFriendsAction(struct BigGuy* _Guy) {
	return 1;
}

int BGSufficientEnemies(const void* _Data, const void* _Extra) {
	return 1;
}

int BGSufficientEnemiesAction(struct BigGuy* _Guy) {
	return 1;
}

int UtilitySufficientFriends(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	*_Min = 0;
	*_Max = 0;
	return 1;
}


int UtilitySufficientEnemies(const struct BigGuy* _Guy, int* _Min, int* _Max, struct WorldState* _State) {
	*_Min = 0;
	*_Max = 0;
	return 1;
}

void AgentActionImproveRelations(struct GOAPPlanner* _GoPlan) {
	GoapAddPostcond(_GoPlan, "Improve Relations", "ImproveRelations", 1, WSOP_ADD);
	GoapSetActionCost(_GoPlan, "Improve Relations", BGImproveRelations);
	GoapSetAction(_GoPlan, "Improve Relations", (AgentActionFunc) BGImproveRelationsAction);
	GoapAddUtility(_GoPlan, "ImrpoveRelations", (AgentUtilityFunc)UtilityMakeFriends, (UTILITY_INVERSE | UTILITY_LINEAR));
}

void AgentActionChallangeLeader(struct GOAPPlanner* _GoPlan) {
	//Both of these should use the ImproveRelations action as a precond.
	GoapAddPrecond(_GoPlan, "Challenge Leader", "SufficientEnemies", 1, WSOP_EQUAL);
	GoapAddPrecond(_GoPlan, "Challenge Leader", "SufficientFriends", 1, WSOP_EQUAL);
	GoapAddPostcond(_GoPlan, "Challenge Leader", "IsLeader", 1, WSOP_SET);
	GoapSetActionCost(_GoPlan, "Challenge Leader", BGChallangeLeader);
	GoapSetAction(_GoPlan, "Challenge Leader", (AgentActionFunc) BGChallangeLeaderAction);
	GoapAddUtility(_GoPlan, "Challenge Leader", (AgentUtilityFunc)UtilityChallangeLeader, UTILITY_LINEAR);
}

void AgentActionRaid(struct GOAPPlanner* _GoPlan) {
	GoapAddPrecond(_GoPlan, "Raid", "FyrdRaised", 0, WSOP_EQUAL);
	GoapAddPostcond(_GoPlan, "Raid", "FyrdRaised", 1, WSOP_SET);
	GoapAddPostcond(_GoPlan, "Raid", "Prestige", 2, WSOP_ADD);
	GoapSetActionCost(_GoPlan, "Raid", BGRaiseFyrd);
	GoapSetAction(_GoPlan, "Raid", (AgentActionFunc) BGRaiseFyrdAction);
	GoapAddUtility(_GoPlan, "Raid", (AgentUtilityFunc) UtilityRaiseFyrdFood, (UTILITY_INVERSE | UTILITY_QUADRATIC));
}

void AgentActionSufficientFriends(struct GOAPPlanner* _GoPlan) {
	GoapAddPostcond(_GoPlan, "Sufficient Friends", "SufficientFriends", 1, WSOP_SET);
	GoapSetActionCost(_GoPlan, "Sufficient Friends", BGSufficientFriends);
	GoapSetAction(_GoPlan, "Sufficient Friends", (AgentActionFunc) BGSufficientFriendsAction);
	GoapAddUtility(_GoPlan, "SufficientFriends", (AgentUtilityFunc) UtilitySufficientFriends, UTILITY_LINEAR);
}

void AgentActionSufficientEnemies(struct GOAPPlanner* _GoPlan) {
	GoapAddPostcond(_GoPlan, "Sufficient Enemies", "SufficientEnemies", 1, WSOP_SET);
	GoapSetActionCost(_GoPlan, "Sufficient Friends", BGSufficientEnemies);
	GoapSetAction(_GoPlan, "Sufficient Enemies", (AgentActionFunc) BGSufficientEnemiesAction);
	GoapAddUtility(_GoPlan, "SufficientFriends", (AgentUtilityFunc) UtilitySufficientEnemies, UTILITY_LINEAR);	
}

void BGSetup(struct GOAPPlanner* _Planner, const char** _Atoms, int _AtomSz, AgentActions _Actions) {
	GoapClear(_Planner);
	for(int i = 0; i < _AtomSz; ++i)
		GoapAddAtom(_Planner, _Atoms[i]);
	for(int i = 0; _Actions[i] != NULL; ++i)
		_Actions[i](_Planner);
}

void AIInit(lua_State* _State) {
	AgentActions _Actions = {
		AgentActionImproveRelations,
		AgentActionChallangeLeader,
		AgentActionRaid,
		NULL
	};

	if(BehaviorsInit(_State) == 0) {
		Log(ELOG_ERROR, "Behaviors have failed to load.");
		return;
	}
	LuaAILibInit(_State);
	GoapInit();
	BGSetup(NULL, g_BGStateStr, BGBYTE_SIZE, _Actions);
}

void AIQuit() {
	GoapQuit();
}
