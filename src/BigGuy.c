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
#include "Relation.h"
#include "Retinue.h"

#include "AI/Agent.h"

#include "sys/Math.h"
#include "sys/Constraint.h"
#include "sys/Event.h"
#include "sys/RBTree.h"
#include "sys/Event.h"
#include "sys/LinkedList.h"
#include "video/GuiLua.h"

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

int BigGuyIdInsert(const struct BigGuy* One, const struct BigGuy* Two) {
	return One->Person->Object.Id - Two->Person->Object.Id;
}

int BigGuyIdCmp(const int* Two, const struct BigGuy* BigGuy) {
	return (*Two) - BigGuy->Person->Object.Id;
}

int BigGuyMissionCmp(const struct BigGuy* BigGuy, const struct Mission* Mission) {
	return 0;
}

void BigGuyDeath(struct BigGuy* Guy) {
	struct LinkedList* List = &FamilyGetSettlement(Guy->Person->Family)->BigGuys;
	struct LnkLst_Node* Itr = List->Front;

	while(Itr != NULL) {
		if(Itr->Data == Guy) {
			LnkLstRemove(List, Itr);
			break;
		}
		Itr = Itr->Next;
	}
	RBDelete(&g_GameWorld.BigGuys, Guy->Person);
	DestroyObject(&Guy->Object);
	if(g_GameWorld.Player == Guy) {
		char* Str = NULL;
		if((Str = SAlloc(1024)) == NULL) {
			LogNoMem();
			return;
		}
		struct BigGuy* NewLeader = MonarchyNewLeader(g_GameWorld.Player, MALE);

		//strcpy(Str, g_GameWorld.Player->Person->Name);
		//strcat(Str, " has been killed.");
		if(NewLeader != NULL) {
		//	strcat(Str, " You will now play as ");
		//	strcat(Str, NewLeader->Person->Name);
		//	strcat(Str, " ");
		//	strcat(Str, NewLeader->Person->Family->Name);
		//	strcat(Str, ".");
			if(RBSearch(&g_GameWorld.Agents, NewLeader) != NULL)
				RBDelete(&g_GameWorld.Agents, NewLeader);
			g_GameWorld.Player = NewLeader;
		} else {
			//strcat(Str, " There is no one left to inherit your lands.");
			g_GameWorld.Player = NULL;
		}
		//MessageBox(Str);
	} else 
		RBDelete(&g_GameWorld.Agents, Guy);
	if(Guy->Agent != NULL)
		DestroyAgent(Guy->Agent);
	ILL_CREATE(g_GameWorld.DeadBigGuys, &Guy->Object);
	//DestroyBigGuy(Guy);
}

void BGOnTargetDeath(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct BigGuy* Guy = Data->One;

	EventHookRemove(Data->EventType, Guy, Data->OwnerObj, NULL);
}

/*
void BGOnNewPlot(const struct EventData* Data, void* Extra1) {
	struct BigGuy* Guy = Data->One;
	struct Plot* Plot = Extra1;
	int LeaderRel = 0; 
	int TargetRel = 0;

	if(Guy == PlotLeader(Plot) || Guy == PlotTarget(Plot))
		return;
	LeaderRel = BigGuyRelation(BigGuyGetRelation(Guy, PlotLeader(Plot))); 
	if(PlotTarget(Plot) != NULL) {
		TargetRel = BigGuyRelation(BigGuyGetRelation(Guy, PlotTarget(Plot)));
	}
	if(LeaderRel >= BGREL_LIKE && TargetRel < BGREL_LIKE) {
		PlotJoin(Plot, PLOT_ATTACKERS, Guy);	
	} else if(LeaderRel < BGREL_LIKE && TargetRel >= BGREL_LIKE) {
		PlotJoin(Plot, PLOT_DEFENDERS, Guy);
	}
}*/

struct BigGuy* CreateBigGuy(struct Person* Person, uint8_t (*Stats)[BGSKILL_SIZE], int Motivation) {
	struct BigGuy* BigGuy = (struct BigGuy*) malloc(sizeof(struct BigGuy));

	Person->Flags |= BIGGUY;
	CreateObject((struct Object*)BigGuy, OBJECT_BIGGUY);
	BigGuy->Person = Person;
	BigGuy->IsDirty = 1;
	BigGuy->Relations = NULL;
	memcpy(&BigGuy->Stats, Stats, sizeof(uint8_t) * BGSKILL_SIZE);
	BigGuy->Action = BGACT_NONE;
	BigGuy->Agent = CreateAgent(BigGuy);
	BigGuy->Popularity = BGRandRes(BigGuy, BGSKILL_CHARISMA);
	BigGuy->Glory = 0;
	BigGuy->PopularityDelta = 0;
	RBInsert(&g_GameWorld.BigGuys, BigGuy);
	RBInsert(&g_GameWorld.Agents, BigGuy->Agent);

	ConstructLinkedList(&BigGuy->PlotsAgainst);
	BigGuy->Traits = BGRandTraits(&BigGuy->TraitCt);
	BigGuy->Action = BGACT_NONE;
	for(struct LnkLst_Node* Itr = Person->Family->HomeLoc->BigGuys.Front; Itr != NULL; Itr = Itr->Next) {
		struct BigGuy* Target = Itr->Data;

		CreateBigGuyRelation(BigGuy, Target);
		CreateBigGuyRelation(Target, BigGuy);
	}
	LnkLstPushBack(&FamilyGetSettlement(Person->Family)->BigGuys, BigGuy);
	return BigGuy;
}

void DestroyBigGuy(struct BigGuy* BigGuy) {
	//EventHookRemove(EVENT_NEWPLOT, BigGuyHome(BigGuy), BigGuy, NULL);
	free(BigGuy->Traits);
	free(BigGuy);
}

void BigGuyThink(struct BigGuy* Guy) {
	Guy->Popularity -= g_GameWorld.DecayRate[(int)Guy->Popularity] / YEAR_DAYS;
	//_Guy->Glory -= g_GameWorld.DecayRate[(int)Guy->Glory] / YEAR_DAYS;
	for(struct Relation* Relation = Guy->Relations; Relation != NULL; Relation = Relation->Next) {
		RelUpdate(Relation);
	}
}

void BigGuySetState(struct BigGuy* Guy, int State, int Value) {
	//WorldStateSetAtom(&Guy->State, State, Value);
	Guy->IsDirty = 1;
}

/*struct BigGuy* BigGuyLeaderType(struct Person* Person) {
	while(Person != NULL) {
		if(Gender(Person) == MALE && Person->Age.Years > ADULT_AGE) {
			uint8_t Stats[BGSKILL_SIZE];

			BGStatsWarlord(&Stats, 50);
			return CreateBigGuy(Person, &Stats, BGMOT_RULE); //NOTE: Make sure we aren't making a big guy when the person is already a big guy.
		}
		Person = Person->Next;
	}
	return NULL;
}*/

void BGStatsRandom(int Points, int StatCt, ...) {
	va_list Valist;
	uint8_t* Stats[StatCt];
	uint8_t* Temp = NULL;
	int Rand = 0;
	int Result = 0;
	int PointsLeft = Points;
	double StatDist[StatCt];

	va_start(Valist, StatCt);
	for(int i = 0; i < StatCt; ++i)
		Stats[i] = va_arg(Valist, uint8_t*);
	for(int i = 0; i < StatCt; ++i)
		StatDist[i] = va_arg(Valist, double);
#ifdef DEBUG
	double StatCheck = 0;

	for(int i = 0; i < StatCt; ++i)
		StatCheck += StatDist[i];
	Assert(StatCheck == 1.0);
#endif
	for(int i = 0; i < StatCt; ++i) {
		Temp = Stats[i];
		Rand = Random(0, StatCt - 1);
		Stats[i] = Stats[Rand];
		Stats[Rand] = Temp;
	}
	for(int i = 0; i < StatCt; ++i) {
		Result = ceil(Points * StatDist[i]);
		if(Result > STAT_MAX)
			Result = STAT_MAX;
		if(PointsLeft < Result)
			Result = PointsLeft;
		*Stats[i] = Result;
		PointsLeft -= Result;
	}
	va_end(Valist);
}

int BGRandRes(const struct BigGuy* Guy, int Stat) {
	return (Guy->Stats[Stat] + Random(1, 100)) / 2;
}

/*
 * Generates a stat block based on a caste.
 */
void GenerateStats(uint8_t Caste, uint8_t (*Stats)[BGSKILL_SIZE]) {
	int Points = Random(BG_MINGENSTATS, BG_MAXGENSTATS);

	switch(Caste) {
		case CASTE_THRALL:
			Points -= Points / 4;
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.05, 0.25, 0.25, 0.15, 0.10, 0.10, 0.10);
			return;	
		case CASTE_FARMER:
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.05, 0.25, 0.25, 0.15, 0.10, 0.10, 0.10); 
			return;	
		case CASTE_CRAFTSMAN:
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.05, 0.20, 0.25, 0.15, 0.10, 0.15, 0.10); 
			return;	
		case CASTE_LOWNOBLE:
			Points += Points / 10; 
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.20, 0.15, 0.15, 0.15, 0.10, 0.15, 0.10); 
			return;	
		case CASTE_PRIEST:
			Points += Points / 10; 
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.05, 0.10, 0.10, 0.10, 0.20, 0.25, 0.20); 
			return;	
		case CASTE_WARRIOR:
			Points += Points / 10; 
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.25, 0.15, 0.15, 0.15, 0.10, 0.10, 0.10); 
			return;	
		case CASTE_NOBLE:
			Points += Points / 4;
			BGStatsRandom(Points, BGSKILL_SIZE, &(*Stats)[BGSKILL_COMBAT], &(*Stats)[BGSKILL_STRENGTH], &(*Stats)[BGSKILL_TOUGHNESS],
				&(*Stats)[BGSKILL_AGILITY], &(*Stats)[BGSKILL_WIT], &(*Stats)[BGSKILL_CHARISMA], &(*Stats)[BGSKILL_INTELLIGENCE],
				0.20, 0.15, 0.15, 0.15, 0.10, 0.15, 0.10); 
			return;	
	}
}

struct Trait* RandomTrait(struct Trait** Traits, uint8_t TraitCt, struct HashItr* Itr) {
	int Rand = Random(0, g_Traits.Size - 1);
	int Ct = 0;
	int FirstPick = Rand;

	loop_top:
	//Pick a random trait.
	while(Itr != NULL && Ct < Rand) {
		Itr = HashNext(&g_Traits, Itr);
		++Ct;
	}
	//Determine if trait is a valid option.
	for(int i = 0; i < TraitCt; ++i) {
		//Same trait is picked, pick another if there is another valid trait to be picked.
		if(Traits[i] == HashItrData(Itr))
			goto repick_trait;
		for(int j = 0; Traits[i]->Prevents[j] != NULL; ++j) {
			//One of the already picked traits prevents this trait from being chosen.
			if(Traits[i]->Prevents[j] == HashItrData(Itr))
				goto repick_trait;
		}
	}
	return (struct Trait*) HashItrData(Itr);
	repick_trait:
	++Rand;
	if(Rand >= g_Traits.Size) {
		Rand = 0;	
		HashItrRestart(&g_Traits, Itr);
	}
	//No more valid traits to be picked from
	if(Rand == FirstPick)
		return NULL;
	goto loop_top;
}

struct Trait** BGRandTraits(uint8_t* TraitCt) {
	struct HashItr* Itr = HashCreateItr(&g_Traits);
	struct Trait** Traits = NULL; 
	struct Trait* Trait = NULL;

	*TraitCt = Random(1, 3);
	Traits = calloc(*TraitCt, sizeof(struct Trait*));
	for(uint8_t i = 0; i < *TraitCt; ++i) {
		if((Trait = RandomTrait(Traits, i, Itr)) == NULL) {
			Traits[i] = NULL;
			break;
		}
		Traits[i] = Trait;
		HashItrRestart(&g_Traits, Itr);	
	}
	HashDeleteItr(Itr);
	return Traits;
}

int HasTrait(const struct BigGuy* BigGuy, const struct Trait* Trait) {
	for(int i = 0; i < BigGuy->TraitCt; ++i) {
		if(BigGuy->Traits[i]
			 == Trait)
			return 1;
	}
	return 0;
}

void BigGuySetAction(struct BigGuy* Guy, int Action, struct BigGuy* Target, void* Data) {
//	struct RBNode* Node = NULL;
//	struct BigGuyActionHist* Hist = NULL;
//	struct BigGuyActionHist Search = {_Guy, Action, 0};

	/*
	Guy->Action.Target = Target;
	Guy->Action.Data = Data;
	Guy->Action.Type = Action;
	if((Node = RBSearchNode(&g_GameWorld.ActionHistory, &Search)) != NULL) {
		Hist = (struct BigGuyActionHist*) Node->Data;
		if(DaysBetween(Hist->DayDone, g_GameWorld.Date) >= g_BGActCooldown[Hist->ActionType]) {
			Hist->DayDone = g_GameWorld.Date;
			RBDeleteNode(&g_GameWorld.ActionHistory, Node);	
			RBInsert(&g_GameWorld.ActionHistory, Hist);
		} else {
			return;
		}
	} else {
		Hist = malloc(sizeof(struct BigGuyActionHist));
		Hist->Owner = Guy;
		Hist->ActionType = Action;
		Hist->DayDone = g_GameWorld.Date;
		RBInsert(&g_GameWorld.ActionHistory, Hist);
		EventHook(EVENT_DEATH, BGOnTargetDeath, Target->Person, Guy, NULL);
	}
	*/
	//FIXME: Add a way to store the target.
	/*switch(Action) {
	case BGACT_SABREL:
		MissionAction("RUMOR.1", Guy, Target);
		break;
	case BGACT_STEAL:
		MissionAction("STLCT.1", Guy, Target);
		break;
	case BGACT_DUEL:
		MissionAction("DUEL.2", Target, Guy);
		break;
	case BGACT_MURDER:
		MissionAction("MURDR.1", Target, Guy);
		break;
	case BGACT_PLOTOVERTHROW:
		CreatePlot(PLOT_OVERTHROW, NULL, Guy, Target);
		break;
	default:
		//_Guy->ActionFunc = NULL;
		return;
	}*/
	Guy->Action = Action;
	Guy->ActionTarget = Target;
}

struct Settlement* BigGuyHome(struct BigGuy* Guy) {
	return Guy->Person->Family->HomeLoc;
}

int BigGuyOpposedCheck(const struct BigGuy* One, const struct BigGuy* Two, int Skill) {
	assert(Skill >= 0 && Skill < BGSKILL_SIZE);
	return ((Random(1, 100) + One->Stats[Skill]) - (Random(1, 100) + Two->Stats[Skill])) / 10;
}

int BigGuySkillCheck(const struct BigGuy* Guy, int Skill, int PassReq) {
	assert(Skill >= 0 && Skill < BGSKILL_SIZE);
	return ((Random(1, 100) + Guy->Stats[Skill]) >= PassReq);
}

int BigGuySuccessMargin(const struct BigGuy* Guy, int Skill, int PassReq) {
	int Margin = 0;

	assert(Skill >= 0 && Skill < BGSKILL_SIZE);
	Margin = Random(1, 100) + Guy->Stats[Skill] - PassReq;
	if(Margin >= 0)
		Margin = Margin / 10 + 1;
	else 
		Margin = Margin / 10 - 1;
	return Margin;
}

int BigGuyPopularity(const struct BigGuy* Guy) {
	return Guy->Popularity;
}


int BigGuyPlotPower(const struct BigGuy* Guy) {
	return Guy->Stats[BGSKILL_WIT];
}

struct Retinue* BigGuyRetinue(const struct BigGuy* Leader) {
	struct Settlement* Settlement = PersonHome(Leader->Person);

	for(struct Retinue* Retinue = Settlement->Retinues; Retinue != NULL; Retinue = Retinue->Next) {
		if(Retinue->Leader == Leader)
			return Retinue;
	}
	return NULL;
}

void BigGuyPlotTarget(struct BigGuy* Guy, struct Plot* Plot) {
	LnkLstPushBack(&Guy->PlotsAgainst, Plot);
}

void CreateBigGuyRelation(struct BigGuy* Owner, struct BigGuy* Target) {
	struct Relation* Relation = CreateRelation(Owner, Target, &Owner->Relations);

	for(uint8_t i = 0; i < Owner->TraitCt; ++i) {
		for(uint8_t j = 0; j < Target->TraitCt; ++j) {
			if(TraitDislikes(Owner->Traits[i], Target->Traits[j]) != 0) {
				ChangeRelation(Relation, ACTTYPE_TRAIT, -(REL_TRAIT), OPNLEN_FOREVER, OPINION_STATIC);
			} else if(TraitLikes(Owner->Traits[i], Target->Traits[j])) {
				ChangeRelation(Relation, ACTTYPE_TRAIT, REL_TRAIT, OPNLEN_FOREVER, OPINION_STATIC);
			}
		}
	}
}
