/*
 * File: ATimer.h
 * Author: David Brotz
 */

#ifndef __ATIMER_H
#define __ATIMER_H

#include "RBTree.h"

/*!
 * @Struct ATimer is designed to handle different structures that all are
 * based on doing repeatedly calling a function containing a struct. Each
 * struct that ATimer handles must contain at least five fields in a specific
 * order that is seen in the example below. The field Data must have its first
 * field be an integer who's value was obtained by calling NextId() that
 * is in the file Herald.h.
 * Example:
 * struct Countdown {
 * 	void* Prev;
 * 	void* Next;
 * 	int Type; //Is in the ATT_* Enumeration.
 * 	int StepsLeft;
 * 	struct Data* Data;
 * };
 *
 * struct Data {
 * 	int Id; //Must be same as Countdown Id and obtained by calling NextId() in Herald.h.
 * 	...
 * };
 */

struct Object;

enum {
	ATT_PREGANCY = 0,
	ATT_CONSTRUCTION,
	ATT_SIZE
};

struct ATypeProto {
	struct ATypeProto* Prev;
	struct ATypeProto* Next;
	int Type;
	void* Data;
};

struct ATType {
	int Type;
	int(*Callback)(void*);
	void(*Delete)(void*);
};

struct ATimer {
	struct RBTree* Tree;
	struct ATType** ATypes;
};

int NoThink(void* _Nothing);

int ATimerICallback(const struct ATypeProto* _One, const struct ATypeProto* _Two);
int ATimerSCallback(const struct Object* _One, const struct ATypeProto* _Two);

struct ATType* CreateATType(int _Type, int(*_Callback)(void*), void(*_Delete)(void*));
struct ATType* CopyATType(struct ATType* _Type);
void DestroyATType(struct ATType* _Type);

/*!
 * @Note ATImer will free _Tree when DeleteATimer is called.
 */
struct ATimer* CreateATimer();
struct ATimer* CopyATimer(struct ATimer* _Timer);
void DestroyATimer(struct ATimer* _Timer);

void ATimerAddType(struct ATimer* _Timer, struct ATType* _Type);
void ATImerUpdate(struct ATimer* _Timer);

void* ATimerSearch(struct ATimer* _Timer, struct Object* _Obj, int _Type);
void ATimerInsert(struct ATimer* _Timer, void* _Data);
void ATimerRm(struct ATimer* _Timer, void* _Data);
void ATimerRmNode(struct ATimer* _Timer, void* _Data);
void ATTimerRmAll(struct ATimer* _Timer);

#endif
