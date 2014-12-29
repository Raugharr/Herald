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
#include "sys/LuaHelper.h"
#include "sys/Log.h"
#include "sys/Array.h"
#include "sys/Event.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

int CropStatusGen(struct Field* _Field) {
	return _Field->Acres * WORKMULT;
}

int CropStatusGrow(struct Field* _Field) {
	return _Field->Crop->GrowingDegree;
}

int (*g_CropStatusFuncs[])(struct Field* _Field) = {CropStatusGen, CropStatusGen, CropStatusGen, CropStatusGrow, CropStatusGen};

#define NextStatus(_Crop)															\
{																					\
	if(_Crop->Status == EHARVESTING) {												\
		_Crop->Status = EFALLOW;													\
	} else {																		\
		EventPush(CreateEventFarming(_Crop->X, _Crop->Y, _Crop->Status, _Crop));	\
		++_Crop->Status;															\
		_Crop->StatusTime = g_CropStatusFuncs[_Crop->Status](_Field);				\
	}																				\
}

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, int _NutVal, double _YieldMult, int _GrowingDegree, int _GrowingBase, int _SurviveWinter) {
	struct Crop* _Crop = (struct Crop*) malloc(sizeof(struct Crop));
	struct GoodBase* _Good = NULL;

	_Crop->Id = NextId();
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
	int _GrowTime = 0;
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
			_Return = AddInteger(_State, -1, &_PerAcre);
			_PerAcre *= OUNCE;
		} else if(!strcmp("Type", _Key)) {
			if(lua_isstring(_State, -1)) {
				_Return = AddString(_State, -1, &_TypeStr);
				if(!strcmp("Grass", _TypeStr))
					_Type = EGRASS;
				else
					luaL_error(_State, "Type contains an invalid string.");
			}
		} else if(!strcmp("YieldPerSeed", _Key))
			_Return = AddNumber(_State, -1, &_YieldMult);
		else if(!strcmp("NutritionalValue", _Key))
			_Return = AddInteger(_State, -1, &_NutValue);
		else if(!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("GrowTime", _Key)) {
			_Return = AddInteger(_State, -1, &_GrowTime);
		}
		else if(!strcmp("GrowingBase", _Key)) {
			_Return = AddInteger(_State, -1, &_GrowBase);
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
	return CreateCrop(_Name, _Type, _PerAcre, _NutValue, _YieldMult, _GrowTime, _GrowBase, _SurviveWinter);
}

struct Field* CreateField(int _X, int _Y, const struct Crop* _Crop, int _Acres) {
	struct Field* _Field = NULL;

	_Field = (struct Field*) malloc(sizeof(struct Field));
	CreateObject((struct Object*)_Field, _X, _Y, (int(*)(struct Object*))FieldUpdate);
	_Field->Width = _Acres * ACRE_WIDTH;
	_Field->Length = _Acres * ACRE_LENGTH;
	_Field->Crop = _Crop;
	_Field->YieldTotal = 0;
	_Field->Acres = _Acres;
	_Field->UnusedAcres = 0;
	_Field->Status = EFALLOW;
	_Field->StatusTime = 0;
	return _Field;
}

int FieldCmp(const void* _One, const void* _Two) {
	return ((struct Field*)_One)->Id - ((struct Field*)_Two)->Id;
}

void DestroyField(struct Field* _Field) {
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
	_Field->Acres = FieldAcreage(_Field, _Seeds);
	_Field->UnusedAcres = _Total - _Field->Acres;
	_Seeds->Quantity -= _Field->Crop->PerAcre * _Field->Acres;
	EventPush(CreateEventFarming(_Field->X, _Field->Y, _Field->Status, _Field));
	_Field->Status = EPLOWING;
	_Field->StatusTime = _Field->Acres * WORKMULT;
	return 1;
}

void FieldWork(struct Field* _Field, int _Total, struct Good* _Tool) {
	int _Val = _Total;

	if(_Tool == NULL)
		return;
	switch(_Field->Status) {
		case EFALLOW:
			return;
		case EPLOWING:
			if((((struct ToolBase*)_Tool->Base)->Function & ETOOL_PLOW) != ETOOL_PLOW)
				return;
			break;
		case EGROWING:
			_Field->YieldTotal += (double)_Val / (double)_Field->Crop->GrowingDegree;
			return;
		case EHARVESTING:
			if((((struct ToolBase*)_Tool->Base)->Function & ETOOL_PLOW) != ETOOL_REAP)
				return;
			break;
		default:
			break;
	}
	_Field->StatusTime -= _Total;
	if(_Field->StatusTime <= 0)
		NextStatus(_Field);
}

void FieldHarvest(struct Field* _Field, struct Array* _Goods) {
	struct Good* _Seeds = NULL;
	int _Quantity = _Field->Acres * _Field->Crop->PerAcre;

	if(_Field->Status != EHARVESTING)
		return;
	CheckGoodTbl(_Goods, _Field->Crop->Name, _Seeds, _Field->X, _Field->Y);
	if(_Field->Crop->Type == EGRASS) {
		struct Good* _Straw = NULL;

		CheckGoodTbl(_Goods, "Straw", _Straw, _Field->X, _Field->Y);
		_Straw->Quantity += _Quantity * 4;
	}
	_Field->Acres = _Field->Acres - (_Field->Acres - _Field->StatusTime);
	_Seeds->Quantity += _Quantity;
	FieldReset(_Field);
}

int FieldUpdate(struct Field* _Field) {
	if(_Field->Status == EGROWING) {
		int _Temp = ((struct WorldTile*)g_World->Table[WorldGetTile(_Field->X, _Field->Y)])->Temperature;

		_Field->StatusTime -= GrowingDegree(_Temp, _Temp, _Field->Crop->GrowingBase);
		if(_Field->StatusTime <= 0)
			return 1;
		return 0;
	}
	return 1;
}

void FieldSetAcres(struct Field* _Field, int _Acres) {
	int _Div = _Acres / 2;

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
	int _Quantity = _Seeds->Quantity;
	int _Acres = 0;
	int _TotalAcres = _Field->Acres + _Field->UnusedAcres;

	while(_Quantity > _Field->Crop->PerAcre) {
		_Quantity -= _Field->Crop->PerAcre;
		++_Acres;
	}
	return (_Acres > _TotalAcres) ? (_TotalAcres) : (_Acres);
}

int FieldsGetAcreage(const struct Array* _Fields) {
	int i = 0;
	int _Acreage = 0;

	for(i = 0; i < _Fields->Size; ++i)
		_Acreage = ((struct Field*)_Fields->Table[i])->Acres + ((struct Field*)_Fields->Table[i])->UnusedAcres;
	return _Acreage;
}

int GrowingDegree(int _MinTemp, int _MaxTemp, int _BaseTemp) {
	if(_MaxTemp > GROWDEG_MAX)
		_MaxTemp = _MaxTemp - (_MaxTemp - GROWDEG_MAX);
	if(_MinTemp < _BaseTemp)
		_MinTemp = _MinTemp + (_MinTemp - _BaseTemp);
	if(_MinTemp > _MaxTemp)
		return 0;
	return (_MaxTemp - _MinTemp) / 2;
}
/*
 * TODO: All fields that are adjacent to each other position wise should be put into one field.
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
	struct LinkedList* _Crops = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;
	struct InputReq* _Pair = alloca(sizeof(struct InputReq) * (_Size + 1));

	_Pair[_Ct].Req = HashSearch(&g_Crops, "Wheat");
	_Pair[_Ct].Quantity = ceil(FamilyNutReq(_Family) / ((struct Crop*)_Pair[_Ct].Req)->NutVal / 2); //Divide by 2 because we will grow 2 sets of crops per year. Thus we only need to feed them for half of a year.
	_Acreage += _Pair[_Ct].Quantity;
	LnkLstInsertPriority(_Crops, &_Pair[_Ct++], InputReqQtyCmp);
	if(_Size <= 0)
		goto skip_anloop;
	for(i = 0; i < g_AnFoodDep->Size; ++i) {
		if(((struct AnimalDep*)g_AnFoodDep->Table[i])->Tbl[1] != NULL)
			continue;
		for(j = 0; j < _Size; ++j) {
			//Pick a crop that is needed to feed the population then find out how many acres are needed.
			for(k = 0; j < ((struct Population*)_AnList[j]->Req)->EatsSize; ++k)
				if(((struct Population*)_AnList[j]->Req)->Eats[k]->Id == ((struct AnimalDep*)g_AnFoodDep->Table[i])->Tbl[0]->Id) {
					struct FoodBase* _Eats = ((struct Population*)_AnList[j]->Req)->Eats[k];
					struct Crop* _Crop = NULL;
					struct InputReq* _PairSearch = NULL;

					if(strcmp(_Eats->Name, "Straw") == 0)
						_Crop = HashSearch(&g_Crops, "Rye");
					else
						_Crop = HashSearch(&g_Crops, _Eats->Name);
					_Pair[_Ct].Req = _Crop;
					_Pair[_Ct].Quantity =  ceil(((struct Population*)_AnList[j]->Req)->Nutrition * _AnList[j]->Quantity / _Crop->NutVal);
					//Ensure there is only one entry for each Crop.
					if((_PairSearch = (struct InputReq*)LnkLstSearch(_Crops, &_Pair[_Ct], NULL)) == NULL)
						LnkLstInsertPriority(_Crops, &_Pair[_Ct++], InputReqQtyCmp);
					else
						_PairSearch->Quantity += _Pair[_Ct++].Quantity;
					_Acreage += _Pair[_Ct].Quantity;
				}
		}
	}
	skip_anloop:
	//If we don't have enough acres reduce all fields by a certain percentage.
	if(_Acreage > _TotalAcreage) {
		double _Ratio = (double)_TotalAcreage / _Acreage;

		_Itr = _Crops->Front;
		while(_Itr != NULL) {
			((struct InputReq*)_Itr->Data)->Quantity = floor(((struct InputReq*)_Itr->Data)->Quantity * _Ratio);
			_Itr = _Itr->Next;
		}
	}
	PlanFieldCrops(_Fields, _Crops);
	for(i = 0; i < _Size; ++i)
		DestroyInputReq(_AnList[i]);
	free(_AnList);
	DestroyLinkedList(_Crops);
}

void PlanFieldCrops(struct Array* _Fields, struct LinkedList* _Crops) {
	int i = 0;
	struct LnkLst_Node* _Itr = _Crops->Front;
	struct Crop* _Data = NULL;
	struct Field* _CurrField = NULL;

	_Data = (struct Crop*)((struct InputReq*)_Itr->Data)->Req;
	for(i = 0; i < _Fields->Size; ++i) {
		_CurrField = ((struct Field*)_Fields->Table[i]);

		_CurrField->Crop = _Data;
		FieldSetAcres(_CurrField, _CurrField->Acres);
		if(((struct InputReq*)_Itr->Data)->Quantity > 0 && ((struct InputReq*)_Itr->Data)->Quantity < _CurrField->Acres) {
			/*_CurrField->Acres = _CurrField->Acres - ((struct InputReq*)_Itr->Data)->Quantity;
			_CurrField->Width = _CurrField->Width - (((struct InputReq*)_Itr->Data)->Quantity * ACRE_WIDTH);
			ArrayInsert_S(_Fields, CreateField(_CurrField->X, _CurrField->Y + _CurrField->Width + 1, _Data, ((struct InputReq*)_Itr->Data)->Quantity));*/
			_Itr = _Itr->Next;
			if(_Itr == NULL)
				break;
			_Data = (struct Crop*)((struct InputReq*)_Itr->Data);
		}
		((struct InputReq*)_Itr->Data)->Quantity -= _CurrField->Acres;
	}
}
