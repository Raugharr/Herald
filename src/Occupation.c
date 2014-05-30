/*
 * File: Occupation.c
 * Author: David Brotz
 */

#include "Occupation.h"

#include <stdlib.h>

static int g_Id = 0;

struct Occupation* CreateOccupation(const char* _Name, struct Good* _Output, struct Building* _Workplace) {
	struct Occupation* 	_Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = ++g_Id;
	_Occupation->Name = _Name;
	_Occupation->Job.Workplace->Output = _Output;
	_Occupation->Job.Workplace->Workplace = _Workplace;
	_Occupation->SpecialJob = 0;
	return 	_Occupation;
}
struct Occupation* CopyOccupation(const struct Occupation* _Job) {
	struct Occupation* 	_Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = _Job->Id;
	_Occupation->Name = _Job->Name;
	if(_Job->SpecialJob > 0)
		_Occupation->Job.Occupation = _Job->Job.Occupation;
	else
		_Occupation->Job.Workplace = _Job->Job.Workplace;
	_Occupation->SpecialJob = _Job->SpecialJob;
	_Occupation->AgeConst = _Job->AgeConst;
	return 	_Occupation;
}
void DestroyJob(struct Occupation* _Occupation) {
	free(_Occupation);
}

