/*
 * Author: David Brotz
 * File: Constraint.h
 */

#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include <stdarg.h>

struct Constraint {
	int Min;
	int Max;
};

struct Constraint* CreateConstraint(int _Min, int _Max);
void DestroyConstraint(struct Constraint* _Constraint);
void DestroyConstrntBnds(struct Constraint** _Constraint);
//! _Size will contain how many Constraint* that are made.
struct Constraint** CreateConstrntLst(int* _Size, int _Min, int _Max, int _Interval);
struct Constraint** CreateConstrntBnds(int _Size, ...);
struct Constraint** CreateConstrntVaBnds(int _Size, va_list _List);
int Fuzify(struct Constraint** _List, int _Value);

#endif

