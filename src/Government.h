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

typedef struct BigGuy*(*GovernmentSuccession)(const struct Government*);

enum {
	GOVREL_NONE,
	GOVREL_TRIBUTE
};

enum {
	GOVRULE_ELECTIVE = (1 << 0),
	GOVRULE_MONARCHY = (1 << 1),
	GOVRULE_MASK = GOVRULE_ELECTIVE | GOVRULE_MONARCHY,
	GOVTYPE_ABSOLUTE = (1 << 2),
	GOVTYPE_CONSTITUTIONAL = (1 << 3),
	GOVTYPE_REPUBLIC = (1 << 4),
	GOVTYPE_DEMOCRATIC = (1 << 5),
	GOVTYPE_THEOCRATIC = (1 << 6),
	GOVTYPE_CONSENSUS = (1 << 7),
	GOVTYPE_MASK =  GOVTYPE_ABSOLUTE | GOVTYPE_CONSTITUTIONAL | GOVTYPE_REPUBLIC | GOVTYPE_DEMOCRATIC |
	GOVTYPE_THEOCRATIC | GOVTYPE_CONSENSUS,
	GOVSTCT_TRIBAL = (1 << 8),
	GOVSTCT_CONFEDERACY = (1 << 9),
	GOVSTCT_HEGOMONY = (1 << 10),
	GOVSTCT_FEDERATION = (1 << 11),
	GOVSTCT_FEUDAL = (1 << 12),
	GOVSTCT_DESPOTIC = (1 << 13),
	GOVSTCT_CHIEFDOM = (1 << 14),
	GOVSTCT_CLAN = (1 << 15),
	GOVSTCT_MASK = GOVSTCT_TRIBAL | GOVSTCT_CONFEDERACY | GOVSTCT_HEGOMONY |
	GOVSTCT_FEDERATION | GOVSTCT_FEUDAL | GOVSTCT_DESPOTIC | GOVSTCT_CHIEFDOM | GOVSTCT_CLAN,
	GOVTYPE_SIZE = 15
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


struct GovRelation {
	struct Government* Government;
	int Relation;
};

struct Government {
	uint16_t GovType;
	uint16_t  GovRank;
	struct Settlement* Location;
	struct BigGuy* Leader;
	struct BigGuy* NextLeader;
	struct GovRelation Owner;
	struct LinkedList SubGovernments;
	struct LinkedList Advisors;
	struct LinkedList PolicyList;
	struct {
		struct BigGuy* Judge;
		struct BigGuy* Marshall;
		struct BigGuy* Steward;
	} Appointments;
	uint8_t TaxRate;//Percent from 0 to 100
	uint8_t RulerGender;
	uint8_t AllowedSubjects;
	uint8_t AllowedMilLeaders;
	int8_t* PolicyPop; //How popular a specific policy is. Determines the effective percentage of bets.
	int8_t* PolicyOp; //How unpopular a specific policy is. Determines the effective percentage of bets against a policy.
	SDL_Color ZoneColor;
};

struct RepublicGovernment {
	int GovType;
	int GovRank;
	struct BigGuy* Leader;
	struct LinkedList SubGovernments;
	int NextElection;
};


struct Government* CreateGovernment(int GovType, int GovRank, struct Settlement* Settlement);
void DestroyGovernment(struct Government* Gov);

void GovernmentThink(struct Government* Gov);
const char* GovernmentTypeToStr(int GovType, int Mask);
/**
 * Sets Gov's government rank to NewRank. If Gov cannot contain all of its subjects because of its new rank they will be
 * popped from its SubGovernment list and then placed into ReleasedSubjects.
 */
void GovernmentLowerRank(struct Government* Gov, int NewRank, struct LinkedList* ReleasedSubjects);

/*
 * Sets Subject as a subject government of Parent.
 * If Subject's government rank is equal to or higher than Parent's rank GovernmentLowerRank will be called
 * and the released subjects added to Parent's subjects.
 */
void GovernmentLesserJoin(struct Government* Parent, struct Government* Subject, int Relation);

struct BigGuy* MonarchyNewLeader(const struct Government* Gov);
struct BigGuy* ElectiveNewLeader(const struct Government* Gov);
struct BigGuy* ElectiveMonarchyNewLeader(const struct Government* Gov);
/*
 * Returns the top most parent of Gov.
 */
struct Government* GovernmentTop(struct Government* Gov);
void GovernmentSetLeader(struct Government* Gov, struct BigGuy* Guy);

void GovernmentAddPolicy(struct Government* Gov, const struct Policy* Policy);
void GovernmentRemovePolicy(struct Government* Gov, const struct Policy* Policy);
void GovernmentUpdatePolicy(struct Government* Gov, struct ActivePolicy* OldPolicy, const struct ActivePolicy* Policy);
int GovernmentHasPolicy(const struct Government* Gov, const struct Policy* Policy);

#endif
