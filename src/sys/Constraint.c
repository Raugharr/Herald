/*
 * Author: David Brotz
 * File: Constraint.c
 */

#include "Constraint.h"

#include <stdlib.h>
#include <stdarg.h>

struct Constraint* CreateConstraint(int Min, int Max) {
	struct Constraint* Constraint = (struct Constraint*) malloc(sizeof(struct Constraint));

	Constraint->Min = Min;
	Constraint->Max = Max;
	return Constraint;
}

struct Constraint* CopyConstraint(struct Constraint* Constrnt) {
	return CreateConstraint(Constrnt->Min, Constrnt->Max);
}

struct Constraint** CopyConstraintBnds(struct Constraint** Constrnt) {
	int Size = ConstrntLen(Constrnt);
	struct Constraint** New = calloc(Size + 1, sizeof(struct Constraint*));
	int i;

	for(i = 0; i < Size; ++i)
		New[i] = CopyConstraint(Constrnt[i]);
	New[i] = NULL;
	return New;
}

void DestroyConstraint(struct Constraint* Constraint) {
	free(Constraint);
}

void DestroyConstrntBnds(struct Constraint** Constraint) {
	struct Constraint** Array = Constraint;

	if(Constraint == NULL)
		return;
	while((*Constraint) != NULL) {
		free(*Constraint);
		++Constraint;
	}
	free(Array);
}

struct Constraint** CreateConstrntLst(int* Size, int Min, int Max, int Interval) {
	int CurrMin = Min;
	int CurrMax = Min + Interval;
	int SizeCt = 0;
	struct Constraint** List = NULL;
	int i;

	SizeCt = (Max - Min) / Interval;
	if(Interval * SizeCt < Max)
		++SizeCt;
	List = (struct Constraint**) malloc(sizeof(struct Constraint) * (SizeCt + 1));
	for(i = 0; i < SizeCt; ++i) {
		if(CurrMax >= Max) {
			List[i] = CreateConstraint(CurrMin, Max);
			break;
		}
		List[i] = CreateConstraint(CurrMin, CurrMax - 1);
		CurrMin += Interval;
		CurrMax += Interval;
	}
	List[i + 1] = NULL;
	if(Size != NULL)
		*Size = SizeCt;
	return List;
}

struct Constraint** CreateConstrntBnds(int Size, ...) {
	va_list Valist;
	struct Constraint** List = NULL;

	va_start(Valist, Size);
	List = CreateConstrntVaBnds(Size + 1, Valist);
	List[Size] = NULL;
	va_end(Valist);
	return List;
}

struct Constraint** CreateConstrntVaBnds(int Size, va_list List) {
	int CurrMin = -1;
	int CurrMax = -1;
	int i;
	struct Constraint** Constrnt = (struct Constraint**) malloc(sizeof(struct Constraint) * (Size));

	--Size;
	CurrMin = va_arg(List, int);
	CurrMax = va_arg(List, int);
	for(i = 0; i < Size; ++i) {
		Constrnt[i] = CreateConstraint(CurrMin, CurrMax);
		CurrMin = CurrMax + 1;
		CurrMax = va_arg(List, int);
	}
	Constrnt[Size] = NULL;
	return Constrnt;
}

int Fuzify(struct Constraint* const * List, int Value) {
	for(int i = 0; *List != NULL; ++i, ++List)
		if((*List)->Min <= Value && (*List)->Max >= Value)
			return i;
	return -1;
}

int ConstrntLen(struct Constraint** List) {
	int i;
	int Size = 0;

	for(i = 0; List[i] != NULL; ++i)
		++Size;
	return Size;
}
