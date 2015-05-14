/*
 * File: Government.h
 * Author: David Brotz
 */
#ifndef __GOVERNMENT_H
#define __GOVERNMENT_H

#include "sys/LinkedList.h"

struct BigGuy;
struct Settlement;
struct Reform;

extern struct Reform** g_Reforms;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define REFORM_MAXCHOICE (6)

struct Government;

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

struct Government {
	int GovType;
	int GovRank;
	int AllowedSubjects;
	int AllowedMilLeaders;
	int AuthorityLevel;
	int RulerGender;
	struct BigGuy* Leader;
	struct LinkedList SubGovernments;
	struct Government* ParentGovernment;
	struct LinkedList PossibleReforms;
	struct LinkedList PassedReforms;
	struct LinkedList Advisors;
	void (*LeaderDeath)(struct Government*);
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

void InitReforms(void);
void QuitReforms(void);

struct Reform* CreateReform(const char* _Name, int _AllowedGovs, int _AllowedGovRanks, int _Category, struct ReformOp* _OpCode);
void DestroyReform(struct Reform* _Reform);

void ReformOnPass(struct Government* _Gov, const struct Reform* _Reform);
int CanPassReform(const struct Government* _Gov, const struct Reform* _Reform);

const char* GovernmentTypeToStr(int _GovType, int _Mask);
void GovernmentLowerRank(struct Government* _Gov, int _NewRank, struct LinkedList* _ReleasedSubjects);

void GovernmentLesserJoin(struct Government* _Parent, struct Government* _Subject);
void GovernmentLoadReforms(struct Government* _Gov, struct Reform** _Reforms);
void GovernmentPassReform(struct Government* _Gov, struct Reform* _Reform);

void MonarchyLeaderDeath(struct Government* _Gov);
void ElectiveLeaderDeath(struct Government* _Gov);
void ElectiveMonarchyLeaderDeath(struct Government* _Gov);

struct Government* CreateGovernment(int _GovType, int _GovRank);
void DestroyGovernment(struct Government* _Gov);
int GovernmentLeaderElection(const struct Reform* _Reform, struct Settlement* _Settlement);
int GovernmentLeaderPreferred(const struct Reform* _Reform, struct Settlement* _Settlement);

#endif
