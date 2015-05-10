/*
 * File: Government.h
 * Author: David Brotz
 */
#ifndef __GOVERNMENT_H
#define __GOVERNMENT_H

#include "sys/LinkedList.h"

struct BigGuy;
struct Settlement;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define REFORM_MAXCHOICE (6)

/*
 * Organize by power structure, then by how leader is elective.
 */

enum {
	GOV_TRIBAL,
	GOV_CONFEDERACY,
	GOV_HEGEMONY
};

enum {
	GOVRULE_ELECTIVE,
	GOVRULE_MONARCHY
};

enum {
	GOVTYPE_ABSOLUTE = (1 << 0),
	GOVTYPE_CONSTITUTIONAL = (1 << 1),
	GOVTYPE_REPUBLIC = (1 << 2),
	GOVTYPE_DEMOCRATIC = (1 << 3),
	GOVTYPE_THEOCRATIC = (1 << 4),
	GOVTYPE_CONSENSUS = (1 << 5),
	GOVSTCT_TRIBAL = (1 << 6),
	GOVSTCT_CONFEDERACY = (1 << 7),
	GOVSTCT_HEGOMONY = (1 << 8),
	GOVSTCT_FEDERATION = (1 << 9),
	GOVSTCT_FEUDAL = (1 << 10),
	GOVSTCT_DESPOTIC = (1 << 11)
};

struct Reform {
	const char* Name;
	int AllowedGovs;
	int (*CanPass)(const struct Reform*, struct Settlement*);
	void (*OnPass)(struct Government*);//Use government state to determine if its possible instead of a function.
	int (*OnEvent)(const struct Reform*, struct Settlement*);
	struct Reform* Next[REFORM_MAXCHOICE];
	struct Reform* Prev;
};

struct Government {
	int GovType;
	int GovRank;
	struct BigGuy* Leader;
	struct LinkedList SubGovernments;
	struct Reform* LeaderDeath;
	struct Reform* LesserGovLeave;
	struct Reform* LesserGovJoin;
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

void GovernmentLesserJoin(struct Government* _Gov);
void GovernmentLesserLeave(struct Government* _Gov);

struct Government* CreateGovernment(int _GovType);
int GovernmentLeaderElection(const struct Reform* _Reform, struct Settlement* _Settlement);
int GovernmentLeaderPreferred(const struct Reform* _Reform, struct Settlement* _Settlement);
static struct Reform g_Foo[] = {
		{"Elected Leader", NULL, NULL, NULL, NULL},
		{"Preferred Leader Election", NULL, NULL, NULL, NULL},
		{"Hereditary Leader", NULL, NULL, NULL, NULL},
		{"Become Military Leader", NULL, NULL, NULL, NULL},
		{"Create Retinue", NULL, NULL, NULL, NULL},
		{"Become Military Leader", NULL, NULL, NULL, NULL},
		{"Appoint Military Leader", NULL, NULL, NULL, NULL},
};

#endif
