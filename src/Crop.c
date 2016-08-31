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

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, double _NutVal, double _YieldMult, int _GrowingDegree, int _GrowingBase, int _SurviveWinter) {
	struct Crop* _Crop = NULL;
	struct FoodBase* _Output = NULL;

	if(_PerAcre <= 0 || _NutVal <= 0 || _YieldMult <= 0
			|| _GrowingDegree <= 0 || _GrowingBase <= 0) {
		Log(ELOG_WARNING, "Crop is missing a parameter.");
		return NULL;
	}
	_Output = CreateFoodBase(_Name, GOOD_SEED, _NutVal);
	_Crop = (struct Crop*) malloc(sizeof(struct Crop));
	_Crop->Id = ++g_CropId;
	*(const struct FoodBase**)&_Crop->Output = _Output;
	_Crop->Name = _Output->Name;
	_Crop->Type = _Type;
	_Crop->SeedsPerAcre = _PerAcre;
	_Crop->NutVal = _NutVal;
	_Crop->YieldMult = _YieldMult;
	_Crop->PlantMult = ToPound(_PerAcre) / ((float)BUSHEL);
	_Crop->FlourRemain = 1.0f;
	_Crop->NutritionScore = ToPound(_PerAcre) * (_YieldMult - 1) * _NutVal * _Crop->FlourRemain;
	_Crop->GrowingDegree = _GrowingDegree;
	_Crop->GrowingBase = _GrowingBase;
	_Crop->SurviveWinter = _SurviveWinter;
	HashInsert(&g_Goods, _Output->Name, CreateFoodBase(_Output->Name, GOOD_SEED, _NutVal));
	Log(ELOG_INFO, "Crop loaded %s.", _Output->Name);
	return _Crop;
}

void DestroyCrop(struct Crop* _Crop) {
	free(_Crop);
}

struct Crop* CropLoad(lua_State* _State, int _Index) {
	int _Type = 0;
	int _PerAcre = 0;
	double _NutValue = 0;
	int _GrowingDegree = 0;
	int _Return = -2;
	int _SurviveWinter = 0;
	int _GrowBase = 0;
	double _YieldMult = 0;
	const char* _Name = NULL;
	const char* _TypeStr = NULL;
	const char* _Key = NULL;

	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("PoundsPerAcre", _Key)) {
			_Return = LuaGetInteger(_State, -1, &_PerAcre);
			_PerAcre *= OUNCE;
		} else if(!strcmp("Type", _Key)) {
			if(lua_isstring(_State, -1)) {
				_Return = LuaGetString(_State, -1, &_TypeStr);
				if(!strcmp("Grass", _TypeStr))
					_Type = EGRASS;
				else
					luaL_error(_State, "Type contains an invalid string.");
			}
		} else if(!strcmp("YieldPerSeed", _Key))
			_Return = LuaGetNumber(_State, -1, &_YieldMult);
		else if(!strcmp("NutritionalValue", _Key))
			_Return = LuaGetNumber(_State, -1, &_NutValue);
		else if(!strcmp("Name", _Key))
			_Return = LuaGetString(_State, -1, &_Name);
		else if(!strcmp("GrowingDegree", _Key)) {
			_Return = LuaGetInteger(_State, -1, &_GrowingDegree);
		}
		else if(!strcmp("GrowingBase", _Key)) {
			_Return = LuaGetInteger(_State, -1, &_GrowBase);
		}
		else if(!strcmp("SurviveWinter", _Key)) {
			if(lua_type(_State, -1) != LUA_TBOOLEAN)
				_Return = 0;
			_SurviveWinter = lua_toboolean(_State, -1);
		}
		lua_pop(_State, 1);
		if(!(_Return > 0)) {
			lua_pop(_State, 1);
			Log(ELOG_WARNING, "%s is not a crop parameter.", _Key);
			return NULL;
		}
	}
	return CreateCrop(_Name, _Type, _PerAcre, _NutValue, _YieldMult, _GrowingDegree, _GrowBase, _SurviveWinter);
}

struct Field* CreateField(int _X, int _Y, const struct Crop* _Crop, int _Acres, struct Family* _Owner) {
	struct Field* _Field = NULL;

	_Field = (struct Field*) malloc(sizeof(struct Field));
	CreateObject((struct Object*)_Field, OBJECT_CROP, (void(*)(struct Object*))FieldUpdate);
	_Field->Pos.x = _X;
	_Field->Pos.y = _Y;
	_Field->Crop = _Crop;
	_Field->YieldTotal = 0;
	_Field->Acres = _Acres;
	_Field->UnusedAcres = 0;
	_Field->Status = ENONE;
	_Field->StatusTime = 0;
	_Field->Owner = _Owner;
	return _Field;
}

int FieldCmp(const void* _One, const void* _Two) {
	return ((struct Field*)_One)->Id - ((struct Field*)_Two)->Id;
}

void DestroyField(struct Field* _Field) {
	DestroyObject((struct Object*)_Field);
	free(_Field);
}

int InputReqFieldCmp(const void* _One, const void* _Two) {
	return FieldCmp(((struct InputReq*)_One)->Req, ((struct InputReq*)_Two)->Req);
}

void FieldReset(struct Field* _Crop) {
	_Crop->YieldTotal = 0;
	FieldClearAcres(_Crop);
	_Crop->Status = EFALLOW;
}

int FieldPlant(struct Field* _Field, struct Good* _Seeds) {
	int _TempAcres = _Seeds->Quantity / _Field->Crop->SeedsPerAcre;

	if(_Field->Crop == NULL) {
		FieldReset(_Field);
		return 0;
	}
	if(_Field->Status != EFALLOW)
		return 0;
	if(_TempAcres < _Field->UnusedAcres) {
		int _Total = _Field->Acres + _Field->UnusedAcres;

		_Field->Acres += _TempAcres;
		_Field->UnusedAcres = _Total - _TempAcres;
	} else {
		_Field->Acres = _Field->UnusedAcres;
		_Field->UnusedAcres = _Field->Acres;
	}
	_Seeds->Quantity -= _Field->Crop->SeedsPerAcre * _Field->Acres;
	_Field->Status = EPLOWING - 1;
	FieldChangeStatus(_Field);
	return 1;
}

void FieldWork(struct Field* _Field, int _Total, const struct Array* _Goods) {
	int _Modifier = 1000;

	if(_Field->Status == EGROWING)
		return;
	if(_Field->Status == EPLOWING) {
		const struct ToolBase* _Tool = NULL;

		for(int i = 0; i < _Goods->Size; ++i) {
			_Tool = (struct ToolBase*) ((struct Good*)_Goods->Table[i])->Base;
			if(_Tool->Category != GOOD_TOOL)
				continue;
			if((_Tool->Function & ETOOL_PLOW) != ETOOL_PLOW)
				continue;
			_Modifier = _Tool->Quality;
			break;
		}
	}
//	if(_Field->Status == EHARVESTING) {
	//	const struct ToolBase* _Tool = NULL;

//		for(int i = 0; i < _Goods->Size; ++i) {
//			_Tool = (struct ToolBase*) ((struct Good*)_Goods->Table[i])->Base;
//			if(_Tool->Category != GOOD_TOOL)
//				continue;
//			if((_Tool->Function & ETOOL_REAP) != ETOOL_REAP)
//				continue;
//			_Modifier = _Tool->Quality;
//			break;
//		}
//	}
	_Field->StatusTime -= (_Total * _Modifier) / MAX_WORKRATE;
	if(_Field->StatusTime <= 0) {
		FieldChangeStatus(_Field);
	}
}

int FieldHarvest(struct Field* _Field, struct Array* _Goods, float _HarvestMod) {
	struct Good* _Seeds = NULL;
	int _Quantity = FieldHarvestMod(_Field, _HarvestMod);
	int _SeedQuantity = 0;
	int _TempAcres = 0;

	Assert(_Field->Status == EHARVESTING);
	_Seeds = CheckGoodTbl(_Goods, CropName(_Field->Crop), &g_Goods, _Field->Pos.x, _Field->Pos.y);
	if(_Field->Crop->Type == EGRASS) {
		struct Good* _Straw = NULL;

		_Straw = CheckGoodTbl(_Goods, "Straw", &g_Goods, _Field->Pos.x, _Field->Pos.y);
		_Straw->Quantity += _Quantity * 4;
	}
	_TempAcres = _Field->Acres;
	_Field->Acres = _Field->Acres - (_Field->Acres - _Field->UnusedAcres);
	_Field->UnusedAcres = _TempAcres;
	//FieldReset(_Field);
	if(_Field->Acres > _Field->UnusedAcres)
		_SeedQuantity = ToPound(_Field->Acres * _Field->Crop->SeedsPerAcre);
	else
		_SeedQuantity = ToPound(_Field->UnusedAcres * _Field->Crop->SeedsPerAcre);
	_Seeds->Quantity += _SeedQuantity;
	return (ToPound(_Quantity) - _SeedQuantity) * _Field->Crop->NutVal;
}

void FieldUpdate(struct Field* _Field) {
	if(_Field->Status == EGROWING) {
		int _Temp = g_TemperatureList[TO_MONTHS(g_GameWorld.Date)];

		_Field->StatusTime -= GrowingDegree(_Temp, _Temp, _Field->Crop->GrowingBase);
		if(_Field->StatusTime <= 0) {
			FieldChangeStatus(_Field);
		}
	}
}

void FieldSetAcres(struct Field* _Field, int _Acres) {
	_Field->Acres = 0;
	_Field->UnusedAcres = _Acres;
}

void FieldDivideAcres(struct Field* _Field, int _Acres) {
	int _Div = _Acres / CROP_ROTATIONS;

	_Field->Acres = _Div;
	_Field->UnusedAcres = _Div;
	if((_Acres & 1) == 1)
		++_Field->Acres;
}

void FieldClearAcres(struct Field* _Field) {
	_Field->UnusedAcres = _Field->UnusedAcres + _Field->Acres;
	_Field->Acres = 0;
}

void FieldRotateCrops(struct Field* _Field) {
	int _Temp = _Field->Acres;

	_Field->Acres = _Field->UnusedAcres;
	_Field->UnusedAcres = _Temp;
}

void FieldAcreage(struct Field* _Field, const struct Good* _Seeds) {
	const struct Crop* _Crop = _Field->Crop;
	int _SeedReq = _Crop->SeedsPerAcre * _Field->Acres;
	int _Acres = 0;
	int _TotalAcres = _Field->Acres + _Field->UnusedAcres;

	if(_SeedReq < _Seeds->Quantity)
		return;
	_Acres = _Seeds->Quantity / _Crop->SeedsPerAcre;
	_Field->Acres = _Acres;
	_Field->UnusedAcres = _TotalAcres - _Acres;
}

int GrowingDegree(int _MinTemp, int _MaxTemp, int _BaseTemp) {
	int _Mean = 0;
	if(_MaxTemp > GROWDEG_MAX)
		_MaxTemp = GROWDEG_MAX;
	//if(_MinTemp < _BaseTemp)
	//	_MinTemp = _MinTemp + (_BaseTemp - _MinTemp);
	_Mean = (_MaxTemp + _MinTemp) / 2;
	return (_Mean < _BaseTemp) ? (0) : (_Mean - 32);
}

int CropListAdd(struct LinkedList* _CropList, const struct Array* const _Goods, const struct Crop* _Crop, struct GoodBase* _CropGood, int _NutReq, int _PopCt) {
	struct InputReq* _CropInput = (struct InputReq*) malloc(sizeof(struct InputReq));
	struct InputReq* _CropSearch = NULL;

	_CropInput->Req = NULL;
	_CropInput->Quantity = 0;
	for(int i = 0; i < _Goods->Size; ++i) {
		if(((struct Good*)_Goods->Table[i])->Base->Id == _CropGood->Id) {
			_CropInput->Req = _Crop;
			_CropInput->Quantity =  ceil(((double)_NutReq * _PopCt * YEAR_DAYS) / ((double)_Crop->NutVal * (_Crop->SeedsPerAcre / OUNCE) * (_Crop->YieldMult - 1.0f)));
			if((_CropSearch = (struct InputReq*)LnkLstSearch(_CropList, &_CropInput, InputReqCropCmp)) == NULL)
				LnkLstInsertPriority(_CropList, _CropInput, InputReqQtyCmp);
			else
				_CropSearch->Quantity += _CropInput->Quantity;
			break;
		}
	}
	return (int) _CropInput->Quantity;
}

/*
 * TODO: Families seem to get the same crop for both of their fields.
 */
int SelectCrops(struct Family* _Family, struct Field* _Fields[], int _FieldSz, int _FieldMaxSz) {
	int _TotalAcreage = FieldsGetAcreage(_Fields, _FieldSz) / CROP_ROTATIONS;
	int _Acreage = 0;
	int _AnimalCt = _Family->Animals.Size;
	int _FamilySize = FamilySize(_Family);
	int _NewMaxSz = 0;
	struct InputReq** _AnList = AnimalTypeCount(&_Family->Animals, &_AnimalCt);
	struct LinkedList _Crops = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	double _Ratio = 0;

	if(_AnimalCt <= 0)
		return _NewMaxSz;
	for(int i = 0; g_GameWorld.HumanEats[i] != NULL; ++i) {
		_Acreage = _Acreage + CropListAdd(&_Crops, &_Family->Goods, HashSearch(&g_Crops, g_GameWorld.HumanEats[i]->Name), HashSearch(&g_Goods, g_GameWorld.HumanEats[i]->Name), NUTRITION_DAILY, _FamilySize);
	}
	for(int i = 0; i < _AnimalCt; ++i) {
		//Pick a crop that is needed to feed the population then find out how many acres are needed.
		for(int j = 0; j < ((struct Population*)_AnList[i]->Req)->EatsSize; ++j) {
				struct FoodBase* _Eats = ((struct Population*)_AnList[i]->Req)->Eats[j];

				if(strcmp(_Eats->Name, "Hay") == 0)
					continue;
				_Acreage = _Acreage + CropListAdd(&_Crops, &_Family->Goods, HashSearch(&g_Crops, _Eats->Name), HashSearch(&g_Goods, _Eats->Name), ((struct Population*)_AnList[i]->Req)->Nutrition, _AnList[i]->Quantity);
		}
	}
	//If we don't have enough acres reduce all fields by a certain percentage.
	_Ratio = (double)_TotalAcreage / _Acreage;
	struct LnkLst_Node* _LastItr = NULL;

	_Itr = _Crops.Front;
	while(_Itr != NULL) {
		((struct InputReq*)_Itr->Data)->Quantity = floor(((struct InputReq*)_Itr->Data)->Quantity * _Ratio);
		_TotalAcreage -= ((struct InputReq*)_Itr->Data)->Quantity;
		_LastItr = _Itr;
		_Itr = _Itr->Next;
	}
	if(_TotalAcreage > 0)
		((struct InputReq*)_LastItr->Data)->Quantity += _TotalAcreage;
	if(_Crops.Size > 0)
		_NewMaxSz = PlanFieldCrops(_Fields, &_Crops, _Family, _FieldSz, _FieldMaxSz);
	_Itr = _Crops.Front;
	while(_Itr != NULL) {
		free(_Itr->Data);
		_Itr = _Itr->Next;
	}
	for(int i = 0; i < _AnimalCt; ++i)
		DestroyInputReq(_AnList[i]);
	free(_AnList);
	return _NewMaxSz;
}

int PlanFieldCrops(struct Field* _Fields[], struct LinkedList* _Crops, struct Family* _Family, int _FieldSz, int _FieldMaxSz) {
	struct LnkLst_Node* _Itr = _Crops->Front;
	struct Field* _CurrField = NULL;
	struct Crop* _PlantedCrop = NULL;
	int _Acres = 0;
	int _FieldAcres = 0;
	int _FieldIdx = 0;

	FieldAbosrb(_Fields, _FieldSz);
	_Acres = _Fields[0]->UnusedAcres;
	while(_Itr != NULL) {
		_FieldAcres = ((struct InputReq*)_Itr->Data)->Quantity * CROP_ROTATIONS;
		if(((struct InputReq*)_Itr->Data)->Quantity > 0 && _FieldAcres <= _Acres) {
			_PlantedCrop = (struct Crop*)((struct InputReq*)_Itr->Data)->Req;
			if(_FieldIdx < _FieldMaxSz) {
				_CurrField = _Fields[_FieldIdx];
				_CurrField->Acres = _FieldAcres;
				++_FieldIdx;
			} else {
				return _FieldIdx;
				//_CurrField = CreateField(_Family->HomeLoc->Pos.x, _Family->HomeLoc->Pos.y, NULL, _FieldAcres, _Family);
				//ArrayInsert_S(_Fields, _CurrField);
			}
			_CurrField->Crop = _PlantedCrop;
			_CurrField->Status = EFALLOW;
			_CurrField->Owner = _Family;
			FieldDivideAcres(_CurrField, _CurrField->Acres);
			_Acres = _Acres - _CurrField->Acres;
		}
		_Itr = _Itr->Next;
	}

	for(int i = _FieldIdx; i < _FieldSz; ++i) {
		DestroyField(_Fields[i]);
		_Fields[i] = NULL;
	}
	return _FieldIdx;
}

void FieldAbosrb(struct Field* _Fields[], int _FieldSz) {
	struct Field* _Parent = NULL;
	struct Field* _Field = NULL;

	if(_FieldSz < 1)
		return;
	_Parent = _Fields[0];
	_Parent->UnusedAcres = _Parent->UnusedAcres + _Parent->Acres;
	_Parent->Acres = 0;
	for(int i = 1; i < _FieldSz; ++i) {
		_Field = _Fields[i];
		_Parent->UnusedAcres = _Parent->UnusedAcres + _Field->Acres + _Field->UnusedAcres;
		_Field->UnusedAcres = 0;
		_Field->Acres = 0;
	}
}

void FieldChangeStatus(struct Field* _Field) {
	if(_Field->Status == EHARVESTING) {	
		_Field->Status = EFALLOW;
	} else {
		if(_Field->Owner != NULL)
			PushEvent(EVENT_FARMING, _Field->Owner, _Field);
		++_Field->Status;
		switch(_Field->Status) {
			case EPLANTING:
				_Field->StatusTime = _Field->Acres * PLANT_TIME * DAYSWORK;
				break;
			case EGROWING:
				_Field->StatusTime = _Field->Crop->GrowingDegree;
				break;
			case EPLOWING:
				_Field->StatusTime = _Field->Acres * DAYSWORK;
			default:
				_Field->StatusTime = _Field->Acres * DAYSWORK;
				break;
		}
	}
}
