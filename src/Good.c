/*
 * File: Good.c
 * Author: David Brotz
 */

#include "Good.h"

#include "Person.h"
#include "Herald.h"
#include "World.h"
#include "Crop.h"
#include "Location.h"
#include "Family.h"

#include "sys/Log.h"
#include "sys/LuaCore.h"
#include "sys/Array.h"
#include "sys/LinkedList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <malloc.h>

struct GoodOutput** g_GoodOutputs = NULL;
int g_GoodOutputsSz = 0;
static int g_GoodId = 0;
/*
 * TODO: Replace with a simple GoodCopy.
 */
struct Good*(*g_GoodCopy[])(const struct Good*) = {
	FoodGoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy,
	GoodCopy
};

static const char* g_GoodCatStr[] = {
	"Food",
	"Ingredient",
	"Animal",
	"Seed",
	"Tool",
	"Material",
	"Clothing",
	"Weapon",
	"Armor",
	"Other",
	NULL
};

int GoodDepCmp(const struct GoodDep* _One, const struct GoodDep* _Two) {
	return _One->Good->Id - _Two->Good->Id;
}

int GoodBaseDepCmp(const struct GoodBase* _Good, const struct GoodDep* _Pair) {
	return _Good->Id - _Pair->Good->Id;
}

int InputReqGoodCmp(const struct InputReq* _One, const struct Good* _Two) {
	return ((struct GoodBase*)_One->Req)->Id - _Two->Base->Id;
}

struct GoodBase* InitGoodBase(struct GoodBase* _Good, const char* _Name, int _Category) {
	_Good->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	strcpy(_Good->Name, _Name);
	_Good->Category = _Category;
	_Good->Id = NextId();
	_Good->InputGoods = NULL;
	_Good->IGSize = 0;
	return _Good;
}

struct GoodBase* CopyGoodBase(const struct GoodBase* _Good) {
	int i;
	struct GoodBase* _NewGood = (struct GoodBase*) malloc(sizeof(struct GoodBase));

	_NewGood->Name = (char*) malloc(sizeof(char) * strlen(_Good->Name) + 1);
	_NewGood->Category = _Good->Category;
	_NewGood->Id = _Good->Id;
	strcpy(_NewGood->Name, _Good->Name);
	_NewGood->InputGoods = calloc(_Good->IGSize, sizeof(struct InputReq*));
	_NewGood->IGSize = _Good->IGSize;
	for(i = 0; i < _Good->IGSize; ++i) {
		struct InputReq* _Req = CreateInputReq();
		_Req->Req = ((struct InputReq*)_Good->InputGoods[i])->Req;
		_Req->Quantity = ((struct InputReq*)_Good->InputGoods[i])->Quantity;
		_NewGood->InputGoods[i] = _Req;
	}
	return _NewGood;
}

int GoodBaseCmp(const void* _One, const void* _Two) {
	return ((struct GoodBase*)_One)->Id - ((struct GoodBase*)_Two)->Id;
}

void DestroyGoodBase(struct GoodBase* _Good) {
	int i;

	for(i = 0; i < _Good->IGSize; ++i)
		DestroyInputReq(_Good->InputGoods[i]);
	free(_Good->Name);
	free(_Good->InputGoods);
	free(_Good);
}

struct BuyRequest* CreateBuyRequest(struct Family* _Family, const struct GoodBase* _Base, int _Quantity) {
	struct BuyRequest* _BuyReq = (struct BuyRequest*) malloc(sizeof(struct BuyRequest));

	SDL_assert(_Base != NULL);
	_BuyReq->Time = g_GameWorld.Date;
	_BuyReq->Base = _Base;
	_BuyReq->Owner = _Family;
	_BuyReq->Quantity = _Quantity;
	ILL_CREATE(_Family->HomeLoc->BuyOrders, _BuyReq);
	return _BuyReq;
}

void DestroyBuyRequest(struct BuyRequest* _BuyReq) {
	ILL_DESTROY(_BuyReq->Owner->HomeLoc->BuyOrders, _BuyReq);
	free(_BuyReq);
}

struct SellRequest* CreateSellRequest(const struct Family* _Seller, const struct GoodBase* _Base, int _Quantity, int _Cost) {
	struct SellRequest* _SellReq = (struct SellRequest*) malloc(sizeof(struct SellRequest));

	_SellReq->Base = _Base;
	_SellReq->Owner = _Seller;
	_SellReq->Quantity = _Quantity;
	_SellReq->Cost = _Cost;
	ILL_CREATE(_Seller->HomeLoc->Market, _SellReq);
	return _SellReq;
}

void DestroySellRequest(struct SellRequest* _SellReq) {
	free(_SellReq);
}

struct Good* GoodCopy(const struct Good* _Good) {
	struct Good* _NewGood = CreateGood(_Good->Base, _Good->Pos.x, _Good->Pos.y);

	return _NewGood;
}

struct Good* FoodGoodCopy(const struct Good* _Good) {
	struct Good* _NewGood = (struct Good*) CreateFood((struct FoodBase*)_Good->Base, _Good->Pos.x, _Good->Pos.y);

	((struct Food*)_NewGood)->Parts = ((struct Food*)_Good)->Parts;
	return _NewGood;
}

int GoodInpGdCmp(const void* _One, const void* _Two) {
	return ((struct GoodBase*)((struct InputReq*)_One)->Req)->Id - ((struct GoodBase*)((struct InputReq*)_Two)->Req)->Id;
}

int GoodThink(struct Good* _Good) {
	return 1;
}

struct GoodBase* GoodLoad(lua_State* _State, int _Index) {
	struct GoodBase* _Good = NULL;
	const char* _Name = NULL;
	const char* _Temp = NULL;
	int _Category = 0;
	int _Return = -2;
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2)) {
			if(!strcmp("Name", lua_tostring(_State, -2)))
				_Return = LuaGetString(_State, -1, &_Name);
			else if(!strcmp("Category", lua_tostring(_State, -2))) {
					if(lua_isstring(_State, -1) == 1) {
						_Temp = lua_tostring(_State, -1);
						_Return = 1;
					if(!strcmp("Food", _Temp))
						_Category = GOOD_FOOD;
					else if(!strcmp("Ingredient", _Temp))
						_Category = GOOD_INGREDIENT;
					else if(!strcmp("Animal", _Temp))
						_Category = GOOD_ANIMAL;
					else if(!strcmp("Seed", _Temp))
						_Category = GOOD_SEED;
					else if(!strcmp("Tool", _Temp))
						_Category = GOOD_TOOL;
					else if(!strcmp("Material", _Temp))
						_Category = GOOD_MATERIAL;
					else if(!strcmp("Clothing", _Temp))
						_Category = GOOD_CLOTHING;
					else if(!strcmp("Weapon", _Temp))
						_Category = GOOD_WEAPON;
					else if(!strcmp("Armor", _Temp))
						_Category = GOOD_ARMOR;
					else if(!strcmp("Other", _Temp))
						_Category = GOOD_OTHER;
					else _Return = -1;
					}
				if(_Category <= 0 && _Return <= 0) {
					Log(ELOG_WARNING, "%s is not a valid category.", _Temp);
					goto fail;
				}
			}
		}
		lua_pop(_State, 1);
	}
	if(_Name == NULL)
		goto fail;
	if(_Category == GOOD_TOOL) {
		int _Function = 0;
		int _Quality = 0;

		lua_pushstring(_State, "Function");
		lua_rawget(_State, -2);
		if(LuaGetString(_State, -1, &_Temp) == 0) {
			Log(ELOG_WARNING, "Tool requires Function field.");
			goto fail;
		}
		lua_pop(_State, 1);
		lua_pushstring(_State, "Quality");
		lua_rawget(_State, -2);
		if(LuaGetInteger(_State, -1, &_Quality) == 0) {
			Log(ELOG_WARNING, "Tool requires Quality field.");
			goto fail;
		}
		lua_pop(_State, 1);
		if(!strcmp(_Temp, "Plow"))
			_Function = ETOOL_PLOW;
		else if(!strcmp(_Temp, "Reap"))
			_Function = ETOOL_REAP;
		else if(!strcmp(_Temp, "Cut"))
			_Function = ETOOL_CUT;
		else if(!strcmp(_Temp, "Logging"))
			_Function = ETOOL_LOGGING;
		else {
			Log(ELOG_WARNING, "Tool contains an invalid Function field: %s", _Temp);
			goto fail;
		}
		_Good = (struct GoodBase*) CreateToolBase(_Name, _Category, _Function, _Quality);
	} else if(_Category == GOOD_FOOD || _Category == GOOD_INGREDIENT || _Category == GOOD_SEED) {
		_Good = (struct GoodBase*) CreateFoodBase(_Name, _Category, 0);
	} else if(_Category == GOOD_CLOTHING) {
		int* _Locations = NULL;

		_Good = (struct GoodBase*) CreateClothingBase(_Name, _Category);
		ClothingBaseLoad(_State, _Good, _Locations);
	} else if(_Category == GOOD_WEAPON) {
		_Good = (struct GoodBase*) CreateWeaponBase(_Name, _Category);
		if(WeaponBaseLoad(_State, (struct WeaponBase*)_Good) == 0) {
			_Top = lua_gettop(_State);
			DestroyWeaponBase((struct WeaponBase*)_Good);
			return NULL;
		}
	//} else if(_Category == EARMOR) {
		//_Good = (struct GoodBase*) CreateArmorBase(_Name, _Category);
		//if(ArmorBaseLoad(_State, (struct ArmorBase*)_Good) == 0) {

		//}
	} else
		_Good = InitGoodBase((struct GoodBase*)malloc(sizeof(struct GoodBase)), _Name, _Category);
	if(_Return > 0) {
		return _Good;
	}
	fail:
	if(_Good != NULL)
		DestroyGoodBase(_Good);
	lua_settop(_State, _Top);
	return NULL;
}

int GoodLoadInput(lua_State* _State, struct GoodBase* _Good) {
	const char* _Name = NULL;
	int _Top = lua_gettop(_State);
	int i;
	struct InputReq* _Req = NULL;
	struct LinkedList _List = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;

	if(_Good == NULL)
		return 0;
	if(HashSearch(&g_Crops, _Good->Name) != NULL || _Good->InputGoods != NULL || _Good->IGSize > 0)
		return 1;

	lua_getglobal(_State, "Goods");
	lua_pushstring(_State, _Good->Name);
	lua_rawget(_State, -2);
	SDL_assert(lua_type(_State, -1) == LUA_TTABLE);
	lua_remove(_State, -2);
	lua_pushstring(_State, "InputGoods");
	lua_rawget(_State, -2);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushnil(_State);
		if(lua_type(_State, -2) != LUA_TTABLE) {
			Log(ELOG_WARNING, "Good %s's InputGoods contains a non table element.", _Good->Name);
			goto fail;
		}
		if(lua_next(_State, -2) == 0)
			goto fail;
		if(lua_isstring(_State, -1) == 1) {
			_Req = CreateInputReq();
			_Name = lua_tostring(_State, -1);
			if(strcmp(_Good->Name, _Name) == 0) {
				Log(ELOG_WARNING, "Good %s is attempting to make itself its own input good.", _Good->Name);
				goto fail;
			}
			if((_Req->Req = HashSearch(&g_Goods, _Name)) != NULL) {
				lua_pop(_State, 1);
				if(lua_next(_State, -2) == 0)
					goto fail;
				if(LuaGetNumber(_State, -1, &_Req->Quantity) == -1) {
					goto fail;
				}
				LnkLstPushBack(&_List, _Req);
			} else {
				Log(ELOG_WARNING, "Good %s cannot add %s as an input good: %s does not exist.", _Good->Name, _Name, _Name);
				goto fail;
			}
			lua_pop(_State, 2);
		} else
			lua_pop(_State, 2);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	_Good->IGSize = _List.Size;
	_Good->InputGoods = calloc(_Good->IGSize, sizeof(struct InputReq*));
	_Itr = _List.Front;
	for(i = 0; i < _List.Size; ++i) {
		_Good->InputGoods[i] = (struct InputReq*)_Itr->Data;
		_Itr = _Itr->Next;
	}
	if(_Good->Category == GOOD_FOOD || _Good->Category == GOOD_INGREDIENT || _Good->Category == GOOD_SEED)
		GoodLoadConsumableInput(_State, _Good, &_List);
	lua_pop(_State, 1);
	InsertionSort(_Good->InputGoods, _Good->IGSize, GoodInpGdCmp, sizeof(*_Good->InputGoods));
	LnkLstClear(&_List);
	return 1;
	fail:
	DestroyInputReq(_Req);
	lua_settop(_State, _Top);
	LnkLstClear(&_List);
	HashDelete(&g_Goods, _Good->Name);
	DestroyGoodBase(_Good);
	return 0;
}

int GoodLoadOutput(lua_State* _State, struct GoodBase* _Good) {
	struct GoodBase* _OutputGood = NULL;

	if(_Good == NULL)
		return 0;
	lua_getglobal(_State, "Goods");
	lua_pushstring(_State, _Good->Name);
	lua_rawget(_State, -2);
	SDL_assert(lua_type(_State, -1) == LUA_TTABLE);
	lua_remove(_State, -2);
	lua_pushstring(_State, "OutputGoods");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) == LUA_TNIL) {
		lua_pop(_State, 2);
		return 0;
	}
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_type(_State, -1) != LUA_TTABLE)
			goto loop_end;
		lua_pushinteger(_State, 1);
		lua_rawget(_State, -2);
		if(lua_type(_State, -1) != LUA_TSTRING) {
			lua_pop(_State, 2);
			continue;
		}
		if((_OutputGood = HashSearch(&g_Goods, lua_tostring(_State, -1))) == NULL) {
			lua_pop(_State, 2);
			return 0;
		}
		lua_pop(_State, 1);
		lua_pushinteger(_State, 2);
		lua_rawget(_State, -2);
		if(lua_isnumber(_State, -1) == 0) {
			lua_pop(_State, 2);
			continue;
		}
		lua_pop(_State, 1);
		//_Time = lua_tointeger(_State, -1);
		loop_end:
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	return 1;
}

void GoodLoadConsumableInput(lua_State* _State, struct GoodBase* _Good, struct LinkedList* _List) {
	int i = 0;
	double _Nutrition = 0;

	if(_Good->IGSize == 0) {
		lua_pushstring(_State, "Nutrition");
		lua_rawget(_State, -2);
		if(lua_tointeger(_State, -1) == 0) {
			Log(ELOG_WARNING, "Warning: Good %s cannot be a food because it contains no InputGoods or a field named Nutrition containing an integer.", _Good->Name);
			lua_pop(_State, 1);
			return;
		}
		_Nutrition = lua_tointeger(_State, -1);
		lua_pop(_State, 1);
	} else {
		for(i = 0; i < _List->Size; ++i) {
			GoodLoadInput(_State, ((struct GoodBase*)_Good->InputGoods[i]->Req));
			_Nutrition += GoodNutVal((struct GoodBase*)_Good);
		}
	}
	((struct FoodBase*)_Good)->Nutrition = _Nutrition;
}

void ClothingBaseLoad(lua_State* _State, struct GoodBase* _Good, int* _Locations) {
	/*_Locations = alloca(EBODY_SIZE * sizeof(int));

	lua_pushstring(_State, "Locations");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		Log(ELOG_WARNING, "Warning: Good %s cannot be a wearable because it has an invalid Locations field.", _Good->Name);
		return;
	}
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_type(_State, -1) != LUA_TSTRING) {
			Log(ELOG_WARNING, "Warning: Good %s contains an invalid Location.", _Good->Name);
			goto endloop;
		}
		endloop:
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);*/
}

int WeaponBaseLoad(lua_State* _State, struct WeaponBase* _Weapon) {
	const char* _Type = NULL;

	lua_pushstring(_State, "Type");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING) {
		Log(ELOG_WARNING, "Weapon type is not a string.");
		return 0;
	}
	_Type = lua_tostring(_State, -1);
	lua_pop(_State, 1);
	if(strcmp(_Type, "Spear") == 0) {
		_Weapon->WeaponType = EWEAPON_SPEAR;
		_Weapon->Range = MELEE_RANGE;
		_Weapon->RangeAttack = 0;
		goto melee;
	} else if(strcmp(_Type, "Sword") == 0) {
		_Weapon->WeaponType = EWEAPON_SWORD;
		_Weapon->Range = MELEE_RANGE;
		_Weapon->RangeAttack = 0;
		goto melee;
	} else if(strcmp(_Type, "Javelin") == 0) {
		_Weapon->WeaponType = EWEAPON_JAVELIN;
		_Weapon->Range = 3;
	} else if(strcmp(_Type, "Bow") == 0) {
		_Weapon->WeaponType = EWEAPON_BOW;
		_Weapon->Range = 5;
		_Weapon->MeleeAttack = 0;
		_Weapon->Charge = 0;
	} else {
		Log(ELOG_WARNING, "%s is not a weapon type.", _Type);
		return 0;
	}
	lua_pushstring(_State, "RangeAttack");
	lua_rawget(_State, -2);
	LuaGetInteger(_State, -1, &_Weapon->RangeAttack);
	lua_pop(_State, 1);
	if(_Weapon->WeaponType == EWEAPON_BOW)
		return 1;
	melee:
	lua_pushstring(_State, "MeleeAttack");
	lua_rawget(_State, -2);
	LuaGetInteger(_State, -1, &_Weapon->MeleeAttack);
	lua_pushstring(_State, "Charge");
	lua_rawget(_State, -3);
	LuaGetInteger(_State, -1, &_Weapon->Charge);
	lua_pop(_State, 2);
	return 1;
}

int ArmorBaseLoad(lua_State* _State, struct ArmorBase* _Armor) {
	const char* _Type = NULL;

	lua_pushstring(_State, "Type");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING) {
		Log(ELOG_WARNING, "Weapon type is not a string.");
		return 0;
	}
	_Type = lua_tostring(_State, -1);
	lua_pop(_State, 1);
	if(strcmp(_Type, "Armor") == 0) {
		_Armor->ArmorType = EARMOR_BODY;
	} else if(strcmp(_Type, "Shield") == 0) {
		_Armor->ArmorType = EARMOR_SHIELD;
	} else {

	}
	return 1;
}

struct GoodDep* CreateGoodDep(const struct GoodBase* _Good) {
	struct GoodDep* _GoodDep = (struct GoodDep*) malloc(sizeof(struct GoodDep));

	_GoodDep->DepTbl = CreateArray(5);
	_GoodDep->Good = _Good;
	return _GoodDep;
}

void DestroyGoodDep(struct GoodDep* _GoodDep) {
	DestroyArray(_GoodDep->DepTbl);
	free(_GoodDep);
}

struct Good* CreateGood(const struct GoodBase* _Base, int _X, int _Y) {
	struct Good* _Good = NULL;

	if(_Base == NULL)
		return NULL;
	_Good = (struct Good*) malloc(sizeof(struct Good));
	CreateObject((struct Object*)_Good, OBJECT_GOOD, NULL);
	_Good->Id = ++g_GoodId;
	_Good->Pos.x = _X;
	_Good->Pos.y = _Y;
	_Good->Base = _Base;
	_Good->Quantity = 0;
	return _Good;
}

int GoodCmp(const void* _One, const void* _Two) {
	return ((struct Good*)_One)->Id - ((struct Good*)_Two)->Id;
}

void DestroyGood(struct Good* _Good) {
	free(_Good);
}

int GoodGBaseCmp(const struct GoodBase* _One, const struct Good* _Two) {
	return _One->Id - _Two->Base->Id;
}

int GoodBaseGoodCmp(const struct GoodBase* _One, const struct Good* _Two) {
	return _One->Id - _Two->Base->Id;
}

struct ToolBase* CreateToolBase(const char* _Name, int _Category, int _Function, int _Quality) {
	struct ToolBase* _Tool = (struct ToolBase*) InitGoodBase((struct GoodBase*)malloc(sizeof(struct ToolBase)), _Name, _Category);

	_Tool->Function = _Function;
	_Tool->Quality = _Quality;
	return _Tool;
}

void DestroyToolBase(struct ToolBase* _Tool) {
	free(_Tool);
}

struct FoodBase* CreateFoodBase(const char* _Name, int _Category, int _Nutrition) {
	struct FoodBase* _Food = (struct FoodBase*) InitGoodBase((struct GoodBase*) malloc(sizeof(struct FoodBase)), _Name, _Category);

	_Food->Nutrition = _Nutrition;
	return _Food;
}

void DestroyFoodBase(struct FoodBase* _Food) {
	free(_Food);
}

struct Food* CreateFood(const struct FoodBase* _Base, int _X, int _Y) {
	struct Food* _Food = (struct Food*) malloc(sizeof(struct Food));
	
	CreateObject((struct Object*)_Food, OBJECT_GOOD, NULL);
	_Food->Pos.x = _X;
	_Food->Pos.y = _Y;
	_Food->Base = _Base;
	_Food->Quantity = 0;
	_Food->Parts = FOOD_MAXPARTS;
	return _Food;
}

void DestroyFood(struct Food* _Food) {
	free(_Food);
}

struct ClothingBase* CreateClothingBase(const char* _Name, int _Category) {
	return (struct ClothingBase*) InitGoodBase((struct GoodBase*) malloc(sizeof(struct ClothingBase)), _Name, _Category);
}

void DestroyClothingBase(struct ClothingBase* _Clothing) {
	free(_Clothing->Locations);
	free(_Clothing);
}

struct WeaponBase* CreateWeaponBase(const char* _Name, int _Category) {
	return (struct WeaponBase*) InitGoodBase((struct GoodBase*) malloc(sizeof(struct WeaponBase)), _Name, _Category);
}

void DestroyWeaponBase(struct WeaponBase* _Weapon) {
	free(_Weapon);
}

struct RBTree* GoodBuildDep(const struct HashTable* _GoodList) {
	struct HashItrCons* _Itr = NULL;
	struct RBTree* _Prereq = CreateRBTree((int(*)(const void*, const void*))&GoodDepCmp, (int(*)(const void*, const void*))&GoodBaseDepCmp);
	struct GoodDep* _Dep = NULL;

	_Itr = HashCreateItrCons(_GoodList);
	while(_Itr != NULL) {
		_Dep = GoodDependencies(_Prereq, ((const struct GoodBase*)_Itr->Node->Pair)); //Fails when _Itr->Key == "Shear". _Itr->Pair is invalid.
		if(RBSearch(_Prereq, _Dep->Good) == NULL)
			RBInsert(_Prereq, _Dep);
		_Itr = HashNextCons(_GoodList, _Itr);
	}
	HashDeleteItrCons(_Itr);
	return _Prereq;
}

struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct GoodBase* _Good) {
	struct GoodDep* _Pair = NULL;
	struct GoodDep* _PairItr = NULL;
	int i;

	if((_Pair = RBSearch(_Tree, _Good)) == NULL) {
		_Pair = CreateGoodDep(_Good);
		for(i = 0; i < _Good->IGSize; ++i) {
			if((_PairItr = RBSearch(_Tree, ((struct GoodBase*)_Good->InputGoods[i]))) == NULL)
				_PairItr = GoodDependencies(_Tree, ((struct GoodBase*)_Good->InputGoods[i]->Req));
			if(ArrayInsert(_PairItr->DepTbl, _Pair) == 0) {
				ArrayResize(_PairItr->DepTbl);
				ArrayInsert(_PairItr->DepTbl, _PairItr);
			}
			RBInsert(_Tree, _PairItr);
		}
	}
	return _Pair;
}

double GoodNutVal(struct GoodBase* _Base) {
	struct Crop* _Crop = NULL;
	double _Nut = 0;
	int i;

	for(i = 0; i < _Base->IGSize; ++i) {
		if(((struct GoodBase*)_Base->InputGoods[i]->Req)->Category == GOOD_SEED) {
			if((_Crop = HashSearch(&g_Crops, ((struct GoodBase*)_Base->InputGoods[i]->Req)->Name)) == NULL) {
				Log(ELOG_WARNING, "Crop %s not found.", ((struct GoodBase*)_Base->InputGoods[i]->Req)->Name);
				continue;
			}
			_Nut += _Crop->NutVal / OUNCE * _Base->InputGoods[i]->Quantity;
		} else
			_Nut += ((struct FoodBase*)_Base->InputGoods[i]->Req)->Nutrition * _Base->InputGoods[i]->Quantity;
	}
	return _Nut;
}

struct InputReq** GoodBuildList(const struct Array* _Goods, int* _Size, int _Categories) {
	int i;
	int j;
	int _TblSize = _Goods->Size;
	int _Amount = 0;
	void** _Tbl = _Goods->Table;
	int _OutputSize = 0;
	struct GoodDep* _Dep = NULL;
	struct InputReq** _Outputs = (struct InputReq**)calloc(_TblSize, sizeof(struct InputReq*));
	const struct GoodBase* _Output = NULL;
	struct Good* _Good = NULL;

	memset(_Outputs, 0, sizeof(struct InputReq*) * _TblSize);
	for(i = 0; i < _TblSize; ++i) {
		_Good = (struct Good*)_Tbl[i];
		if((_Categories &_Good->Base->Category) != _Good->Base->Category && _Good->Quantity > 0)
			continue;
		_Dep = RBSearch(g_GameWorld.GoodDeps, _Good->Base);
		for(j = 0; j < _Dep->DepTbl->Size; ++j) {
			_Output = ((struct GoodDep*)_Dep->DepTbl->Table[j])->Good;
			if((_Amount = GoodCanMake(_Output, _Goods)) != 0) {
				struct InputReq* _Temp = CreateInputReq();

				_Temp->Req = (void*)_Output;
				_Temp->Quantity = _Amount;
				_Outputs[_OutputSize++] = _Temp;
			}
		}
	}

	if(_Size)
		*_Size = _OutputSize;
	_Outputs = realloc(_Outputs, sizeof(struct InputReq*) * _OutputSize);
	return _Outputs;
}

int GoodCanMake(const struct GoodBase* _Good, const struct Array* _Goods) {
	int i;
	int _Max = INT_MAX;
	int _Quantity = 0;
	struct Good** _Tbl = (struct Good**) _Goods->Table;
	struct Good* _Temp = NULL;
	
	for(i = 0; i < _Good->IGSize; ++i) {
		if((_Temp = LinearSearch(_Good->InputGoods[i], _Tbl, _Goods->Size, (int(*)(const void*, const void*))InputReqGoodCmp)) == NULL
				|| (_Temp->Quantity < _Good->InputGoods[i]->Quantity))
			return 0;
		_Quantity = _Temp->Quantity / _Good->InputGoods[i]->Quantity;
		if(_Quantity < _Max)
			_Max = _Quantity;
	}
	return _Max;
}

struct Good* GoodMostAbundant(struct Array* _Goods, int _Category) {
	struct Good* _Good = NULL;
	struct Good* _BestGood = NULL;
	int _BestQuantity = 0;
	int i = 0;

	for(i = 0; i < _Goods->Size; ++i) {
		_Good = (struct Good*)_Goods->Table[i];
		if(_Good->Base->Category == GOOD_SEED) {
			_BestGood = _Good;
			_BestQuantity = _Good->Quantity;
			break;
		}
	}

	for(; i < _Goods->Size; ++i) {
		_Good = (struct Good*)_Goods->Table[i];
		if(_Good->Base->Category == GOOD_SEED) {
			if(_Good->Quantity > _BestQuantity) {
				_BestGood = _Good;
				_BestQuantity = _Good->Quantity;
			}
		}
	}
	return _BestGood;
}

void GoodSell(const struct Family* _Seller, const struct GoodBase* _Base, int _Quantity) {
	struct SellRequest* _SellReq = _Seller->HomeLoc->Market;

	while(_SellReq != NULL) {
		if(_SellReq->Owner == _Seller && _SellReq->Base == _Base) {
			_SellReq->Quantity = _SellReq->Quantity + _Quantity;
			return;
		}
		_SellReq = _SellReq->Next;
	}

	CreateSellRequest(_Seller, _Base, GoodGetValue(_Base), _Quantity);
}

int GoodBuy(struct Family* _Family, const struct GoodBase* _Base, int _Quantity) {
	struct Settlement* _Settlement = _Family->HomeLoc;
	struct SellRequest* _Itr = _Settlement->Market;
	int _Sold = 0;
			
	while(_Itr != NULL) {
		if(GoodBaseCmp(_Itr, _Base) == 0) {
			if(_Itr->Quantity <= _Quantity) {
				_Sold = _Sold + (_Quantity - GoodPay(_Family, _Itr));
				_Quantity = _Quantity - _Sold;
				ILL_DESTROY(_Settlement->Market, _Itr);
			} else {
				_Itr->Quantity = _Itr->Quantity - _Quantity;
				GoodPay(_Family, _Itr);
				return _Quantity;
			}
		}
		_Itr = _Itr->Next;
	}
	CreateBuyRequest(_Family, _Base, _Quantity);
	return _Sold;
}

int GoodPay(struct Family* _Buyer, const struct SellRequest* _SellReq) {
	int _Quantity = 0;

	GoodPayInKind(_Buyer, _SellReq->Cost, _SellReq->Base, &_Quantity);
	SellItem(_Buyer, _SellReq);
	return _Quantity;
}

const struct GoodBase* GoodPayInKind(const struct Family* _Buyer, int _Cost, const struct GoodBase* _PayGood, int* _Quantity) {
	const struct Good* _Good = NULL;
	int _Value = 0;

	switch(_Buyer->Caste->Type) {
		case CASTE_PEASANT:
			for(int i = 0; i < _Buyer->Goods->Size; ++i) {
				_Good = _Buyer->Goods->Table[i];
				if(strcmp(_Good->Base->Name, "Flour") == 0) {
					_Value = GoodGetValue(_Good->Base);
					goto func_end;
				}
			}
		break;
	}
	//Return the Good that will be sold and how many need to be sold.
	func_end:
	(*_Quantity) = _Cost / _Value;
	if((*_Quantity) * _Value < _Cost)
		++(*_Quantity);
	return _Good->Base;
}

void SellItem(struct Family* _Buyer, const struct SellRequest* _SellReq) {
	struct Array* _Goods = _Buyer->Goods;
	struct Good* _Good = NULL;

	//Search for the boughten good in the _Buyers good array.
	for(int i = 0; _Goods->Size; ++i) {
		_Good = (struct Good*) _Goods->Table[i];
		if(GoodBaseCmp(_Good->Base, _SellReq) == 0) {
			_Good->Quantity = _Good->Quantity + _SellReq->Quantity;
			return;
		}
	}
	//Good is not found create a good then add it to the good array.
	_Good = CreateGood(_SellReq->Base, _Buyer->HomeLoc->Pos.x, _Buyer->HomeLoc->Pos.y);
	_Good->Quantity = _SellReq->Quantity;
	ArrayInsert_S(_Buyer->Goods, _Good);
}

int GoodGetValue(const struct GoodBase* _Base) {
	return 1;
}

struct Good* GoodMake(const struct GoodBase* _Base, int _Quantity, struct Array* _Inputs, int _X, int _Y) {
	struct Good* _Good = NULL;

	for(int i = 0; i < _Base->IGSize; ++i) {
		_Good = LinearSearch(_Base->InputGoods[i], _Inputs->Table, _Inputs->Size, (int(*)(const void*, const void*))InputReqGoodCmp);
		_Good->Quantity -= _Quantity * _Base->InputGoods[i]->Quantity;
	}
	if((_Good = LinearSearch(_Base, _Inputs->Table, _Inputs->Size, (int(*)(const void*, const void*))GoodGBaseCmp)) == NULL) {
		switch(_Base->Category) {
		case GOOD_FOOD:
			_Good =  (struct Good*) CreateFood((const struct FoodBase*)_Base, _X, _Y);
			break;
		default:
			_Good = CreateGood(_Base, _X, _Y);
			break;
		}
		ArrayInsert_S(_Inputs, _Good);
	}
	_Good->Quantity += _Quantity;
	return _Good;
}

const struct LinkedList* GoodGetCategory(const char* _Category) {
	for(int i = 0; g_GoodCatStr[i] != NULL; ++i) {
		if(strcmp(_Category, g_GoodCatStr[i]) == 0) {
			return &g_GoodCats[i];
		}
	}
	return NULL;
}
