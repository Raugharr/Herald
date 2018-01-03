/*
 * File: Treaty.h
 * Author: David Brotz
 */

#ifndef __TREATY_H
#define __TREATY_H

#include "sys/Array.h"
#include "Date.h"

enum {
	TREATY_ALLIANCE,
	TREATY_CEASEFIRE,
	TREATY_TRIBUTE,
	TREATY_SIZE
};

struct Treaties {
	const struct Government* Target;
	const struct Government* Owner;
	struct Array Treaties; //Array of struct Treaty*
};

struct Treaty {
	unsigned int Type;
	DATE Date;
	DATE End;//End Date of treaty.
	union {
		uint16_t Tribute;
	};
};

struct Treaty* CreateTreaty(unsigned int Type, unsigned int Length);
void DestroyTreaty(struct Treaty* Treaty);

struct Treaty* FindTreaty(struct Treaties* List, void* Target);

/**
 * Tribute amount is in money, will be converted to goods if the tribute cannot pay.
 */
void TreatySetTribute(struct Treaty* Treaty, uint16_t Tribute);
void CollectTribute(struct Government* Tribute, struct Government* Parent, uint16_t Payment);

struct Treaties* CreateTreatyList(struct Government* Owner, struct Government* Target);
void DestroyTreatyList(struct Treaties* List);
void TreatyListThink(struct Treaties* List);
const char* TreatyGetName(int Index);

#endif
