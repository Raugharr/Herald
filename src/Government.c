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

const char* g_GovernmentTypeStr[GOVTYPE_SIZE] = {
		"Elective",
		"Monarchy",
		"Absolute",
		"Constitutional",
		"Republic",
		"Democratic",
		"Theocratic",
		"Confederacy",
		"Hegomony",
		"Federation",
		"Feudal",
		"Despotic",
		"Clan",
		"Consensus",
		"Tribal",
		"Chiefdom"
};
GovernmentSuccession g_GovernmentSuccession[GOVSUCCESSION_SIZE] = {
		ElectiveNewLeader,
		MonarchyNewLeader,
		ElectiveMonarchyNewLeader
};

struct BigGuy* CreateNewLeader(struct Government* Gov) {
	for(int i = 0; i < Gov->Location->People.Size; ++i) {
		struct Person* Person = Gov->Location->People.Table[i];
		
		if((Person->Flags & BIGGUY) == BIGGUY)
			continue;
		if((CanGovern(Gov, Person)) == true) {
			uint8_t Stats[BGSKILL_SIZE];

			GenerateStats(Person->Family->Caste, &Stats); 
			return CreateBigGuy(Person, &Stats, BGMOT_RULE);
		}
	}
	return NULL;
}

void GovOnNewPolicy(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Government* Gov = Data->OwnerObj;
	struct Policy* Policy = Extra2;

	GovernmentAddPolicy(Gov, Policy);
}

void GovOnChangePolicy(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Government* Gov = Data->OwnerObj;
	struct ActivePolicy* Policy = Extra2;
	struct ActivePolicy* OldPolicy = NULL;

	for(struct LnkLst_Node* Itr = Gov->PolicyList.Front; Itr != NULL; Itr = Itr->Next) {
		OldPolicy = Itr->Data;
		if(OldPolicy->Policy != Policy->Policy)
			continue;
		GovernmentUpdatePolicy(Gov, OldPolicy, Policy);
		break;
	}
}

void GovOnSucessorDeath(const struct EventData* Data, void* Extra1, void* Extra2) {
	//struct Person* Person = Extra1;
	struct Government* Gov = Data->Two;

	Gov->NextLeader = g_GovernmentSuccession[(Gov->GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) - 1](Gov->Leader, Gov);
	if(Gov->NextLeader == NULL) Gov->NextLeader = CreateNewLeader(Gov); 
	Assert(Gov->NextLeader != NULL);
	EventHookRemove(Data->EventType, Data->OwnerObj, Extra1, NULL);
	EventHook(EVENT_DEATH, GovOnSucessorDeath, Gov->NextLeader->Person, Gov->NextLeader, Gov);
};

void GovOnLeaderDeath(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Government* Gov = Data->Two;//Person->Family->HomeLoc->Government;

	/*if(Gov->NextLeader == NULL) {
		DestroySettlement(Gov->Location);
		return;
	}*/
	GovernmentSetLeader(Gov, Gov->NextLeader);//g_GovernmentSuccession[(Gov->GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) - 1](Gov->Leader, Gov->RulerGender));
	//EventHook(EVENT_DEATH, GovOnLeaderDeath, Guy->Person, Gov, Guy);
}

/*void GovOnNewLeader(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct BigGuy* NewLeader = Extra1;
	struct Government* Gov = Data->One;

	EventHookRemove(Data->EventType, Data->OwnerObj, Gov, NULL);
	GovernmentSetLeader(Gov, NewLeader);
}*/

struct Government* CreateGovernment(struct Settlement* Settlement, uint32_t GovRule, uint32_t GovStruct, uint32_t GovType, uint32_t GovMix, uint32_t GovRank) {
	struct Government* Gov = NULL;

	/*if((GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) == 0) {
		Log(ELOG_ERROR, "Governement has no rule type.");
		return NULL;
	}*/
	Gov = (struct Government*) malloc(sizeof(struct Government));
	CreateObject(&Gov->Object, OBJECT_GOVERNMENT);
	Gov->GovType = GovRule | GovStruct | GovType | GovMix;
	Gov->GovRank = (1 << GovRank);
	Gov->GenderLaw = GOVGEN_MALE;
	CtorArray(&Gov->SubGovernments, 4);
	ConstructLinkedList(&Gov->PolicyList);

	Gov->Location = Settlement;
	//Gov->AllowedSubjects = 6;
	Gov->Owner = NULL;
	Gov->Leader = NULL;
	Gov->Relations = NULL;
	Gov->NextLeader = NULL;
	Gov->TaxRate = TAX_MIN;
	Gov->PolicyPop = calloc(g_GameWorld.Policies.Size, sizeof(uint8_t));
	Gov->PolicyOp = calloc(g_GameWorld.Policies.Size, sizeof(uint8_t));

	NewZoneColor(&Gov->ZoneColor);
	for(int i = 0; i < g_GameWorld.Policies.Size; ++i) {
		GovernmentAddPolicy(Gov, g_GameWorld.Policies.Table[i]);
	}
	EventHook(EVENT_NEWPOLICY, GovOnNewPolicy, Gov, NULL, NULL);
	EventHook(EVENT_CHANGEPOLICY, GovOnChangePolicy, Gov, NULL, NULL);
	return Gov;
}

void DestroyGovernment(struct Government* Gov) {
	DtorArray(&Gov->SubGovernments);
	DestroyObject(&Gov->Object);
	free(Gov->PolicyPop);
	free(Gov->PolicyOp);
	free(Gov);
}

void GovernmentThink(struct Government* Gov) {
	if(NEWYEAR(g_GameWorld.Date) != 0) {
	}
}

/*void GovernmentLowerRank(struct Government* Gov, int NewRank, struct LinkedList* ReleasedSubjects) {
	if(NewRank >= Gov->GovRank)
		return;
	while(Gov->SubGovernments.Size > Gov->AllowedSubjects) {
		LnkLstPushBack(ReleasedSubjects, LnkLstPopFront(&Gov->SubGovernments));
	}
	Gov->GovRank = NewRank;
}*/

const char* GovernmentTypeToStr(int GovType, int Mask) {
	//int Ct = 0;

	//GovType = ctz(GovType & Mask);
	//GovType = GovType & Mask;
	//while((GovType & 1) != 1) {
	//	GovType = GovType >> 1;
	//	++Ct;
	//}
//	if(Ct >= GOVTYPE_SIZE)
//		return "None";
	return g_GovernmentTypeStr[ctz(GovType & Mask)];
}

void GovernmentLesserJoin(struct Government* Parent, struct Government* Subject) {
	/*if(Subject->GovRank >= Parent->GovRank) {
		struct LinkedList List = {0, NULL, NULL};
		struct LnkLst_Node* Itr = NULL;

		GovernmentLowerRank(Subject, Parent->GovRank - 1, &List);
		Itr = List.Front;
		while(Itr != NULL) {
			((struct Government*)Itr->Data)->Owner = Parent;
			LnkLstPushBack(&Parent->SubGovernments, (struct Government*)Itr->Data);
			Itr = Itr->Next;
		}
		LnkLstClear(&List);
	}*/
	ArrayInsert_S(&Parent->SubGovernments, Subject);
	Subject->Owner = Parent;
	Subject->ZoneColor = Parent->ZoneColor;
}

struct BigGuy* MonarchyNewLeader(const struct BigGuy* Guy, const struct Government* Gov) {
	struct Family* Family = Guy->Person->Family;
	struct BigGuy* NewLeader = NULL;

	for(int i = CHILDREN; i < CHILDREN + Family->NumChildren; ++i)
		if(CanGovern(Gov, Family->People[i]) == true) {
			if((NewLeader = RBSearch(&g_GameWorld.BigGuys, Family->People[i])) == NULL) {
				uint8_t Stats[BGSKILL_SIZE];


				GenerateStats(Family->Caste, &Stats);
				return CreateBigGuy(Family->People[i], &Stats, BGMOT_WEALTH);
			}
			return NewLeader;	
		}
	return NULL;
}

struct BigGuy* ElectiveNewLeader(const struct BigGuy* OldLeader, const struct Government* Gov) {
	struct LnkLst_Node* Itr = OldLeader->Person->Family->HomeLoc->BigGuys.Front;
	struct BigGuy* Guy = NULL;
	struct Person* Person = NULL;
	struct BigGuy* BestCanidate = NULL;
	int BestPopularity = 0;
	int Popularity = 0;

	while(Itr != NULL) {
		Guy = ((struct BigGuy*)Itr->Data);
		Person = Guy->Person;
		if(CanGovern(Gov, Person) == true 
			&& (Popularity = BigGuyPopularity(Guy)) > BestPopularity
			&& Guy != OldLeader) {
			BestCanidate = Guy;
			BestPopularity = Popularity;
		}
		Itr = Itr->Next;
	}
	return BestCanidate;
}

struct BigGuy* ElectiveMonarchyNewLeader(const struct BigGuy* OldLeader, const struct Government* Gov) {
	struct Settlement* Settlement = OldLeader->Person->Family->HomeLoc; 
	struct LnkLst_Node* Itr = Settlement->BigGuys.Front;
	struct Person* Person = NULL;
	Person = ((struct BigGuy*)Itr->Data)->Person;

	while(Itr != NULL) {
		Person = ((struct BigGuy*)Itr->Data)->Person;
		if(Person->Family->Object.Id == Person->Family->Object.Id && CanGovern(Gov, Person) == true) {
			return (struct BigGuy*)Itr->Data;
		}
		Itr = Itr->Next;
	}
	return NULL;
}

struct Government* GovernmentTop(struct Government* Gov) {
	do {
		if(Gov->Owner != NULL)
			Gov = Gov->Owner;
		else
			break;
	} while(1);
	return Gov;
}

void GovernmentSetLeader(struct Government* Gov, struct BigGuy* Guy) {
	if(Gov->Leader != NULL) EventHookRemove(EVENT_DEATH, Gov->Leader->Person, Gov->Leader, Gov); 
	if(Gov->NextLeader != NULL) EventHookRemove(EVENT_DEATH, Gov->NextLeader->Person, Gov->NextLeader, Gov);
	Gov->Leader = Guy;
	Gov->NextLeader = g_GovernmentSuccession[(Gov->GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) - 1](Gov->Leader, Gov);
	//TODO: If NextLeader is NULL then no sutable BigGuy could be found, a new BigGuy should be created.
	if(Gov->NextLeader == NULL) {
		Gov->NextLeader = CreateNewLeader(Gov);
	}
	EventHook(EVENT_DEATH, GovOnSucessorDeath, Gov->NextLeader->Person, Gov->NextLeader, Gov);
	EventHook(EVENT_DEATH, GovOnLeaderDeath, Gov->Leader->Person, Gov->Leader, Gov);
}

void GovernmentAddPolicy(struct Government* Gov, const struct Policy* Policy) {
	struct ActivePolicy* ActPol = malloc(sizeof(struct ActivePolicy));

	ActPol->Policy = Policy;
	ActPol->OptionSel = 0;
	LnkLstPushBack(&Gov->PolicyList, ActPol);
}

void GovernmentRemovePolicy(struct Government* Gov, const struct Policy* Policy) {
	for(struct LnkLst_Node* Itr = Gov->PolicyList.Front; Itr != NULL; Itr = Itr->Next) {
		if(((struct ActivePolicy*)Itr->Data)->Policy == Policy) {
			LnkLstRemove(&Gov->PolicyList, Itr);
			return;
		}
	}
}

void GovernmentUpdatePolicy(struct Government* Gov, struct ActivePolicy* OldPolicy, const struct ActivePolicy* Policy) {
		OldPolicy->OptionSel = Policy->OptionSel;
}

int GovernmentHasPolicy(const struct Government* Gov, const struct Policy* Policy) {
	struct ActivePolicy* ActPol = NULL;

	for(struct LnkLst_Node* Itr = Gov->PolicyList.Front; Itr != NULL; Itr = Itr->Next) {
		ActPol = Itr->Data;
		if(ActPol->Policy == Policy)
			return 1;
	}
	return 0;
}

const char* GovernmentRankStr(const struct Government* Gov) {
	switch(Gov->GovRank) {
		case GOVRANK_SETTLEMENT:
			return "Clan";
		case GOVRANK_COUNTY:
			return "Tribe";
		case GOVRANK_KINGDOM:
			return "Kingdom";
		case GOVRANK_EMPIRE:
			return "Empire";
	}
	return NULL;
}

void GovernmentRaiseArmy(struct Government* Gov) {
	switch(Gov->ArmyLaw) {
		case GOVARMY_NO:
			break;	
		case GOVARMY_ASK:
			break;
		case GOVARMY_YES:
			break;
	}
}

bool CanGovern(const struct Government* Gov, const struct Person* Person) {
	bool CanGovern = false;

	switch(Gov->GenderLaw) {
		case GOVGEN_MALEPERF:
		case GOVGEN_MALE: 
			CanGovern = (Gender(Person) == MALE);
			break;
		case GOVGEN_BOTH: 
			CanGovern = true;
			break;
		case GOVGEN_FEMALE: 
			CanGovern = (Gender(Person) == FEMALE);
			break;
	}
	if(CanGovern == false) return false;
	return (PersonMature(Person) == true);
}
