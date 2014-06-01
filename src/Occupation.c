/*
 * File: Occupation.c
 * Author: David Brotz
 */

#include "Occupation.h"

#include <stdlib.h>

static int g_Id = 0;

struct Occupation* CreateOccupation(const char* _Name, struct Good* _Output, struct Building* _Workplace, struct Constraint* _AgeConst) {
	struct Occupation* _Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = ++g_Id;
	_Occupation->Name = _Name;
	_Occupation->Job.Output = _Output;
	_Occupation->Job.Workplace = _Workplace;
	_Occupation->SpecialJob = ENONE;
	_Occupation->AgeConst = _AgeConst;
	return 	_Occupation;
}

struct Occupation* CreateOccupationSpecial(const char* _Name, int _Job) {
	struct Occupation* _Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = ++g_Id;
	_Occupation->Name = _Name;
	_Occupation->SpecialJob = _Job;
	return _Occupation;
}

struct Occupation* CopyOccupation(const struct Occupation* _Job) {
	struct Occupation* 	_Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = _Job->Id;
	_Occupation->Name = _Job->Name;
	_Occupation->SpecialJob = _Job->SpecialJob;
	if(_Job->SpecialJob == ENONE)
		_Occupation->Job.Workplace = _Job->Job.Workplace;
	_Occupation->AgeConst = _Job->AgeConst;
	return 	_Occupation;
}
void DestroyJob(struct Occupation* _Occupation) {
	free(_Occupation);
}

