/*
 * File: BigGuy.c
 * Author: David Brotz
 */

#include "BigGuy.h"

#include "Person.h"
#include "Family.h"
#include "World.h"
#include "Location.h"
#include "Feud.h"
#include "Good.h"
#include "Government.h"
#include "Trait.h"
#include "Mission.h"

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

const char* g_CrisisStateStr[CRISIS_SIZE] = {
	"WarDeath"
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
	return _One->Person->Id - _Two->Person->Id;
}

int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy) {
	return (*_Two) - _BigGuy->Person->Id;
}

int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission) {
	return 0;
}

void BigGuyActionImproveRel(struct BigGuy* _Guy, const struct BigGuyAction* _Action) {
	struct BigGuyRelation* _Relation = NULL;
	int _Mod = _Action->Modifier;

	if(_Mod > 0 && Random(BIGGUYSTAT_MIN, BIGGUYSTAT_MAX) > _Guy->Stats[BGSKILL_CHARISMA])
		--_Mod;
	if((_Relation = BigGuyGetRelation(_Action->Target, _Guy)) == NULL) {
		_Relation = CreateBigGuyRelation(_Action->Target, _Guy);
		CreateBigGuyOpinion(_Relation, OPINION_SMALL, _Mod);
		BigGuyRelationUpdate(_Relation);
	} else if(_Relation->Relation < BGREL_LIKE) {
		BigGuyAddRelation(_Action->Target, _Relation, OPINION_SMALL, _Mod);
	}
}

void BigGuyActionGift(struct BigGuy* _Guy, const struct BigGuyAction* _Action) {
	struct Family* _Family = _Action->Target->Person->Family;
	struct GoodBase* _Base = (struct GoodBase*) _Action->Data;

	for(int i = 0; i < _Family->Goods->Size; ++i)
		if(_Base == ((struct Good*)_Family->Goods->Table[i])->Base) {
			struct Good* _Taken = FamilyTakeGood(_Family, i, _Action->Modifier);

			FamilyGetGood(_Guy->Person->Family, _Taken, _Taken->Quantity);
		}
}

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

struct Crisis* CreateCrisis(int _Type, struct BigGuy* _Guy) {
	struct Crisis* _Crisis = NULL;
	struct BigGuy* _Ruler = _Guy->Person->Family->HomeLoc->Government->Leader;

	if(_Guy == _Ruler)
		return NULL;
	_Crisis = (struct Crisis*) malloc(sizeof(struct Crisis));
	WorldStateClear(&_Crisis->State);
	WorldStateSetAtom(&_Crisis->State, _Type, 1);
	_Crisis->Guy = _Guy;
	_Crisis->TriggerMask = 0;
	PushEvent(EVENT_CRISIS, _Guy, _Ruler);
	//FIXME: If a crisis already exists then this current crisis will not be recorded.
	if(RBSearch(&g_GameWorld.Crisis, &_Guy->Id) == NULL)
		RBInsert(&g_GameWorld.Crisis, _Crisis);
	return _Crisis;
}

void DestroyCrisis(struct Crisis* _Crisis) {
	free(_Crisis);
}

int CrisisSearch(const struct Crisis* _One, const struct Crisis* _Two) {
	//if(_One->BigGuyId - _Two->BigGuyId == 0)
	//	return _One->Type - _Two->Type;
	return _One->Guy->Id - _Two->Guy->Id;
}

int CrisisInsert(const int* _One, const struct Crisis* _Two) {
	return ((*_One) - _Two->Guy->Id);
}

void BGOnDeath(int _EventId, struct Person* _Person, struct BigGuy* _Guy, void* _None) {
	EventHookRemove(_EventId, _Guy, _Person, NULL);
	RBDelete(&g_GameWorld.Agents, _Guy);
	DestroyBigGuy(_Guy);
	DestroyPerson(_Person);
	DestroyAgent(_Guy->Agent);
}

void BGOnTargetDeath(int _EventId, struct Person* _Person, struct BigGuy* _Guy, void* _None) {
	EventHookRemove(_EventId, _Guy, _Person, NULL);
	_Guy->Action.Type = BGACT_NONE;
	_Guy->ActionFunc = NULL;
	_Guy->Action.Target = NULL;
	//Have the agent rethink of a plan that doesn't involve _Person.
	//AgentThink(_Guy->Agent);
}

struct BigGuy* CreateBigGuy(struct Person* _Person, uint8_t _Stats[BGSKILL_SIZE], int _Motivation) {
	struct BigGuy* _BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	_BigGuy->Person = _Person;
	_BigGuy->Authority = 0;
	_BigGuy->Prestige = 0;
	_BigGuy->IsDirty = 1;
	_BigGuy->Relations = NULL;
	memcpy(&_BigGuy->Stats, _Stats, sizeof(uint8_t) * BGSKILL_SIZE);
	_BigGuy->ActionFunc = NULL;
	_BigGuy->TriggerMask = 0;
	_BigGuy->Motivation = _Motivation;
	_BigGuy->Agent = CreateAgent(_BigGuy);
	_BigGuy->Popularity = BGRandPopularity(_BigGuy);
	RBInsert(&g_GameWorld.BigGuys, _BigGuy);
	RBInsert(&g_GameWorld.BigGuyStates, _BigGuy);
	RBInsert(&g_GameWorld.Agents, _BigGuy->Agent);
	LnkLstPushBack(&FamilyGetSettlement(_Person->Family)->BigGuys, _BigGuy);
	EventHook(EVENT_DEATH, (EventCallback) BGOnDeath, _Person, _BigGuy, NULL);
	CreateObject((struct Object*)_BigGuy, OBJECT_BIGGUY, (ObjectThink) BigGuyThink);

	_BigGuy->Feuds.Size = 0;
	_BigGuy->Feuds.Front = NULL;
	_BigGuy->Feuds.Back = NULL;
	_BigGuy->Personality = Random(0, BIGGUY_PERSONALITIES - 1);
	_BigGuy->Traits = BGRandTraits();
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
	LnkLstClear(&_BigGuy->Feuds);
	RBDelete(&g_GameWorld.BigGuys, _BigGuy->Person);
	RBDelete(&g_GameWorld.BigGuyStates, _BigGuy);
	DestroyObject((struct Object*)_BigGuy);
	free(_BigGuy->Traits);
	free(_BigGuy);
}

void BigGuyThink(struct BigGuy* _Guy) {
	struct BigGuyRelation* _Relation = _Guy->Relations;
	struct BigGuyOpinion* _Opinion = NULL;

	if(_Guy->ActionFunc != NULL)
		_Guy->ActionFunc(_Guy, &_Guy->Action);
	while(_Relation != NULL) {
		_Opinion = _Relation->Opinions;
		while(_Opinion != NULL) {
			_Opinion->RelMod = _Opinion->RelMod - AbsAdd(_Opinion->RelMod, 1);
			_Opinion = _Opinion->Next;
		}
		BigGuyRelationUpdate(_Relation);
		//if(_Relation->Relation < BGREL_NEUTURAL)
		//	BigGuyAddFeud(_Guy, CreateFeud(_Guy, _Relation->Person));
		_Relation = _Relation->Next;
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

			BGStatsWarlord(_Stats, 50);
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
		if(_Result > BIGGUYSTAT_MAX)
			_Result = BIGGUYSTAT_MAX;
		if(_PointsLeft < _Result)
			_Result = _PointsLeft;
		*_Stats[i] = _Result;
		_PointsLeft -= _Result;
	}
	va_end(_Valist);
}

int BGRandPopularity(const struct BigGuy* _Guy) {
	int _PopPer = (_Guy->Stats[BGSKILL_CHARISMA] + Random(1, 100)) / 2;
	int _AdultPop = SettlementAdultPop(_Guy->Person->Family->HomeLoc) - SettlementBigGuyCt(_Guy->Person->Family->HomeLoc);

	return (_AdultPop * _PopPer) / 100; //Divide like this to prevent using floting point numbers.
}

void BGStatsWarlord(uint8_t _Stats[BGSKILL_SIZE], int _Points) {
	int _WarPoints = (_Points <= 400) ? (_Points / 2) : (240);
	int _RemainPoints = _Points - _WarPoints;

	/*
	 * TODO: The percentages given to each stat should be randomized slightly.
	 */
	BGStatsRandom(_WarPoints, 3, &_Stats[BGSKILL_STRATEGY], &_Stats[BGSKILL_TACTICS], &_Stats[BGSKILL_WARFARE], 0.36, 0.32, 0.32);
	BGStatsRandom(_RemainPoints, 5, &_Stats[BGSKILL_ADMINISTRATION], &_Stats[BGSKILL_CHARISMA], &_Stats[BGSKILL_INTELLEGENCE]
		, &_Stats[BGSKILL_INTRIGUE], &_Stats[BGSKILL_PIETY], 0.22, 0.2, 0.2, 0.2, 0.18);
}

void BGSetAuthority(struct BigGuy* _Guy, float _Authority) {
	_Guy->Authority = _Authority;
	_Guy->IsDirty = 1;
}

void BGSetPrestige(struct BigGuy* _Guy, float _Prestige) {
	_Guy->Prestige = _Prestige;
	_Guy->IsDirty = 1;
}

struct Trait* RandomTrait(struct Trait** _Traits, int _TraitSz, struct HashItr* _Itr) {
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
	for(int i = 0; i < _TraitSz; ++i) {
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

struct Trait** BGRandTraits() {
	struct HashItr* _Itr = HashCreateItr(&g_Traits);
	int _TraitCt = Random(1, 3);
	struct Trait** _Traits = calloc(_TraitCt + 1, sizeof(struct Trait*));
	struct Trait* _Trait = NULL;

	_Traits[_TraitCt] = NULL;
	for(int i = 0; i < _TraitCt; ++i) {
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

void BigGuySetAction(struct BigGuy* _Guy, int _Action, struct BigGuy* _Target, void* _Data) {
	struct RBNode* _Node = NULL;
	struct BigGuyActionHist* _Hist = NULL;
	struct BigGuyActionHist _Search = {_Guy, _Action, 0};

	_Guy->Action.Target = _Target;
	_Guy->Action.Data = _Data;
	_Guy->Action.Type = _Action;
	if((_Node = RBSearchNode(&g_GameWorld.ActionHistory, &_Search)) != NULL) {
		_Hist = (struct BigGuyActionHist*) _Node->Data;
		if(DaysBetween(_Hist->DayDone, g_GameWorld.Date) >= g_BGActCooldown[_Hist->ActionType]) {
			RBDeleteNode(&g_GameWorld.ActionHistory, _Node);	
			free(_Hist);
			EventHookRemove(EventUserOffset() + EVENT_DEATH, _Target->Person, _Guy, NULL);
		} else {
			return;
		}
	} else {
		_Hist = malloc(sizeof(struct BigGuyActionHist));
		_Hist->Owner = _Guy;
		_Hist->ActionType = _Action;
		_Hist->DayDone = g_GameWorld.Date;
		RBInsert(&g_GameWorld.ActionHistory, _Hist);
		EventHook(EVENT_DEATH, (EventCallback) BGOnTargetDeath, _Target->Person, _Guy, NULL);
	}
	switch(_Action) {
	case BGACT_IMRPOVEREL:
		_Guy->ActionFunc = BigGuyActionImproveRel;
		//_Guy->Action.Modifier = RELATIONS_PER_TICK;
		break;
	case BGACT_SABREL:
		MissionAction("RUMOR.1", _Guy, _Target);
		break;
	case BGACT_GIFT:
		_Guy->ActionFunc = BigGuyActionGift;
		_Guy->Action.Modifier = 1;
		break;
	case BGACT_STEALCATTLE:
		MissionAction("STLCT.1", _Guy, _Target);
		break;
	case BGACT_DUEL:
		MissionAction("DUEL.2", _Target, _Guy);
		break;
	case BGACT_MURDER:
		MissionAction("MURDR.1", _Target, _Guy);
		break;
	case BGACT_DISSENT:
		MissionAction("DISNT.1", _Target, _Guy);
		break;
	case BGACT_CONVINCE:
		MissionAction("REL.2", _Guy, _Target);
		break;
	default:
		_Guy->ActionFunc = NULL;
		return;
	}
}

void BigGuyAddFeud(struct BigGuy* _Guy, struct Feud* _Feud) {
	LnkLstPushBack(&_Guy->Feuds, _Feud);
}

int BigGuyLikeTrait(const struct BigGuy* _Guy, const struct BigGuy* _Target) {
	switch(_Guy->Personality) {
	case 0:
	case 1:
		if(_Target->Personality > 1)
			return 0;
		return 1;
	case 2:
	case 3:
		if(_Target->Personality < 2)
			return 0;
		return 1;
	}
	return 0;
}

double BigGuyOpinionMod(const struct BigGuy* _Guy, const struct BigGuy* _Target) {
	static double _TableMod[][4] = {{2, 1.5, 1, .5}, {1.5, 2, .5, 1}, {.5, 1, 2, 1.5}, {1, .5, 1.5, 2}};

	return _TableMod[_Guy->Personality][_Target->Personality];
}

int BigGuyOpposedCheck(const struct BigGuy* _One, const struct BigGuy* _Two, int _Skill) {
	assert(_Skill >= 0 && _Skill < BGSKILL_SIZE);
	return (Random(1, 100) + _One->Stats[_Skill]) - (Random(1, 100) + _Two->Stats[_Skill]) / 10;
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
	int _BGPop = 0;
	struct BigGuyRelation* _Relation = _Guy->Relations;

	while(_Relation != NULL) {
		if(_Relation->Relation > BGREL_NEUTURAL)
			_BGPop++; 
		_Relation = _Relation->Next;
	}
	return _BGPop + _Guy->Popularity;
}
