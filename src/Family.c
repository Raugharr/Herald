/*
 * Author: David Brotz
 * File: Family.c
 */

#include "Family.h"

#include "Person.h"
#include "Profession.h"
#include "Herald.h"
#include "Crop.h"
#include "Good.h"
#include "Population.h"
#include "BigGuy.h"
#include "Location.h"
#include "LuaFamily.h"
#include "Retinue.h"

#include "sys/Event.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Math.h"
#include "sys/LuaCore.h"
#include "sys/Log.h"
#include "sys/RBTree.h"
#include "sys/ITree.h"

#include "AI/LuaLib.h"
#include "AI/Setup.h"

#include <stdlib.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <assert.h>
#include <lua/lauxlib.h>

static struct Array* g_FirstNames = NULL;

/*
 * TODO: Remove Family_Init and Family_Quit.
 */
void Family_Init(struct Array* _Array) {
	g_FirstNames = _Array;
}

void Family_Quit() {
	DestroyArray(g_FirstNames);
}

struct Family* CreateFamily(const char* _Name, struct Settlement* _Location, struct Family* _Parent) {
	struct Family* _Family = (struct Family*) malloc(sizeof(struct Family));

	_Family->Name = _Name;
	memset(_Family->People, 0, sizeof(struct Person*) * (FAMILY_PEOPLESZ));
	_Family->NumChildren = 0;
	CtorArray(&_Family->Goods, 16);
	CtorArray(&_Family->Animals, 0);
	SettlementPlaceFamily(_Location, _Family);
	_Family->HomeLoc = _Location;
	_Family->Parent = _Parent;
	_Family->Owner = NULL;
	_Family->Profession = NULL;
	_Family->Caste = NULL;
	_Family->FieldCt = 0;
	_Family->BuildingCt = 0;
	for(int i = 0; i < FAMILY_BUILDINGCT; ++i)
		_Family->Buildings[i] = NULL;
	for(int i = 0; i < FAMILY_FIELDCT; ++i)
		_Family->Fields[i] = NULL;
	_Family->Food.SlowSpoiled = 0;
	_Family->Food.FastSpoiled = 0;
	_Family->Food.AnimalFood = 0;
	CreateObject(&_Family->Object, OBJECT_FAMILY, FamilyThink);
	return _Family;
}

struct Family* CreateRandFamily(const char* _Name, int _Size, struct Family* _Parent, struct Constraint * const * const _AgeGroups,
	 struct Constraint * const * const _BabyAvg, int _X, int _Y, struct Settlement* _Location, struct FamilyType** _FamilyTypes, const struct Caste* _Caste) {
	struct Family* _Family = NULL;

	if(_Size > FAMILY_PEOPLESZ)
		return NULL;
	if(_Size >= 2) {
		struct Person* _Wife = NULL;

		_Family = CreateFamily(_Name, _Location, _Parent);
		_Family->Caste = _Caste;
		//FamilyAddGoods(_Family, _Size, g_LuaState, _FamilyTypes, _Location);
		_Family->People[HUSBAND] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(_AgeGroups[TEENAGER]->Min, _AgeGroups[ADULT]->Max), EMALE, 1500, _X, _Y, _Family);
		_Wife = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(_AgeGroups[TEENAGER]->Min, _AgeGroups[ADULT]->Max), EFEMALE, 1500, _X, _Y, _Family);
		_Family->People[WIFE] = _Wife;
		_Size -= 2;
		while(_Size-- > 0) {
			int _Child = CHILDREN + _Family->NumChildren;

			_Family->People[_Child] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(0, (_Wife->Age - _AgeGroups[TEENAGER]->Min)), Random(1, 2), 1500, _X, _Y, _Family);
			++_Family->NumChildren;
		}
	}
	_Family->FieldCt = SelectCrops(_Family, _Family->Fields, _Family->FieldCt, FAMILY_FIELDCT);
	return _Family;
}

void DestroyFamily(struct Family* _Family) {
	int _Max = _Family->NumChildren + 1;
	struct Array* _Array = &_Family->Goods;

	for(int i = 0; i < _Array->Size; ++i) {
		DestroyGood(_Array->Table[i]);
	}
	while(_Max >= 0) {
		if(_Family->People[_Max] != NULL) {
			DestroyPerson(_Family->People[_Max]);
			_Family->People[_Max] = NULL;
		}
		--_Max;
	}
	free(_Family);
}

/*
 * FIXME: Shouldn't return a Food* as it is not used by anything that calls this function.
 */
struct Food* FamilyMakeFood(struct Family* _Family) {
	int _FoodSize;
	struct InputReq** _Foods = GoodBuildList(&_Family->Goods, &_FoodSize, GOOD_FOOD | GOOD_SEED | GOOD_INGREDIENT);
	struct Array* _GoodsArray = NULL;
	struct FoodBase* _Food = NULL;
	struct Food* _FamFood = NULL;
	struct Good* _Good = NULL;
	int i = 0;

	if(_Foods == NULL)
		return NULL;

	for(i = 0; i < _FoodSize; ++i) {
		_Food = ((struct FoodBase*)_Foods[i]->Req);
		for(int j = 0; j < _Food->IGSize; ++j) {
			_Good = LinearSearch(_Food->InputGoods[j], _Family->Goods.Table, _Family->Goods.Size, (int(*)(const void*, const void*))InputReqGoodCmp);
			_Good->Quantity -= _Foods[i]->Quantity * _Food->InputGoods[j]->Quantity;
			_GoodsArray = &_Family->Goods;
			if((_FamFood = LinearSearch(_Food, _GoodsArray->Table, _GoodsArray->Size, (int(*)(const void*, const void*))GoodGBaseCmp)) == NULL) {
				_FamFood =  CreateFood(_Food, _Family->People[1]->Pos.x, _Family->People[1]->Pos.y);
				ArrayInsert_S(_GoodsArray, _FamFood);
			}
			_FamFood->Quantity += _Foods[i]->Quantity;
			goto end;
		}
		DestroyInputReq(_Foods[i]);
	}
	if(i == 0)
		Log(ELOG_WARNING, "Day %i: %i made no food in PAIMakeFood.", DateToDays(g_GameWorld.Date), _Family->Object.Id);
	end:
	free(_Foods);
	if(_FamFood->Base->Category != GOOD_FOOD)
		FamilyMakeFood(_Family);
	return _FamFood;
}

void FamilyWorkField(struct Family* _Family) {
	struct Field* _Field = NULL;
	struct Array* _Array = NULL;

	for(int i = 0; i < _Family->FieldCt; ++i) {
		_Field = _Family->Fields[i];
		if(_Field == NULL)
			break;
		if(_Field->Status != ENONE && _Field->Status != EFALLOW) {
			const struct Crop* _WildCrop = HashSearch(&g_Crops, "Hay");

			_Family->Food.AnimalFood += _Field->UnusedAcres * ToPound(_WildCrop->SeedsPerAcre) * _WildCrop->NutVal;
		}
		if(_Field->Status == ENONE) {
			_Family->FieldCt = SelectCrops(_Family, _Family->Fields, _Family->FieldCt, FAMILY_FIELDCT);
		} else if(_Field->Status == EFALLOW && MONTH(g_GameWorld.Date) >= MARCH && MONTH(g_GameWorld.Date) <= APRIL) {
			_Array = &_Family->Goods;
			for(int j = 0; j < _Array->Size; ++j) {
				if(strcmp(((struct Good*)_Array->Table[j])->Base->Name, CropName(_Field->Crop)) == 0) {
					FieldPlant(_Field, _Array->Table[j]);
					break;
				}
			}
		 } else if(_Field->Status == EPLANTING || _Field->Status == EPLOWING || _Field->Status == EHARVESTING) {
			int _Total = FamilyWorkModifier(_Family);
			
			_Field->StatusTime -= _Total;
			if(_Field->StatusTime <= 0) {
				if(_Field->Status == EHARVESTING)
					_Family->Food.SlowSpoiled += FieldHarvest((struct Field*)_Field, &_Family->Goods, HarvestModifier(&_Family->HomeLoc->HarvestMod));
				FieldChangeStatus(_Field);
			}
		}
	}
}

void FamilyCraftGoods(struct Family* _Family) {
	const struct GoodBase* _Good = NULL;
	struct BuyRequest* _BuyReq = _Family->HomeLoc->BuyOrders;
	int _Quantity = 0;

	while(_BuyReq != NULL) {
		for(int i = 0; _Family->Profession->CraftedGoods[i] != NULL; ++i) {
			_Good = _Family->Profession->CraftedGoods[i];
			if(GoodBaseCmp(_BuyReq->Base, _Good) == 0) {
				if((_Quantity = GoodCanMake(_Good, &_Family->Goods)) > 0) {
				GoodMake(_Good, _Quantity, &_Family->Goods, _Family->HomeLoc->Pos.x, _Family->HomeLoc->Pos.y);
				}
			}
		}
		_BuyReq = _BuyReq->Next;
	}
}

void FamilyObjThink(struct Object* _Obj) {
	struct Object* _Front = _Obj;

	while(_Obj != NULL) {
		FamilyThink(_Obj);
		_Obj = _Obj->Next;
	}
	if(DAY(g_GameWorld.Date) == 0) {
		for(_Obj = _Front; _Obj != NULL; _Obj = _Obj->Next) {
			struct Family* _Family = (struct Family*) _Obj;

			if(FamilyGetNutrition(_Family) / FamilyNutReq(_Family) <= 31)
				PushEvent(EVENT_STARVINGFAMILY, _Family, NULL);
			if(_Family->Caste->Type == CASTE_WARRIOR) {
				struct BigGuy* _Guy = RBSearch(&g_GameWorld.BigGuys, _Family->People[0]);
				struct Retinue* _Retinue = NULL; 

				if(_Guy == NULL)
					break;
				_Retinue = IntSearch(&g_GameWorld.PersonRetinue, _Family->People[0]->Object.Id);
				if(_Guy != NULL && _Retinue->Leader->Person != _Family->People[0]) {
					struct BigGuyRelation* _Rel = BigGuyGetRelation(_Guy, _Retinue->Leader);

					if(Random(-5, 5) + _Rel->Modifier + ((int)_Retinue->Leader->Glory) - _Guy->Glory <= 0) {
						for(struct LnkLst_Node* _Itr = g_GameWorld.Settlements.Front; _Itr != NULL; _Itr = _Itr->Next) {
							struct Settlement* _Settlement = _Itr->Data;
							
							if(_Settlement == _Family->HomeLoc) {
								for(struct Retinue* _NewRet = _Settlement->Retinues; _NewRet != NULL; _NewRet = _NewRet->Next) {
									if(_NewRet->Leader->Glory > _Retinue->Leader->Glory) {
										IntSearchNode(&g_GameWorld.PersonRetinue, _Family->People[0]->Object.Id)->Node.Data = _NewRet;
										RetinueRemoveWarrior(_Retinue, _Family->People[0]);
										RetinueAddWarrior(_NewRet, _Family->People[0]);
										break;		
									}
								}
							}
						}
						//Find another retinue.
					}
				}
			}
		}
	}
}

void FamilyThink(struct Object* _Obj) {
	struct Family* _Family = (struct Family*) _Obj;
	struct Person* _Person = NULL;
	const struct Crop* _Hay = HashSearch(&g_Crops, "Hay");
	int _FallowFood = 0; //Food generated from fallow fields.
	double _Milk = 0;

	if(_Family->IsAlive == false)
		return;

	FamilyWorkField(_Family);
	for(int i = 0; i < _Family->Animals.Size; ++i) {
		struct Animal* _Animal = _Family->Animals.Table[i];
		const struct Population* _PopType = _Animal->PopType;

		if(_Animal->Nutrition <= 0) {
			FamilyTakeAnimal(_Family, i);
			DestroyAnimal(_Animal);
			continue;
		}
		
		if(_Animal->Gender != EFEMALE)
			continue;
		_Milk += _PopType->Milk;
	}
	_Family->Food.FastSpoiled += ((double)ToGallon(_Milk)) * MILK_NUT;
	if(MONTH(g_GameWorld.Date) == MARCH && DAY(g_GameWorld.Date) == 0) {
		struct Animal* _Animal = NULL;
		int _Wool = 0;

		if(_Family->Animals.Size < 1)
			goto escape_months;
		_Animal = _Family->Animals.Table[0];
		for(int i = 0; i < _Family->Animals.Size; ++i) {
			_Animal = (struct Animal*) _Family->Animals.Table[i];
			if(_Animal->PopType->Hair.Shearable != 0) {
				_Wool = _Wool + _Animal->PopType->Hair.Pounds;
			}
		}
		CheckGoodTbl(&_Family->Goods, "Wool", &g_Goods, _Family->HomeLoc->Pos.x, _Family->HomeLoc->Pos.y);	
	}
	if(MONTH(g_GameWorld.Date) == SEPTEMBER && DAY(g_GameWorld.Date) == 0) {
		const struct Population* _PopType = NULL;
		struct Animal* _Animal = NULL;
		int _MaleCt = 0;
		int _FemaleCt = 0;
		int _Skipped = 0;
		int _DeleteBuf[_Family->Animals.TblSize];
		int _DeleteBufSz = 0;
		int _NewAnimals = 0;

		if(_Family->Animals.Size == 0)
			goto escape_months;
		_PopType = ((struct Animal*)_Family->Animals.Table[0])->PopType;
		for(int i = 0; i < _Family->Animals.Size; ++i) {
			_Animal = _Family->Animals.Table[i];
			if(_Animal->PopType != _PopType) {
				_NewAnimals = AnimalsReproduce(_PopType, _MaleCt, _FemaleCt);
				for(int i = 0; i < _NewAnimals; ++i) {
					FamilyAddAnimal(_Family, CreateAnimal(_PopType, 0, 1500, _Family->HomeLoc->Pos.x, _Family->HomeLoc->Pos.y));
				}
				_PopType = _Animal->PopType;
				_MaleCt = 0;
				_FemaleCt = 0;
				continue;
			}
			if(_Animal->Gender == EMALE) {
				if(_Animal->Age >= _Animal->PopType->Ages[AGE_DEATH]->Min && _MaleCt > 1) {
					_DeleteBuf[_DeleteBufSz++] = i;
				} else {
					++_MaleCt;
				}
			} else {
				if(_Animal->Age >= _Animal->PopType->Ages[AGE_DEATH]->Min) {
					if(_Skipped == 0) {
						++_Skipped;
						++_FemaleCt;
					} else {
						_DeleteBuf[_DeleteBufSz++] = i;
						_Skipped = 0;
					}

				} else {
					++_FemaleCt;
				}
			}
		}
		_NewAnimals = AnimalsReproduce(_PopType, _MaleCt, _FemaleCt);
		for(int i = 0; i < _NewAnimals; ++i) {
			FamilyAddAnimal(_Family, CreateAnimal(_PopType, 0, 1500, _Family->HomeLoc->Pos.x, _Family->HomeLoc->Pos.y));
		}
		for(int i = _DeleteBufSz - 1; i >= 0; --i) {
			DestroyAnimal(FamilyTakeAnimal(_Family, _DeleteBuf[i]));
			_Family->Food.FastSpoiled += _PopType->Meat;
		}
		_Family->Food.AnimalFood = 0;
	}
	escape_months:
	/*
	 * Feed people.
	 */
	for(int j = 0; j < FAMILY_PEOPLESZ; ++j) {
		int _MaxFastFood = 0;
		int _MaxSlowFood = 0;

		if(_Family->People[j] == NULL)
			break;
		_Person = _Family->People[j];
		if(IsChild(_Person) == 1) {
			if(_Family->Food.FastSpoiled > NUTRITION_CHILDDAILY / 2) {
				_MaxFastFood = NUTRITION_CHILDDAILY / 2;
			} else {
				_MaxFastFood = _Family->Food.FastSpoiled;
			}
			_MaxSlowFood = NUTRITION_CHILDDAILY - _MaxFastFood;
		} else {
			if(_Family->Food.FastSpoiled > NUTRITION_DAILY / 2) {
				_MaxFastFood = NUTRITION_DAILY / 2;
			} else {
				_MaxFastFood = _Family->Food.FastSpoiled;
			}
			_MaxSlowFood = NUTRITION_DAILY - _MaxFastFood;
		}
		if(_MaxSlowFood > _Family->Food.SlowSpoiled)
			_MaxSlowFood = _Family->Food.SlowSpoiled;
		_Person->Nutrition += _MaxFastFood + _MaxSlowFood;
		_Family->Food.FastSpoiled -= _MaxFastFood;
		_Family->Food.SlowSpoiled -= _MaxSlowFood;
		if(_Person->Nutrition <= 0) {
			PersonDeath(_Person);
		}
	}
	//Feed animals.
	for(int i = 0; i < _Family->FieldCt; ++i) {
		_FallowFood += _Family->Fields[i]->UnusedAcres * CropAcreHarvest(_Hay) / 180;
	}
	for(int i = 0; i < _Family->Animals.Size; ++i) {
		struct Animal* _Animal = _Family->Animals.Table[i];
		int _EatAmt = _Animal->PopType->Nutrition;

		if(_FallowFood >= _EatAmt) {
			_Animal->Nutrition += _EatAmt;
			_FallowFood -= _EatAmt;
		}
		if(_Animal->Nutrition <= 0) {
			FamilyTakeAnimal(_Family, i);
			DestroyAnimal(_Animal);
		}
	}
	return;
}

int FamilySize(const struct Family* _Family) {
	int _Size = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(_Family->People[i] == NULL)
			continue;
		++_Size;
	}
	return _Size;
}

void Marry(struct Person* _Male, struct Person* _Female) {
	assert(_Male->Gender == EMALE && _Female->Gender == EFEMALE);
	struct Family* _Family = CreateFamily(_Male->Family->Name, FamilyGetSettlement(_Male->Family), _Male->Family);

	_Family->People[HUSBAND] = _Male;
	_Family->People[WIFE] = _Female;

}

void FamilySetCaste(struct Family* _Family, const char* _Caste) {
	if(strcmp(_Caste, "Serf") == 0) {
		_Family->Caste = &g_Castes[CASTE_THRALL];
	} else if(strcmp(_Caste, "Peasant") == 0) {
		_Family->Caste = &g_Castes[CASTE_LOWCLASS];
	} else if(strcmp(_Caste, "Craftsman") == 0) {
		_Family->Caste = &g_Castes[CASTE_HIGHCLASS];
	} else if(strcmp(_Caste, "Warrior") == 0) {
		_Family->Caste = &g_Castes[CASTE_NOBLE];
	}
}

void FamilyAddGoods(struct Family* _Family, int _FamilySize, lua_State* _State, struct FamilyType** _FamilyTypes, struct Settlement* _Location) {
	int _FamType = Random(0, 9999);
	int _Quantity = 0;
	int _X = _Location->Pos.x;
	int _Y = _Location->Pos.y;
	const char* _Name = NULL;
	const struct Population* _Population = NULL;
	struct Profession* _Profession = NULL;
	char* _Error = NULL;
	void* _Obj = NULL;

	for(int i = 0; _FamilyTypes[i] != NULL; ++i) {
		if((_Profession = HashSearch(&g_Professions, _FamilyTypes[i]->LuaFunc)) == NULL) {
			Log(ELOG_INFO, "Family type: %s is not a profession.", _FamilyTypes[i]->LuaFunc);
			continue;
		}
		if(_FamilyTypes[i]->Percent * 10000 > _FamType) {
			Log(ELOG_INFO, "Creating Family type: %s", _FamilyTypes[i]->LuaFunc);
			++g_Log.Indents;
			lua_getglobal(_State, _FamilyTypes[i]->LuaFunc);
			lua_pushinteger(_State, _FamilySize);
			LuaCtor(_State, _Location, LOBJ_SETTLEMENT);
			if(LuaCallFunc(_State, 2, 1, 0) == 0) {
				--g_Log.Indents;
				return;
			}
			lua_pushstring(_State, "Caste");
			lua_rawget(_State, -2);
			if(lua_type(_State, -1) != LUA_TSTRING) {
				return (void) luaL_error(_State, "Family type has invalid caste.");
			}
			FamilySetCaste(_Family, lua_tostring(_State, -1));
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Goods");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				_Obj = LuaCheckClass(_State, -1, LOBJ_GOOD);
				((struct Good*)_Obj)->Pos.x = _X;
				((struct Good*)_Obj)->Pos.y = _Y;
				ArrayInsertSort_S(&_Family->Goods, _Obj, GoodCmp);
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Field");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				int _Acres = lua_tointeger(_State, -1);

				_Location->FreeAcres -= _Acres;	
				_Family->Fields[_Family->FieldCt++] = CreateField(_X, _Y, NULL, _Acres, _Family);
				//ArrayInsert_S(&_Family->Fields, CreateField(_X, _Y, NULL, _Acres, _Family));
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Buildings");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if((_Obj = (struct Building*) LuaToObject(_State, -1, LOBJ_BUILDING))!= NULL)
					_Family->Buildings[_Family->BuildingCt++] = _Obj;
					//ArrayInsertSort_S(&_Family->Buildings, _Obj, ObjectCmp);
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Animals");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				lua_pushnil(_State);
				if(lua_next(_State, -2) == 0) {
					_Error = "Animals";
					goto LuaError;
				}
				LuaGetString(_State, -1, &_Name);
				lua_pop(_State, 1);
				if(lua_next(_State, -2) == 0) {
					_Error = "Animals";
					goto LuaError;
				}
				LuaGetInteger(_State, -1, &_Quantity);
				if((_Population = HashSearch(&g_Populations, _Name)) == NULL) {
					Log(ELOG_WARNING, "Cannot find Population %s.", _Name);
					lua_pop(_State, 3);
					continue;
				}
				for(int j = 0; j < _Quantity; ++j)
					ArrayInsertSort_S(&_Family->Animals, CreateAnimal(_Population, Random(0, _Population->Ages[AGE_DEATH]->Max), _Population->MaxNutrition, _X, _Y), AnimalCmp);
				lua_pop(_State, 3);
			}
			lua_pop(_State, 1);
			lua_getfield(_State, -1, "AI");
			if(lua_type(_State, -1) != LUA_TFUNCTION)
				luaL_error(_State, "Lua function expected, got %s", lua_typename(_State, lua_type(_State, -1)));
			_Family->Profession = _Profession;
			lua_pop(_State, 2);
			--g_Log.Indents;
			break;
		}
		_FamType -= _FamilyTypes[i]->Percent * 10000;
		continue;
		LuaError:
		--g_Log.Indents;
		luaL_error(_State, "In function %s the %s table does not contain a valid element.", _FamilyTypes[i]->LuaFunc, _Error);
		return;
	}
}

void FamilyGetGood(struct Family* _Family, struct Good* _Good, int _Quantity) {
	SDL_assert(_Quantity <= _Good->Quantity);
	for(int i = 0; i < _Family->Goods.Size; ++i)
		if(_Good->Base == ((struct Good*)_Family->Goods.Table[i])->Base) {
			((struct Good*)_Family->Goods.Table[i])->Quantity += _Quantity;
			if(_Quantity >= _Good->Quantity) {
				DestroyGood(_Good);
			} else {
				_Good->Quantity = _Good->Quantity - _Quantity;
			}
			return;
		}
	ArrayInsert(&_Family->Goods, _Good);
}

struct Good* FamilyTakeGood(struct Family* _Family, int _Index, int _Quantity) {
	struct Good* _Good = NULL;

	if(_Index < 0 || _Index >= _Family->Goods.Size)
		return NULL;
	_Good = _Family->Goods.Table[_Index];
	if(_Good->Quantity > _Quantity) {
		_Good->Quantity = _Good->Quantity - _Quantity;;
		_Good = g_GoodCopy[_Good->Base->Category](_Good);
	} else {
		ArrayRemove(&_Family->Goods, _Index);
	}
	return _Good;
}

int FamilyNutReq(const struct Family* _Family) {
	int _Nutrition = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(_Family->People[i] == NULL)
			continue;
		if(IsChild(_Family->People[i]) == 0)
			_Nutrition += NUTRITION_DAILY;
		else
			_Nutrition += NUTRITION_CHILDDAILY;
	}
	return _Nutrition;
}

int FamilyGetNutrition(const struct Family* _Family) {
	return _Family->Food.FastSpoiled + _Family->Food.SlowSpoiled;
}

struct Settlement* FamilyGetSettlement(struct Family* _Family) {
	return _Family->HomeLoc;
}

int FamilyCountAcres(const struct Family* _Family) {
	struct Field* _Field = NULL;
	int _Acres = 0;

	for(int i = 0; i < _Family->FieldCt; ++i) {
		_Field = _Family->Fields[i];
		_Acres = _Acres + _Field->Acres + _Field->UnusedAcres;
	}
	return _Acres;
}

int FamilyExpectedYield(const struct Family* _Family) {
	struct Field* _Field = NULL;
	int _Yield = 0;

	for(int i = 0; i < _Family->FieldCt; ++i) {
		_Field = _Family->Fields[i];
		if(_Field->Crop == NULL)
			continue;
		_Yield = _Yield + ((_Field->Acres * ((_Field->Crop->YieldMult - 1) * (_Field->Crop->SeedsPerAcre / OUNCE)) * _Field->Crop->NutVal)); //-1 to YieldMult as we assume some of the seeds will be used to regrow the crop.
	}
	return _Yield;
}

int FamilyCountAnimalTypes(const struct Family* _Family) {
	const struct Population* _AnimalType[_Family->Animals.Size];
	struct Animal* _Animal = NULL;
	int _TypeCt = 0;

	if(_Family->Animals.Size == 0)
		return 0;
	_AnimalType[_TypeCt++] = ((struct Animal*) _Family->Animals.Table[0])->PopType;
	for(int i = 1; i < _Family->Animals.Size; ++i) {
		_Animal = (struct Animal*) _Family->Animals.Table[i];
		if(_Animal->PopType != _AnimalType[_TypeCt - 1]) {
			_AnimalType[_TypeCt++] = _Animal->PopType;
		}
	}
	return _TypeCt;
}

void FamilySlaughterAnimals(struct Family* _Family) {
	const struct Population* _PopType = NULL;
	struct Animal* _Animal = NULL;
	int _MaleCt = 0;
	int _Skipped = 0;
	int _Meat = 0;;

	if(_Family->Animals.Size == 0)
		return;
	_PopType = ((struct Animal*)_Family->Animals.Table[0])->PopType;
	for(int i = 0; i < _Family->Animals.Size; ++i) {
		_Animal = _Family->Animals.Table[i];
		if(_Animal->PopType != _PopType) {
			_PopType = _Animal->PopType;
			_MaleCt = 0;
			continue;
		}
		if(_Animal->Age >= _Animal->PopType->Ages[AGE_DEATH]->Min) {
			if(_Animal->Gender == EMALE) {
				++_MaleCt;
				if(_MaleCt > 1) {
					_Meat = _Meat + _PopType->Meat;
					DestroyAnimal(FamilyTakeAnimal(_Family, i));
				}
			} else {
				if(_Skipped == 0) {
					++_Skipped;
				} else {
				_Meat = _Meat + _PopType->Meat;
				DestroyAnimal(FamilyTakeAnimal(_Family, i));
				_Skipped = 0;
				}
			}
		}
	}
}

void FamilyShearAnimals(struct Family* _Family) {
	struct Animal* _Animal = NULL;
	int _Wool = 0;

	for(int i = 1; i < _Family->Animals.Size; ++i) {
		_Animal = (struct Animal*) _Family->Animals.Table[i];

		if(_Animal->PopType->Hair.Shearable != 0) {
			_Wool = _Wool + _Animal->PopType->Hair.Pounds * OUNCE;
		}
	}
}

void FamilyPlough(struct Family* _Family) {
	_Family->FieldCt = SelectCrops(_Family, _Family->Fields, _Family->FieldCt, FAMILY_FIELDCT);
}

void FamilyPlant(struct Family* _Family) {
	struct Array* _Array = _Array = &_Family->Goods;
	struct Field* _Field = NULL;

	for(int i = 0; i < _Family->FieldCt; ++i) {
		_Field = _Family->Fields[i];
		for(int j = 0; j < _Array->Size; ++j) {
			if(strcmp(((struct Good*)_Array->Table[j])->Base->Name, CropName(_Field->Crop)) == 0) {
				FieldPlant(_Field, _Array->Table[j]);
				break;
			}
		}
	}
}

void FamilyHunt(struct Family* _Family) {
	
}

int FamilyWorkModifier(const struct Family* _Family) {
	int _WorkMod = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(_Family->People[i] == NULL || _Family->People[i]->Gender == EFEMALE)
			continue;
		_WorkMod = _WorkMod + PersonWorkMult(_Family->People[i]);
	}
	return _WorkMod;
}

int FamilyCanMake(const struct Family* _Family, const struct GoodBase* _Good) {
	//const struct GoodBase* _Base = NULL;

	if(_Good->Category == GOOD_SEED) {
		for(int i = 0; i < _Family->FieldCt; ++i) {
			if(strcmp(CropName(_Family->Fields[i]->Crop), _Good->Name) == 0) {
				return 1;
			}
		}
		return 0;
	}
	//FIXME: Check if their profession can make this good.
	/*_Itr = _Family->Caste->ProduceList.Front;
	while(_Itr != NULL) {
		_Base = (const struct GoodBase*) _Itr->Data;
		if(GoodBaseCmp(_Base, _Good) == 0)
			return 1;
		_Itr = _Itr->Next;
	}*/
	return 0;
}

int FamilyGetWealth(const struct Family* _Family) {
	int _Wealth = 0;

	for(int i = 0; i < _Family->Animals.Size; ++i) {
		_Wealth += ((struct Animal*)_Family->Animals.Table[i])->PopType->Wealth;
	}
	return _Wealth / 100;
}
