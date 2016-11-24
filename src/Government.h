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
	uint32_t GovType;
	uint32_t  GovRank;
	uint32_t AllowedSubjects;
	uint32_t AllowedMilLeaders;
	uint32_t AuthorityLevel;
	uint32_t RulerGender;
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
	int CastePreference[CASTE_SIZE];
	SDL_Color ZoneColor;
};

struct RepublicGovernment {
	int GovType;
	int GovRank;
	struct BigGuy* Leader;
	struct LinkedList SubGovernments;
	int NextElection;
};


struct Government* CreateGovernment(int _GovType, int _GovRank, struct Settlement* _Settlement);
void DestroyGovernment(struct Government* _Gov);

void GovernmentThink(struct Government* _Gov);
const char* GovernmentTypeToStr(int _GovType, int _Mask);
/**
 * Sets _Gov's government rank to _NewRank. If _Gov cannot contain all of its subjects because of its new rank they will be
 * popped from its SubGovernment list and then placed into _ReleasedSubjects.
 */
void GovernmentLowerRank(struct Government* _Gov, int _NewRank, struct LinkedList* _ReleasedSubjects);

/*
 * Sets _Subject as a subject government of _Parent.
 * If _Subject's government rank is equal to or higher than _Parent's rank GovernmentLowerRank will be called
 * and the released subjects added to _Parent's subjects.
 */
void GovernmentLesserJoin(struct Government* _Parent, struct Government* _Subject, int _Relation);

struct BigGuy* MonarchyNewLeader(const struct Government* _Gov);
struct BigGuy* ElectiveNewLeader(const struct Government* _Gov);
struct BigGuy* ElectiveMonarchyNewLeader(const struct Government* _Gov);
/*
 * Returns the top most parent of _Gov.
 */
struct Government* GovernmentTop(struct Government* _Gov);
void GovernmentSetLeader(struct Government* _Gov, struct BigGuy* _Guy);

void GovernmentAddPolicy(struct Government* _Gov, const struct Policy* _Policy);
void GovernmentRemovePolicy(struct Government* _Gov, const struct Policy* _Policy);
void GovernmentUpdatePolicy(struct Government* _Gov, struct ActivePolicy* _OldPolicy, const struct ActivePolicy* _Policy);
int GovernmentHasPolicy(const struct Government* _Gov, const struct Policy* _Policy);

#endif
