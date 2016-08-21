/*
 * File: Government.c
 * Author: David Brotz
 */

#include "Government.h"

#include "Person.h"
#include "Family.h"
#include "Location.h"
#include "BigGuy.h"
#include "Policy.h"

#include "video/Video.h"

#include "sys/Event.h"
#include "sys/Log.h"
#include "sys/Math.h"

#include <stdlib.h>
#include <string.h>

enum {
	GOVRANK_MAYOR = 0,
	GOVRANK_COUNTY = 1,
	GOVRANK_PROVINCE = 2,
	GOVRANK_KINGDOM = 4,
	GOVRANK_ALL = 7
};

enum {
	GOVNONE,
	GOVSET_SUCCESSION,
	GOVSET_RULERGENDER,
	GOVSET_MILLEADER,
	GOVSET_AUTHORITY,
	GOVSUB_AUTHORITY,
	GOVADD_AUTHORITY,
};

enum {
	GOVSUCCESSION_ELECTED,
	GOVSUCCESSION_MONARCHY, //Used for change from elected leader to monarchy. Will determine proper inheritance law.
	GOVSUCCESSION_ELECTIVEMONARCHY,
	GOVSUCCESSION_SIZE
};

enum {
	GOVMILLEADER_NONE,
	GOVMILLEADER_RULER,
	GOVMILLEADER_PRIVLEDGED,
	GOVMILLEADER_SIZE
};

const char* g_GovernmentTypeStr[] = {
		"Elective",
		"Monarchy",
		"Absolute",
		"Constitutional",
		"Republic",
		"Democratic",
		"Theocratic",
		"Consensus",
		"Tribal",
		"Confederacy",
		"Hegomony",
		"Federation",
		"Feudal",
		"Despotic",
		"Chiefdom",
		"Clan"
};
GovernmentSuccession g_GovernmentSuccession[GOVSUCCESSION_SIZE] = {
		ElectiveNewLeader,
		MonarchyNewLeader,
		ElectiveMonarchyNewLeader
};

void GovOnNewPolicy(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct Government* _Gov = _Data->OwnerObj;
	struct Policy* _Policy = _Extra2;

	GovernmentAddPolicy(_Gov, _Policy);
}

void GovOnChangePolicy(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct Government* _Gov = _Data->OwnerObj;
	struct ActivePolicy* _Policy = _Extra2;
	struct ActivePolicy* _OldPolicy = NULL;

	for(struct LnkLst_Node* _Itr = _Gov->PolicyList.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_OldPolicy = _Itr->Data;
		if(_OldPolicy->Policy != _Policy->Policy)
			continue;
		GovernmentUpdatePolicy(_Gov, _OldPolicy, _Policy);
		break;
	}
}

void GovOnLeaderDeath(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct Government* _Gov = _Data->One;

	EventHookRemove(_Data->EventType, _Data->OwnerObj, _Gov, NULL);
	_Gov->Leader = _Gov->NextLeader;
	_Gov->NextLeader = g_GovernmentSuccession[(_Gov->GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) - 1](_Gov);
}

void GovOnNewLeader(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct BigGuy* _NewLeader = _Extra1;
	struct Government* _Gov = _Data->One;

	EventHookRemove(_Data->EventType, _Data->OwnerObj, _Gov, NULL);
	GovernmentSetLeader(_Gov, _NewLeader);
}

struct Government* CreateGovernment(int _GovType, int _GovRank, struct Settlement* _Settlement) {
	struct Government* _Gov = NULL;

	if((_GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) == 0) {
		Log(ELOG_ERROR, "Governement has no rule type.");
		return NULL;
	}
	_Gov = (struct Government*) malloc(sizeof(struct Government));
	_Gov->GovType = _GovType;
	_Gov->GovRank = (1 << _GovRank);
	_Gov->RulerGender = EMALE;
	ConstructLinkedList(&_Gov->SubGovernments);
	ConstructLinkedList(&_Gov->PolicyList);

	_Gov->AllowedSubjects = 6;
	_Gov->Owner.Government = NULL;
	_Gov->Owner.Relation = GOVREL_NONE;
	_Gov->Leader = NULL;
	_Gov->NextLeader = NULL;

	_Gov->Location = _Settlement;
	NewZoneColor(&_Gov->ZoneColor);
	EventHook(EVENT_NEWPOLICY, GovOnNewPolicy, _Gov, NULL, NULL);
	EventHook(EVENT_CHANGEPOLICY, GovOnChangePolicy, _Gov, NULL, NULL);
	return _Gov;
}

void DestroyGovernment(struct Government* _Gov) {
	LnkLstClear(&_Gov->SubGovernments);
	free(_Gov);
}

void GovernmentThink(struct Government* _Gov) {
	if(NEWYEAR(g_GameWorld.Date) != 0) {
		for(int i = 0; i < CASTE_SIZE; ++i)
			if(_Gov->CastePreference[i] > 0)
				--_Gov->CastePreference[i];
	}
}

void GovernmentLowerRank(struct Government* _Gov, int _NewRank, struct LinkedList* _ReleasedSubjects) {
	if(_NewRank >= _Gov->GovRank)
		return;
	while(_Gov->SubGovernments.Size > _Gov->AllowedSubjects)
		LnkLstPushBack(_ReleasedSubjects, LnkLstPopFront(&_Gov->SubGovernments));
	_Gov->GovRank = _NewRank;
}

const char* GovernmentTypeToStr(int _GovType, int _Mask) {
	int _Ct = 0;

	_GovType = _GovType & _Mask;
	while((_GovType & 1) != 1) {
		_GovType = _GovType >> 1;
		++_Ct;
	}
	if(_Ct >= GOVTYPE_SIZE)
		return "None";
	return g_GovernmentTypeStr[_Ct];
}

void GovernmentLesserJoin(struct Government* _Parent, struct Government* _Subject, int _Relation) {
	if(_Subject->GovRank >= _Parent->GovRank) {
		struct LinkedList _List = {0, NULL, NULL};
		struct LnkLst_Node* _Itr = NULL;

		GovernmentLowerRank(_Subject, _Parent->GovRank - 1, &_List);
		_Itr = _List.Front;
		while(_Itr != NULL) {
			((struct Government*)_Itr->Data)->Owner.Government = _Parent;
			LnkLstPushBack(&_Parent->SubGovernments, (struct Government*)_Itr->Data);
			_Itr = _Itr->Next;
		}
		LnkLstClear(&_List);
	}
	_Subject->Owner.Government = _Parent;
	_Subject->Owner.Relation = _Relation;
	_Subject->ZoneColor = _Parent->ZoneColor;
}

struct BigGuy* MonarchyNewLeader(const struct Government* _Gov) {
	struct Family* _Family = _Gov->Leader->Person->Family;
	struct BigGuy* _Guy = NULL;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i)
		if(_Family->People[i]->Gender == EMALE && _Family->People[i]->Age >= ADULT_AGE) {
			if((_Guy = RBSearch(&g_GameWorld.BigGuys, _Family->People[i])) == NULL) {
				uint8_t _BGStats[BGSKILL_SIZE];

				BGStatsWarlord(&_BGStats, Random(BG_MINGENSTATS, BG_MAXGENSTATS));
				return CreateBigGuy(_Family->People[i], &_BGStats, BGMOT_WEALTH);
			}
			return _Guy;	
		}
	return NULL;
}

struct BigGuy* ElectiveNewLeader(const struct Government* _Gov) {
	struct LnkLst_Node* _Itr = _Gov->Location->BigGuys.Front;
	struct BigGuy* _Guy = NULL;
	struct Person* _Person = NULL;
	struct BigGuy* _BestCanidate = NULL;
	int _BestPopularity = 0;
	int _Popularity = 0;

	while(_Itr != NULL) {
		_Guy = ((struct BigGuy*)_Itr->Data);
		_Person = _Guy->Person;
		if(_Gov->Leader->Person != _Person 
			&& (_Person->Gender &_Gov->RulerGender) != 0 
			&& _Person->Age >= ADULT_AGE
			&& (_Popularity = BigGuyPopularity(_Guy)) > _BestPopularity) {
			_BestCanidate = _Guy;
			_BestPopularity = _Popularity;
		}
		_Itr = _Itr->Next;
	}
	return _BestCanidate;
}

struct BigGuy* ElectiveMonarchyNewLeader(const struct Government* _Gov) {
	struct Settlement* _Settlement = _Gov->Location; 
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct Person* _Person = NULL;
	_Person = ((struct BigGuy*)_Itr->Data)->Person;

	while(_Itr != NULL) {
		_Person = ((struct BigGuy*)_Itr->Data)->Person;
		if(_Person->Family->Id == _Person->Family->Id && _Person->Gender == EMALE && _Person->Age >= ADULT_AGE) {
			return (struct BigGuy*)_Itr->Data;
		}
		_Itr = _Itr->Next;
	}
	return NULL;
}

struct Government* GovernmentTop(struct Government* _Gov) {
	do {
		if(_Gov->Owner.Government != NULL)
			_Gov = _Gov->Owner.Government;
		else
			break;
	} while(1);
	return _Gov;
}

void GovernmentSetLeader(struct Government* _Gov, struct BigGuy* _Guy) {
	_Gov->Leader = _Guy;
	_Gov->NextLeader = g_GovernmentSuccession[(_Gov->GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) - 1](_Gov);
	if((_Gov->GovType & GOVSTCT_CHIEFDOM) == GOVSTCT_CHIEFDOM) {
		_Gov->Appointments.Steward = _Guy;
		_Gov->Appointments.Judge = _Guy;
		_Gov->Appointments.Marshall = _Guy;
	}
	EventHook(EVENT_DEATH, GovOnLeaderDeath, _Guy->Person, _Gov, _Guy);
}

void GovernmentAddPolicy(struct Government* _Gov, const struct Policy* _Policy) {
	struct ActivePolicy* _ActPol = malloc(sizeof(struct ActivePolicy));
	int _Idx = 0;

	Assert(_Policy->OptionsSz < POLICY_SUBSZ);
	_ActPol->Policy = _Policy;
	for(int i = 0; _Policy->Options.Size[i] > 0 && i < POLICY_SUBSZ; ++i) {
		for(int j = 0; j < CASTE_SIZE; ++j) {
			_Gov->CastePreference[j] += _Policy->Options.Options[_Idx].CastePreference[j];
		}
		_ActPol->OptionSel[i] = 0;
		_Idx += _Policy->Options.Size[i];
	}
	LnkLstPushBack(&_Gov->PolicyList, _ActPol);
}

void GovernmentRemovePolicy(struct Government* _Gov, const struct Policy* _Policy) {
	for(struct LnkLst_Node* _Itr = _Gov->PolicyList.Front; _Itr != NULL; _Itr = _Itr->Next) {
		if(((struct ActivePolicy*)_Itr->Data)->Policy == _Policy) {
			LnkLstRemove(&_Gov->PolicyList, _Itr);
			return;
		}
	}
}

void GovernmentUpdatePolicy(struct Government* _Gov, struct ActivePolicy* _OldPolicy, const struct ActivePolicy* _Policy) {
	struct BigGuy* _Guy = NULL;
	struct BigGuy* _Leader = _Gov->Leader;
	const struct PolicyOption* _Option = NULL;

	for(int i = 0; i < _OldPolicy->Policy->OptionsSz; ++i) {
		if(_OldPolicy->OptionSel[i] == _Policy->OptionSel[i] || _Policy->OptionSel[i] == POLICYACT_IGNORE)
			continue;
		_Option = PolicyRow(_Policy->Policy, i, _Policy->OptionSel[i]);
		for(int j = 0; j < CASTE_SIZE; ++j) {
			_Gov->CastePreference[j] += _Option->CastePreference[j];
		}
		for(struct LnkLst_Node* _Itr = _Gov->Location->BigGuys.Front; _Itr != NULL; _Itr = _Itr->Next) {
			_Guy = _Itr->Data;
			if(_Guy == _Leader)
				continue;
			BigGuyAddOpinion(
				_Guy,
				_Leader,
				ACTTYPE_POLICY,
				_Option->CastePreference[PERSON_CASTE(_Guy->Person)],
				OPNLEN_MEDIUM,
				OPINION_AVERAGE	
				);
		}
		_OldPolicy->OptionSel[i] = _Policy->OptionSel[i];
		break;
	}
}

int GovernmentHasPolicy(const struct Government* _Gov, const struct Policy* _Policy) {
	struct ActivePolicy* _ActPol = NULL;

	for(struct LnkLst_Node* _Itr = _Gov->PolicyList.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_ActPol = _Itr->Data;
		if(_ActPol->Policy == _Policy)
			return 1;
	}
	return 0;
}
