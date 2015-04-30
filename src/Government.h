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

enum {
	GOV_TRIBAL,
	GOV_CONFEDERACY
};

struct Reform {
	const char* Name;
	int PassiveReform;
	int (*CanPass)(const struct Reform*, struct Settlement*);
	void (*OnPass)(struct Government*);
	int (*OnEvent)(const struct Reform*, struct Settlement*);
	struct Reform* Next;
	struct Reform* Prev;
};

struct Government {
	int GovType;
	struct BigGuy* Leader;
	struct LinkedList SubGovernments;
	struct Reform* LeaderDeath;
	struct Reform* LesserGovLeave;
	struct Reform* LesserGovJoin;
};


struct Government* CreateGovernment(int _GovType);
int GovernmentLeaderElection(const struct Reform* _Reform, struct Settlement* _Settlement);
int GovernmentLeaderPreferred(const struct Reform* _Reform, struct Settlement* _Settlement);
static struct Reform g_Foo[] = {
		{"Elected Leader", NULL, NULL, NULL, NULL},
		{"Preferred Leader Election", NULL, NULL, NULL, NULL},
		{"Hereditary Leader", NULL, NULL, NULL, NULL}
};

#endif
