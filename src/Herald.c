/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Constraint.h"
#include "Person.h"
#include "Manor.h"
#include "Family.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AGEDIST_SIZE (17)
#define MANORSZ_MIN (50)
#define MANORSZ_INTRVL (50)
#define MANORSZ_MAX (800)

int g_Date = 0;
struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_Buildings;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeGroups;
struct Constraint** g_AgeConstraints;
struct Constraint** g_BabyAvg;
struct Constraint** g_ManorSize;
struct LinkedList* g_ManorList;

char g_DataFld[] = "Data/";

void HeraldInit() {
	g_Crops.TblSize = 512;
	g_Crops.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Crops.TblSize);
	SetArray((void***)&g_Crops.Table, g_Crops.TblSize, NULL);
	g_Goods.TblSize = 512;
	g_Goods.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Goods.TblSize);
	SetArray((void***)&g_Goods.Table, g_Goods.TblSize, NULL);
	g_Buildings.TblSize = 512;
	g_Buildings.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Buildings.TblSize);
	SetArray((void***)&g_Buildings.Table, g_Buildings.TblSize, NULL);

	g_FamilySize = CreateConstrntBnds(5, 1, 5, 15, 40, 75, 100);
	g_AgeGroups = CreateConstrntBnds(5, 0, 71, 155, 191, 719, 1200);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	g_BabyAvg = CreateConstrntBnds(8, 0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999);
	g_ManorSize = CreateConstrntLst(NULL, MANORSZ_MIN, MANORSZ_MAX, MANORSZ_INTRVL);
	
	World_Init();
}

void HeraldDestroy() {
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeGroups);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_BabyAvg);
	//DestroyConstrntBnds(g_ManorSize);

	World_Quit();
}

struct Array* LoadFile(const char* _File, char _Delimiter) {
	int _Pos = 0;
	int _Size = 1;
	char* _Name = NULL;
	char _Char = 0;
	char _Buffer[256];
	FILE* _FilePtr = fopen(_File, "r");
	struct Array* _Array = NULL;

	if(_FilePtr == NULL)
		return NULL;

	while((_Char = fgetc(_FilePtr)) != EOF) {
		if(_Char == _Delimiter)
			++_Size;
	}
	rewind(_FilePtr);
	_Array = CreateArray(_Size);
	while((_Char = fgetc(_FilePtr)) != EOF) {
			if(_Char == _Delimiter && _Pos > 0) {
				_Buffer[_Pos] = 0;
				_Name = (char*) malloc(sizeof(char) * _Pos + 1);
				_Name[0] = 0;
				strncat(_Name, _Buffer, _Pos);
				Array_Insert(_Array, _Name);
				_Pos = 0;
			} else {
				if(_Pos >= 256)
					return _Array;
				_Buffer[_Pos++] = _Char;
			}
		}
	return _Array;
}

void World_Init() {
	g_ManorList = (struct LinkedList*) CreateLinkedList();
	struct Array* _Array = NULL;

	chdir(g_DataFld);
	_Array = LoadFile("FirstNames.txt", '\n');
	Person_Init();
	Family_Init(_Array);

	LnkLst_PushBack(g_ManorList, CreateManor("Test", (Fuzify(g_ManorSize, Random(MANORSZ_MIN, MANORSZ_MAX)) * MANORSZ_INTRVL) + MANORSZ_INTRVL));

	Person_Quit();
	Family_Quit();
}

void World_Quit() {
	struct LnkLst_Node* _Itr = g_ManorList->Front;

	while(_Itr != NULL) {
		DestroyManor(_Itr->Data);
		_Itr = _Itr->Next;
	}
	DestroyLinkedList(g_ManorList);
}
