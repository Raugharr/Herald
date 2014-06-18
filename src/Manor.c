/*
 * File: Manor.c
 * Author: David Brotz
 */

#include "Manor.h"

#include "Herald.h"
#include "Good.h"
#include "Building.h"
#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/HashTable.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

int PersonISCallback(const int* _One, const int* _Two) {
	return (*_One) - (*_Two);
}

void PopulateManor(struct Manor* _Manor, int _Population) {
	int i;
	int _FamilySize = -1;
	struct Family* _Family = NULL;
	struct Family* _Parent = NULL;

	while(_Population > 0) {
		_FamilySize = Fuzify(g_FamilySize, Random(1, 100));
		_Parent = CreateRandFamily("Bar", Fuzify(g_BabyAvg, Random(0, 9999)) + 2);
		RBInsert(&g_Families, _Parent);
		while(_FamilySize-- > 0) {
			_Family = CreateRandFamily("Bar", Fuzify(g_BabyAvg, Random(0, 9999)) + 2);
			RBInsert(&g_Families, _Family);
			for(i = 0; i < CHILDREN + CHILDREN_SIZE; ++i) {
				if(_Family->People[i] != NULL)
					RBInsert(&_Manor->People, _Family->People[i]);
			}
			_FamilySize -= FamilySize(_Family);
		}
		_Population -= FamilySize(_Parent);
	}
}

struct Manor* CreateManor(const char* _Name, int _Population) {
	struct Manor* _Manor = (struct Manor*) malloc(sizeof(struct Manor));

	_Manor->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Manor->Acres = 0;
	_Manor->FreeAcres = 600;
	_Manor->People.Size = 0;
	_Manor->People.Table = NULL;
	_Manor->People.Size = 0;
	_Manor->People.ICallback = (int(*)(const void*, const void*))&PersonISCallback;
	_Manor->People.SCallback = (int(*)(const void*, const void*))&PersonISCallback;
	PopulateManor(_Manor, _Population);
	strcpy(_Manor->Name, _Name);

	_Manor->Goods.TblSize = 1024;
	_Manor->Goods.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * _Manor->Goods.TblSize);
	_Manor->Goods.Size = 0;
	memset(_Manor->Goods.Table, 0, _Manor->Goods.TblSize * sizeof(struct HashNode*));

	_Manor->Crops.Size = 0;
	_Manor->Crops.Front = NULL;
	_Manor->Crops.Back = NULL;

	_Manor->Animals.Size = 0;
	_Manor->Animals.Front = NULL;
	_Manor->Animals.Back = NULL;

	return _Manor;
}

void DestroyManor(struct Manor* _Manor) {
	struct LnkLst_Node* _Itr = _Manor->Crops.Front;

	RBRemoveAll(&_Manor->People, (void(*)(void*))DestroyPerson);
	while(_Itr != NULL) {
		DestroyField((struct Field*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Manor->Animals.Front;
	while(_Itr != NULL) {
		DestroyPopulation((struct Population*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	free(_Manor->Name);
	free(_Manor);
}

int AddBuilding(struct Manor* _Manor, const struct Building* _Building) {
	int i;
	const char* _GoodName = NULL;
	struct Good* _Good = NULL;
	struct Good* _Output = NULL;
	struct Building* _NewBuilding = NULL;
	struct LinkedList* _List = NULL;
	struct Array* _OutputGoods = NULL;

	if(_Building == NULL)
		return 0;
	_OutputGoods = _Building->OutputGoods;
	for(i = 0; i < _OutputGoods->Size; ++i) {
		_Output = ((struct Good*)_OutputGoods->Table[i]);
		_GoodName = _Output->Name;
		if((_Good = HashSearch(&_Manor->Goods, _GoodName)) == 0) {
			if((_Good = HashSearch(&g_Goods, _GoodName)) == 0)
				return 0;
			HashInsert(&_Manor->Goods, _GoodName, CopyGood(_Good));
		}
		if((_List = HashSearch(&_Manor->Production, _Output->Name)) == 0) {
			CreateLinkedList(_List);
			LnkLst_PushBack(_List, _NewBuilding);
			HashInsert(&_Manor->Production, _Output->Name, _List);
			return 1;
		}
		LnkLst_PushBack(_List, _NewBuilding);
	}
	_NewBuilding = CopyBuilding(_Building);
	return 1;
}

int ManorUpdate(struct Manor* _Manor) {
	RBIterate(&_Manor->People, (int(*)(void*))PersonUpdate);
	return 1;
}
