/*
 * File: Occupation.h
 * Author: David Brotz
 */

#ifndef __OCCUPATION_H
#define __OCCUPATION_H

typedef struct lua_State lua_State;

enum {
	ENONE = 0,
	EFARMER
};

struct Occupation {
	int Id;
	const char* Name;
	int SpecialJob;
	struct Constraint* AgeConst;
};

struct Occupation* CreateOccupation(const char* _Name, struct GoodBase* _Output, struct Building* _Workplace, struct Constraint* AgeConst);
struct Occupation* CreateOccupationSpecial(const char* _Name, int _Job);
struct Occupation* CopyOccupation(const struct Occupation* _Occupation);
void DestroyOccupation(struct Occupation* _Occupation);
struct Occupation* OccupationLoad(lua_State* _State, int _Index);

#endif
