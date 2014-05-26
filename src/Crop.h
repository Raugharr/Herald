/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

struct Crop {
	char* Name;
	//int Price;//Standard price.
	int PerAcre;//How many pounds of seeds it takes to fill an acre.
	int NutVal; //Nutritional Value per pound.
	double YieldMult;//How many pounds of seed to expect from one pound.
};

struct Crop* CreateCrop(const char* _Name, int _PerAcre, int _NutVal, double _YieldMult);
void DestroyCrop(struct Crop* _Crop);

#endif
