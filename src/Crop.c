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
	strcpy(_GoodName, _Name);
	strcat(_GoodName, CROPGOOD);
	RBInsert(&g_Strings, _GoodName);
	if(Hash_Find(&g_Goods, _GoodName, (void**)&_Good))
		DestroyGood(_Good);

	Hash_Insert(&g_Goods, _GoodName, _Good = CreateGood(_GoodName, ESEED));
	Hash_Insert(&g_Goods, _Name, _Good = CreateGood(_Name, EINGREDIENT));
	_Crop->Name = _Name;
	return _Crop;
}

struct Crop* CopyCrop(const struct Crop* _Crop) {
	struct Crop* _NewCrop = (struct Crop*) malloc(sizeof(struct Crop));

	_NewCrop->Name = _Crop->Name;
	_NewCrop->PerAcre = _Crop->PerAcre;
	_NewCrop->NutVal = _Crop->NutVal;
	_NewCrop->YieldMult = _Crop->YieldMult;
	return _NewCrop;
}

void DestroyCrop(struct Crop* _Crop) {
	free(_Crop);
}

struct Field* CreateField() {
	struct Field* _Field = (struct Field*) malloc(sizeof(struct Field));

	_Field->Status = EFALLOW;
	return _Field;
}

void DestroyField(struct Field* _Field) {
	free(_Field);
}

void FieldReset(struct Field* _Crop) {
	_Crop->YieldTotal = 0;
	_Crop->Acres = 0;
	_Crop->Status = EFALLOW;
}

int FieldPlant(struct Field* _Field, struct Good* _Seeds) {
	char _SeedName[strlen(_Field->Crop->Name) + strlen(CROPGOOD) + 1];

	if(_Field->Status != EFALLOW)
		return 0;

	strcpy(_SeedName, _Field->Crop->Name);
	strcat(_SeedName, CROPGOOD);
	if(strcmp(_Seeds->Name, _SeedName) != 0)
		return 0;
	if(_Seeds->Quantity < _Field->Crop->PerAcre * _Field->Acres)
		return 0;
	_Seeds->Quantity -= _Field->Crop->PerAcre * _Field->Acres;
	_Field->Status = EPLOWING;
	_Field->StatusTime = _Field->Acres * WORKMULT;
	return 1;
}

void FieldWork(struct Field* _Field, int _Total) {
	int _Val = _Total;

	switch(_Field->Status) {
		case EGROWING:
			_Field->YieldTotal += _Val / _Field->GrowDays;
			break;
		case EFALLOW:
			return;
		default:
			_Field->StatusTime -= _Total * WORKMULT;
			if(_Field->StatusTime <= 0)
				NextStatus(_Field);
			break;
	}
}

void FieldHarvest(struct Field* _Field, struct Good* _Seeds) {
	char _SeedName[strlen(_Field->Crop->Name) + strlen(CROPGOOD) + 1];

	strcpy(_SeedName, _Field->Crop->Name);
	strcat(_SeedName, CROPGOOD);
	if(strcmp(_Seeds->Name, _SeedName) != 0)
		return;
	if(_Field->Status != EHARVESTING)
		return;
	_Field->Acres = _Field->Acres - (_Field->Acres - _Field->StatusTime);
	_Seeds->Quantity = TOPOUND(_Field->Acres * _Field->Crop->PerAcre / WORKMULT);
}

int FieldUpdate(struct Field* _Field) {
	if(_Field->Status == EGROWING)
		--_Field->StatusTime;
	return 1;
}
