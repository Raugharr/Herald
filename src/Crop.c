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

int CropStatusGen(struct Field* _Field) {
	return _Field->Acres * (WORKMULT / 10);
}

int CropStatusGrow(struct Field* _Field) {
	return _Field->Crop->GrowingDegree;
}

int (*g_CropStatusFuncs[])(struct Field* _Field) = {CropStatusGen, CropStatusGen, CropStatusGen, CropStatusGen, CropStatusGrow, CropStatusGen};
static int g_CropId = 0;

#define NextStatus(_Crop)															\
{																					\
	if(_Crop->Status == EHARVESTING) {												\
		_Crop->Status = EFALLOW;													\
	} else {																		\
		EventPush(CreateEventFarming(_Crop->Status, _Crop));	\
		++_Crop->Status;															\
		_Crop->StatusTime = g_CropStatusFuncs[_Crop->Status](_Field);				\
	}																				\
}

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, int _NutVal, double _YieldMult, int _GrowingDegree, int _GrowingBase, int _SurviveWinter) {
	struct Crop* _Crop = NULL;
	struct GoodBase* _Good = NULL;

	if(_Name == NULL || _PerAcre <= 0 || _NutVal <= 0 || _YieldMult <= 0
			|| _GrowingDegree <= 0 || _GrowingBase <= 0) {
		Log(ELOG_WARNING, "Crop %s is missing a parameter.", _Name);
		return NULL;
	}
	_Crop = (struct Crop*) malloc(sizeof(struct Crop));
	_Crop->Id = ++g_CropId;
	_Crop->Name = (char*) calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Crop->Name, _Name);
	_Crop->Type = _Type;
	_Crop->PerAcre = _PerAcre;
	_Crop->NutVal = _NutVal;
	_Crop->YieldMult = _YieldMult;
	_Crop->GrowingDegree = _GrowingDegree;
	_Crop->GrowingBase = _GrowingBase;
	_Crop->SurviveWinter = _SurviveWinter;
	if((_Good = HashSearch(&g_Goods, _Name)))
		DestroyGoodBase(_Good);
	HashInsert(&g_Goods, _Name, CreateFoodBase(_Name, ESEED, _NutVal));	
	return _Crop;
}

void DestroyCrop(struct Crop* _Crop) {
	free(_Crop->Name);
	free(_Crop);
}

struct Crop* CropLoad(lua_State* _State, int _Index) {
	int _Type = 0;
	int _PerAcre = 0;
	int _NutValue = 0;
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
			_Return = LuaGetInteger(_State, -1, &_NutValue);
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
	_Field->Width = _Acres * ACRE_WIDTH;
	_Field->Length = _Acres * ACRE_LENGTH;
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
	_Crop->Acres = 0;
	_Crop->Status = EFALLOW;
}

int FieldPlant(struct Field* _Field, struct Good* _Seeds) {
	int _Total = _Field->Acres + _Field->UnusedAcres;

	if(_Field->Crop == NULL) {
		FieldReset(_Field);
		return 0;
	}
	if(_Field->Status != EFALLOW)
		return 0;
	_Field->Acres = FieldAcreage(_Field, _Seeds) / CROP_ROTATIONS;
	_Field->UnusedAcres = _Total - _Field->Acres;
	_Seeds->Quantity -= _Field->Crop->PerAcre * _Field->Acres;
	EventPush(CreateEventFarming(_Field->Status, _Field));
	_Field->Status = EPLOWING;
	_Field->StatusTime = _Field->Acres * WORKMULT;
	return 1;
}

void FieldWork(struct Field* _Field, int _Total) {
	//int _Val = _Total;

	//if(_Field->Status == EGROWING)
	//	_Field->YieldTotal += (double)_Val / (double)_Field->Crop->GrowingDegree;
	if(_Field->Status == EPLOWING)
		_Total *= 10;
	if(_Field->Status == EGROWING)
		return;
	_Field->StatusTime -= _Total;
	if(_Field->StatusTime <= 0)
		NextStatus(_Field);
}

void FieldHarvest(struct Field* _Field, struct Array* _Goods) {
	struct Good* _Seeds = NULL;
	int _Quantity = _Field->Acres * _Field->Crop->PerAcre;

	if(_Field->Status != EHARVESTING)
		return;
	CheckGoodTbl(_Goods, _Field->Crop->Name, _Seeds, _Field->Pos.x, _Field->Pos.y);
	if(_Field->Crop->Type == EGRASS) {
		struct Good* _Straw = NULL;

		CheckGoodTbl(_Goods, "Straw", _Straw, _Field->Pos.x, _Field->Pos.y);
		_Straw->Quantity += _Quantity * 4;
	}
	_Field->Acres = _Field->Acres - (_Field->Acres - _Field->StatusTime);
	_Seeds->Quantity += _Quantity;
	FieldReset(_Field);
}

void FieldUpdate(struct Field* _Field) {
	if(_Field->Status == EGROWING) {
		int _Temp = g_TemperatureList[TO_MONTHS(g_GameWorld.Date)];

		_Field->StatusTime -= GrowingDegree(_Temp, _Temp, _Field->Crop->GrowingBase);
		if(_Field->StatusTime <= 0) {
			_Field->YieldTotal = 100;
			++_Field->Status;
		}
	}
}

void FieldSetAcres(struct Field* _Field, int _Acres) {
	int _Div = _Acres / CROP_ROTATIONS;

	_Field->Acres = _Div;
	_Field->UnusedAcres = _Div;
	if((_Acres & 1) == 1)
		++_Field->Acres;
}

void FieldRotateCrops(struct Field* _Field) {
	int _Temp = _Field->Acres;

	_Field->Acres = _Field->UnusedAcres;
	_Field->UnusedAcres = _Temp;
}

int FieldAcreage(const struct Field* _Field, const struct Good* _Seeds) {
	return _Field->Acres + _Field->UnusedAcres;
}

int FieldsGetAcreage(const struct Array* _Fields) {
	int i = 0;
	int _Acreage = 0;

	for(i = 0; i < _Fields->Size; ++i)
		_Acreage = FieldTotalAcres(((struct Field*)_Fields->Table[i]));
	return _Acreage / CROP_ROTATIONS;
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
/*
 * TODO: Families seem to get the same crop for both of their fields.
 */
void SelectCrops(struct Family* _Family, struct Array* _Fields) {
	int _TotalAcreage = FieldsGetAcreage(_Fields);
	int _Acreage = 0;
	int _Size = _Family->Animals->Size;
	int _Ct = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	struct InputReq** _AnList = AnimalTypeCount(_Family->Animals, &_Size);
	struct LinkedList _Crops = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	struct InputReq* _Pair = alloca(sizeof(struct InputReq) * (_Size + 1));
	double _Ratio = 0;
	//struct Crop* _MainCrop = HashSearch(&g_Crops, GoodMostAbundant(_Family->Goods, ESEED)->Base->Name);

		//_Pair[_Ct].Req = _MainCrop;
		//_Pair[_Ct].Quantity = ceil(FamilyNutReq(_Family) / ((struct Crop*)_Pair[_Ct].Req)->NutVal / CROP_ROTATIONS); //Divide by 2 because we will grow 2 sets of crops per year. Thus we only need to feed them for half of a year.
		//_Acreage += _Pair[_Ct].Quantity;
		//LnkLstInsertPriority(&_Crops, &_Pair[_Ct++], InputReqQtyCmp);
	if(_Size <= 0)
		return;
	for(i = 0; i < _Size; ++i) {
		//Pick a crop that is needed to feed the population then find out how many acres are needed.
		for(j = 0; j < ((struct Population*)_AnList[i]->Req)->EatsSize; ++j) {
				struct FoodBase* _Eats = ((struct Population*)_AnList[i]->Req)->Eats[j];
				struct Crop* _Crop = NULL;
				struct GoodBase* _CropGood = NULL;
				struct InputReq* _PairSearch = NULL;

				if(strcmp(_Eats->Name, "Straw") == 0  || strcmp(_Eats->Name, "Hay") == 0)
					continue;
				else
					_Crop = HashSearch(&g_Crops, _Eats->Name);
				_CropGood = HashSearch(&g_Goods, _Eats->Name);
				//Ensure there is only one entry for each Crop.
				for(k = 0; k < _Family->Goods->Size; ++k) {
					if(((struct Good*)_Family->Goods->Table[k])->Base->Id == _CropGood->Id) {
						_Pair[_Ct].Req = _Crop;
						_Pair[_Ct].Quantity =  ceil(((double)((struct Population*)_AnList[i]->Req)->Nutrition * _AnList[i]->Quantity) / ((double)_Crop->NutVal));
						_Acreage += _Pair[_Ct].Quantity;
						if((_PairSearch = (struct InputReq*)LnkLstSearch(&_Crops, &_Pair[_Ct], InputReqCropCmp)) == NULL)
							LnkLstInsertPriority(&_Crops, &_Pair[_Ct++], InputReqQtyCmp);
						else
							_PairSearch->Quantity += _Pair[_Ct].Quantity;
						break;
					}
				}
		}
	}
	//If we don't have enough acres reduce all fields by a certain percentage.
	_Ratio = (double)_TotalAcreage / _Acreage;
	if(_Acreage > _TotalAcreage) {
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
	} else {
		_Itr = _Crops.Front;
		while(_Itr != NULL) {
			((struct InputReq*)_Itr->Data)->Quantity = floor(((struct InputReq*)_Itr->Data)->Quantity * _Ratio);
			_Itr = _Itr->Next;
		}
	}
	if(_Crops.Size > 0)
		PlanFieldCrops(_Fields, &_Crops, _Family);
	for(i = 0; i < _Size; ++i)
		DestroyInputReq(_AnList[i]);
	free(_AnList);
}

void PlanFieldCrops(struct Array* _Fields, struct LinkedList* _Crops, struct Family* _Family) {
	int i = 0;
	int j = 0;
	int _FieldSize = 0;
	struct LnkLst_Node* _Itr = _Crops->Front;
	struct Crop* _PlantedCrop = NULL;
	struct Field* _CurrField = NULL;

	for(i = 0; i < _Fields->Size; ++i) {
		_PlantedCrop = (struct Crop*)((struct InputReq*)_Itr->Data)->Req;
		_CurrField = ((struct Field*)_Fields->Table[i]);
		_CurrField->Crop = _PlantedCrop;
		if(((struct InputReq*)_Itr->Data)->Quantity > 0 && ((struct InputReq*)_Itr->Data)->Quantity < _CurrField->Acres) {
			_CurrField->Width = _CurrField->Width - (((struct InputReq*)_Itr->Data)->Quantity * ACRE_WIDTH);
			_FieldSize = ((struct InputReq*)_Itr->Data)->Quantity * CROP_ROTATIONS;
			if(_Itr->Next != NULL) {
				struct Crop* _NextCrop = (struct Crop*)((struct InputReq*)_Itr->Next->Data)->Req;
				for(j = 0; j < i; ++j) {
					if(((struct Field*)_Fields->Table[j])->Crop->Type == _NextCrop->Type)
						goto skip_newfield;
				}
				ArrayInsert_S(_Fields, CreateField(_CurrField->Pos.x, _CurrField->Pos.y + _CurrField->Width + 1, NULL, FieldTotalAcres(_CurrField) - _FieldSize, _Family));
			}
			skip_newfield:
			_CurrField->Acres = _FieldSize;
			_CurrField->Status = EFALLOW;
			FieldSetAcres(_CurrField, _CurrField->Acres);
			((struct InputReq*)_Itr->Data)->Quantity -= _CurrField->Acres;
			_Itr = _Itr->Next;
			if(_Itr == NULL)
				break;
		}
	}
}

void FieldAbosrb(struct Array* _Fields) {
	int i = 0;
	int j = 0;
	struct Field* _Parent = NULL;

	for(i = 0; i < _Fields->Size; ++i) {
		_Parent = ((struct Field*)_Fields->Table[i]);
		for(j = i + 1; j < _Fields->Size;) {
			if(_Parent->Pos.x == ((struct Field*)_Fields->Table[j])->Pos.x && _Parent->Pos.y == ((struct Field*)_Fields->Table[j])->Pos.y - ((struct Field*)_Fields->Table[j])->Width - 1) {
				_Parent->Width += ((struct Field*)_Fields->Table[j])->Width;
				_Parent->Acres += ((struct Field*)_Fields->Table[j])->Acres;
				_Parent->UnusedAcres += ((struct Field*)_Fields->Table[j])->UnusedAcres;
				ArrayRemove(_Fields, j);
			} else
				++j;
		}
	}
}
