/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

struct Crop {
	const char* Name;
	//int Price;//Standard price.
	int PerAcre;//How many pounds of seeds it takes to fill an acre.
	int NutVal; //Nutritional Value per pound.
	double YieldMult; //How many pounds of seed to expect from one pound.
	int StartMonth; //Month the crop can be planted.
	int EndMonth; //Month the crop must be harvested by.
	double YieldTotal; //How much of the field as a percent of up to 100, that has been successfully grown.
};

struct Crop* CreateCrop(const char* _Name, int _PerAcre, int _NutVal, double _YieldMult, int _StartMonth, int _EndMonth);
struct Crop* CopyCrop(const struct Crop* _Crop);
void DestroyCrop(struct Crop* _Crop);
void CropWork(struct Crop* _Crop, int _Total);

#endif
