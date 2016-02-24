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

#include "AI/Utility.h"
#include "AI/goap.h"
#include "AI/Setup.h"
#include "AI/Agent.h"

#include "sys/Math.h"
#include "sys/Constraint.h"
#include "sys/Event.h"
#include "sys/RBTree.h"
#include "sys/Event.h"
#include "sys/LinkedList.h"

#include <stdlib.h>

#define RELATIONS_PER_TICK (2)

const char* g_BGStateStr[BGBYTE_SIZE] = {
		"PassReform",
		"IsLeader",
		"ImproveRelations",
		"Authority",
		"Prestige",
		"Feud",
		"FyrdRaised"
};

const char* g_BGMission[BGACT_SIZE] = {
		"Improve Relations"
};

const char* g_CrisisStateStr[CRISIS_SIZE] = {
	"WarDeath"
};

void BigGuyActionImproveRel(struct BigGuy* _Guy, const struct BigGuyAction* _Action) {
	struct BigGuyRelation* _Relation = NULL;
	int _Mod = _Action->Modifier;

	if(_Mod > 0 && Random(BIGGUYSTAT_MIN, BIGGUYSTAT_MAX) > _Guy->Stats.Charisma)
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

struct BigGuyOpinion* CreateBigGuyOpinion(struct BigGuyRelation* _Relation, int _Action, int _Modifier) {
	struct BigGuyOpinion* _Opinion = (struct BigGuyOpinion*) malloc(sizeof(struct BigGuyOpinion));

	_Opinion->Action = _Action;
	_Opinion->RelMod = _Modifier;
	_Opinion->Next = _Relation->Opinions;
	_Relation->Opinions = _Opinion;
	return _Opinion;
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

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, struct BigGuy* _Actor) {
	struct BigGuyRelation* _Relation = (struct BigGuyRelation*) malloc(sizeof(struct BigGuyRelation));

	_Relation->Relation = BGREL_NEUTURAL;
	_Relation->Modifier = 0;
	_Relation->Next = _Guy->Relations;
	_Guy->Relations = _Relation;
	_Relation->Opinions = NULL;
	_Relation->Person = _Actor;

	for(int i = 0; _Guy->Traits[i] != NULL; ++i) {
		for(int j = 0; _Actor->Traits[j] != NULL; ++j) {
			if(TraitDislikes(_Guy->Traits[i], _Actor->Traits[j]) != 0) {
				BigGuyAddRelation(_Guy, _Relation, ACTTYPE_TRAIT, -(BIGGUY_TRAITREL)); 
			} else if(TraitLikes(_Guy->Traits[i], _Actor->Traits[j])) {
				BigGuyAddRelation(_Guy, _Relation, ACTTYPE_TRAIT, BIGGUY_TRAITREL); 
			}
		}
	}
	return _Relation;
}

void BGOnDeath(int _EventId, struct BigGuy* _Guy, struct Person* _Person, void* _None) {
	EventHookRemove(_EventId, _Guy, _Person, NULL);
	RBDelete(&g_GameWorld.Agents, _Guy);
	DestroyBigGuy(_Guy);
	DestroyPerson(_Person);
	DestroyAgent(_Guy->Agent);
}

void BGOnTargetDeath(int _EventId, struct BigGuy* _Guy, struct Person* _Person, void* _None) {
	EventHookRemove(_EventId, _Guy, _Person, NULL);
	_Guy->Action.Type = BGACT_NONE;
	_Guy->ActionFunc = NULL;
	_Guy->Action.Target = NULL;
	//Have the agent rethink of a plan that doesn't involve _Person.
	//AgentThink(_Guy->Agent);
}

struct BigGuy* CreateBigGuy(struct Person* _Person, struct BigGuyStats* _Stats) {
	struct BigGuy* _BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	_BigGuy->Person = _Person;
	WorldStateClear(&_BigGuy->State);
	WorldStateCare(&_BigGuy->State);
	_BigGuy->Authority = 0;
	_BigGuy->Prestige = 0;
	_BigGuy->IsDirty = 1;
	_BigGuy->Relations = NULL;
	_BigGuy->Stats = *_Stats;
	_BigGuy->ActionFunc = NULL;
	_BigGuy->TriggerMask = 0;
	_BigGuy->Agent = CreateAgent(_BigGuy);
	RBInsert(&g_GameWorld.BigGuys, _BigGuy);
	RBInsert(&g_GameWorld.BigGuyStates, _BigGuy);
	RBInsert(&g_GameWorld.Agents, _BigGuy->Agent);
	LnkLstPushBack(&FamilyGetSettlement(_Person->Family)->BigGuys, _BigGuy);
	EventHook(EVENT_DEATH, (EventCallback) BGOnDeath, _BigGuy, _Person, NULL);
	CreateObject((struct Object*)_BigGuy, OBJECT_BIGGUY, (ObjectThink) BigGuyThink);

	_BigGuy->Feuds.Size = 0;
	_BigGuy->Feuds.Front = NULL;
	_BigGuy->Feuds.Back = NULL;
	_BigGuy->Personality = Random(0, BIGGUY_PERSONALITIES - 1);
	_BigGuy->Traits = calloc(1, sizeof(void*));
	_BigGuy->Traits[0] = NULL;
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
		if(_Relation->Relation < BGREL_NEUTURAL)
			BigGuyAddFeud(_Guy, CreateFeud(_Guy, _Relation->Person));
		_Relation = _Relation->Next;
	}
}

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return _One->Person->Id - _Two->Person->Id;
}

int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy) {
	return (*_Two) - _BigGuy->Person->Id;
}

int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two) {
	return WorldStateCmp(&_One->State, &_Two->State);
}

int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission) {
	return 0;
}

void BigGuySetState(struct BigGuy* _Guy, int _State, int _Value) {
	WorldStateSetAtom(&_Guy->State, _State, _Value);
	_Guy->IsDirty = 1;
}

struct BigGuy* BigGuyLeaderType(struct Person* _Person) {
	while(_Person != NULL) {
		if(_Person->Gender == EMALE && DateToDays(_Person->Age) > ADULT_AGE) {
			struct BigGuyStats _Stats;

			BGStatsWarlord(&_Stats, 50);
			return CreateBigGuy(_Person, &_Stats); //NOTE: Make sure we aren't making a big guy when the person is already a big guy.
		}
		_Person = _Person->Next;
	}
	return NULL;
}

void BigGuyAddRelation(struct BigGuy* _Guy, struct BigGuyRelation* _Relation, int _Action, int _Modifier) {
	struct BigGuyOpinion* _Opinion = _Relation->Opinions;

	while(_Opinion != NULL) {
		if(_Opinion->Action == _Action) {
			_Opinion->RelMod += _Modifier;
			goto add_mods;
		}
		_Opinion = _Opinion->Next;
	}
	CreateBigGuyOpinion(_Relation, _Action, _Modifier);
	add_mods:
	_Relation->Modifier = _Relation->Modifier + (((float)_Modifier) * BigGuyOpinionMod(_Guy, _Relation->Person));
	_Relation->Relation = Fuzify(g_OpinionMods, _Relation->Modifier);
}

void BigGuyChangeOpinion(struct BigGuy* _Guy, struct BigGuy* _Target, int _Action, int _Modifier) {
	struct BigGuyRelation* _Relation = _Guy->Relations;
	struct BigGuyOpinion* _Opinion = NULL;

	while(_Relation != NULL) {
		_Opinion = _Relation->Opinions;
		if(_Target != _Relation->Person)
			goto rel_end;
		while(_Opinion != NULL) {
			if(_Opinion->Action != _Action)
				goto opin_end;
			_Opinion->RelMod += _Modifier;
			opin_end:
			_Opinion = _Opinion->Next;
		}
		rel_end:
		_Relation = _Relation->Next;
	}
	_Relation = CreateBigGuyRelation(_Guy, _Target);
	CreateBigGuyOpinion(_Relation, _Action, _Modifier);
}

void BigGuyRelationUpdate(struct BigGuyRelation* _Relation) {
	struct BigGuyOpinion* _Opinion = _Relation->Opinions;
	int _Modifier = 0;

	while(_Opinion != NULL) {
		_Modifier += _Opinion->RelMod;
		_Opinion = _Opinion->Next;
	}
	_Relation->Relation = Fuzify(g_OpinionMods, _Modifier);
	_Relation->Modifier = _Modifier;
}

struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target) {
	struct BigGuyRelation* _Relation = _Guy->Relations;

	while(_Relation != NULL) {
		if(_Relation->Person == _Target)
			return _Relation;
		_Relation = _Relation->Next;
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

void BGStatsWarlord(struct BigGuyStats* _Stats, int _Points) {
	int _WarPoints = _Points / 2;
	int _RemainPoints = _Points - _WarPoints;

	/*
	 * TODO: The percentages given to each stat should be randomized slightly.
	 */
	BGStatsRandom(_WarPoints, 3, &_Stats->Strategy, &_Stats->Tactics, &_Stats->Warfare, 0.5, 0.25, 0.25);
	BGStatsRandom(_RemainPoints, 5, &_Stats->Administration, &_Stats->Charisma, &_Stats->Intellegence, &_Stats->Intrigue, &_Stats->Piety, 0.3, 0.3, 0.15, 0.15, 0.1);
}

void BGSetAuthority(struct BigGuy* _Guy, float _Authority) {
	_Guy->Authority = _Authority;
	_Guy->IsDirty = 1;
}

void BGSetPrestige(struct BigGuy* _Guy, float _Prestige) {
	_Guy->Prestige = _Prestige;
	_Guy->IsDirty = 1;
}

void BigGuySetAction(struct BigGuy* _Guy, int _Action, struct BigGuy* _Target, void* _Data) {
	_Guy->Action.Target = _Target;
	_Guy->Action.Data = _Data;
	_Guy->Action.Type = _Action;
	EventHookRemove(EventUserOffset() + EVENT_DEATH, _Guy, _Target->Person, NULL);
	EventHook(EVENT_DEATH, (EventCallback) BGOnTargetDeath, _Guy, _Target->Person, NULL);

	switch(_Action) {
	case BGACT_IMRPOVEREL:
		_Guy->ActionFunc = BigGuyActionImproveRel;
		_Guy->Action.Modifier = RELATIONS_PER_TICK;
		break;
	case BGACT_SABREL:
		MissionAction("RUMOR.1", _Guy, _Target);
		break;
	case BGACT_GIFT:
		_Guy->ActionFunc = BigGuyActionGift;
		_Guy->Action.Modifier = 1;
		break;
	case BGACT_STEALANIMAL:
		_Guy->ActionFunc = NULL;
		_Guy->Action.Modifier = 1;
		break;
	case BGACT_DUEL:
		MissionAction("DUEL.2", _Target, _Guy);
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
