/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

#define CROP_ROTATIONS (2)
#define FieldTotalAcres(_Field) ((_Field)->Acres + (_Field)->UnusedAcres)
#define GROWDEG_MAX (88)

typedef struct lua_State lua_State;
struct LinkedList;
struct Family;
struct Good;
struct Array;
struct Object;

enum {
	ENONE,
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
	int GrowingDegree;
	int GrowingBase; //The minimum temperature it must be for this crop to grow.
	double YieldMult; //How many pounds of seed to expect from one pound.
	int SurviveWinter;
};

struct Field {
	int Id;
	int Type;
	int X;
	int Y;
	int(*Think)(struct Object*);
	int Width;
	int Length;
	const struct Crop* Crop;
	double YieldTotal; //How much of the field as a percent of up to 100, that has been successfully grown.
	int Acres;
	int UnusedAcres;
	int Status;
	int StatusTime; //How much more time it will take to reach the next status.
};

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, int _NutVal, double _YieldMult, int _GrowingDegree, int _GrowingBase, int _SurviveWinter);
void DestroyCrop(struct Crop* _Crop);
struct Crop* CropLoad(lua_State* _State, int _Index);

struct Field* CreateField(int _X, int _Y, const struct Crop* _Crop, int _Acres);
int FieldCmp(const void* _One, const void* _Two);
void DestroyField(struct Field* _Field);
int InputReqFieldCmp(const void* _One, const void* _Two);
/**
 * Sets all the Fields data about a specific field to default values.
 */
void FieldReset(struct Field* _Field);
int FieldPlant(struct Field* _Field, struct Good* _Seeds);
void FieldWork(struct Field* _Field, int _Total, struct Good* _Tool);
void FieldHarvest(struct Field* _Field, struct Array* _Goods);
int FieldUpdate(struct Field* _Field);
void FieldSetAcres(struct Field* _Field, int _Acres);
void FieldRotateCrops(struct Field* _Field);
/**
 * Returns how many acres the field can support given the amount of seeds.
 */
int FieldAcreage(const struct Field* _Field, const struct Good* _Seeds);
int GrowingDegree(int _MinTemp, int _MaxTemp, int _BaseTemp);
/**
 * Every element in _Fields must be a non NULL pointer to a struct Field.
 */
int FieldsGetAcreage(const struct Array* _Fields);
/**
 * Fills the array _Fields with elements of type struct Field* that contain
 * crops necessary to feed the family and their animals.
 */
void SelectCrops(struct Family* _Family, struct Array* _Fields);
/**
 * @pre Every element in _Fields must contain a struct Field*. Every
 * element in _Crops must be a struct InputReq where the Req variable is
 * a struct Crop*.
 */
void PlanFieldCrops(struct Array* _Fields, struct LinkedList* _Crops);
/**
 * Finds fields that are adjacent to each other and combines them.
 */
void FieldAbosrb(struct Array* _Fields);

#endif
