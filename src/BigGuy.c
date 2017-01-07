/*
 * Author: David Brotz
 * File: BigGuy.c
 */

#include "BigGuy.h"

#include "Person.h"
#include "Family.h"
#include "World.h"
#include "Location.h"
#include "Good.h"
#include "Government.h"
#include "Trait.h"
#include "Mission.h"
#include "Plot.h"
#include "BigGuyRelation.h"
#include "Retinue.h"

#include "AI/Agent.h"

#include "sys/Math.h"
#include "sys/Constraint.h"
#include "sys/Event.h"
#include "sys/RBTree.h"
#include "sys/Event.h"
#include "sys/LinkedList.h"

#include <stdlib.h>
#include <assert.h>

const char* g_BGMission[BGACT_SIZE] = {
	"Improve Relations"
};

int g_BGActCooldown[BGACT_SIZE] = {
	0,
	30,
	30,
	30,
	30,
	180
};

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->Person->Object.Id - _Two->Person->Object.Id;
}

int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy) {
	return (*_Two) - _BigGuy->Person->Object.Id;
}

int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission) {
	return 0;
}

void BigGuyActionImproveRel(struct BigGuy* _Guy, const struct BigGuyAction* _Action) {
	struct BigGuyRelation* _Relation = NULL;
	int _Mod = _Action->Modifier;

	if(_Mod > 0 && Random(STAT_MIN, STAT_MAX) > _Guy->Stats[BGSKILL_CHARISMA])
		--_Mod;
	if((_Relation = BigGuyGetRelation(_Action->Target, _Guy)) == NULL) {
		//_Relation = CreateBigGuyRelation(_Action->Target, _Guy);
		//CreateBigGuyOpinion(_Relation, OPINION_SMALL, _Mod);
		//BigGuyRelationUpdate(_Relation);
	} else if(_Relation->Relation < BGREL_LIKE) {
		//BigGuyAddRelation(_Relation, OPINION_SMALL, _Mod, );
	}
}

void BigGuyActionGift(struct BigGuy* _Guy, const struct BigGuyAction* _Action) {
	struct Family* _Family = _Action->Target->Person->Family;
	struct GoodBase* _Base = (struct GoodBase*) _Action->Data;

	for(int i = 0; i < _Family->Goods.Size; ++i)
		if(_Base == ((struct Good*)_Family->Goods.Table[i])->Base) {
			struct Good* _Taken = FamilyTakeGood(_Family, i, _Action->Modifier);

			FamilyGetGood(_Guy->Person->Family, _Taken, _Taken->Quantity);
		}
}

/*
struct BigGuyActionHist* CreateBGActionHist(struct BigGuy* _Owner, int _Action) {
	struct BigGuyActionHist* _Hist = (struct BigGuyActionHist*) malloc(sizeof(struct BigGuyActionHist));

	_Hist->Owner = _Owner;
	_Hist->ActionType = _Action;
	_Hist->DayDone = g_GameWorld.Date;
	return _Hist;
}

int BigGuyActionHistIS(const struct BigGuyActionHist* _One, const struct BigGuyActionHist* _Two) {
	int _Diff = _One->Owner - _Two->Owner;
	
	if(_Diff != 0)
		return _Diff;
	return _One->ActionType - _Two->ActionType;
}

void DestroyBGActionHist(struct BigGuyActionHist* _Hist) {
	free(_Hist);
}
*/

void BGOnDeath(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct BigGuy* _Guy = _Data->One;
	struct Person* _Person = _Data->OwnerObj;

	EventHookRemove(_Data->EventType, _Guy, _Person, NULL);
	RBDelete(&g_GameWorld.Agents, _Guy);
	if(_Guy->Agent != NULL)
		DestroyAgent(_Guy->Agent);
	DestroyPerson(_Person);
	DestroyBigGuy(_Guy);
}

void BGOnTargetDeath(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct BigGuy* _Guy = _Data->One;

	EventHookRemove(_Data->EventType, _Guy, _Data->OwnerObj, NULL);
}

/*
void BGOnNewPlot(const struct EventData* _Data, void* _Extra1) {
	struct BigGuy* _Guy = _Data->One;
	struct Plot* _Plot = _Extra1;
	int _LeaderRel = 0; 
	int _TargetRel = 0;

	if(_Guy == PlotLeader(_Plot) || _Guy == PlotTarget(_Plot))
		return;
	_LeaderRel = BigGuyRelation(BigGuyGetRelation(_Guy, PlotLeader(_Plot))); 
	if(PlotTarget(_Plot) != NULL) {
		_TargetRel = BigGuyRelation(BigGuyGetRelation(_Guy, PlotTarget(_Plot)));
	}
	if(_LeaderRel >= BGREL_LIKE && _TargetRel < BGREL_LIKE) {
		PlotJoin(_Plot, PLOT_ATTACKERS, _Guy);	
	} else if(_LeaderRel < BGREL_LIKE && _TargetRel >= BGREL_LIKE) {
		PlotJoin(_Plot, PLOT_DEFENDERS, _Guy);
	}
}*/

struct BigGuy* CreateBigGuy(struct Person* _Person, uint8_t (*_Stats)[BGSKILL_SIZE], int _Motivation) {
	struct BigGuy* _BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	CreateObject((struct Object*)_BigGuy, OBJECT_BIGGUY, (ObjectThink) BigGuyThink);
	_BigGuy->Person = _Person;
	_BigGuy->IsDirty = 1;
	_BigGuy->Relations = NULL;
	memcpy(&_BigGuy->Stats, _Stats, sizeof(uint8_t) * BGSKILL_SIZE);
	_BigGuy->Action = BGACT_NONE;
	_BigGuy->Motivation = _Motivation;
	_BigGuy->Agent = CreateAgent(_BigGuy);
	_BigGuy->Popularity = BGRandRes(_BigGuy, BGSKILL_CHARISMA);
	_BigGuy->Glory = 0;
	_BigGuy->PopularityDelta = 0;
	RBInsert(&g_GameWorld.BigGuys, _BigGuy);
	RBInsert(&g_GameWorld.Agents, _BigGuy->Agent);
	LnkLstPushBack(&FamilyGetSettlement(_Person->Family)->BigGuys, _BigGuy);
	EventHook(EVENT_DEATH, BGOnDeath, _Person, _BigGuy, NULL);
	//EventHook(EVENT_NEWPLOT, BGOnNewPlot, BigGuyHome(_BigGuy), _BigGuy, NULL);

	ConstructLinkedList(&_BigGuy->PlotsAgainst);
	_BigGuy->Traits = BGRandTraits(&_BigGuy->TraitCt);
	_BigGuy->Action = BGACT_NONE;
	return _BigGuy;
}

void DestroyBigGuy(struct BigGuy* _BigGuy) {
	struct LinkedList* _List = &FamilyGetSettlement(_BigGuy->Person->Family)->BigGuys;
	struct LnkLst_Node* _Itr = _List->Front;

	while(_Itr != NULL) {
		if(_Itr->Data == _BigGuy) {
			LnkLstRemove(_List, _Itr);
			break;
		}
		_Itr = _Itr->Next;
	}
	RBDelete(&g_GameWorld.BigGuys, _BigGuy->Person);
	EventHookRemove(EVENT_NEWPLOT, BigGuyHome(_BigGuy), _BigGuy, NULL);
	DestroyObject((struct Object*)_BigGuy);
	free(_BigGuy->Traits);
	free(_BigGuy);
}

void BigGuyThink(struct BigGuy* _Guy) {
	_Guy->Popularity -= g_GameWorld.DecayRate[(int)_Guy->Popularity] / YEAR_DAYS;
	//_Guy->Glory -= g_GameWorld.DecayRate[(int)_Guy->Glory] / YEAR_DAYS;
	for(struct BigGuyRelation* _Relation = _Guy->Relations; _Relation != NULL; _Relation = _Relation->Next) {
		BigGuyRelationUpdate(_Relation);
	}
}

void BigGuySetState(struct BigGuy* _Guy, int _State, int _Value) {
	//WorldStateSetAtom(&_Guy->State, _State, _Value);
	_Guy->IsDirty = 1;
}

struct BigGuy* BigGuyLeaderType(struct Person* _Person) {
	while(_Person != NULL) {
		if(_Person->Gender == EMALE && DateToDays(_Person->Age) > ADULT_AGE) {
			uint8_t _Stats[BGSKILL_SIZE];

			BGStatsWarlord(&_Stats, 50);
			return CreateBigGuy(_Person, &_Stats, BGMOT_RULE); //NOTE: Make sure we aren't making a big guy when the person is already a big guy.
		}
		_Person = _Person->Next;
	}
	return NULL;
}

void BGStatsRandom(int _Points, int _StatCt, ...) {
	va_list _Valist;
	uint8_t* _Stats[_StatCt];
	uint8_t* _Temp = NULL;
	int _Rand = 0;
	int _Result = 0;
	int _PointsLeft = _Points;
	double _StatDist[_StatCt];

	va_start(_Valist, _StatCt);
	for(int i = 0; i < _StatCt; ++i)
		_Stats[i] = va_arg(_Valist, uint8_t*);
	for(int i = 0; i < _StatCt; ++i)
		_StatDist[i] = va_arg(_Valist, double);
	for(int i = 0; i < _StatCt; ++i) {
		_Temp = _Stats[i];
		_Rand = Random(0, _StatCt - 1);
		_Stats[i] = _Stats[_Rand];
		_Stats[_Rand] = _Temp;
	}
	for(int i = 0; i < _StatCt; ++i) {
		_Result = ceil(_Points * _StatDist[i]);
		if(_Result > STAT_MAX)
			_Result = STAT_MAX;
		if(_PointsLeft < _Result)
			_Result = _PointsLeft;
		*_Stats[i] = _Result;
		_PointsLeft -= _Result;
	}
	va_end(_Valist);
}

int BGRandRes(const struct BigGuy* _Guy, int _Stat) {
	return (_Guy->Stats[_Stat] + Random(1, 100)) / 2;
}

void BGStatsWarlord(uint8_t (*_Stats)[BGSKILL_SIZE], int _Points) {
	//int _WarPoints = (_Points <= 400) ? (_Points / 2) : (240);
	int _WarPoints = (_Points / 2);
	int _RemainPoints = _Points - _WarPoints;

	/*
	 * TODO: The percentages given to each stat should be randomized slightly.
	 */
	BGStatsRandom(_WarPoints, 3, &(*_Stats)[BGSKILL_COMBAT], &(*_Stats)[BGSKILL_STRENGTH], &(*_Stats)[BGSKILL_TOUGHNESS], 0.36, 0.32, 0.32);
	BGStatsRandom(_RemainPoints, 4, &(*_Stats)[BGSKILL_AGILITY], &(*_Stats)[BGSKILL_WIT],
		&(*_Stats)[BGSKILL_CHARISMA], &(*_Stats)[BGSKILL_INTELLIGENCE], 0.25, 0.25, 0.25, 0.25);
}

struct Trait* RandomTrait(struct Trait** _Traits, uint8_t _TraitCt, struct HashItr* _Itr) {
	int _Rand = Random(0, g_Traits.Size - 1);
	int _Ct = 0;
	int _FirstPick = _Rand;

	loop_top:
	//Pick a random trait.
	while(_Itr != NULL && _Ct < _Rand) {
		_Itr = HashNext(&g_Traits, _Itr);
		++_Ct;
	}
	//Determine if trait is a valid option.
	for(int i = 0; i < _TraitCt; ++i) {
		//Same trait is picked, pick another if there is another valid trait to be picked.
		if(_Traits[i] == HashItrData(_Itr))
			goto repick_trait;
		for(int j = 0; _Traits[i]->Prevents[j] != NULL; ++j) {
			//One of the already picked traits prevents this trait from being chosen.
			if(_Traits[i]->Prevents[j] == HashItrData(_Itr))
				goto repick_trait;
		}
	}
	return (struct Trait*) HashItrData(_Itr);
	repick_trait:
	++_Rand;
	if(_Rand >= g_Traits.Size) {
		_Rand = 0;	
		HashItrRestart(&g_Traits, _Itr);
	}
	//No more valid traits to be picked from
	if(_Rand == _FirstPick)
		return NULL;
	goto loop_top;
}

struct Trait** BGRandTraits(uint8_t* _TraitCt) {
	struct HashItr* _Itr = HashCreateItr(&g_Traits);
	struct Trait** _Traits = NULL; 
	struct Trait* _Trait = NULL;

	*_TraitCt = Random(1, 3);
	_Traits = calloc(*_TraitCt, sizeof(struct Trait*));
	for(uint8_t i = 0; i < *_TraitCt; ++i) {
		if((_Trait = RandomTrait(_Traits, i, _Itr)) == NULL) {
			_Traits[i] = NULL;
			break;
		}
		_Traits[i] = _Trait;
		HashItrRestart(&g_Traits, _Itr);	
	}
	HashDeleteItr(_Itr);
	return _Traits;
}

int HasTrait(const struct BigGuy* _BigGuy, const struct Trait* _Trait) {
	for(int i = 0; i < _BigGuy->TraitCt; ++i) {
		if(_BigGuy->Traits[i]
			 == _Trait)
			return 1;
	}
	return 0;
}

void BigGuySetAction(struct BigGuy* _Guy, int _Action, struct BigGuy* _Target, void* _Data) {
//	struct RBNode* _Node = NULL;
//	struct BigGuyActionHist* _Hist = NULL;
//	struct BigGuyActionHist _Search = {_Guy, _Action, 0};

	/*
	_Guy->Action.Target = _Target;
	_Guy->Action.Data = _Data;
	_Guy->Action.Type = _Action;
	if((_Node = RBSearchNode(&g_GameWorld.ActionHistory, &_Search)) != NULL) {
		_Hist = (struct BigGuyActionHist*) _Node->Data;
		if(DaysBetween(_Hist->DayDone, g_GameWorld.Date) >= g_BGActCooldown[_Hist->ActionType]) {
			_Hist->DayDone = g_GameWorld.Date;
			RBDeleteNode(&g_GameWorld.ActionHistory, _Node);	
			RBInsert(&g_GameWorld.ActionHistory, _Hist);
		} else {
			return;
		}
	} else {
		_Hist = malloc(sizeof(struct BigGuyActionHist));
		_Hist->Owner = _Guy;
		_Hist->ActionType = _Action;
		_Hist->DayDone = g_GameWorld.Date;
		RBInsert(&g_GameWorld.ActionHistory, _Hist);
		EventHook(EVENT_DEATH, BGOnTargetDeath, _Target->Person, _Guy, NULL);
	}
	*/
	//FIXME: Add a way to store the target.
	/*switch(_Action) {
	case BGACT_SABREL:
		MissionAction("RUMOR.1", _Guy, _Target);
		break;
	case BGACT_STEAL:
		MissionAction("STLCT.1", _Guy, _Target);
		break;
	case BGACT_DUEL:
		MissionAction("DUEL.2", _Target, _Guy);
		break;
	case BGACT_MURDER:
		MissionAction("MURDR.1", _Target, _Guy);
		break;
	case BGACT_PLOTOVERTHROW:
		CreatePlot(PLOT_OVERTHROW, NULL, _Guy, _Target);
		break;
	default:
		//_Guy->ActionFunc = NULL;
		return;
	}*/
	_Guy->Action = _Action;
	_Guy->ActionTarget = _Target;
}

struct Settlement* BigGuyHome(struct BigGuy* _Guy) {
	return _Guy->Person->Family->HomeLoc;
}

int BigGuyOpposedCheck(const struct BigGuy* _One, const struct BigGuy* _Two, int _Skill) {
	assert(_Skill >= 0 && _Skill < BGSKILL_SIZE);
	return ((Random(1, 100) + _One->Stats[_Skill]) - (Random(1, 100) + _Two->Stats[_Skill])) / 10;
}

int BigGuySkillCheck(const struct BigGuy* _Guy, int _Skill, int _PassReq) {
	assert(_Skill >= 0 && _Skill < BGSKILL_SIZE);
	return ((Random(1, 100) + _Guy->Stats[_Skill]) >= _PassReq);
}

int BigGuySuccessMargin(const struct BigGuy* _Guy, int _Skill, int _PassReq) {
	int _Margin = 0;

	assert(_Skill >= 0 && _Skill < BGSKILL_SIZE);
	_Margin = Random(1, 100) + _Guy->Stats[_Skill] - _PassReq;
	if(_Margin >= 0)
		_Margin = _Margin / 10 + 1;
	else 
		_Margin = _Margin / 10 - 1;
	return _Margin;
}

int BigGuyPopularity(const struct BigGuy* _Guy) {
	return _Guy->Popularity;
	/*int _BGPop = 0;
	struct BigGuyRelation* _Relation = _Guy->Relations;

	while(_Relation != NULL) {
		if(_Relation->Relation > BGREL_NEUTURAL)
			_BGPop++; 
		_Relation = _Relation->Next;
	}
	return _BGPop + _Guy->Popularity;*/
}


int BigGuyPlotPower(const struct BigGuy* _Guy) {
	return _Guy->Stats[BGSKILL_WIT];
}

struct Retinue* BigGuyRetinue(const struct BigGuy* _Leader) {
	struct Settlement* _Settlement = PersonHome(_Leader->Person);

	for(struct Retinue* _Retinue = _Settlement->Retinues; _Retinue != NULL; _Retinue = _Retinue->Next) {
		if(_Retinue->Leader == _Leader)
			return _Retinue;
	}
	return NULL;
}

void BigGuyPlotTarget(struct BigGuy* _Guy, struct Plot* _Plot) {
	LnkLstPushBack(&_Guy->PlotsAgainst, _Plot);
}
