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

#define CROPGOOD " Seeds"

struct Crop* CreateCrop(const char* _Name, int _PerAcre, int _NutVal, double _YieldMult, int _StartMonth, int _EndMonth) {
	struct Crop* _Crop = (struct Crop*) malloc(sizeof(struct Crop));
	char* _GoodName = (char*) malloc(sizeof(char*) * strlen(_Name) + strlen(CROPGOOD) + 1);
	struct Good* _Good = NULL;

	_Crop->Name = _Name;
	_Crop->PerAcre = _PerAcre;
	_Crop->NutVal = _NutVal;
	_Crop->YieldMult = _YieldMult;
	_Crop->StartMonth = _StartMonth;
	_Crop->EndMonth = _EndMonth;
	_Crop->YieldTotal = 0;
	strcpy(_GoodName, _Name);
	strcat(_GoodName, CROPGOOD);
	RBTree_Insert(&g_Strings, _GoodName);
	if(Hash_Find(&g_Goods, _GoodName, _Good))
		DestroyGood(_Good);
	if(Hash_Find(&g_Goods, _Name, _Good))
		DestroyGood(_Good);

	Hash_Insert(&g_Goods, _GoodName, CreateGood(_GoodName, ESEED));
	Hash_Insert(&g_Goods, _GoodName, CreateGood(_Name, EINGREDIENT));
	_Crop->Name = _Name;
	return _Crop;
}

struct Crop* CopyCrop(const struct Crop* _Crop) {
	struct Crop* _NewCrop = (struct Crop*) malloc(sizeof(struct Crop));

	_NewCrop->Name = _Crop->Name;
	_NewCrop->PerAcre = _Crop->PerAcre;
	_NewCrop->NutVal = _Crop->NutVal;
	_NewCrop->YieldMult = _Crop->YieldMult;
	_NewCrop->StartMonth = _Crop->StartMonth;
	_NewCrop->EndMonth = _Crop->EndMonth;
	_NewCrop->YieldTotal = 0;
	return _NewCrop;
}

void DestroyCrop(struct Crop* _Crop) {
	free(_Crop);
}

void CropWork(struct Crop* _Crop, int _Total) {
	int _Val = _Total;

	_Crop->YieldTotal += _Val / DaysBetween(TO_DATE(0, _Crop->StartMonth, 0), TO_DATE(0, _Crop->EndMonth, 0));
}

