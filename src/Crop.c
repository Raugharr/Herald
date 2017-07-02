/*
 * File: Crop.c
 * Author: David Brotz
 */

#include "Crop.h"

#include "Good.h"
#include "Herald.h"
#include "World.h"
#include "Family.h"
#include "Person.h"
#include "Population.h"
#include "Family.h"
#include "Location.h"
#include "Person.h"

#include "sys/RBTree.h"
#include "sys/LuaCore.h"
#include "sys/Log.h"
#include "sys/Array.h"
#include "sys/Event.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

static int g_CropId = 0;

int LuaCropRegister(lua_State* State, const struct Crop* Crop) {
	int Ref = 0;
	
	lua_pushvalue(State, LUA_REGISTRYINDEX);
	lua_createtable(State, 0, 7);

	lua_pushstring(State, "Id");
	lua_pushinteger(State, Crop->Id);
	lua_rawset(State, -3);

	lua_pushstring(State, "Name");
	lua_pushstring(State, Crop->Name);
	lua_rawset(State, -3);

	lua_pushstring(State, "Type");
	lua_pushinteger(State, Crop->Type);
	lua_rawset(State, -3);

	lua_pushstring(State, "PerAcre");
	lua_pushinteger(State, Crop->SeedsPerAcre);
	lua_rawset(State, -3);

	lua_pushstring(State, "NutVal");
	lua_pushinteger(State, Crop->NutVal);
	lua_rawset(State, -3);

	lua_pushstring(State, "GrowDays");
	lua_pushinteger(State, Crop->GrowingDegree);
	lua_rawset(State, -3);

	lua_pushstring(State, "YieldMult");
	lua_pushnumber(State, Crop->YieldMult);
	lua_rawset(State, -3);
	Ref = luaL_ref(State, -2);
	lua_pop(State, 1);
	return Ref;
}

struct Crop* CreateCrop(lua_State* State, const char* Name, int Type, int PerAcre, double NutVal, double YieldMult, int GrowingDegree, int GrowingBase, int SurviveWinter, float FlourRemain) {
	struct Crop* Crop = NULL;
	struct FoodBase* Output = NULL;

	if(PerAcre <= 0 || NutVal <= 0 || YieldMult <= 0
			|| GrowingDegree <= 0 || GrowingBase <= 0) {
		Log(ELOG_WARNING, "Crop is missing a parameter.");
		return NULL;
	}
	Output = CreateFoodBase(Name, GOOD_SEED, NutVal);
	Crop = (struct Crop*) malloc(sizeof(struct Crop));
	Crop->Id = ++g_CropId;
	*(const struct FoodBase**)&Crop->Output = Output;
	Crop->Name = Output->Base.Name;
	Crop->Type = Type;
	Crop->SeedsPerAcre = PerAcre;
	Crop->NutVal = NutVal;
	Crop->YieldMult = YieldMult;
	Crop->PlantMult = ToPound(PerAcre) / ((float)BUSHEL);
	Crop->FlourRemain = 0.7f;
	Crop->NutritionScore = ToPound(PerAcre) * (YieldMult - 1) * NutVal * Crop->FlourRemain;
	Crop->GrowingDegree = GrowingDegree;
	Crop->GrowingBase = GrowingBase;
	Crop->SurviveWinter = SurviveWinter;
	Crop->LuaRef = LuaCropRegister(State, Crop);
	HashInsert(&g_Goods, Output->Base.Name, Output);
	Log(ELOG_INFO, "Crop loaded %s.", Output->Base.Name);
	return Crop;
}

void DestroyCrop(struct Crop* Crop) {
	free(Crop);
}

struct Crop* CropLoad(lua_State* State, int Index) {
	int Type = 0;
	int PerAcre = 0;
	double NutValue = 0;
	double FlourRemain = 0;
	int GrowingDegree = 0;
	int Return = -2;
	int SurviveWinter = 0;
	int GrowBase = 0;
	double YieldMult = 0;
	const char* Name = NULL;
	const char* TypeStr = NULL;
	const char* Key = NULL;

	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_isstring(State, -2))
			Key = lua_tostring(State, -2);
		else
			continue;
		if(!strcmp("PoundsPerAcre", Key)) {
			Return = LuaGetInteger(State, -1, &PerAcre);
			PerAcre *= OUNCE;
		} else if(!strcmp("Type", Key)) {
			if(lua_isstring(State, -1)) {
				Return = LuaGetString(State, -1, &TypeStr);
				if(!strcmp("Grass", TypeStr))
					Type = EGRASS;
				else
					luaL_error(State, "Type contains an invalid string.");
			}
		} else if(!strcmp("YieldPerSeed", Key))
			Return = LuaGetNumber(State, -1, &YieldMult);
		else if(!strcmp("NutritionalValue", Key))
			Return = LuaGetNumber(State, -1, &NutValue);
		else if(!strcmp("Name", Key))
			Return = LuaGetString(State, -1, &Name);
		else if(!strcmp("GrowingDegree", Key)) {
			Return = LuaGetInteger(State, -1, &GrowingDegree);
		}
		else if(!strcmp("GrowingBase", Key)) {
			Return = LuaGetInteger(State, -1, &GrowBase);
		}
		else if(!strcmp("SurviveWinter", Key)) {
			if(lua_type(State, -1) != LUA_TBOOLEAN)
				Return = 0;
			SurviveWinter = lua_toboolean(State, -1);
		}
		else if(!strcmp("MillingCost", Key)) {
			Return = LuaGetNumber(State, -1, &FlourRemain);
		}
		lua_pop(State, 1);
		if(!(Return > 0)) {
			lua_pop(State, 1);
			Log(ELOG_WARNING, "%s is not a crop parameter.", Key);
			return NULL;
		}
	}
	return CreateCrop(State, Name, Type, PerAcre, NutValue, YieldMult, GrowingDegree, GrowBase, SurviveWinter, FlourRemain);
}

struct Field* CreateField(const struct Crop* Crop, int Acres, struct Family* Owner) {
	struct Field* Field = NULL;

	Field = (struct Field*) malloc(sizeof(struct Field));
	CreateObject((struct Object*)Field, OBJECT_CROP);
	Field->Crop = Crop;
	Field->YieldTotal = 0;
	Field->Acres = Acres;
	Field->UnusedAcres = 0;
	Field->Status = ENONE;
	Field->StatusTime = 0;
	Field->Owner = Owner;
	return Field;
}

int FieldCmp(const void* One, const void* Two) {
	return ((struct Field*)One)->Object.Id - ((struct Field*)Two)->Object.Id;
}

void DestroyField(struct Field* Field) {
	DestroyObject((struct Object*)Field);
	free(Field);
}

int InputReqFieldCmp(const void* One, const void* Two) {
	return FieldCmp(((struct InputReq*)One)->Req, ((struct InputReq*)Two)->Req);
}

void FieldReset(struct Field* Crop) {
	Crop->YieldTotal = 0;
	FieldClearAcres(Crop);
	Crop->Status = EFALLOW;
}

void FieldPlant(struct Field* Field, struct Good* Seeds) {
	int TempAcres = Seeds->Quantity / Field->Crop->SeedsPerAcre;
#ifdef DEBUG
	uint16_t TotalAcres = Field->Acres + Field->UnusedAcres;
#endif

	if(Field->Crop == NULL) {
		FieldReset(Field);
		return;
	}
	if(Field->Status != EFALLOW)
		return;
	if(TempAcres < Field->UnusedAcres) {
		FieldSwap(Field);
		//int Total = Field->Acres + Field->UnusedAcres;

		//Field->Acres = TempAcres;
		//Field->UnusedAcres = Total - TempAcres;
	} else {
		uint16_t Temp = Field->Acres;

		Field->Acres = Field->UnusedAcres;
		Field->UnusedAcres = Temp;
	}
	Seeds->Quantity -= Field->Crop->SeedsPerAcre * Field->Acres;
	Field->Status = EPLOWING - 1;
	FieldChangeStatus(Field);
#ifdef DEBUG
	Assert(TotalAcres == Field->Acres + Field->UnusedAcres);
#endif
}

void FieldWork(struct Field* Field, int Total, const struct Array* Goods) {
	uint32_t Modifier = 1000;

	if(Field->Status == EGROWING)
		return;
	if(Field->Status == EPLOWING) {
		const struct ToolBase* Tool = NULL;

		for(int i = 0; i < Goods->Size; ++i) {
			Tool = (struct ToolBase*) ((struct Good*)Goods->Table[i])->Base;
			if(Tool->Base.Category != GOOD_TOOL)
				continue;
			if((Tool->Function & ETOOL_PLOW) != ETOOL_PLOW)
				continue;
			Modifier = Tool->Quality;
			break;
		}
	}
//	if(Field->Status == EHARVESTING) {
	//	const struct ToolBase* Tool = NULL;

//		for(int i = 0; i < Goods->Size; ++i) {
//			Tool = (struct ToolBase*) ((struct Good*)Goods->Table[i])->Base;
//			if(Tool->Category != GOOD_TOOL)
//				continue;
//			if((Tool->Function & ETOOL_REAP) != ETOOL_REAP)
//				continue;
//			Modifier = Tool->Quality;
//			break;
//		}
//	}
	Field->StatusTime -= (Total * Modifier) / MAX_WORKRATE;
	if(Field->StatusTime <= 0) {
		FieldChangeStatus(Field);
	}
}

int FieldHarvest(struct Field* Field, struct Array* Goods, float HarvestMod) {
	struct Good* Seeds = NULL;
	int Quantity = FieldHarvestMod(Field, HarvestMod);
	int SeedQuantity = 0;
	int TempAcres = 0;

	Assert(Field->Status == EHARVESTING);
	Seeds = CheckGoodTbl(Goods, CropName(Field->Crop), &g_Goods);
	if(Field->Crop->Type == EGRASS) {
		struct Good* Straw = NULL;

		Straw = CheckGoodTbl(Goods, "Straw", &g_Goods);
		Straw->Quantity += Quantity * 4;
	}
	//TempAcres = Field->Acres;
	//Field->Acres = Field->Acres - (Field->Acres - Field->UnusedAcres);
	//Field->UnusedAcres = TempAcres;
	FieldSwap(Field);
	TempAcres = (Field->Acres > Field->UnusedAcres) ? (Field->Acres) : (Field->UnusedAcres);
	SeedQuantity = ToPound(TempAcres * Field->Crop->SeedsPerAcre);
	if(HarvestMod >= 1.0f && Seeds->Quantity * 2 < Field->Crop->SeedsPerAcre * TempAcres) {
		SeedQuantity += (Quantity - SeedQuantity) / 8;
	}
	/*/if(Quantity - SeedQuantity < 0) {
		SeedQuantity = Quantity / 2;
	}*/
	Seeds->Quantity += ToOunce(SeedQuantity);
	Assert(Quantity - SeedQuantity > 0);
	return (Quantity - SeedQuantity) * Field->Crop->NutVal * Field->Crop->FlourRemain;
}

void FieldUpdate(struct Field* Field) {
	if(Field->Status == EGROWING) {
		int Temp = g_TemperatureList[MONTH(g_GameWorld.Date)];

		Field->StatusTime -= GrowingDegree(Temp, Temp, Field->Crop->GrowingBase);
		if(Field->StatusTime <= 0) {
			FieldChangeStatus(Field);
		}
	}
}

void FieldSetAcres(struct Field* Field, int Acres) {
	Field->Acres = 0;
	Field->UnusedAcres = Acres;
}

void FieldDivideAcres(struct Field* Field, int Acres) {
	int Div = Acres / CROP_ROTATIONS;

	Field->Acres = Div;
	Field->UnusedAcres = Div;
	if((Acres & 1) == 1)
		++Field->Acres;
}

void FieldClearAcres(struct Field* Field) {
	Field->UnusedAcres = Field->UnusedAcres + Field->Acres;
	Field->Acres = 0;
}

void FieldRotateCrops(struct Field* Field) {
	int Temp = Field->Acres;

	Field->Acres = Field->UnusedAcres;
	Field->UnusedAcres = Temp;
}

void FieldAcreage(struct Field* Field, const struct Good* Seeds) {
	const struct Crop* Crop = Field->Crop;
	int SeedReq = Crop->SeedsPerAcre * Field->Acres;
	int Acres = 0;
	int TotalAcres = Field->Acres + Field->UnusedAcres;

	if(SeedReq < Seeds->Quantity)
		return;
	Acres = Seeds->Quantity / Crop->SeedsPerAcre;
	Field->Acres = Acres;
	Field->UnusedAcres = TotalAcres - Acres;
}

int GrowingDegree(int MinTemp, int MaxTemp, int BaseTemp) {
	int Mean = 0;
	if(MaxTemp > GROWDEG_MAX)
		MaxTemp = GROWDEG_MAX;
	//if(MinTemp < BaseTemp)
	//	MinTemp = MinTemp + (BaseTemp - MinTemp);
	Mean = (MaxTemp + MinTemp) / 2;
	return (Mean < BaseTemp) ? (0) : (Mean - 32);
}

int CropListAdd(struct LinkedList* CropList, const struct Array* const Goods, const struct Crop* Crop, struct GoodBase* CropGood, int NutReq, int PopCt) {
	struct InputReq* CropInput = (struct InputReq*) malloc(sizeof(struct InputReq));
	struct InputReq* CropSearch = NULL;

	CropInput->Req = NULL;
	CropInput->Quantity = 0;
	for(int i = 0; i < Goods->Size; ++i) {
		if(((struct Good*)Goods->Table[i])->Base->Id == CropGood->Id) {
			CropInput->Req = Crop;
			CropInput->Quantity =  ceil(((double)NutReq * PopCt * YEAR_DAYS) / ((double)Crop->NutVal * (Crop->SeedsPerAcre / OUNCE) * (Crop->YieldMult - 1.0f)));
			if((CropSearch = (struct InputReq*)LnkLstSearch(CropList, &CropInput, InputReqCropCmp)) == NULL)
				LnkLstInsertPriority(CropList, CropInput, InputReqQtyCmp);
			else
				CropSearch->Quantity += CropInput->Quantity;
			break;
		}
	}
	return (int) CropInput->Quantity;
}

/*
 * TODO: Families seem to get the same crop for both of their fields.
 */
int SelectCrops(struct Family* Family, struct Field* Fields[], int FieldSz, int FieldMaxSz) {
	int TotalAcreage = FieldsGetAcreage(Fields, FieldSz) / CROP_ROTATIONS;
	int Acreage = 0;
	int AnimalCt = Family->Animals.Size;
	int FamSize = FamilySize(Family);
	int NewMaxSz = 0;
	struct InputReq** AnList = AnimalTypeCount(&Family->Animals, &AnimalCt);
	struct LinkedList Crops = {0, NULL, NULL}; //List of selectable crops.
	struct LnkLst_Node* Itr = NULL;
	double Ratio = 0;

//	if(AnimalCt <= 0)
//		return NewMaxSz;
	for(int i = 0; g_GameWorld.HumanEats[i] != NULL; ++i) {
		Acreage = Acreage + CropListAdd(&Crops, &Family->Goods, HashSearch(&g_Crops, g_GameWorld.HumanEats[i]->Base.Name), HashSearch(&g_Goods, g_GameWorld.HumanEats[i]->Base.Name), NUTRITION_DAILY, FamSize);
	}
	for(int i = 0; i < AnimalCt; ++i) {
		//Pick a crop that is needed to feed the population then find out how many acres are needed.
		for(int j = 0; j < ((struct Population*)AnList[i]->Req)->EatsSize; ++j) {
				struct FoodBase* Eats = ((struct Population*)AnList[i]->Req)->Eats[j];

				if(strcmp(Eats->Base.Name, "Hay") == 0)
					continue;
				Acreage = Acreage + CropListAdd(&Crops, &Family->Goods, HashSearch(&g_Crops, Eats->Base.Name), HashSearch(&g_Goods, Eats->Base.Name), ((struct Population*)AnList[i]->Req)->Nutrition, AnList[i]->Quantity);
		}
	}
	if(Crops.Size == 0) return 0;
	//If we don't have enough acres reduce all fields by a certain percentage.
	Ratio = (double)TotalAcreage / Acreage;
	struct LnkLst_Node* LastItr = NULL;

	Itr = Crops.Front;
	while(Itr != NULL) {
		((struct InputReq*)Itr->Data)->Quantity = floor(((struct InputReq*)Itr->Data)->Quantity * Ratio);
		TotalAcreage -= ((struct InputReq*)Itr->Data)->Quantity;
		LastItr = Itr;
		Itr = Itr->Next;
	}
	if(TotalAcreage > 0)
		((struct InputReq*)LastItr->Data)->Quantity += TotalAcreage;
	if(Crops.Size > 0)
		NewMaxSz = PlanFieldCrops(Fields, &Crops, Family, FieldSz, FieldMaxSz);
	Itr = Crops.Front;
	while(Itr != NULL) {
		free(Itr->Data);
		Itr = Itr->Next;
	}
	for(int i = 0; i < AnimalCt; ++i)
		DestroyInputReq(AnList[i]);
	free(AnList);
	return NewMaxSz;
}

int PlanFieldCrops(struct Field* Fields[], struct LinkedList* Crops, struct Family* Family, int FieldSz, int FieldMaxSz) {
	struct LnkLst_Node* Itr = Crops->Front;
	struct Field* CurrField = NULL;
	struct Crop* PlantedCrop = NULL;
	int Acres = 0;
	int FieldAcres = 0;
	int FieldIdx = 0;

	FieldAbosrb(Fields, FieldSz);
	Acres = Fields[0]->UnusedAcres;
	while(Itr != NULL) {
		FieldAcres = ((struct InputReq*)Itr->Data)->Quantity * CROP_ROTATIONS;
		if(((struct InputReq*)Itr->Data)->Quantity > 0 && FieldAcres <= Acres) {
			PlantedCrop = (struct Crop*)((struct InputReq*)Itr->Data)->Req;
			if(FieldIdx < FieldMaxSz) {
				CurrField = Fields[FieldIdx];
				CurrField->Acres = FieldAcres;
				++FieldIdx;
			} else {
				return FieldIdx;
				//_CurrField = CreateField(Family->HomeLoc->Pos.x, Family->HomeLoc->Pos.y, NULL, FieldAcres, Family);
				//ArrayInsert_S(Fields, CurrField);
			}
			CurrField->Crop = PlantedCrop;
			CurrField->Status = EFALLOW;
			CurrField->Owner = Family;
			FieldDivideAcres(CurrField, CurrField->Acres);
			Acres = Acres - CurrField->Acres;
		}
		Itr = Itr->Next;
	}

	for(int i = FieldIdx; i < FieldSz; ++i) {
		DestroyField(Fields[i]);
		Fields[i] = NULL;
	}
	return FieldIdx;
}

void FieldAbosrb(struct Field* Fields[], int FieldSz) {
	struct Field* Parent = NULL;
	struct Field* Field = NULL;

	if(FieldSz < 1)
		return;
	Parent = Fields[0];
	Parent->UnusedAcres = Parent->UnusedAcres + Parent->Acres;
	Parent->Acres = 0;
	for(int i = 1; i < FieldSz; ++i) {
		Field = Fields[i];
		Parent->UnusedAcres = Parent->UnusedAcres + Field->Acres + Field->UnusedAcres;
		Field->UnusedAcres = 0;
		Field->Acres = 0;
	}
}

void FieldChangeStatus(struct Field* Field) {
	if(Field->Status == EHARVESTING) {	
		Field->Status = EFALLOW;
	} else {
		if(Field->Owner != NULL)
			PushEvent(EVENT_FARMING, Field->Owner, Field);
		++Field->Status;
		switch(Field->Status) {
			case EPLANTING:
				Field->StatusTime = Field->Acres * PLANT_TIME * DAYSWORK;
				break;
			case EGROWING:
				Field->StatusTime = Field->Crop->GrowingDegree;
				break;
			case EPLOWING:
				Field->StatusTime = Field->Acres * DAYSWORK;
			default:
				Field->StatusTime = Field->Acres * DAYSWORK;
				break;
		}
	}
}
