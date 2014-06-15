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
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/HashTable.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

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
					LnkLst_PushBack(&_Manor->People, _Family->People[i]);
			}
			_FamilySize -= Family_Size(_Family);
		}
		_Population -= Family_Size(_Parent);
	}
}

struct Manor* CreateManor(const char* _Name, int _Population) {
	struct Manor* _Manor = (struct Manor*) malloc(sizeof(struct Manor));
	struct Field* _Field = NULL;
	struct Good* _Wheat = NULL;

	_Manor->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Manor->Acres = 0;
	_Manor->FreeAcres = 600;
	_Manor->People.Size = 0;
	_Manor->People.Front = NULL;
	_Manor->People.Back = NULL;
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


	_Field = CreateField();
	Hash_Find(&g_Crops, "Wheat", (void**)&(_Field->Crop));
	FieldReset(_Field);
	LnkLst_PushBack(&_Manor->Crops, _Field);
	Hash_Find(&g_Goods, "Wheat Seeds", (void**)&_Wheat);
	_Wheat = CopyGood(_Wheat);
	_Wheat->Quantity = _Field->Crop->PerAcre * _Manor->FreeAcres;
	Hash_Insert(&_Manor->Goods, "Wheat Seeds", _Wheat);
	_Field->Acres = _Manor->FreeAcres;
	_Manor->Acres = _Manor->FreeAcres;
	_Manor->FreeAcres = 0;
	return _Manor;
}

void DestroyManor(struct Manor* _Manor) {
	struct LnkLst_Node* _Itr = _Manor->People.Front;

	while(_Itr != NULL) {
		DestroyPerson((struct Person*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Manor->Crops.Front;
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
	const char* _GoodName = NULL;
	struct Good* _Good = NULL;
	struct Good* _Output = NULL;
	struct Building* _NewBuilding = NULL;
	struct LinkedList* _List = NULL;

	if(_Building == NULL)
		return 0;
	_GoodName = _Building->OutputGood->Name;
	if(Hash_Find(&_Manor->Goods, _GoodName, (void**)&_Good) == 0) {
		if(Hash_Find(&g_Goods, _GoodName, (void**)&_Good) == 0)
			return 0;
		Hash_Insert(&_Manor->Goods, _GoodName, CopyGood(_Good));
	}
	_NewBuilding = CopyBuilding(_Building, _Good);
	_Output = _NewBuilding->OutputGood;
	if(Hash_Find(&_Manor->Production, _Output->Name, (void**)&_List) == 0) {
		CreateLinkedList(_List);
		LnkLst_PushBack(_List, _NewBuilding);
		Hash_Insert(&_Manor->Production, _Output->Name, _List);
		return 1;
	}
	LnkLst_PushBack(_List, _NewBuilding);
	return 1;
}

int ManorUpdate(struct Manor* _Manor) {
	struct LnkLst_Node* _Itr = _Manor->People.Front;
	int _Population = 0;

	while(_Itr != NULL) {
		PersonUpdate((struct Person*)_Itr->Data, 1500);
		_Population += Family_Size(_Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Manor->Crops.Front;
	while(_Itr != NULL) {
		struct Field* _Field = (struct Field*)_Itr->Data;
		struct Good* _Good = NULL;
		char _SeedName[strlen(_Field->Crop->Name) + strlen(CROPGOOD) + 1];

		FieldUpdate(_Field);
		strcpy(_SeedName, _Field->Crop->Name);
		strcat(_SeedName, CROPGOOD);
		Hash_Find(&_Manor->Goods, _SeedName, (void**)&_Good);
		if(_Field->Status == EFALLOW) {
			if(_Good == NULL) {
				_Itr = _Itr->Next;
				continue;
			}
			FieldPlant(_Field, _Good);
		} else if(_Field->Status == EHARVESTING) {
			struct Good* _CropSeed = NULL;

			if(Hash_Find(&g_Goods, _SeedName, (void**)&_CropSeed) == 0)
				return 0;
			_Good = CopyGood(_CropSeed);
			if(_Good == NULL)
				Hash_Insert(&_Manor->Goods, _SeedName, _Good);
			FieldHarvest(_Field, _Good);
		}
		_Itr = _Itr->Next;
	}
	return 1;
}
