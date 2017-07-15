/*
 * File: Government.h
 * Author: David Brotz
 */
#ifndef __GOVERNMENT_H
#define __GOVERNMENT_H

#include "sys/LinkedList.h"

#include "Family.h"

#include <SDL2/SDL.h>

struct BigGuy;
struct Settlement;
struct ActivePolicy;
struct Policy;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define TAX_MAX (100)
#define TAX_MIN (0)

struct Government;

typedef struct BigGuy*(*GovernmentSuccession)(const struct BigGuy*, const struct Government*);

enum {
	GOVREL_NONE,
	GOVREL_TRIBUTE
};

enum {
	GOVRANK_SETTLEMENT = 0, //Controls a single settlement.
	GOVRANK_COUNTY = 1, //Controls several settlements.
	GOVRANK_PROVINCE = 2, //Controls several counties.
	GOVRANK_KINGDOM = 4, //Controls several provinces.
	GOVRANK_EMPIRE = 8, //Controls several kingdoms.
	GOVRANK_ALL = 15
};

enum {
	GOVARMY_NO,
	GOVARMY_ASK,
	GOVARMY_YES
};

enum {
	GOVGEN_MALE,
	GOVGEN_BOTH,
	GOVGEN_MALEPERF,
	GOVGEN_FEMALE
};

enum {
	GOVRULE_ELECTIVE = (1 << 0),
	GOVRULE_MONARCHY = (1 << 1),
	GOVRULE_MASK = GOVRULE_ELECTIVE | GOVRULE_MONARCHY,
	GOVTYPE_ABSOLUTE = (1 << 2), //Leader determines all the rules.
	GOVTYPE_CONSTITUTIONAL = (1 << 3), //Leader enacts rules, 
	GOVTYPE_REPUBLIC = (1 << 4), //People vote for who to create rules.
	GOVTYPE_DEMOCRATIC = (1 << 5), //Every eligible person is allowed a vote.
	GOVTYPE_THEOCRATIC = (1 << 6), //Ruler must be religious
	GOVTYPE_MASK =  GOVTYPE_ABSOLUTE | GOVTYPE_CONSTITUTIONAL | GOVTYPE_REPUBLIC | GOVTYPE_DEMOCRATIC | GOVTYPE_THEOCRATIC,
	GOVSTCT_CONFEDERACY = (1 << 7),
	GOVSTCT_HEGOMONY = (1 << 8),
	GOVSTCT_FEDERATION = (1 << 9),
	GOVSTCT_FEUDAL = (1 << 10),
	GOVSTCT_DESPOTIC = (1 << 11),
	GOVSTCT_CLAN = (1 << 12),	
	GOVSTCT_MASK = GOVSTCT_CONFEDERACY | GOVSTCT_HEGOMONY |
	GOVSTCT_FEDERATION | GOVSTCT_FEUDAL | GOVSTCT_DESPOTIC | GOVSTCT_CLAN,
	GOVMIX_CONCENSUS = (1 << 13),
	GOVMIX_TRIBAL = (1 << 14),
	GOVMIX_CHIEFDOM = (1 << 15),
	GOVMIX_MASK = GOVMIX_CONCENSUS | GOVMIX_TRIBAL | GOVMIX_CHIEFDOM,
	GOVTYPE_SIZE = 16
};

/*
 * Used to sort all the reforms in the UI.
 */
enum {
	GOVCAT_MILITARY = (1 << 0),
	GOVCAT_DIPLOMANCY = (1 << 1),
	GOVCAT_RELIGION = (1 << 2),
	GOVCAT_INTERNAL = (1 << 3),
	GOVCAT_LEADER = (1 << 4),
	GOVCAT_INTERNALLEADER = (1 << 5),
	GOVCAT_MILITARYLEADER = (1 << 6)
};

enum {
	GOVSTAT_AUTHORITY,
	GOVSTAT_SIZE
};

struct Government {
	struct Object Object;
	uint16_t GovType;
	uint16_t GovRank;
	struct Settlement* Location;
	struct BigGuy* Leader;
	struct BigGuy* NextLeader;
	struct Government* Owner;
	struct Relation* Relations;
	struct Array SubGovernments;
	struct LinkedList Advisors;
	struct LinkedList PolicyList;
	uint8_t TaxRate;//Percent from 0 to 100
	//uint8_t AllowedSubjects;
	uint8_t AllowedMilLeaders;
	int8_t* PolicyPop; //How popular a specific policy is. Determines the effective percentage of bets.
	int8_t* PolicyOp; //How unpopular a specific policy is. Determines the effective percentage of bets against a policy.
	uint8_t GenderLaw : 4;
	uint8_t ArmyLaw : 4;
	SDL_Color ZoneColor;
};

struct Treaty {
	int Type;
	uint32_t Year : 27;
	uint32_t Month : 5;
	struct Government* Target;
	struct Government* From;
};

struct Government* CreateGovernment(struct Settlement* Settlement, uint32_t GovRule, uint32_t GovStruct, uint32_t GovType, uint32_t GovMix, uint32_t GovRank);
void DestroyGovernment(struct Government* Gov);

void GovernmentThink(struct Government* Gov);
const char* GovernmentTypeToStr(int GovType, int Mask);
/**
 * Sets Gov's government rank to NewRank. If Gov cannot contain all of its subjects because of its new rank they will be
 * popped from its SubGovernment list and then placed into ReleasedSubjects.
 */
//void GovernmentLowerRank(struct Government* Gov, int NewRank, struct LinkedList* ReleasedSubjects);

/*
 * Sets Subject as a subject government of Parent.
 * If Subject's government rank is equal to or higher than Parent's rank GovernmentLowerRank will be called
 * and the released subjects added to Parent's subjects.
 */
void GovernmentLesserJoin(struct Government* Parent, struct Government* Subject);

/**
 * Find the first adult male that is a child of Guy to replace him creating a Big Guy if necessary.
 */
struct BigGuy* MonarchyNewLeader(const struct BigGuy* Guy, const struct Government* Gov);
struct BigGuy* ElectiveNewLeader(const struct BigGuy* Guy, const struct Government* Gov);
struct BigGuy* ElectiveMonarchyNewLeader(const struct BigGuy* Guy, const struct Government* Gov);
/*
 * Returns the top most parent of Gov.
 */
struct Government* GovernmentTop(struct Government* Gov);
void GovernmentSetLeader(struct Government* Gov, struct BigGuy* Guy);

void GovernmentAddPolicy(struct Government* Gov, const struct Policy* Policy);
void GovernmentRemovePolicy(struct Government* Gov, const struct Policy* Policy);
void GovernmentUpdatePolicy(struct Government* Gov, struct ActivePolicy* OldPolicy, const struct ActivePolicy* Policy);
int GovernmentHasPolicy(const struct Government* Gov, const struct Policy* Policy);
struct BigGuy* CreateNewLeader(struct Government* Gov);
const char* GovernmentRankStr(const struct Government* Gov);
void GovernmentRaiseArmy(struct Government* Gov);
bool CanGovern(const struct Government* Gov, const struct Person* Person);

#endif
