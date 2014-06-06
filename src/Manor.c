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

void PopulateManor(struct Manor* _Manor) {
	int _Population = _Manor->Population;
	int _FamilySize = -1;
	struct Family* _Family = NULL;
	struct Family* _Parent = NULL;

	while(_Population > 0) {
		_FamilySize = Fuzify(g_FamilySize, Random(1, 100));
		_Parent = CreateRandFamily("Bar", Fuzify(g_BabyAvg, Random(0, 9999)) + 2);
		while(_FamilySize-- > 0) {
			_Family = CreateRandFamily("Bar", Fuzify(g_BabyAvg, Random(0, 9999)) + 2);
			LnkLst_PushBack(&_Manor->Families, _Family);
			_FamilySize -= Family_Size(_Family);
		}
		_Population -= Family_Size(_Parent);
	}
}

struct Manor* CreateManor(const char* _Name, int _Population) {
	struct Manor* _Manor = (struct Manor*) malloc(sizeof(struct Manor));

	_Manor->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Manor->Population = _Population;
	_Manor->Acres = 600;
	_Manor->FreeAcres = 0;
	_Manor->Treasury = Random(2, 4) * _Population;
	_Manor->Income = 0;
	_Manor->Families.Size = 0;
	_Manor->Families.Front = NULL;
	_Manor->Families.Back = NULL;
	PopulateManor(_Manor);
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

#ifdef DEBUG
	struct Crop* _Crop = NULL;
	struct Population* _CowInfo = NULL;
	struct Good* _Wheat = NULL;
	struct Building* _Building = NULL;

	Hash_Find(&g_Crops, "Wheat", _Crop);
	Hash_Find(&g_Crops, "Cow", _CowInfo);
	LnkLst_PushBack(&_Manor->Crops, CreateManorCrop(_Crop, 400));
	LnkLst_PushBack(&_Manor->Animals, CopyPopulation(_CowInfo, 100));
	_Manor->Acres = 400;
	Hash_Find(&g_Goods, "Wheat", _Wheat);
	_Wheat = CopyGood(_Wheat);
	_Wheat->Quantity = _Manor->PopCenter.AdultFood * _Population * 5;
	Hash_Insert(&_Manor->Goods, "Wheat", _Wheat);

	Hash_Find(&g_Buildings, "Edge Mill", _Building);
	AddBuilding(_Manor, _Building);

	Hash_Find(&g_Buildings, "House", _Building);
#endif
	return _Manor;
}

void DestroyManor(struct Manor* _Manor) {
	struct LnkLst_Node* _Itr = _Manor->Families.Front;

	while(_Itr != NULL) {
		DestroyFamily(_Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Manor->Crops.Front;
	while(_Itr != NULL) {
		DestroyCrop(_Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Manor->Animals.Front;
	while(_Itr != NULL) {
		DestroyPopulation(_Itr->Data);
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

int Manor_Update(struct Manor* _Manor) {
	struct LnkLst_Node* _Itr = _Manor->Families.Front;

	while(_Itr != NULL) {
		Family_Update(_Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Manor->Crops.Front;
	while(_Itr != NULL) {
		struct Crop* _Crop = (struct Crop*)_Itr->Data;
		struct Good* _Good = NULL;
		char _SeedName[strlen(_Crop->Name) + strlen(CROPGOOD) + 1];

		Crop_Update(_Itr->Data);
		Hash_Find(&_Manor->Goods, _SeedName, (void**)&_Good);
		if(_Crop->Status == EFALLOW) {
			if(_Good == NULL)
				continue;
			CropPlant(_Itr->Data, _Good);
		} else if(_Crop->Status == EHARVESTING) {
			struct Good* _CropSeed = NULL;

			if(Hash_Find(&g_Goods, _SeedName, (void**)&_CropSeed) == 0)
				return 0;
			_Good = CopyGood(_CropSeed);
			if(_Good == NULL)
				Hash_Insert(&_Manor->Goods, _SeedName, _Good);
			CropHarvest(_Crop, _Good);
		}
		_Itr = _Itr->Next;
	}
	return 1;
}
