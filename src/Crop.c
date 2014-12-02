/*
 * File: Crop.c
 * Author: David Brotz
 */

#include "Crop.h"

#include "Good.h"
#include "Herald.h"
#include "World.h"
#include "sys/RBTree.h"
#include "sys/LuaHelper.h"
#include "sys/Log.h"
#include "sys/Event.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define NextStatus(_Crop)															\
{																					\
	if(_Crop->Status == EHARVESTING) {												\
		_Crop->Status = EFALLOW;													\
	} else {																		\
		EventPush(CreateEventFarming(_Crop->X, _Crop->Y, _Crop->Status, _Crop));	\
		++_Crop->Status;															\
		_Crop->StatusTime = _Field->Acres * WORKMULT;								\
	}																				\
}

struct Crop* CreateCrop(const char* _Name, int _Type, int _PerAcre, int _NutVal, double _YieldMult, int _GrowDays) {
	struct Crop* _Crop = (struct Crop*) malloc(sizeof(struct Crop));
	struct GoodBase* _Good = NULL;

	_Crop->Id = NextId();
	_Crop->Name = (char*) calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Crop->Name, _Name);
	_Crop->Type = _Type;
	_Crop->PerAcre = _PerAcre;
	_Crop->NutVal = _NutVal;
	_Crop->YieldMult = _YieldMult;
	_Crop->GrowDays = _GrowDays;
	if((_Good = HashSearch(&g_Goods, _Name)))
		DestroyGoodBase(_Good);
	HashInsert(&g_Goods, _Name, CreateFoodBase(_Name, ESEED, _NutVal));	
	return _Crop;
}

struct Crop* CopyCrop(const struct Crop* _Crop) {
	struct Crop* _NewCrop = (struct Crop*) malloc(sizeof(struct Crop));

	_NewCrop->Id = NextId();
	_NewCrop->Name = (char*) calloc(strlen(_Crop->Name) + 1, sizeof(char));
	strcpy(_NewCrop->Name, _Crop->Name);
	_NewCrop->Type = _Crop->Type;
	_NewCrop->PerAcre = _Crop->PerAcre;
	_NewCrop->NutVal = _Crop->NutVal;
	_NewCrop->YieldMult = _Crop->YieldMult;
	_NewCrop->GrowDays = _Crop->GrowDays;
	return _NewCrop;
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
		lua_pop(_State, 1);
		if(!(_Return > 0)) {
			Log(ELOG_WARNING, "%s is not a crop parameter.", _Key);
			return NULL;
		}
	}
	return CreateCrop(_Name, _Type, _PerAcre, _NutValue, _YieldMult, _GrowTime);
}

struct Field* CreateField(int _X, int _Y, const struct Crop* _Crop, int _Acres) {
	struct Field* _Field = (struct Field*) malloc(sizeof(struct Field));

	_Field->Id = NextId();
	_Field->X = _X;
	_Field->Y = _Y;
	_Field->Crop = _Crop;
	_Field->YieldTotal = 0;
	_Field->Acres = _Acres;
	_Field->UnusedAcres = 0;
	_Field->Status = EFALLOW;
	_Field->StatusTime = 0;
	return _Field;
}

void DestroyField(struct Field* _Field) {
	free(_Field);
}

void FieldReset(struct Field* _Crop) {
	_Crop->YieldTotal = 0;
	_Crop->Acres = 0;
	_Crop->Status = EFALLOW;
}

int FieldPlant(struct Field* _Field, struct Good* _Seeds) {
	if(_Field->Crop == NULL) {
		FieldReset(_Field);
		return 0;
	}
	if(_Field->Status != EFALLOW)
		return 0;

	if(_Seeds->Quantity < _Field->Crop->PerAcre * _Field->Acres || strcmp(_Seeds->Base->Name, _Field->Crop->Name) != 0) {
		int _Quantity = _Seeds->Quantity;
		int _Acres = 0;

		while(_Quantity > _Field->Crop->PerAcre) {
			_Quantity -= _Field->Crop->PerAcre;
			++_Acres;
		}
		_Field->UnusedAcres += _Field->Acres - _Acres;
		_Field->Acres = _Acres;
	}
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
			_Field->YieldTotal += (double)_Val / (double)_Field->Crop->GrowDays;
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

void FieldHarvest(struct Field* _Field, struct Good* _Seeds) {
	if(_Field->Status != EHARVESTING)
		return;
	_Field->Acres = _Field->Acres - (_Field->Acres - _Field->StatusTime);
	_Seeds->Quantity = TO_POUND(_Field->Acres * _Field->Crop->PerAcre / WORKMULT);
	FieldReset(_Field);
}

int FieldUpdate(struct Field* _Field) {
	if(_Field->Status == EGROWING) {
		if(--_Field->StatusTime <= 0)
			return 1;
		return 0;
	}
	return 1;
}
