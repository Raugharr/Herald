/*
 * File: Government.c
 * Author: David Brotz
 */

#include "Government.h"

#include "Person.h"
#include "Family.h"
#include "Location.h"
#include "BigGuy.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>

struct Reform** g_Reforms = NULL;
const char* g_GovernmentTypeStr[] = {
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

void InitReforms(void) {
	struct LinkedList _List = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	int i = 0;

	LnkLstPushBack(&_List, CreateReform("Elected Leader", GOVSTCT_TRIBAL, GOVRANK_ALL, GOVCAT_INTERNALLEADER | GOVCAT_LEADER));
	LnkLstPushBack(&_List, CreateReform("Leader is warlord.", GOVSTCT_TRIBAL, GOVRANK_ALL, GOVCAT_INTERNALLEADER | GOVCAT_LEADER));
	g_Reforms = calloc(_List.Size + 1, sizeof(struct Reform*));
	_Itr = _List.Front;
	for(i = 0; i < _List.Size && _Itr != NULL; ++i, _Itr = _Itr->Next)
		g_Reforms[i] = (struct Reform*)_Itr->Data;
	LnkLstClear(&_List);
}

void QuitReforms(void) {
	int i = 0;

	for(i = 0; g_Reforms[i] != NULL; ++i)
		DestroyReform(g_Reforms[i]);
	free(g_Reforms);
}

struct Reform* CreateReform(const char* _Name, int _AllowedGovs, int _AllowedGovRanks, int _Category) {
	struct Reform* _Reform = (struct Reform*) malloc(sizeof(struct Reform));
	int i = 0;

	_Reform->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Reform->Name, _Name);
	_Reform->AllowedGovs = _AllowedGovs;
	_Reform->AllowedGovRanks = _AllowedGovRanks;
	_Reform->Category = _Category;
	_Reform->OnPass = NULL;
	for(i = 0; i < REFORM_MAXCHOICE; ++i)
		_Reform->Next[i] = NULL;
	_Reform->Prev = NULL;
	return _Reform;
}

void DestroyReform(struct Reform* _Reform) {
	free(_Reform->Name);
	free(_Reform);
}

struct Government* CreateGovernment(int _GovType, int _GovRank) {
	struct Government* _Gov = (struct Government*) malloc(sizeof(struct Government));
	int i = 0;

	_Gov->GovType = _GovType;
	_Gov->GovRank = (1 << _GovRank);
	_Gov->SubGovernments.Front = NULL;
	_Gov->SubGovernments.Back = NULL;
	_Gov->SubGovernments.Size = 0;

	_Gov->PossibleReforms.Front = NULL;
	_Gov->PossibleReforms.Back = NULL;
	_Gov->PossibleReforms.Size = 0;

	_Gov->PassedReforms.Front = NULL;
	_Gov->PassedReforms.Back = NULL;
	_Gov->PassedReforms.Size = 0;
	_Gov->Leader = NULL;
	_Gov->LeaderDeath = NULL;
	_Gov->ParentGovernment = NULL;

	for(i = 0; g_Reforms[i] != NULL; ++i) {
		if(g_Reforms[i]->Prev == NULL
				&& (g_Reforms[i]->AllowedGovRanks & _Gov->GovRank) != 0
				&& (g_Reforms[i]->AllowedGovs & _Gov->GovType) != 0) {
			LnkLstPushBack(&_Gov->PossibleReforms, g_Reforms[i]);
		}

	}
	return _Gov;
}

void DestroyGovernment(struct Government* _Gov) {
	LnkLstClear(&_Gov->PassedReforms);
	LnkLstClear(&_Gov->PossibleReforms);
	LnkLstClear(&_Gov->SubGovernments);
	free(_Gov);
}

int GovernmentLeaderElection(const struct Reform* _Reform, struct Settlement* _Settlement) {
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct BigGuy* _Best = NULL;
	int _BestAuth = 0;

	while(_Itr != NULL) {
		if(((struct BigGuy*)_Itr->Data)->Authority > _BestAuth) {
			_Best = (struct BigGuy*)_Itr->Data;
			_BestAuth = ((struct BigGuy*)_Itr->Data)->Authority;
		}
		_Itr = _Itr->Next;
	}
	if(_Best == NULL) {
		Log(ELOG_WARNING, "GovernmentLeaderElection has no leader possible.");
		return 1;
	}
	_Settlement->Government->Leader = _Best;
	return 1;
}

void GovernmentLowerRank(struct Government* _Gov, int _NewRank, struct LinkedList* _ReleasedSubjects) {
	if(_NewRank >= _Gov->GovRank)
		return;
	while(_Gov->SubGovernments.Size > _Gov->AllowedSubjects)
		LnkLstPushBack(_ReleasedSubjects, LnkLst_PopFront(&_Gov->SubGovernments));
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

void GovernmentLesserJoin(struct Government* _Parent, struct Government* _Subject) {
	if(_Subject->GovRank >= _Parent->GovRank) {
		struct LinkedList _List = {0, NULL, NULL};
		struct LnkLst_Node* _Itr = NULL;

		GovernmentLowerRank(_Subject, _Parent->GovRank - 1, &_List);
		_Itr = _List.Front;
		while(_Itr != NULL) {
			((struct Government*)_Itr->Data)->ParentGovernment = _Parent;
			LnkLstPushBack(&_Parent->SubGovernments, (struct Government*)_Itr->Data);
			_Itr = _Itr->Next;
		}
		LnkLstClear(&_List);
	}
	_Subject->ParentGovernment = _Parent;
}

void GovernmentLoadReforms(struct Government* _Gov, struct Reform** _Reforms) {
	int i = 0;

	while(_Reforms[i] != NULL) {
		if((_Reforms[i]->AllowedGovRanks & _Gov->GovRank) != 0 && (_Reforms[i]->AllowedGovs & _Gov->GovType) != 0)
			LnkLstPushBack(&_Gov->PossibleReforms, _Reforms[i]);
		++i;
	}
}

void GovernmentPassReform(struct Government* _Gov, struct Reform* _Reform) {
	struct LnkLst_Node* _Itr = _Gov->PossibleReforms.Front;

	if(CanPassReform(_Gov, _Reform) == 0)
		return;
	while(_Itr != NULL) {
		if(((struct Reform*)_Itr->Data) == _Reform) {
			_Gov->Leader->State = _Gov->Leader->State | BGSTATE_PASSREFORM;
			LnkLst_Remove(&_Gov->PossibleReforms, _Itr);
			LnkLstPushBack(&_Gov->PassedReforms, _Reform);
			return;
		}
		_Itr = _Itr->Next;
	}

}

void MonarchyLeaderDeath(struct Government* _Gov) {
	int i = 0;
	struct Family* _Family = _Gov->Leader->Person->Family;

	for(i = 0; i < FAMILY_PEOPLESZ; ++i)
		if(_Family->People[i]->Gender == EMALE && _Family->People[i]->Age >= ADULT_AGE) {
			_Gov->Leader->Person = _Family->People[i];
			return;
		}
}

void ElectiveLeaderDeath(struct Government* _Gov) {
	struct Settlement* _Settlement = _Gov->Leader->Person->Family->HomeLoc;
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct Person* _Person = NULL;

	while(_Itr != NULL) {
		_Person = ((struct BigGuy*)_Itr->Data)->Person;
		if(_Person->Gender == EMALE && _Person->Age >= ADULT_AGE) {
			_Gov->Leader->Person = _Person;
			return;
		}
		_Itr = _Itr->Next;
	}
}

void ElectiveMonarchyLeaderDeath(struct Government* _Gov) {
	struct Settlement* _Settlement = _Gov->Leader->Person->Family->HomeLoc;
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct Person* _Person = NULL;
	_Person = ((struct BigGuy*)_Itr->Data)->Person;

	while(_Itr != NULL) {
		_Person = ((struct BigGuy*)_Itr->Data)->Person;
		if(_Person->Family->FamilyId == _Person->Family->FamilyId && _Person->Gender == EMALE && _Person->Age >= ADULT_AGE) {
			_Gov->Leader->Person = _Person;
			return;
		}
		_Itr = _Itr->Next;
	}
}

