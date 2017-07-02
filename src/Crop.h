/*
 * File: Crop.h
 * Author: David Brotz
 */

#ifndef __CROP_H
#define __CROP_H

#include "Herald.h"
#include "Family.h"

#include <SDL2/SDL.h>

#define CROP_ROTATIONS (2)
#define FieldTotalAcres(Field) ((Field)->Acres + (Field)->UnusedAcres)
#define GROWDEG_MAX (88)
#define TEMP_AVG (70)
#define PLANT_TIME (2)
#define CropName(Crop) ((Crop)->Name)
#define FieldHarvestMod(Field, Mod) ((Field)->Acres * ToPound((Field)->Crop->SeedsPerAcre) * (Field)->Crop->YieldMult * HarvestMod)
#define PastureHarvestMod(Mod) (ToPound(PastureCrop()->SeedsPerAcre) * PastureCrop()->YieldMult * Mod * PastureCrop()->NutVal)

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
	uint32_t Id;
	const struct FoodBase* const Output;
	const char* Name;
	uint16_t Type;
	uint32_t LuaRef;//Integer of lua reference to a Lua table.
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
	const struct Crop* Crop;
	struct Family* Owner;
	double YieldTotal; //How much of the field as a percent of up to 100, that has been successfully grown.
	int16_t Acres;
	int16_t UnusedAcres;
	int16_t StatusTime; //How much more time it will take to reach the next status.
	uint8_t Status;
};

struct Crop* CreateCrop(lua_State* State, const char* Name, int Type, int PerAcre, double NutVal, double YieldMult, int GrowingDegree, int GrowingBase, int SurviveWinter, float FlourRemain);
void DestroyCrop(struct Crop* Crop);
struct Crop* CropLoad(lua_State* State, int Index);

struct Field* CreateField(const struct Crop* Crop, int Acres, struct Family* Owner);
int FieldCmp(const void* One, const void* Two);
void DestroyField(struct Field* Field);
int InputReqFieldCmp(const void* One, const void* Two);
/**
 * Sets all the Fields data about a specific field to default values.
 */
void FieldReset(struct Field* Field);
/**
 * Sets the state of the field to be planting and removed enough seeds from Seeds
 * to fill up the field.
 */
void FieldPlant(struct Field* Field, struct Good* Seeds);
int FieldHarvest(struct Field* Field, struct Array* Goods, float HarvestMod);
void FieldUpdate(struct Field* Field);
void FieldSetAcres(struct Field* Field, int Acres);
void FieldDivideAcres(struct Field* Field, int Acres);
void FieldClearAcres(struct Field* Field);
void FieldRotateCrops(struct Field* Field);
/**
 * Sets Fields's acres to how many acres grown with Seeds->Quantity.
 */
void FieldAcreage(struct Field* Field, const struct Good* Seeds);
/**
 * Returns how many degrees a plant has grown over a day given the plants
 * minimum and maximum degrees that it can grow in and the temperature for the day.
 */
int GrowingDegree(int MinTemp, int MaxTemp, int BaseTemp);
/**
 * Returns how many acres are in the aarray Fields.
 */
static inline int FieldsGetAcreage(struct Field* const * Fields, int FieldSz) {
	int Acreage = 0;

	for(int i = 0; i < FieldSz; ++i) {
		Acreage = Acreage + FieldTotalAcres(Fields[i]);
	}
	return Acreage;
}

/**
 * Fills the array Fields with elements of type struct Field* that contain
 * crops necessary to feed the family and their animals.
 * \note Fields should be an array of Field* that has a length of FAMILY_FIELDCT.
 * \return The new size of Fields.
 */
int SelectCrops(struct Family* Family, struct Field* Fields[], int FieldSz, int FieldMaxSz);
/**
 * @pre Every element in Fields must contain a struct Field*. Every
 * element in Crops must be a struct InputReq where the Req variable is
 * a struct Crop*.
 */
int PlanFieldCrops(struct Field* Fields[], struct LinkedList* Crops, struct Family* Family, int FieldSz, int FieldMaxSz);
/**
 * Combines all fields in the array into one.
 */
void FieldAbosrb(struct Field* Fields[], int FieldSz);

void FieldChangeStatus(struct Field* Field);
static inline const char* FieldStatusName(const struct Field* const Field) {
	switch(Field->Status) {
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

static inline int FieldStatusDays(const struct Field* const Field) {
	switch(Field->Status) {
		case EGROWING:
			return Field->StatusTime / TEMP_AVG;	
		case EFALLOW:
			return 0;
		default:
			return Field->StatusTime / FamilyWorkModifier(Field->Owner);
	}
	return 0;
}

static inline int CropAcreHarvest(const struct Crop* Crop) {
		return Crop->NutVal * ToPound(Crop->SeedsPerAcre) * Crop->YieldMult;
}

static inline void FieldSwap(struct Field* Field) {
	uint16_t Temp = Field->Acres;

	Field->Acres = Field->UnusedAcres;
	Field->UnusedAcres = Temp;
}

#endif
