/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "Government.h"
#include "BigGuy.h"
#include "World.h"
#include "Good.h"
#include "Population.h"
#include "Mission.h"
#include "Warband.h"
#include "Battle.h"
#include "Events.h"

#include "AI/BehaviorTree.h"
#include "AI/Pathfind.h"

#include "sys/Stack.h"
#include "sys/RBTree.h"
#include "sys/Math.h"
#include "sys/LinkedList.h"
#include "sys/TaskPool.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"
#include "sys/LuaCore.h"
#include "sys/Constraint.h"
#include "sys/Log.h"
#include "sys/Rule.h"
#include "sys/Event.h"
#include "sys/GenIterator.h"

#include "video/Animation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define AGEDIST_SIZE (17)
#define STACKALLOC_SZ (2048)

struct LinkedList g_GoodCats[GOOD_SIZE];
struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_Profs;
struct HashTable g_BuildMats;
struct HashTable g_Populations;
struct HashTable g_Animations;
struct HashTable g_Traits;
struct HashTable g_Professions;
struct HashTable g_BhvVars;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeConstraints;

struct ObjectList {
	struct RBTree SearchTree;
	struct {
		struct Object* Front;
		ObjectThink ObjThink; //Think that is called once for every object class.
		ObjectThink Think; //Think called for every object of this type.
		size_t Size;
		uint32_t StartCt;
		uint32_t ExtendCt;
		struct MemoryPool Pool;
		struct Object* DeadPool; //List of objects that need to be cleaned up from the last tick.
		void (*OnDestroy)(void*);
	} ObjectList[OBJECT_SIZE];
};

struct MissionEngine g_MissionEngine;

//TODO: SearchTree does nothing but be inserted to and removed from. ObjectList should be removed and have a LinkedList for storing thinks replace it.`
static struct ObjectList g_Objects = {
	{NULL, 0, (int(*)(const void*, const void*))ObjectCmp, (int(*)(const void*, const void*))ObjectCmp},
	{{NULL, PersonObjThink, PersonThink, sizeof(struct Person), 200000, 100000, {0}, NULL, (void(*)(void*))DestroyPerson},
	{NULL, NULL, (ObjectThink) FieldUpdate, sizeof(struct Field), 35000, 5000, {0}, NULL, (void(*)(void*))DestroyField},
	{NULL, NULL, (ObjectThink) ArmyThink, sizeof(struct Army), 1000, 500,{0}, NULL, (void(*)(void*))DestroyArmy},
	{NULL, NULL, NULL, sizeof(struct Battle), 1000, 500, {0}, NULL, (void(*)(void*))DestroyBattle},
	{NULL, NULL, NULL, sizeof(struct Pregnancy), 50000, 1000, {0}, NULL, (void(*)(void*))DestroyPregnancy},
	{NULL, SettlementObjThink, (ObjectThink) SettlementThink, sizeof(struct Settlement), 20000, 1000, {0}, NULL, (void(*)(void*))DestroySettlement},
	{NULL, NULL, (ObjectThink) BigGuyThink, sizeof(struct BigGuy), 10000, 1000, {0}, NULL, (void(*)(void*))DestroyBigGuy},
	{NULL, FamilyObjThink, FamilyThink, sizeof(struct Family), 35000, 5000, {0}, NULL, (void(*)(void*))DestroyFamily},
	{NULL, NULL, (ObjectThink) GovernmentThink, sizeof(struct Government), 20000, 1000, {0}, NULL, (void(*)(void*))DestroyGovernment},
	{NULL, NULL, (ObjectThink) RetinueThink, sizeof(struct Retinue), 20000, 1000, {0}, NULL, (void(*)(void*))DestroyRetinue}
	}
};

static uint32_t g_Id = 0;
struct Constraint** g_OpinionMods = NULL;

int IdISCallback(const int* One, const int* Two) {
	return *(One) - *(Two);
}

void SigSev(int Sig) {
	void* Array[16];
	size_t Size = 0;
	char** Symbols = NULL;

	Size = backtrace(Array, 16);
	Symbols = backtrace_symbols(Array, Size);
	for(int i = 2; i < Size; ++i) {
		Log(ELOG_ERROR, Symbols[i]);
		//free(Symbols[i]);
	}
	free(Symbols);
}

int HeraldInit() {
	signal(SIGSEGV, SigSev);
	signal(SIGINT, SigSev);
	/*FIXME: Is g_Crops, g_GOods, g_BuildMats, and g_Populations not free their memory when HeraldDestroy is called or at al?
	 */
	CtorHashTable(&g_Crops, 16);
	CtorHashTable(&g_Goods, 16);
	CtorHashTable(&g_BuildMats, 16);
	CtorHashTable(&g_Populations, 16);
	CtorHashTable(&g_Animations, 16);
	CtorHashTable(&g_Professions, 16);

	g_BhvVars.TblSize = 0;
	g_BhvVars.Table = calloc(sizeof(void*), 64);
	g_BhvVars.Size = 64;
	InitTaskPool();
	for(int i = 0; i < OBJECT_SIZE; ++i) {
		CtorMemoryPool(&g_Objects.ObjectList[i].Pool, g_Objects.ObjectList[i].Size, g_Objects.ObjectList[i].StartCt); 
	}
	g_FamilySize = CreateConstrntBnds(FAMILYSIZE, 2, 10, 20, 40, 75, 100);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	g_OpinionMods = CreateConstrntBnds(5, -REL_MAX, -76, -26, 25, 75, REL_MAX);
	EventInit(g_LuaState);
	PathfindInit();
	MathInit();
	
	EventSetCallback(EVENT_WARBNDHOME, EventWarbandHome);
	EventSetCallback(EVENT_CRISIS, EventCrisis);
	EventSetCallback(EVENT_ENDPLOT, EventEndPlot);
	EventSetCallback(EVENT_BATTLE, EventBattle);
	EventSetCallback(EVENT_NEWLEADER, EventNewLeader);
	EventSetCallback(EVENT_JOINRETINUE, EventJoinRetinue);

	ConstructMissionEngine(&g_MissionEngine);
	return 1;
}

void HeraldDestroy() {
	struct HashItr* Itr = HashCreateItr(&g_Animations);

	while(Itr != NULL) {
		DestroyAnimation(Itr->Node->Pair);
		Itr = HashNext(&g_Animations, Itr);
	}
	HashDeleteItr(Itr);
	free(g_BhvVars.Table);
	QuitTaskPool();
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_OpinionMods);
	EventQuit();
	PathfindQuit();
	DestroyMissionEngine(&g_MissionEngine);
}

void ClearObjects() {
	for(int i = 0; i < OBJECT_SIZE; ++i) {
		while(g_Objects.ObjectList[i].Front != NULL) 
			DestroyObject(g_Objects.ObjectList[i].Front);
	}
}

struct InputReq* CreateInputReq() {
	struct InputReq* Mat = (struct InputReq*) malloc(sizeof(struct InputReq));

	Mat->Req = NULL;
	Mat->Quantity = 0;
	return Mat;
}

void DestroyInputReq(struct InputReq* Mat) {
	free(Mat);
}

int InputReqQtyCmp(const void* One, const void* Two) {
	return (int) ((struct InputReq*)One)->Quantity - ((struct InputReq*)Two)->Quantity;
}

int InputReqCropCmp(const void* One, const void* Two) {
	return (int) ((struct Crop*)((struct InputReq*)One)->Req)->Id - ((struct Crop*)((struct InputReq*)Two)->Req)->Id;
}

struct Array* FileLoad(const char* File, char Delimiter) {
	int Pos = 0;
	int Size = 1;
	char* Name = NULL;
	char Char = 0;
	char Buffer[256];
	FILE* FilePtr = fopen(File, "r");
	struct Array* Array = NULL;

	if(FilePtr == NULL)
		return NULL;

	while((Char = fgetc(FilePtr)) != EOF) {
		if(Char == Delimiter)
			++Size;
	}
	rewind(FilePtr);
	Array = CreateArray(Size);
	while((Char = fgetc(FilePtr)) != EOF) {
			if(Char == Delimiter && Pos > 0) {
				Buffer[Pos] = 0;
				Name = (char*) malloc(sizeof(char) * Pos + 1);
				Name[0] = 0;
				strncat(Name, Buffer, Pos);
				ArrayInsert(Array, Name);
				Pos = 0;
			} else {
				if(Pos >= 256)
					return Array;
				Buffer[Pos++] = Char;
			}
		}
	return Array;
}

struct Array* ListToArray(const struct LinkedList* List) {
	struct Array* Array = NULL;
	struct LnkLst_Node* Itr = List->Front;

	if(List->Size < 1)
		return NULL;
	Array = CreateArray(List->Size);
	while(Itr != NULL) {
		ArrayInsert(Array, Itr->Data);
		Itr = Itr->Next;
	}
	return Array;
}

//FIXME: We should be able to easily calculate the size of the outgoing table, there should be no need to allocate memory here.
void* PowerSet_Aux(void* Tbl, int Size, int ArraySize, uint32_t* BrSz, struct StackNode* Stack) {
	struct StackNode Node;
	void** Return = NULL;
	int i;

	if(Size == 0) {
		Return = calloc(ArraySize + 1, sizeof(void*));
		for(i = ArraySize - 1; i >= 0; --i) {
			Return[i] = Stack->Data;
			Stack = Stack->Prev;
		}
		Return[ArraySize] = NULL;
		*BrSz = ArraySize + 1;
	} else {
		uint32_t LLen = 0;
		uint32_t  RLen = 0;
		void** Left = NULL;
		void** Right = NULL;

		Node.Prev = Stack;
		Node.Data = (void*)*(intptr_t*)Tbl;
		Left = PowerSet_Aux(Tbl + sizeof(void*), Size - 1, ArraySize, &LLen, Stack);
		Right = PowerSet_Aux(Tbl + sizeof(void*), Size - 1, ArraySize + 1, &RLen, &Node);

		//Len = (1 << (Size - 1));
		Return = calloc(LLen + RLen + 1, sizeof(void*));
		for(i = 0; i < LLen; ++i) {
			Return[i] = Left[i];
		}
		for(i = 0; i < RLen; ++i)
			Return[i + LLen] = Right[i];
		Return[LLen + RLen] = NULL;
		if(BrSz != NULL) *BrSz = LLen + RLen + 1;
	}
	//end:
	return Return;
}

/* Use a single array to store the power set.
 * To distinguish the sets and to allow them to be easy to use add an extra Size to the front of the table such that
 * given {a, b, c} we return the array 
 * {addr(8), addr(9), addr(11), addr(13), addr(15), addr(18), addr(21), addr(24) 
 	NULL,
	a, NULL,
	b, NULL,
	c NULL,
	a, b, NULL,
	a c, NULL,
	b c, NULL,
	a, b, c, NULL}
	where addr(x) is the address of the xth elemenet of the array.
*/
/*void* PowerSet_Aux(void* Tbl, int Size, int ArraySize, struct StackNode* Stack) {
	void* Array = calloc(Size << 1, sizeof(void*));
	uint64_t Mask = 1;
	uint32_t Set = 1;

	Array[0] = NULL;
	while(Mask < (Size << 1)) {
		for(int i = 0; i < Set; ++i) {
			if(((i << 0) & Set) != (i << 0))
				continue;
		}
	}
}*/

void* CreateObject(uint8_t Type) {
	struct Object* Obj = MemPoolAlloc(&g_Objects.ObjectList[Type].Pool);

	memset(Obj, 0, g_Objects.ObjectList[Type].Size); 
	*(uint32_t*)&Obj->Id = NextId();
	Obj->Type = Type;
	Obj->Flags = 0;
	RBInsert(&g_Objects.SearchTree, Obj);
	Assert(RBSearch(&g_Objects.SearchTree, Obj));
	ILL_CREATE(g_Objects.ObjectList[Type].Front, Obj);
	return Obj;
}

void DestroyObject(struct Object* Obj) {
	RBDelete(&g_Objects.SearchTree, Obj);
	//Destroy is called here and in ObjectDie to ensure the object is removed even if ObjectDie is not called.
	if(ObjectAlive(Obj)) {
		ILL_DESTROY(g_Objects.ObjectList[Obj->Type].Front, Obj);
	} else {
		ILL_DESTROY(g_Objects.ObjectList[Obj->Type].DeadPool, Obj);
	}
	MemPoolFree(&g_Objects.ObjectList[Obj->Type].Pool, Obj);
}

void ObjectsThink() {
	for(int i = 0; i < OBJECT_SIZE; ++i) {
		struct Object* NextObj = NULL;

		for(struct Object* Obj = g_Objects.ObjectList[i].DeadPool; Obj != NULL; Obj = NextObj) {
			NextObj = Obj->Next;
			g_Objects.ObjectList[i].OnDestroy(Obj);
		}
		g_Objects.ObjectList[i].DeadPool = NULL;
	}
	for(int i = 0; i < OBJECT_SIZE; ++i) {
		if(g_Objects.ObjectList[i].ObjThink != NULL) {
			g_Objects.ObjectList[i].ObjThink(g_Objects.ObjectList[i].Front);
		}
		struct Object* Obj = g_Objects.ObjectList[i].Front;
		struct Object* Next = NULL;

		while(Obj != NULL) {
			Next = Obj->Next;
			g_Objects.ObjectList[i].Think(Obj);
			Obj = Next;
		}
	}
}

int NextId() {return g_Id++;}

void BhvHashFree(char* Str) {
	free(Str);
}

void BehaviorRun(const struct Behavior* Tree, struct Family* Family) {
	HashDeleteAll(&g_BhvVars, (void(*)(void*)) BhvHashFree);
	BHVRun(Tree, Family, &g_BhvVars);
}

int ObjectCmp(const void* One, const void* Two) {
	return *((int*)One) - *((int*)Two);
}

struct Object* FindObject(ObjectId Id) {
	return RBSearch(&g_Objects.SearchTree, &Id);
}

void ObjectDie(struct Object* Obj) {
	Obj->Flags |= OBJFLAG_DEAD;
	ILL_DESTROY(g_Objects.ObjectList[Obj->Type].Front, Obj);
	ILL_CREATE(g_Objects.ObjectList[Obj->Type].DeadPool, Obj);
}
/*

void* PowerSet_Aux(void* Tbl, int Size, int ArraySize, struct StackNode* Stack) {
	struct StackNode Node;
	void** Return = NULL;
	int i;

	if(Size == 0) {
		//No Elements exist in Tbl return set containing NULL.
		if(Stack == NULL) {
			Return = malloc(sizeof(void*));
			*((int**)Return) = 0;
			goto end;
		}
		Return = calloc(ArraySize + 1, sizeof(void*));
		for(i = ArraySize - 1; i >= 0; --i) {
			Return[i] = Stack->Data;
			Stack = Stack->Prev;
		}
		Return[ArraySize] = NULL;
	} else {
		int Len = 0;
		void** Left = NULL;
		void** Right = NULL;

		Node.Prev = Stack;
		Node.Data = (void*)*(int*)Tbl;
		Left = PowerSet_Aux(Tbl + sizeof(void*), Size - 1, ArraySize, Stack);
		Right = PowerSet_Aux(Tbl + sizeof(void*), Size - 1, ArraySize + 1, &Node);

		if(Size == 1) {
			Return = calloc(3, sizeof(void*));
			Return[0] = Left;
			Return[1] = Right;
			Return[2] = NULL;
			goto end;
		}
		Len = ArrayLen(Left);
		Return = calloc(Len * 2 + 1, sizeof(void*));
		for(i = 0; i < Len; ++i) {
			Return[i] = Left[i];
		}
		for(i = 0; i < Len; ++i)
			Return[i + Len] = Right[i];
		Return[Len * 2] = NULL;
	}
	end:
	return Return;
}*/
