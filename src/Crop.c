/*
 * File: Crop.c
 * Author: David Brotz
 */

#include "Crop.h"

#include "Good.h"
#include "Herald.h"
#include "World.h"
#include "sys/RBTree.h"

#include <stdlib.h>
#include <string.h>

#define WORKMULT (1000)

#define NextStatus(__Crop)				\
{										\
	if(__Crop->Status == EHARVESTING) {	\
		__Crop->Status = EFALLOW;		\
	} else {							\
		++__Crop->Status;				\
	}									\
}

struct Crop* CreateCrop(const char* _Name, int _PerAcre, int _NutVal, double _YieldMult, int _GrowDays) {
	struct Crop* _Crop = (struct Crop*) malloc(sizeof(struct Crop));
	char* _GoodName = (char*) malloc(sizeof(char*) * strlen(_Name) + strlen(CROPGOOD) + 1);
	struct Good* _Good = NULL;

	_Crop->Name = _Name;
	_Crop->PerAcre = _PerAcre;
	_Crop->NutVal = _NutVal;
	_Crop->YieldMult = _YieldMult;
	_Crop->GrowDays = _GrowDays;
	_Crop->YieldTotal = 0;
	_Crop->Acres = 0;
	_Crop->Status = EFALLOW;
	strcpy(_GoodName, _Name);
	strcat(_GoodName, CROPGOOD);
	RBInsert(&g_Strings, _GoodName);
	if(Hash_Find(&g_Goods, _GoodName, (void**)&_Good))
		DestroyGood(_Good);
	if(Hash_Find(&g_Goods, _Name, (void**)&_Good))
		DestroyGood(_Good);

	Hash_Insert(&g_Goods, _GoodName, CreateGood(_GoodName, ESEED));
	Hash_Insert(&g_Goods, _Name, CreateGood(_Name, EINGREDIENT));
	_Crop->Name = _Name;
	return _Crop;
}

struct Crop* CopyCrop(const struct Crop* _Crop) {
	struct Crop* _NewCrop = (struct Crop*) malloc(sizeof(struct Crop));

	_NewCrop->Name = _Crop->Name;
	_NewCrop->PerAcre = _Crop->PerAcre;
	_NewCrop->NutVal = _Crop->NutVal;
	_NewCrop->YieldMult = _Crop->YieldMult;
	_NewCrop->GrowDays = _Crop->GrowDays;
	_NewCrop->YieldTotal = _Crop->YieldTotal;
	_NewCrop->Acres = _Crop->Acres;
	_NewCrop->Status = _Crop->Status;
	return _NewCrop;
}

void DestroyCrop(struct Crop* _Crop) {
	free(_Crop);
}

int CropPlant(struct Crop* _Crop, struct Good* _Seeds) {
	char _SeedName[strlen(_Crop->Name) + strlen(CROPGOOD) + 1];

	if(_Crop->Status != EFALLOW)
		return 0;

	strcpy(_SeedName, _Crop->Name);
	strcat(_SeedName, CROPGOOD);
	if(strcmp(_Seeds->Name, _SeedName) != 0)
		return 0;
	if(_Seeds->Quantity < _Crop->PerAcre * _Crop->Acres)
		return 0;
	_Seeds->Quantity -= _Crop->PerAcre * _Crop->Acres;
	_Crop->Status = EPLOWING;
	_Crop->StatusTime = _Crop->Acres * WORKMULT;
	return 1;
}

void CropWork(struct Crop* _Crop, int _Total) {
	int _Val = _Total;

	switch(_Crop->Status) {
		case EGROWING:
			_Crop->YieldTotal += _Val / _Crop->GrowDays;
			break;
		case EFALLOW:
			return;
		default:
			_Crop->StatusTime -= _Total * WORKMULT;
			if(_Crop->StatusTime <= 0)
				NextStatus(_Crop);
			break;
	}
}

void CropHarvest(struct Crop* _Crop, struct Good* _Seeds) {
	char _SeedName[strlen(_Crop->Name) + strlen(CROPGOOD) + 1];

	strcpy(_SeedName, _Crop->Name);
	strcat(_SeedName, CROPGOOD);
	if(strcmp(_Seeds->Name, _SeedName) != 0)
		return;
	if(_Crop->Status != EHARVESTING)
		return;
	_Crop->Acres = _Crop->Acres - (_Crop->Acres -_Crop->StatusTime);
	_Seeds->Quantity = TOPOUND(_Crop->Acres * _Crop->PerAcre / WORKMULT);
}

int Crop_Update(struct Crop* _Crop) {
	if(_Crop->Status == EGROWING)
		--_Crop->StatusTime;
	return 1;
}
