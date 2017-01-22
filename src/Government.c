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

void GovOnLeaderDeath(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Government* Gov = Data->One;
	struct BigGuy* NewLeader = Extra1;

	EventHookRemove(Data->EventType, Data->OwnerObj, Gov, NULL);
	GovernmentSetLeader(Gov, NewLeader);
	//EventHook(EVENT_DEATH, GovOnLeaderDeath, Guy->Person, Gov, Guy);
}

void GovOnNewLeader(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct BigGuy* NewLeader = Extra1;
	struct Government* Gov = Data->One;

	EventHookRemove(Data->EventType, Data->OwnerObj, Gov, NULL);
	GovernmentSetLeader(Gov, NewLeader);
}

struct Government* CreateGovernment(int GovType, int GovRank, struct Settlement* Settlement) {
	struct Government* Gov = NULL;

	if((GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) == 0) {
		Log(ELOG_ERROR, "Governement has no rule type.");
		return NULL;
	}
	Gov = (struct Government*) malloc(sizeof(struct Government));
	Gov->GovType = GovType;
	Gov->GovRank = (1 << GovRank);
	Gov->RulerGender = EMALE;
	ConstructLinkedList(&Gov->SubGovernments);
	ConstructLinkedList(&Gov->PolicyList);

	Gov->AllowedSubjects = 6;
	Gov->Owner.Government = NULL;
	Gov->Owner.Relation = GOVREL_NONE;
	Gov->Leader = NULL;
	Gov->NextLeader = NULL;
	Gov->TaxRate = TAX_MIN;
	Gov->PolicyPop = calloc(g_GameWorld.Policies.Size, sizeof(uint8_t));
	Gov->PolicyOp = calloc(g_GameWorld.Policies.Size, sizeof(uint8_t));

	Gov->Location = Settlement;
	NewZoneColor(&Gov->ZoneColor);
	EventHook(EVENT_NEWPOLICY, GovOnNewPolicy, Gov, NULL, NULL);
	EventHook(EVENT_CHANGEPOLICY, GovOnChangePolicy, Gov, NULL, NULL);
	return Gov;
}

void DestroyGovernment(struct Government* Gov) {
	LnkLstClear(&Gov->SubGovernments);
	free(Gov->PolicyPop);
	free(Gov->PolicyOp);
	free(Gov);
}

void GovernmentThink(struct Government* Gov) {
	if(NEWYEAR(g_GameWorld.Date) != 0) {
	}
}

void GovernmentLowerRank(struct Government* Gov, int NewRank, struct LinkedList* ReleasedSubjects) {
	if(NewRank >= Gov->GovRank)
		return;
	while(Gov->SubGovernments.Size > Gov->AllowedSubjects)
		LnkLstPushBack(ReleasedSubjects, LnkLstPopFront(&Gov->SubGovernments));
	Gov->GovRank = NewRank;
}

const char* GovernmentTypeToStr(int GovType, int Mask) {
	int Ct = 0;

	GovType = GovType & Mask;
	while((GovType & 1) != 1) {
		GovType = GovType >> 1;
		++Ct;
	}
	if(Ct >= GOVTYPE_SIZE)
		return "None";
	return g_GovernmentTypeStr[Ct];
}

void GovernmentLesserJoin(struct Government* Parent, struct Government* Subject, int Relation) {
	if(Subject->GovRank >= Parent->GovRank) {
		struct LinkedList List = {0, NULL, NULL};
		struct LnkLst_Node* Itr = NULL;

		GovernmentLowerRank(Subject, Parent->GovRank - 1, &List);
		Itr = List.Front;
		while(Itr != NULL) {
			((struct Government*)Itr->Data)->Owner.Government = Parent;
			LnkLstPushBack(&Parent->SubGovernments, (struct Government*)Itr->Data);
			Itr = Itr->Next;
		}
		LnkLstClear(&List);
	}
	Subject->Owner.Government = Parent;
	Subject->Owner.Relation = Relation;
	Subject->ZoneColor = Parent->ZoneColor;
}

struct BigGuy* MonarchyNewLeader(const struct Government* Gov) {
	struct Family* Family = Gov->Leader->Person->Family;
	struct BigGuy* Guy = NULL;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i)
		if(Gender(Family->People[i]) == EMALE && Family->People[i]->Age >= ADULT_AGE) {
			if((Guy = RBSearch(&g_GameWorld.BigGuys, Family->People[i])) == NULL) {
				uint8_t BGStats[BGSKILL_SIZE];

				BGStatsWarlord(&BGStats, Random(BG_MINGENSTATS, BG_MAXGENSTATS));
				return CreateBigGuy(Family->People[i], &BGStats, BGMOT_WEALTH);
			}
			return Guy;	
		}
	return NULL;
}

struct BigGuy* ElectiveNewLeader(const struct Government* Gov) {
	struct LnkLst_Node* Itr = Gov->Location->BigGuys.Front;
	struct BigGuy* Guy = NULL;
	struct Person* Person = NULL;
	struct BigGuy* BestCanidate = NULL;
	int BestPopularity = 0;
	int Popularity = 0;

	while(Itr != NULL) {
		Guy = ((struct BigGuy*)Itr->Data);
		Person = Guy->Person;
		if(Gov->Leader->Person != Person 
			&& (Gender(Person) &Gov->RulerGender) != 0 
			&& Person->Age >= ADULT_AGE
			&& (Popularity = BigGuyPopularity(Guy)) > BestPopularity) {
			BestCanidate = Guy;
			BestPopularity = Popularity;
		}
		Itr = Itr->Next;
	}
	return BestCanidate;
}

struct BigGuy* ElectiveMonarchyNewLeader(const struct Government* Gov) {
	struct Settlement* Settlement = Gov->Location; 
	struct LnkLst_Node* Itr = Settlement->BigGuys.Front;
	struct Person* Person = NULL;
	Person = ((struct BigGuy*)Itr->Data)->Person;

	while(Itr != NULL) {
		Person = ((struct BigGuy*)Itr->Data)->Person;
		if(Person->Family->Object.Id == Person->Family->Object.Id && Gender(Person) == EMALE && Person->Age >= ADULT_AGE) {
			return (struct BigGuy*)Itr->Data;
		}
		Itr = Itr->Next;
	}
	return NULL;
}

struct Government* GovernmentTop(struct Government* Gov) {
	do {
		if(Gov->Owner.Government != NULL)
			Gov = Gov->Owner.Government;
		else
			break;
	} while(1);
	return Gov;
}

void GovernmentSetLeader(struct Government* Gov, struct BigGuy* Guy) {
	Gov->Leader = Guy;
	Gov->NextLeader = g_GovernmentSuccession[(Gov->GovType & (GOVRULE_ELECTIVE | GOVRULE_MONARCHY)) - 1](Gov);
	if((Gov->GovType & GOVSTCT_CHIEFDOM) == GOVSTCT_CHIEFDOM) {
		Gov->Appointments.Steward = Guy;
		Gov->Appointments.Judge = Guy;
		Gov->Appointments.Marshall = Guy;
	}
}

void GovernmentAddPolicy(struct Government* Gov, const struct Policy* Policy) {
	struct ActivePolicy* ActPol = malloc(sizeof(struct ActivePolicy));
	int Idx = 0;

	Assert(Policy->OptionsSz < POLICY_SUBSZ);
	ActPol->Policy = Policy;
	for(int i = 0; Policy->Options.Size[i] > 0 && i < POLICY_SUBSZ; ++i) {
//		for(int j = 0; j < CASTE_SIZE; ++j) {
			//_Gov->CastePreference[j] += Policy->Options.Options[Idx].CastePreference[j];
//		}
		ActPol->OptionSel[i] = 0;
		Idx += Policy->Options.Size[i];
	}
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
	//struct BigGuy* Guy = NULL;
	//struct BigGuy* Leader = Gov->Leader;
	//const struct PolicyOption* Option = NULL;

	for(int i = 0; i < OldPolicy->Policy->OptionsSz; ++i) {
		if(OldPolicy->OptionSel[i] == Policy->OptionSel[i] || Policy->OptionSel[i] == POLICYACT_IGNORE)
			continue;
		//_Option = PolicyRow(Policy->Policy, i, Policy->OptionSel[i]);
		/*for(struct LnkLst_Node* Itr = Gov->Location->BigGuys.Front; Itr != NULL; Itr = Itr->Next) {
			Guy = Itr->Data;
			if(Guy == Leader)
				continue;
			BigGuyAddOpinion(
				Guy,
				Leader,
				ACTTYPE_POLICY,
				Option->CastePreference[PERSON_CASTE(Guy->Person)],
				OPNLEN_MEDIUM,
				OPINION_AVERAGE	
				);
		}*/
		OldPolicy->OptionSel[i] = Policy->OptionSel[i];
		break;
	}
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
