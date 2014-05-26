/*
 * File: Crop.c
 * Author: David Brotz
 */

#include "Crop.h"

#include <stdlib.h>
#include <string.h>

struct Crop* CreateCrop(const char* _Name, int _PerAcre, int _NutVal, double _YieldMult) {
	struct Crop* _Crop = (struct Crop*) malloc(sizeof(struct Crop));

	_Crop->Name = (char*) malloc(strlen(_Name) + 1);
	_Crop->PerAcre = _PerAcre;
	_Crop->NutVal = _NutVal;
	_Crop->YieldMult = _YieldMult;
	strcpy(_Crop->Name, _Name);
	return _Crop;
}
void DestroyCrop(struct Crop* _Crop) {
	free(_Crop->Name);
	free(_Crop);
}


