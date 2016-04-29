/*
 * File: Government.h
 * Author: David Brotz
 */
#ifndef __GOVERNMENT_H
#define __GOVERNMENT_H

#include "sys/LinkedList.h"

#include <SDL2/SDL.h>

struct BigGuy;
struct Settlement;
struct Reform;

extern struct Reform** g_Reforms;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define REFORM_MAXCHOICE (6)
#define REFORM_POPULARITYMAX (1000)
#define REFORM_PASSVOTE (0.7f)

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

#define GOVSTAT_MAX (6)

struct ReformOp {
	unsigned char OpCode;
	unsigned char Value;
};

struct Reform {
	char* Name;
	int AllowedGovs;
	int AllowedGovRanks;
	int Category;
	int AuthLvlReq;
	int GovType;
	int LeaderReqs[GOVSTAT_SIZE];
	int LeaderCosts[GOVSTAT_SIZE];
	struct ReformOp OpCode;
	struct Reform* Next[REFORM_MAXCHOICE];
	struct Reform* Prev;
};

struct ReformPassing {
	struct Reform* Reform;
	struct Government* Gov;
	int MaxVotes;
	int VotesFor;
	int Popularity;
	int Escalation;
};

struct GovRelation {
	struct Government* Government;
	int Relation;
};

struct Government {
	int GovType;
	int GovRank;
	int AllowedSubjects;
	int AllowedMilLeaders;
	int AuthorityLevel;
	int RulerGender;
	struct Settlement* Location;
	struct BigGuy* Leader;
	struct BigGuy* NextLeader;
	struct GovRelation Owner;
	struct ReformPassing* Reform;
	struct LinkedList SubGovernments;
	struct LinkedList PossibleReforms;
	struct LinkedList PassedReforms;
	struct LinkedList Advisors;
	SDL_Color ZoneColor;
};

struct RepublicGovernment {
	int GovType;
	int GovRank;
	struct BigGuy* Leader;
	struct LinkedList SubGovernments;
	struct Reform* LeaderDeath;
	struct Reform* LesserGovLeave;
	struct Reform* LesserGovJoin;
	int NextElection;
};

//TODO: Move InitReforms and QuitReforms to InitHerald or somewhere else more appropriate.
int InitReforms(void);
void QuitReforms(void);

struct Government* CreateGovernment(int _GovType, int _GovRank, struct Settlement* _Settlement);
void DestroyGovernment(struct Government* _Gov);

struct ReformPassing* CreateReformPassing(struct Reform* _Reform, struct Government* _Gov);
void DestroyReformPassing(struct ReformPassing* _Reform);
void ReformEscalate(struct ReformPassing* _Reform, const struct BigGuy* _Guy);
void ReformImprovePopularity(struct ReformPassing* _Reform, const struct BigGuy* _Guy);

struct Reform* CreateReform(const char* _Name, int _AllowedGovs, int _AllowedGovRanks, int _Category, struct ReformOp* _OpCode);
void DestroyReform(struct Reform* _Reform);

void ReformOnPass(struct Government* _Gov, const struct Reform* _Reform);
int CanPassReform(const struct Government* _Gov, const struct Reform* _Reform);

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
void GovernmentLoadReforms(struct Government* _Gov, struct Reform** _Reforms);
void GovernmentPassReform(struct Government* _Gov, struct Reform* _Reform);

struct BigGuy* MonarchyNewLeader(const struct Government* _Gov);
struct BigGuy* ElectiveNewLeader(const struct Government* _Gov);
struct BigGuy* ElectiveMonarchyNewLeader(const struct Government* _Gov);
/*
 * Returns the top most parent of _Gov.
 */
struct Government* GovernmentTop(struct Government* _Gov);
void GovernmentSetLeader(struct Government* _Gov, struct BigGuy* _Guy);

#endif
