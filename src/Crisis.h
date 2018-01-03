/**
 * File: Crisis.h
 * Author: David Brotz
 */

#ifndef __CRISIS_H
#define __CRISIS_H

#ifndef NULL
#define NULL ((void*)0)
#endif

#include <stdint.h>

struct Settlement;
struct BigGuy;

enum {
	CRISIS_RAPE,
	CRISIS_MURDER,
	CRISIS_THEFT,
	CRISIS_SIZE
};

enum ECrisisDemands {
	CRIME_COMPENSATION,
	CRIME_DUEL,
	CRIME_OUTCAST,
	CRIME_TRIAL,
	CRIME_DEATH,
	CRIME_SIZE
};

extern const char* g_CrisisDemands[];
extern const char* g_CrisisMissions[];

/**
 * 
 */
struct Crisis {
	uint32_t Type;
	struct Person* Defender; //Person who the Offender targeted.
	struct Person* Offender; //Person who commited the action to start the crisis.
	/**
	 * Index of the type of compensation the defender demands to be satisfied.
	   If the defender is given compensation value that is less than the value in Demand then they will not be satisfied.
	   This variable should contain a value from ECrisisDemands.
	 */
	uint8_t Demand;
};

void CrisisProcess(struct Crisis* Crisis, uint32_t Option);
void GenerateCrisis(struct Settlement* Settlement);
struct Crisis* CreateCrisis(struct Person* Offender, struct Person* Defender, uint8_t Type);
#endif


