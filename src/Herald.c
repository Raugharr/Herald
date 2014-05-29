/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Constraint.h"
#include "Person.h"
#include "sys/LinkedList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_Buildings;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeGroups;
struct Constraint** g_AgeConstraints;
struct Constraint** g_AgeDistr;
struct Constraint** g_BabyAvg;
struct MemoryPool* g_PersonPool;
//double g_BabyAvg[] = {3.125, 6.25, 12.5, 25.0, 50.0, 25.0, 12.5, 6.25, 3.125};

void HeraldInit() {
	int _Size = 0;

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
	g_AgeConstraints = CreateConstrntLst(&_Size, 0, 1068, 60);
	g_AgeDistr = CreateConstrntBnds(19, 0, 737, 1464, 2157, 2867, 3632, 4489, 5368, 6162, 6870, 7472, 7885, 8317, 8744, 9150, 9471, 9717, 9875, 9999);
	g_BabyAvg = CreateConstrntBnds(8, 0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999);
	g_PersonPool = CreateMemoryPool(sizeof(struct Person), 10000);
}

void HeraldDestroy() {
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeGroups);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_AgeDistr);
	DestroyConstrntBnds(g_BabyAvg);
	DestroyMemoryPool(g_PersonPool);
}

void LoadCSV(const char* _File, char*** _Array, int* _Size) {
	char _Char = 0;
	char _Buffer[256];
	char* _Name;
	int _Pos = 0;
	struct LinkedList _List;
	struct LnkLst_Node* _Itr = NULL;
	int i = 0;
	FILE* _FilePtr = fopen(_File, "r");

	_List.Size = 0;
	_List.Front = NULL;
	_List.Back = NULL;

	if(_FilePtr == NULL)
		return;

	while((_Char = fgetc(_FilePtr)) != EOF) {
		if(_Char == ',' && _Pos > 0) {
			_Name = (char*) malloc(sizeof(char) * _Pos + 1);
			strncpy(_Name, _Buffer, _Pos);
			LnkLst_PushBack(&_List, _Name);
			++(*_Size);
			_Pos = 0;
		} else {
			if(_Pos >= 256)
				return;
			_Buffer[_Pos++] = _Char;
		}
	}
	_Itr = _List.Front;
	*_Array = (char**) malloc(sizeof(char*) * *_Size);
	while(_Itr != NULL) {
		*_Array[i++] = (char*) _Itr->Data;
		_Itr = _Itr->Next;
	}
	fclose(_FilePtr);
}

void SetArray(void*** _Array, int _Size, void* _Value) {
	int i;
	for(i = 0; i < _Size; ++i)
		_Array[i] = NULL;
}
