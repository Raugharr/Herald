/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

typedef struct lua_State lua_State;
struct Good;

enum {
	EFALLOW,
	EPLOWING,
	EPLANTING,
	EGROWING,
	EHARVESTING
};

enum {
	EGRASS
};

struct Crop {
	int Id;
	char* Name;
	int Type;
	int PerAcre;//How many ounces of seeds it takes to fill an acre.
	int NutVal; //Nutritional Value per pound.
	int GrowDays;
	double YieldMult; //How many pounds of seed to expect from one pound.
};

struct Field {
	int Id;
	int X;
	int Y;
	const struct Crop* Crop;
	double YieldTotal; //How much of the field as a percent of up to 100, that has been successfully grown.
	int Acres;
	int UnusedAcres;
	int Status;
	int StatusTime; //How much more time it will take to reach the next status.
};

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, int _NutVal, double _YieldMult, int _GrowDays);
struct Crop* CopyCrop(const struct Crop* _Crop);
void DestroyCrop(struct Crop* _Crop);
struct Crop* CropLoad(lua_State* _State, int _Index);

struct Field* CreateField(int _X, int _Y, const struct Crop* _Crop, int _Acres);
void DestroyField(struct Field* _Field);
//Sets all the Fields data about a specific field to default values.
void FieldReset(struct Field* _Field);
int FieldPlant(struct Field* _Field, struct Good* _Seeds);
void FieldWork(struct Field* _Field, int _Total, struct Good* _Tool);
void FieldHarvest(struct Field* _Field, struct Good* _Seeds);
int FieldUpdate(struct Field* _Field);

#endif
