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
	"Weapon",
	"Armor",
	"Other",
	NULL
};

int GoodDepCmp(const struct GoodDep* One, const struct GoodDep* Two) {
	return One->Good->Id - Two->Good->Id;
}

int GoodBaseDepCmp(const struct GoodBase* Good, const struct GoodDep* Pair) {
	return Good->Id - Pair->Good->Id;
}

int InputReqGoodCmp(const struct InputReq* One, const struct Good* Two) {
	return ((struct GoodBase*)One->Req)->Id - Two->Base->Id;
}

struct GoodBase* InitGoodBase(struct GoodBase* Good, const char* Name, int Category) {
	Good->Name = (char*) malloc(sizeof(char) * strlen(Name) + 1);
	strcpy(Good->Name, Name);
	Good->Category = Category;
	Good->Id = NextId();
	Good->InputGoods = NULL;
	Good->IGSize = 0;
	return Good;
}

struct GoodBase* CopyGoodBase(const struct GoodBase* Good) {
	int i;
	struct GoodBase* NewGood = (struct GoodBase*) malloc(sizeof(struct GoodBase));

	NewGood->Name = (char*) malloc(sizeof(char) * strlen(Good->Name) + 1);
	NewGood->Category = Good->Category;
	NewGood->Id = Good->Id;
	strcpy(NewGood->Name, Good->Name);
	NewGood->InputGoods = calloc(Good->IGSize, sizeof(struct InputReq*));
	NewGood->IGSize = Good->IGSize;
	for(i = 0; i < Good->IGSize; ++i) {
		struct InputReq* Req = CreateInputReq();
		Req->Req = ((struct InputReq*)Good->InputGoods[i])->Req;
		Req->Quantity = ((struct InputReq*)Good->InputGoods[i])->Quantity;
		NewGood->InputGoods[i] = Req;
	}
	return NewGood;
}

int GoodBaseCmp(const void* One, const void* Two) {
	return ((struct GoodBase*)One)->Id - ((struct GoodBase*)Two)->Id;
}

void DestroyGoodBase(struct GoodBase* Good) {
	int i;

	for(i = 0; i < Good->IGSize; ++i)
		DestroyInputReq(Good->InputGoods[i]);
	free(Good->Name);
	free(Good->InputGoods);
	free(Good);
}

struct BuyRequest* CreateBuyRequest(struct Family* Family, const struct GoodBase* Base, int Quantity) {
	struct BuyRequest* BuyReq = (struct BuyRequest*) malloc(sizeof(struct BuyRequest));

	SDL_assert(Base != NULL);
	BuyReq->Time = g_GameWorld.Date;
	BuyReq->Base = Base;
	BuyReq->Owner = Family;
	BuyReq->Quantity = Quantity;
	ILL_CREATE(Family->HomeLoc->BuyOrders, BuyReq);
	return BuyReq;
}

void DestroyBuyRequest(struct BuyRequest* BuyReq) {
	ILL_DESTROY(BuyReq->Owner->HomeLoc->BuyOrders, BuyReq);
	free(BuyReq);
}

struct SellRequest* CreateSellRequest(const struct Family* Seller, const struct GoodBase* Base, int Quantity, int Cost) {
	struct SellRequest* SellReq = (struct SellRequest*) malloc(sizeof(struct SellRequest));

	SellReq->Base = Base;
	SellReq->Owner = Seller;
	SellReq->Quantity = Quantity;
	SellReq->Cost = Cost;
	ILL_CREATE(Seller->HomeLoc->Market, SellReq);
	return SellReq;
}

void DestroySellRequest(struct SellRequest* SellReq) {
	free(SellReq);
}

struct Good* GoodCopy(const struct Good* Good) {
	struct Good* NewGood = CreateGood(Good->Base, Good->Pos.x, Good->Pos.y);

	return NewGood;
}

struct Good* FoodGoodCopy(const struct Good* Good) {
	struct Good* NewGood = (struct Good*) CreateFood((struct FoodBase*)Good->Base, Good->Pos.x, Good->Pos.y);

	((struct Food*)NewGood)->Parts = ((struct Food*)Good)->Parts;
	return NewGood;
}

int GoodInpGdCmp(const void* One, const void* Two) {
	return ((struct GoodBase*)((struct InputReq*)One)->Req)->Id - ((struct GoodBase*)((struct InputReq*)Two)->Req)->Id;
}

int GoodThink(struct Good* Good) {
	return 1;
}

struct GoodBase* GoodLoad(lua_State* State, int Index) {
	struct GoodBase* Good = NULL;
	const char* Name = NULL;
	const char* Temp = NULL;
	int Category = 0;
	int Return = -2;
	int Top = lua_gettop(State);

	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_isstring(State, -2)) {
			if(!strcmp("Name", lua_tostring(State, -2)))
				Return = LuaGetString(State, -1, &Name);
			else if(!strcmp("Category", lua_tostring(State, -2))) {
					if(lua_isstring(State, -1) == 1) {
						Temp = lua_tostring(State, -1);
						Return = 1;
					if(!strcmp("Food", Temp))
						Category = GOOD_FOOD;
					else if(!strcmp("Ingredient", Temp))
						Category = GOOD_INGREDIENT;
					else if(!strcmp("Animal", Temp))
						Category = GOOD_ANIMAL;
					else if(!strcmp("Seed", Temp))
						Category = GOOD_SEED;
					else if(!strcmp("Tool", Temp))
						Category = GOOD_TOOL;
					else if(!strcmp("Material", Temp))
						Category = GOOD_MATERIAL;
					else if(!strcmp("Weapon", Temp))
						Category = GOOD_WEAPON;
					else if(!strcmp("Armor", Temp))
						Category = GOOD_ARMOR;
					else if(!strcmp("Other", Temp))
						Category = GOOD_OTHER;
					else Return = -1;
					}
				if(Category <= 0 && Return <= 0) {
					Log(ELOG_WARNING, "%s is not a valid category.", Temp);
					goto fail;
				}
			}
		}
		lua_pop(State, 1);
	}
	if(Name == NULL)
		goto fail;
	if(Category == GOOD_TOOL) {
		int Function = 0;
		int Quality = 0;

		lua_pushstring(State, "Function");
		lua_rawget(State, -2);
		if(LuaGetString(State, -1, &Temp) == 0) {
			Log(ELOG_WARNING, "Tool requires Function field.");
			goto fail;
		}
		lua_pop(State, 1);
		lua_pushstring(State, "Quality");
		lua_rawget(State, -2);
		if(LuaGetInteger(State, -1, &Quality) == 0) {
			Log(ELOG_WARNING, "Tool requires Quality field.");
			goto fail;
		}
		lua_pop(State, 1);
		if(!strcmp(Temp, "Plow"))
			Function = ETOOL_PLOW;
		else if(!strcmp(Temp, "Reap"))
			Function = ETOOL_REAP;
		else if(!strcmp(Temp, "Cut"))
			Function = ETOOL_CUT;
		else if(!strcmp(Temp, "Logging"))
			Function = ETOOL_LOGGING;
		else {
			Log(ELOG_WARNING, "Tool contains an invalid Function field: %s", Temp);
			goto fail;
		}
		Good = (struct GoodBase*) CreateToolBase(Name, Category, Function, Quality);
	} else if(Category == GOOD_FOOD || Category == GOOD_INGREDIENT || Category == GOOD_SEED) {
		Good = (struct GoodBase*) CreateFoodBase(Name, Category, 0);
	} else if(Category == GOOD_WEAPON) {
		Good = (struct GoodBase*) CreateWeaponBase(Name, Category);
		if(WeaponBaseLoad(State, (struct WeaponBase*)Good) == 0) {
			Top = lua_gettop(State);
			DestroyWeaponBase((struct WeaponBase*)Good);
			return NULL;
		}
	} else if(Category == GOOD_ARMOR) {
		Good = InitGoodBase((struct GoodBase*) malloc(sizeof(struct WeaponBase)), Name, Category);//(struct GoodBase*)// CreateArmorBase(Name, Category);
		if(ArmorBaseLoad(State, (struct ArmorBase*)Good) == 0) {
			Top = lua_gettop(State);
			DestroyWeaponBase((struct WeaponBase*)Good);
			return NULL;
		}
	} else
		Good = InitGoodBase((struct GoodBase*)malloc(sizeof(struct GoodBase)), Name, Category);
	if(Return > 0) {
		return Good;
	}
	fail:
	if(Good != NULL)
		DestroyGoodBase(Good);
	lua_settop(State, Top);
	return NULL;
}

int GoodLoadInput(lua_State* State, struct GoodBase* Good) {
	const char* Name = NULL;
	int Top = lua_gettop(State);
	int i;
	struct InputReq* Req = NULL;
	struct LinkedList List = {0, NULL, NULL};
	struct LnkLst_Node* Itr = NULL;

	if(Good == NULL)
		return 0;
	if(HashSearch(&g_Crops, Good->Name) != NULL || Good->InputGoods != NULL || Good->IGSize > 0)
		return 1;

	lua_getglobal(State, "Goods");
	lua_pushstring(State, Good->Name);
	lua_rawget(State, -2);
	SDL_assert(lua_type(State, -1) == LUA_TTABLE);
	lua_remove(State, -2);
	lua_pushstring(State, "InputGoods");
	lua_rawget(State, -2);
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		lua_pushnil(State);
		if(lua_type(State, -2) != LUA_TTABLE) {
			Log(ELOG_WARNING, "Good %s's InputGoods contains a non table element.", Good->Name);
			goto fail;
		}
		if(lua_next(State, -2) == 0)
			goto fail;
		if(lua_isstring(State, -1) == 1) {
			Req = CreateInputReq();
			Name = lua_tostring(State, -1);
			if(strcmp(Good->Name, Name) == 0) {
				Log(ELOG_WARNING, "Good %s is attempting to make itself its own input good.", Good->Name);
				goto fail;
			}
			if((Req->Req = HashSearch(&g_Goods, Name)) != NULL) {
				lua_pop(State, 1);
				if(lua_next(State, -2) == 0)
					goto fail;
				if(LuaGetNumber(State, -1, &Req->Quantity) == -1) {
					goto fail;
				}
				LnkLstPushBack(&List, Req);
			} else {
				Log(ELOG_WARNING, "Good %s cannot add %s as an input good: %s does not exist.", Good->Name, Name, Name);
				goto fail;
			}
			lua_pop(State, 2);
		} else
			lua_pop(State, 2);
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
	Good->IGSize = List.Size;
	Good->InputGoods = calloc(Good->IGSize, sizeof(struct InputReq*));
	Itr = List.Front;
	for(i = 0; i < List.Size; ++i) {
		Good->InputGoods[i] = (struct InputReq*)Itr->Data;
		Itr = Itr->Next;
	}
	if(Good->Category == GOOD_FOOD || Good->Category == GOOD_INGREDIENT || Good->Category == GOOD_SEED)
		GoodLoadConsumableInput(State, Good, &List);
	lua_pop(State, 1);
	InsertionSort(Good->InputGoods, Good->IGSize, GoodInpGdCmp, sizeof(*Good->InputGoods));
	LnkLstClear(&List);
	return 1;
	fail:
	DestroyInputReq(Req);
	lua_settop(State, Top);
	LnkLstClear(&List);
	HashDelete(&g_Goods, Good->Name);
	DestroyGoodBase(Good);
	return 0;
}

int GoodLoadOutput(lua_State* State, struct GoodBase* Good) {
	struct GoodBase* OutputGood = NULL;

	if(Good == NULL)
		return 0;
	lua_getglobal(State, "Goods");
	lua_pushstring(State, Good->Name);
	lua_rawget(State, -2);
	SDL_assert(lua_type(State, -1) == LUA_TTABLE);
	lua_remove(State, -2);
	lua_pushstring(State, "OutputGoods");
	lua_rawget(State, -2);
	if(lua_type(State, -1) == LUA_TNIL) {
		lua_pop(State, 2);
		return 0;
	}
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_type(State, -1) != LUA_TTABLE)
			goto loop_end;
		lua_pushinteger(State, 1);
		lua_rawget(State, -2);
		if(lua_type(State, -1) != LUA_TSTRING) {
			lua_pop(State, 2);
			continue;
		}
		if((OutputGood = HashSearch(&g_Goods, lua_tostring(State, -1))) == NULL) {
			lua_pop(State, 2);
			return 0;
		}
		lua_pop(State, 1);
		lua_pushinteger(State, 2);
		lua_rawget(State, -2);
		if(lua_isnumber(State, -1) == 0) {
			lua_pop(State, 2);
			continue;
		}
		lua_pop(State, 1);
		//_Time = lua_tointeger(State, -1);
		loop_end:
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
	return 1;
}

void GoodLoadConsumableInput(lua_State* State, struct GoodBase* Good, struct LinkedList* List) {
	int i = 0;
	double Nutrition = 0;

	if(Good->IGSize == 0) {
		lua_pushstring(State, "Nutrition");
		lua_rawget(State, -2);
		if(lua_tointeger(State, -1) == 0) {
			Log(ELOG_WARNING, "Warning: Good %s cannot be a food because it contains no InputGoods or a field named Nutrition containing an integer.", Good->Name);
			lua_pop(State, 1);
			return;
		}
		Nutrition = lua_tointeger(State, -1);
		lua_pop(State, 1);
	} else {
		for(i = 0; i < List->Size; ++i) {
			GoodLoadInput(State, ((struct GoodBase*)Good->InputGoods[i]->Req));
			Nutrition += GoodNutVal((struct GoodBase*)Good);
		}
	}
	((struct FoodBase*)Good)->Nutrition = Nutrition;
}

int WeaponBaseLoad(lua_State* State, struct WeaponBase* Weapon) {
	const char* Type = NULL;
	int RangeAttack = 0;
	int MeleeAttack = 0;

	lua_pushstring(State, "Type");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TSTRING) {
		Log(ELOG_WARNING, "Weapon type is not a string.");
		return 0;
	}
	Type = lua_tostring(State, -1);
	lua_pop(State, 1);
	if(strcmp(Type, "Spear") == 0) {
		Weapon->WeaponType = EWEAPON_SPEAR;
		Weapon->Range = MELEE_RANGE;
		Weapon->RangeAttack = 0;
		goto melee;
	} else if(strcmp(Type, "Sword") == 0) {
		Weapon->WeaponType = EWEAPON_SWORD;
		Weapon->Range = MELEE_RANGE;
		Weapon->RangeAttack = 0;
		goto melee;
	} else if(strcmp(Type, "Javelin") == 0) {
		Weapon->WeaponType = EWEAPON_JAVELIN;
		Weapon->Range = 3;
	} else if(strcmp(Type, "Bow") == 0) {
		Weapon->WeaponType = EWEAPON_BOW;
		Weapon->Range = 5;
		Weapon->MeleeAttack = 0;
	} else {
		Log(ELOG_WARNING, "%s is not a weapon type.", Type);
		return 0;
	}
	lua_pushstring(State, "RangeAttack");
	lua_rawget(State, -2);
	LuaGetInteger(State, -1, &RangeAttack);
	Weapon->RangeAttack = RangeAttack;
	lua_pop(State, 1);
	if(Weapon->WeaponType == EWEAPON_BOW)
		return 1;
	melee:
	lua_pushstring(State, "MeleeAttack");
	lua_rawget(State, -2);
	LuaGetInteger(State, -1, &MeleeAttack);
	Weapon->MeleeAttack = MeleeAttack;
	lua_pop(State, 1);
	return 1;
}

int ArmorBaseLoad(lua_State* State, struct ArmorBase* Armor) {
	const char* Type = NULL;

	lua_pushstring(State, "Type");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TSTRING) {
		Log(ELOG_WARNING, "Weapon type is not a string.");
		return 0;
	}
	Type = lua_tostring(State, -1);
	lua_pop(State, 1);
	if(strcmp(Type, "Armor") == 0) {
		Armor->ArmorType = EARMOR_BODY;
	} else if(strcmp(Type, "Shield") == 0) {
		Armor->ArmorType = EARMOR_SHIELD;
	}
	return 1;
}

struct GoodDep* CreateGoodDep(const struct GoodBase* Good) {
	struct GoodDep* GoodDep = (struct GoodDep*) malloc(sizeof(struct GoodDep));

	GoodDep->DepTbl = CreateArray(5);
	GoodDep->Good = Good;
	return GoodDep;
}

void DestroyGoodDep(struct GoodDep* GoodDep) {
	DestroyArray(GoodDep->DepTbl);
	free(GoodDep);
}

struct Good* CreateGood(const struct GoodBase* Base, int X, int Y) {
	struct Good* Good = NULL;

	if(Base == NULL)
		return NULL;
	Good = (struct Good*) malloc(sizeof(struct Good));
	CreateObject((struct Object*)Good, OBJECT_GOOD, NULL);
	Good->Id = ++g_GoodId;
	Good->Pos.x = X;
	Good->Pos.y = Y;
	Good->Base = Base;
	Good->Quantity = 0;
	return Good;
}

int GoodCmp(const void* One, const void* Two) {
	return ((struct Good*)One)->Id - ((struct Good*)Two)->Id;
}

void DestroyGood(struct Good* Good) {
	free(Good);
}

int GoodGBaseCmp(const struct GoodBase* One, const struct Good* Two) {
	return One->Id - Two->Base->Id;
}

int GoodBaseGoodCmp(const struct GoodBase* One, const struct Good* Two) {
	return One->Id - Two->Base->Id;
}

struct ToolBase* CreateToolBase(const char* Name, int Category, int Function, int Quality) {
	struct ToolBase* Tool = (struct ToolBase*) InitGoodBase((struct GoodBase*)malloc(sizeof(struct ToolBase)), Name, Category);

	Tool->Function = Function;
	Tool->Quality = Quality;
	return Tool;
}

void DestroyToolBase(struct ToolBase* Tool) {
	free(Tool);
}

struct FoodBase* CreateFoodBase(const char* Name, int Category, int Nutrition) {
	struct FoodBase* Food = (struct FoodBase*) InitGoodBase((struct GoodBase*) malloc(sizeof(struct FoodBase)), Name, Category);

	Food->Nutrition = Nutrition;
	return Food;
}

void DestroyFoodBase(struct FoodBase* Food) {
	free(Food);
}

struct Food* CreateFood(const struct FoodBase* Base, int X, int Y) {
	struct Food* Food = (struct Food*) malloc(sizeof(struct Food));
	
	CreateObject((struct Object*)Food, OBJECT_GOOD, NULL);
	Food->Pos.x = X;
	Food->Pos.y = Y;
	Food->Base = Base;
	Food->Quantity = 0;
	Food->Parts = FOOD_MAXPARTS;
	return Food;
}

void DestroyFood(struct Food* Food) {
	free(Food);
}

struct WeaponBase* CreateWeaponBase(const char* Name, int Category) {
	return (struct WeaponBase*) InitGoodBase((struct GoodBase*) malloc(sizeof(struct WeaponBase)), Name, Category);
}

void DestroyWeaponBase(struct WeaponBase* Weapon) {
	free(Weapon);
}

struct RBTree* GoodBuildDep(const struct HashTable* GoodList) {
	struct HashItrCons* Itr = NULL;
	struct RBTree* Prereq = CreateRBTree((int(*)(const void*, const void*))&GoodDepCmp, (int(*)(const void*, const void*))&GoodBaseDepCmp);
	struct GoodDep* Dep = NULL;

	Itr = HashCreateItrCons(GoodList);
	while(Itr != NULL) {
		Dep = GoodDependencies(Prereq, ((const struct GoodBase*)Itr->Node->Pair)); //Fails when Itr->Key == "Shear". Itr->Pair is invalid.
		if(RBSearch(Prereq, Dep->Good) == NULL)
			RBInsert(Prereq, Dep);
		Itr = HashNextCons(GoodList, Itr);
	}
	HashDeleteItrCons(Itr);
	return Prereq;
}

struct GoodDep* GoodDependencies(struct RBTree* Tree, const struct GoodBase* Good) {
	struct GoodDep* Pair = NULL;
	struct GoodDep* PairItr = NULL;
	int i;

	if((Pair = RBSearch(Tree, Good)) == NULL) {
		Pair = CreateGoodDep(Good);
		for(i = 0; i < Good->IGSize; ++i) {
			if((PairItr = RBSearch(Tree, ((struct GoodBase*)Good->InputGoods[i]))) == NULL)
				PairItr = GoodDependencies(Tree, ((struct GoodBase*)Good->InputGoods[i]->Req));
			if(ArrayInsert(PairItr->DepTbl, Pair) == 0) {
				ArrayResize(PairItr->DepTbl);
				ArrayInsert(PairItr->DepTbl, PairItr);
			}
			RBInsert(Tree, PairItr);
		}
	}
	return Pair;
}

double GoodNutVal(struct GoodBase* Base) {
	struct Crop* Crop = NULL;
	double Nut = 0;

	for(int i = 0; i < Base->IGSize; ++i) {
		if(((struct GoodBase*)Base->InputGoods[i]->Req)->Category == GOOD_SEED) {
			if((Crop = HashSearch(&g_Crops, ((struct GoodBase*)Base->InputGoods[i]->Req)->Name)) == NULL) {
				Log(ELOG_WARNING, "Crop %s not found.", ((struct GoodBase*)Base->InputGoods[i]->Req)->Name);
				continue;
			}
			Nut += Crop->NutVal * ToPound(Base->InputGoods[i]->Quantity);
		} else
			Nut += ((struct FoodBase*)Base->InputGoods[i]->Req)->Nutrition * Base->InputGoods[i]->Quantity;
	}
	return Nut;
}

//FIXME: This shouldn't allocate memory but instead be given an array that can be filled with the list the function builds.
struct InputReq** GoodBuildList(const struct Array* Goods, int* Size, int Categories) {
	int TblSize = Goods->Size;
	int Amount = 0;
	void** Tbl = Goods->Table;
	int OutputSize = 0;
	struct GoodDep* Dep = NULL;
	struct InputReq** Outputs = (struct InputReq**)calloc(TblSize, sizeof(struct InputReq*));
	const struct GoodBase* Output = NULL;
	struct Good* Good = NULL;

	memset(Outputs, 0, sizeof(struct InputReq*) * TblSize);
	for(int i = 0; i < TblSize; ++i) {
		Good = (struct Good*)Tbl[i];
		if((Categories &Good->Base->Category) != Good->Base->Category && Good->Quantity > 0)
			continue;
		Dep = RBSearch(g_GameWorld.GoodDeps, Good->Base);
		for(int j = 0; j < Dep->DepTbl->Size; ++j) {
			Output = ((struct GoodDep*)Dep->DepTbl->Table[j])->Good;
			if((Amount = GoodCanMake(Output, Goods)) != 0) {
				struct InputReq* Temp = CreateInputReq();

				Temp->Req = (void*)Output;
				Temp->Quantity = Amount;
				Outputs[OutputSize++] = Temp;
			}
		}
	}

	if(Size)
		*Size = OutputSize;
	Outputs = realloc(Outputs, sizeof(struct InputReq*) * OutputSize);
	return Outputs;
}

int GoodCanMake(const struct GoodBase* Good, const struct Array* Goods) {
	int Max = INT_MAX;
	int Quantity = 0;
	struct Good** Tbl = (struct Good**) Goods->Table;
	struct Good* Temp = NULL;
	
	for(int i = 0; i < Good->IGSize; ++i) {
		if((Temp = LinearSearch(Good->InputGoods[i], Tbl, Goods->Size, (int(*)(const void*, const void*))InputReqGoodCmp)) == NULL
				|| (Temp->Quantity < Good->InputGoods[i]->Quantity))
			return 0;
		Quantity = Temp->Quantity / Good->InputGoods[i]->Quantity;
		if(Quantity < Max)
			Max = Quantity;
	}
	return Max;
}

struct Good* GoodMostAbundant(struct Array* Goods, int Category) {
	struct Good* Good = NULL;
	struct Good* BestGood = NULL;
	int BestQuantity = 0;
	int i = 0;

	for(i = 0; i < Goods->Size; ++i) {
		Good = (struct Good*)Goods->Table[i];
		if(Good->Base->Category == GOOD_SEED) {
			BestGood = Good;
			BestQuantity = Good->Quantity;
			break;
		}
	}

	for(; i < Goods->Size; ++i) {
		Good = (struct Good*)Goods->Table[i];
		if(Good->Base->Category == GOOD_SEED) {
			if(Good->Quantity > BestQuantity) {
				BestGood = Good;
				BestQuantity = Good->Quantity;
			}
		}
	}
	return BestGood;
}

void GoodSell(const struct Family* Seller, const struct GoodBase* Base, int Quantity) {
	struct SellRequest* SellReq = Seller->HomeLoc->Market;

	while(SellReq != NULL) {
		if(SellReq->Owner == Seller && SellReq->Base == Base) {
			SellReq->Quantity = SellReq->Quantity + Quantity;
			return;
		}
		SellReq = SellReq->Next;
	}

	CreateSellRequest(Seller, Base, GoodGetValue(Base), Quantity);
}

int GoodBuy(struct Family* Family, const struct GoodBase* Base, int Quantity) {
	struct Settlement* Settlement = Family->HomeLoc;
	struct SellRequest* Itr = Settlement->Market;
	int Sold = 0;
			
	while(Itr != NULL) {
		if(GoodBaseCmp(Itr, Base) == 0) {
			if(Itr->Quantity <= Quantity) {
				Sold = Sold + (Quantity - GoodPay(Family, Itr));
				Quantity = Quantity - Sold;
				ILL_DESTROY(Settlement->Market, Itr);
			} else {
				Itr->Quantity = Itr->Quantity - Quantity;
				GoodPay(Family, Itr);
				return Quantity;
			}
		}
		Itr = Itr->Next;
	}
	CreateBuyRequest(Family, Base, Quantity);
	return Sold;
}

int GoodPay(struct Family* Buyer, const struct SellRequest* SellReq) {
	int Quantity = 0;

	GoodPayInKind(Buyer, SellReq->Cost, SellReq->Base, &Quantity);
	SellItem(Buyer, SellReq);
	return Quantity;
}

const struct GoodBase* GoodPayInKind(const struct Family* Buyer, int Cost, const struct GoodBase* PayGood, int* Quantity) {
	const struct Good* Good = NULL;
	int Value = 0;

	switch(Buyer->Caste) {
		case CASTE_FARMER:
			for(int i = 0; i < Buyer->Goods.Size; ++i) {
				Good = Buyer->Goods.Table[i];
				if(strcmp(Good->Base->Name, "Flour") == 0) {
					Value = GoodGetValue(Good->Base);
					goto func_end;
				}
			}
		break;
	}
	//Return the Good that will be sold and how many need to be sold.
	func_end:
	(*Quantity) = Cost / Value;
	if((*Quantity) * Value < Cost)
		++(*Quantity);
	return Good->Base;
}

void SellItem(struct Family* Buyer, const struct SellRequest* SellReq) {
	struct Array* Goods = &Buyer->Goods;
	struct Good* Good = NULL;

	//Search for the boughten good in the Buyers good array.
	for(int i = 0; Goods->Size; ++i) {
		Good = (struct Good*) Goods->Table[i];
		if(GoodBaseCmp(Good->Base, SellReq) == 0) {
			Good->Quantity = Good->Quantity + SellReq->Quantity;
			return;
		}
	}
	//Good is not found create a good then add it to the good array.
	Good = CreateGood(SellReq->Base, Buyer->HomeLoc->Pos.x, Buyer->HomeLoc->Pos.y);
	Good->Quantity = SellReq->Quantity;
	ArrayInsert_S(&Buyer->Goods, Good);
}

int GoodGetValue(const struct GoodBase* Base) {
	return 1;
}

struct Good* GoodMake(const struct GoodBase* Base, int Quantity, struct Array* Inputs, int X, int Y) {
	struct Good* Good = NULL;

	for(int i = 0; i < Base->IGSize; ++i) {
		Good = LinearSearch(Base->InputGoods[i], Inputs->Table, Inputs->Size, (int(*)(const void*, const void*))InputReqGoodCmp);
		Good->Quantity -= Quantity * Base->InputGoods[i]->Quantity;
	}
	if((Good = LinearSearch(Base, Inputs->Table, Inputs->Size, (int(*)(const void*, const void*))GoodGBaseCmp)) == NULL) {
		switch(Base->Category) {
		case GOOD_FOOD:
			Good =  (struct Good*) CreateFood((const struct FoodBase*)Base, X, Y);
			break;
		default:
			Good = CreateGood(Base, X, Y);
			break;
		}
		ArrayInsert_S(Inputs, Good);
	}
	Good->Quantity += Quantity;
	return Good;
}

const struct LinkedList* GoodGetCategory(const char* Category) {
	for(int i = 0; g_GoodCatStr[i] != NULL; ++i) {
		if(strcmp(Category, g_GoodCatStr[i]) == 0) {
			return &g_GoodCats[i];
		}
	}
	return NULL;
}

void ArrayAddGood(struct Array* GoodList, struct Good* Good, uint32_t Quantity) {
	Assert(Quantity <= Good->Quantity);
	
	for(int i = 0; i < GoodList->Size; ++i)
		if(Good->Base == ((struct Good*)GoodList->Table[i])->Base) {
			((struct Good*)GoodList->Table[i])->Quantity += Quantity;
			if(Quantity >= Good->Quantity) {
				DestroyGood(Good);
			} else {
				Good->Quantity = Good->Quantity - Quantity;
			}
			return;
		}
	ArrayInsert(GoodList, Good);
}

struct Good* ArrayRemoveGood(struct Array* GoodList, uint32_t Index, uint32_t  Quantity) {
	struct Good* Good = NULL;

	if(Index < 0 || Index >= GoodList->Size)
		return NULL;
	Good = GoodList->Table[Index];
	if(Good->Quantity > Quantity) {
		Good->Quantity = Good->Quantity - Quantity;
		Good = g_GoodCopy[Good->Base->Category](Good);
	} else {
		ArrayRemove(GoodList, Index);
	}
	return Good;
}

