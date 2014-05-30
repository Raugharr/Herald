/*
 * File: Occupation.h
 * Author: David Brotz
 */

#ifndef __OCCUPATION_H
#define __OCCUPATION_H

enum {
	EFARMER
};

struct Workplace {
	struct Good* Output;
	struct Building* Workplace;
};

union JobUnion{
	struct Workplace* Workplace;
	int Occupation;
};

struct Occupation {
	int Id;
	const char* Name;
	union JobUnion Job;
	int SpecialJob;
	struct Constraint* AgeConst;
};

struct Occupation* CreateOccupation(const char* _Name, struct Good* _Output, struct Building* _Workplace);
struct Occupation* CopyOccupation(const struct Occupation* _Occupation);
void DestroyOccupation(struct Occupation* _Occupation);

#endif
