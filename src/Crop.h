/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

#include "sys/Event.h"

#include <SDL2/SDL.h>

#define CROP_ROTATIONS (2)
#define FieldTotalAcres(_Field) ((_Field)->Acres + (_Field)->UnusedAcres)
#define GROWDEG_MAX (88)
#define TEMP_AVG (70)
#define PLANT_TIME (2)
#define CropName(_Crop) ((_Crop)->Name)
#define FieldHarvestMod(_Field, _Mod) ((_Field)->Acres * (_Field)->Crop->SeedsPerAcre * (_Field)->Crop->YieldMult * _HarvestMod)

typedef struct lua_State lua_State;
struct LinkedList;
struct LnkLst_Node;
struct Family;
struct Good;
struct Array;
struct Object;
struct FoodBase;

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
	const struct FoodBase* const Output;
	const char* Name;
	int Type;
	double NutVal; //Nutritional Value per pound.
	double YieldMult; //How many pounds of seed to expect from one pound.
	double PlantMult; //Multiplier to time required to plant this crop.
	float FlourRemain; //Percent of how much of the crop remains after milling.
	uint16_t SeedsPerAcre;//How many ounces of seeds it takes to fill an acre.
	uint16_t NutritionScore; //How much nutrition this crop gives after taking into account the amount needed to replant it.
	int16_t GrowingDegree;
	int16_t GrowingBase; //The minimum temperature it must be for this crop to grow.
	uint8_t SurviveWinter;
};

struct Field {
	struct Object Object;
	SDL_Point Pos;
	const struct Crop* Crop;
	struct Family* Owner;
	double YieldTotal; //How much of the field as a percent of up to 100, that has been successfully grown.
	int16_t Acres;
	int16_t UnusedAcres;
	int16_t Status;
	int16_t StatusTime; //How much more time it will take to reach the next status.
};

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, double _NutVal, double _YieldMult, int _GrowingDegree, int _GrowingBase, int _SurviveWinter);
void DestroyCrop(struct Crop* _Crop);
struct Crop* CropLoad(lua_State* _State, int _Index);

struct Field* CreateField(int _X, int _Y, const struct Crop* _Crop, int _Acres, struct Family* _Owner);
int FieldCmp(const void* _One, const void* _Two);
void DestroyField(struct Field* _Field);
int InputReqFieldCmp(const void* _One, const void* _Two);
/**
 * Sets all the Fields data about a specific field to default values.
 */
void FieldReset(struct Field* _Field);
/**
 * Sets the state of the field to be planting and removed enough seeds from _Seeds
 * to fill up the field.
 */
int FieldPlant(struct Field* _Field, struct Good* _Seeds);
int FieldHarvest(struct Field* _Field, struct Array* _Goods, float _HarvestMod);
void FieldUpdate(struct Field* _Field);
void FieldSetAcres(struct Field* _Field, int _Acres);
void FieldDivideAcres(struct Field* _Field, int _Acres);
void FieldClearAcres(struct Field* _Field);
void FieldRotateCrops(struct Field* _Field);
/**
 * Sets _Fields's acres to how many acres grown with _Seeds->Quantity.
 */
void FieldAcreage(struct Field* _Field, const struct Good* _Seeds);
/**
 * Returns how many degrees a plant has grown over a day given the plants
 * minimum and maximum degrees that it can grow in and the temperature for the day.
 */
int GrowingDegree(int _MinTemp, int _MaxTemp, int _BaseTemp);
/**
 * Returns how many acres are in the aarray _Fields.
 */
static inline int FieldsGetAcreage(struct Field* const * _Fields, int _FieldSz) {
	int _Acreage = 0;

	for(int i = 0; i < _FieldSz; ++i) {
		_Acreage = _Acreage + FieldTotalAcres(_Fields[i]);
	}
	return _Acreage;
}

/**
 * Fills the array _Fields with elements of type struct Field* that contain
 * crops necessary to feed the family and their animals.
 * \note _Fields should be an array of Field* that has a length of FAMILY_FIELDCT.
 * \return The new size of _Fields.
 */
int SelectCrops(struct Family* _Family, struct Field* _Fields[], int _FieldSz, int _FieldMaxSz);
/**
 * @pre Every element in _Fields must contain a struct Field*. Every
 * element in _Crops must be a struct InputReq where the Req variable is
 * a struct Crop*.
 */
int PlanFieldCrops(struct Field* _Fields[], struct LinkedList* _Crops, struct Family* _Family, int _FieldSz, int _FieldMaxSz);
/**
 * Combines all fields in the array into one.
 */
void FieldAbosrb(struct Field* _Fields[], int _FieldSz);

void FieldChangeStatus(struct Field* _Field);
static inline const char* FieldStatusName(const struct Field* const _Field) {
	switch(_Field->Status) {
		case EFALLOW:
			return "Fallow";
		case EPLOWING:
			return "Plowing";
		case EPLANTING:
			return "Planting";
		case EGROWING:
			return "Growing";
		case EHARVESTING:
			return "Harvesting";
	}
	return "None";
}

static inline int FieldStatusDays(const struct Field* const _Field) {
	switch(_Field->Status) {
		case EGROWING:
			return _Field->StatusTime / TEMP_AVG;	
		case EFALLOW:
			return 0;
		default:
			return _Field->StatusTime / FamilyWorkModifier(_Field->Owner);
	}
	return 0;
}

static inline int CropAcreHarvest(const struct Crop* _Crop) {
		return _Crop->NutVal * ToPound(_Crop->SeedsPerAcre) * _Crop->YieldMult;
}

#endif
