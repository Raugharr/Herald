/*
 * File: Manor.c
 * Author: David Brotz
 */

#include "Manor.h"

#include "Herald.h"
#include "Random.h"
#include "Good.h"
#include "LinkedList.h"
#include "Building.h"
#include "HashTable.h"
#include "Person.h"
#include "Constraint.h"
#include "Family.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

struct ManorCrop {
	struct Crop* Crop;
	int Acres;
	/*
	 * m_FreeAcres represents the number of acres that couldn't be
	 * used because of a lack of seeds.
	 */
	int FreeAcres;
	int Seeds;
};

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
		_Population -= Family_Size(_Parent);;
	}
}

struct ManorCrop* CreateManorCrop(struct Crop* _Crop, int _Acres) {
	struct ManorCrop* _ManorCrop = (struct ManorCrop*) malloc(sizeof(struct ManorCrop));

	_ManorCrop->Crop = _Crop;
	_ManorCrop->Acres = _Acres;
	_ManorCrop->FreeAcres = 0;
	_ManorCrop->Seeds = 0;

	return _ManorCrop;
}

struct Manor* CreateManor(const char* _Name, int _Population) {
	struct Manor* _Manor = (struct Manor*) malloc(sizeof(struct Manor));

	Srand(time(NULL));
	_Manor->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Manor->Population = _Population;
	_Manor->Treasury = Random(2, 4) * _Population;
	_Manor->Families.Size = 0;
	_Manor->Families.Front = NULL;
	_Manor->Families.Back = NULL;
	PopulateManor(_Manor);
	strcpy(_Manor->Name, _Name);

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
	if(Hash_Find(&_Manor->Goods, _GoodName, _Good) == 0) {
		if(Hash_Find(&g_Goods, _GoodName, _Good) == 0)
			return 0;
		Hash_Insert(&_Manor->Goods, _GoodName, CopyGood(_Good));
	}
	_NewBuilding = CopyBuilding(_Building, _Good);
	_Output = _NewBuilding->OutputGood;
	if(Hash_Find(&_Manor->Production, _Output->Name, _List) == 0) {
		CreateLinkedList(_List);
		LnkLst_PushBack(_List, _NewBuilding);
		Hash_Insert(&_Manor->Production, _Output->Name, _List);
		return 1;
	}
	LnkLst_PushBack(_List, _NewBuilding);
	return 1;
}
