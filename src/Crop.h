/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

#define CROPGOOD " Seeds"

struct Good;

enum {
	EFALLOW,
	EPLOWING,
	EPLANTING,
	EGROWING,
	EHARVESTING
};

struct Crop {
	const char* Name;
	int PerAcre;//How many pounds of seeds it takes to fill an acre.
	int NutVal; //Nutritional Value per pound.
	double YieldMult; //How many pounds of seed to expect from one pound.
	int GrowDays;
	double YieldTotal; //How much of the field as a percent of up to 100, that has been successfully grown.
	int Acres;
	int Status;
	int StatusTime; //How much more time it will take to reach the next status.
};

struct Crop* CreateCrop(const char* _Name, int _PerAcre, int _NutVal, double _YieldMult, int _GrowDays);
struct Crop* CopyCrop(const struct Crop* _Crop);
void DestroyCrop(struct Crop* _Crop);
int CropPlant(struct Crop* _Crop, struct Good* _Seeds);
void CropWork(struct Crop* _Crop, int _Total);
void CropHarvest(struct Crop* _Crop, struct Good* _Seeds);
int Crop_Update(struct Crop* _Crop);

#endif
