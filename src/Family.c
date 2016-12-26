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
void Family_Init(struct Array* Array) {
	g_FirstNames = Array;
}

void Family_Quit() {
	DestroyArray(g_FirstNames);
}

struct Family* CreateFamily(const char* Name, struct Settlement* Location, struct Family* Parent) {
	struct Family* Family = (struct Family*) malloc(sizeof(struct Family));

	Family->Name = Name;
	memset(Family->People, 0, sizeof(struct Person*) * (FAMILY_PEOPLESZ));
	Family->NumChildren = 0;
	CtorArray(&Family->Goods, 16);
	CtorArray(&Family->Animals, 0);
	SettlementPlaceFamily(Location, Family);
	Family->HomeLoc = Location;
	Family->Parent = Parent;
	Family->Owner = NULL;
	Family->Profession = NULL;
	Family->Caste = CASTE_THRALL;
	Family->FieldCt = 0;
	Family->BuildingCt = 0;
	Family->Faction = FACTION_IDNONE;
	for(int i = 0; i < FAMILY_BUILDINGCT; ++i)
		Family->Buildings[i] = NULL;
	for(int i = 0; i < FAMILY_FIELDCT; ++i)
		Family->Fields[i] = NULL;
	Family->Food.SlowSpoiled = 0;
	Family->Food.FastSpoiled = 0;
	Family->Food.AnimalFood = 0;
	CreateObject(&Family->Object, OBJECT_FAMILY, FamilyThink);
	return Family;
}

struct Family* CreateRandFamily(const char* Name, int Size, struct Family* Parent, struct Constraint * const * const AgeGroups,
	 struct Constraint * const * const BabyAvg, struct Settlement* Location, uint8_t Caste) {
	struct Family* Family = NULL;
	int X = Location->Pos.x;
	int Y = Location->Pos.y;

	Assert(Size > 0);
	if(Size > FAMILY_PEOPLESZ)
		return NULL;
	if(Size >= 2) {
		struct Person* Wife = NULL;

		Family = CreateFamily(Name, Location, Parent);
		Family->Caste = Caste;
		Family->People[HUSBAND] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(AgeGroups[TEENAGER]->Min, AgeGroups[ADULT]->Max), EMALE, 1500, X, Y, Family);
		Wife = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(AgeGroups[TEENAGER]->Min, AgeGroups[ADULT]->Max), EFEMALE, 1500, X, Y, Family);
		Family->People[WIFE] = Wife;
		Size -= 2;
		while(Size-- > 0) {
			int Child = CHILDREN + Family->NumChildren;

			Family->People[Child] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(0, (Wife->Age - AgeGroups[TEENAGER]->Min)), Random(1, 2), 1500, X, Y, Family);
			++Family->NumChildren;
		}
	}
	Family->FieldCt = SelectCrops(Family, Family->Fields, Family->FieldCt, FAMILY_FIELDCT);
	Family->Food.SlowSpoiled = ((NUTRITION_REQ * 2) + (NUTRITION_CHILDREQ * Family->NumChildren)) * 2;
	return Family;
}

void DestroyFamily(struct Family* Family) {
	int Max = Family->NumChildren + 1;
	struct Array* Array = &Family->Goods;

	for(int i = 0; i < Array->Size; ++i) {
		DestroyGood(Array->Table[i]);
	}
	while(Max >= 0) {
		if(Family->People[Max] != NULL) {
			DestroyPerson(Family->People[Max]);
			Family->People[Max] = NULL;
		}
		--Max;
	}
	free(Family);
}

/*
 * FIXME: Shouldn't return a Food* as it is not used by anything that calls this function.
 */
struct Food* FamilyMakeFood(struct Family* Family) {
	int FoodSize;
	struct InputReq** Foods = GoodBuildList(&Family->Goods, &FoodSize, GOOD_FOOD | GOOD_SEED | GOOD_INGREDIENT);
	struct Array* GoodsArray = NULL;
	struct FoodBase* Food = NULL;
	struct Food* FamFood = NULL;
	struct Good* Good = NULL;
	int i = 0;

	if(Foods == NULL)
		return NULL;

	for(i = 0; i < FoodSize; ++i) {
		Food = ((struct FoodBase*)Foods[i]->Req);
		for(int j = 0; j < Food->Base.IGSize; ++j) {
			Good = LinearSearch(Food->Base.InputGoods[j], Family->Goods.Table, Family->Goods.Size, (int(*)(const void*, const void*))InputReqGoodCmp);
			Good->Quantity -= Foods[i]->Quantity * Food->Base.InputGoods[j]->Quantity;
			GoodsArray = &Family->Goods;
			if((FamFood = LinearSearch(Food, GoodsArray->Table, GoodsArray->Size, (int(*)(const void*, const void*))GoodGBaseCmp)) == NULL) {
				FamFood =  CreateFood(Food, Family->People[1]->Pos.x, Family->People[1]->Pos.y);
				ArrayInsert_S(GoodsArray, FamFood);
			}
			FamFood->Quantity += Foods[i]->Quantity;
			goto end;
		}
		DestroyInputReq(Foods[i]);
	}
	if(i == 0)
		Log(ELOG_WARNING, "Day %i: %i made no food in PAIMakeFood.", DateToDays(g_GameWorld.Date), Family->Object.Id);
	end:
	free(Foods);
	if(FamFood->Base->Base.Category != GOOD_FOOD)
		FamilyMakeFood(Family);
	return FamFood;
}

void FamilyWorkField(struct Family* Family) {
	struct Field* Field = NULL;
	struct Array* Array = NULL;

	for(int i = 0; i < Family->FieldCt; ++i) {
		Field = Family->Fields[i];
		if(Field == NULL)
			break;
		if(Field->Status != ENONE && Field->Status != EFALLOW) {
			const struct Crop* WildCrop = HashSearch(&g_Crops, "Hay");

			Family->Food.AnimalFood += Field->UnusedAcres * ToPound(WildCrop->SeedsPerAcre) * WildCrop->NutVal;
		}
		if(Field->Status == ENONE) {
			Family->FieldCt = SelectCrops(Family, Family->Fields, Family->FieldCt, FAMILY_FIELDCT);
		} else if(Field->Status == EFALLOW && MONTH(g_GameWorld.Date) >= MARCH && MONTH(g_GameWorld.Date) <= APRIL) {
			Array = &Family->Goods;
			for(int j = 0; j < Array->Size; ++j) {
				if(strcmp(((struct Good*)Array->Table[j])->Base->Name, CropName(Field->Crop)) == 0) {
					FieldPlant(Field, Array->Table[j]);
					break;
				}
			}
		 } else if(Field->Status == EPLANTING || Field->Status == EPLOWING || Field->Status == EHARVESTING) {
			int Total = FamilyWorkModifier(Family);
			
			Field->StatusTime -= Total;
			if(Field->StatusTime <= 0) {
				if(Field->Status == EHARVESTING)
					Family->Food.SlowSpoiled += FieldHarvest((struct Field*)Field, &Family->Goods, HarvestModifier(&Family->HomeLoc->HarvestMod));
				FieldChangeStatus(Field);
			}
		}
	}
}

void FamilyCraftGoods(struct Family* Family) {
	const struct GoodBase* Good = NULL;
	struct BuyRequest* BuyReq = Family->HomeLoc->BuyOrders;
	int Quantity = 0;

	while(BuyReq != NULL) {
		for(int i = 0; Family->Profession->CraftedGoods[i] != NULL; ++i) {
			Good = Family->Profession->CraftedGoods[i];
			if(GoodBaseCmp(BuyReq->Base, Good) == 0) {
				if((Quantity = GoodCanMake(Good, &Family->Goods)) > 0) {
				GoodMake(Good, Quantity, &Family->Goods, Family->HomeLoc->Pos.x, Family->HomeLoc->Pos.y);
				}
			}
		}
		BuyReq = BuyReq->Next;
	}
}

void FamilyObjThink(struct Object* Obj) {
	struct Object* Front = Obj;

	while(Obj != NULL) {
		FamilyThink(Obj);
		Obj = Obj->Next;
	}
	if(DAY(g_GameWorld.Date) == 0) {
		for(Obj = Front; Obj != NULL; Obj = Obj->Next) {
			struct Family* Family = (struct Family*) Obj;

			if(FamilyGetNutrition(Family) / FamilyNutReq(Family) <= 31)
				PushEvent(EVENT_STARVINGFAMILY, Family, NULL);
			if(Family->Caste == CASTE_WARRIOR) {
				struct BigGuy* Guy = RBSearch(&g_GameWorld.BigGuys, Family->People[0]);
				struct Retinue* Retinue = NULL; 

				if(Guy == NULL)
					break;
				Retinue = IntSearch(&g_GameWorld.PersonRetinue, Family->People[0]->Object.Id);
				if(Guy != NULL && Retinue->Leader->Person != Family->People[0]) {
					struct BigGuyRelation* Rel = BigGuyGetRelation(Guy, Retinue->Leader);

					//Find a different settlement to live in.
					if(Random(-5, 5) + Rel->Modifier + ((int)Retinue->Leader->Glory) - Guy->Glory <= 0) {
						for(struct LnkLst_Node* Itr = g_GameWorld.Settlements.Front; Itr != NULL; Itr = Itr->Next) {
							struct Settlement* Settlement = Itr->Data;
							
							if(Settlement == Family->HomeLoc) {
								for(struct Retinue* NewRet = Settlement->Retinues; NewRet != NULL; NewRet = NewRet->Next) {
									if(NewRet->Leader->Glory > Retinue->Leader->Glory) {
										IntSearchNode(&g_GameWorld.PersonRetinue, Family->People[0]->Object.Id)->Node.Data = NewRet;
										RetinueRemoveWarrior(Retinue, Family->People[0]);
										RetinueAddWarrior(NewRet, Family->People[0]);
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

void FamilyThink(struct Object* Obj) {
	struct Family* Family = (struct Family*) Obj;
	struct Person* Person = NULL;
	const struct Crop* Hay = HashSearch(&g_Crops, "Hay");
	int FallowFood = 0; //Food generated from fallow fields.
	double Milk = 0;

	if(Family->IsAlive == false)
		return;

	FamilyWorkField(Family);
	for(int i = 0; i < Family->Animals.Size; ++i) {
		struct Animal* Animal = Family->Animals.Table[i];
		const struct Population* PopType = Animal->PopType;

		if(Animal->Nutrition <= 0) {
			FamilyTakeAnimal(Family, i);
			DestroyAnimal(Animal);
			continue;
		}
		
		if(Animal->Gender != EFEMALE)
			continue;
		Milk += PopType->Milk;
	}
	Family->Food.FastSpoiled += ((double)ToGallon(Milk)) * MILK_NUT;
	if(MONTH(g_GameWorld.Date) == MARCH && DAY(g_GameWorld.Date) == 0) {
		struct Animal* Animal = NULL;
		int Wool = 0;

		if(Family->Animals.Size < 1)
			goto escape_months;
		Animal = Family->Animals.Table[0];
		for(int i = 0; i < Family->Animals.Size; ++i) {
			Animal = (struct Animal*) Family->Animals.Table[i];
			if(Animal->PopType->Hair.Shearable != 0) {
				Wool = Wool + Animal->PopType->Hair.Pounds;
			}
		}
		CheckGoodTbl(&Family->Goods, "Wool", &g_Goods, Family->HomeLoc->Pos.x, Family->HomeLoc->Pos.y);	
	}
	if(MONTH(g_GameWorld.Date) == SEPTEMBER && DAY(g_GameWorld.Date) == 0) {
		const struct Population* PopType = NULL;
		struct Animal* Animal = NULL;
		int MaleCt = 0;
		int FemaleCt = 0;
		int Skipped = 0;
		int DeleteBuf[Family->Animals.TblSize];
		int DeleteBufSz = 0;
		int NewAnimals = 0;

		if(Family->Animals.Size == 0)
			goto escape_months;
		PopType = ((struct Animal*)Family->Animals.Table[0])->PopType;
		for(int i = 0; i < Family->Animals.Size; ++i) {
			Animal = Family->Animals.Table[i];
			if(Animal->PopType != PopType) {
				NewAnimals = AnimalsReproduce(PopType, MaleCt, FemaleCt);
				for(int i = 0; i < NewAnimals; ++i) {
					FamilyAddAnimal(Family, CreateAnimal(PopType, 0, 1500, Family->HomeLoc->Pos.x, Family->HomeLoc->Pos.y));
				}
				PopType = Animal->PopType;
				MaleCt = 0;
				FemaleCt = 0;
				continue;
			}
			if(Animal->Gender == EMALE) {
				if(Animal->Age >= Animal->PopType->Ages[AGE_DEATH]->Min && MaleCt > 1) {
					DeleteBuf[DeleteBufSz++] = i;
				} else {
					++MaleCt;
				}
			} else {
				if(Animal->Age >= Animal->PopType->Ages[AGE_DEATH]->Min) {
					if(Skipped == 0) {
						++Skipped;
						++FemaleCt;
					} else {
						DeleteBuf[DeleteBufSz++] = i;
						Skipped = 0;
					}

				} else {
					++FemaleCt;
				}
			}
		}
		NewAnimals = AnimalsReproduce(PopType, MaleCt, FemaleCt);
		for(int i = 0; i < NewAnimals; ++i) {
			FamilyAddAnimal(Family, CreateAnimal(PopType, 0, 1500, Family->HomeLoc->Pos.x, Family->HomeLoc->Pos.y));
		}
		for(int i = DeleteBufSz - 1; i >= 0; --i) {
			DestroyAnimal(FamilyTakeAnimal(Family, DeleteBuf[i]));
			Family->Food.FastSpoiled += PopType->Meat;
		}
		Family->Food.AnimalFood = 0;
	}
	escape_months:
	/*
	 * Feed people.
	 */
	for(int j = 0; j < FAMILY_PEOPLESZ; ++j) {
		int MaxFastFood = 0;
		int MaxSlowFood = 0;

		if(Family->People[j] == NULL)
			break;
		Person = Family->People[j];
		if(IsChild(Person) == 1) {
			if(Family->Food.FastSpoiled > NUTRITION_CHILDDAILY / 2) {
				MaxFastFood = NUTRITION_CHILDDAILY / 2;
			} else {
				MaxFastFood = Family->Food.FastSpoiled;
			}
			MaxSlowFood = NUTRITION_CHILDDAILY - MaxFastFood;
		} else {
			if(Family->Food.FastSpoiled > NUTRITION_DAILY / 2) {
				MaxFastFood = NUTRITION_DAILY / 2;
			} else {
				MaxFastFood = Family->Food.FastSpoiled;
			}
			MaxSlowFood = NUTRITION_DAILY - MaxFastFood;
		}
		if(MaxSlowFood > Family->Food.SlowSpoiled)
			MaxSlowFood = Family->Food.SlowSpoiled;
		Person->Nutrition += MaxFastFood + MaxSlowFood;
		Family->Food.FastSpoiled -= MaxFastFood;
		Family->Food.SlowSpoiled -= MaxSlowFood;
		if(Person->Nutrition <= 0) {
			PersonDeath(Person);
		}
	}
	//Feed animals.
	for(int i = 0; i < Family->FieldCt; ++i) {
		FallowFood += Family->Fields[i]->UnusedAcres * CropAcreHarvest(Hay) / 180;
	}
	for(int i = 0; i < Family->Animals.Size; ++i) {
		struct Animal* Animal = Family->Animals.Table[i];
		int EatAmt = Animal->PopType->Nutrition;

		if(FallowFood >= EatAmt) {
			Animal->Nutrition += EatAmt;
			FallowFood -= EatAmt;
		}
		if(Animal->Nutrition <= 0) {
			FamilyTakeAnimal(Family, i);
			DestroyAnimal(Animal);
		}
	}
	return;
}

int FamilySize(const struct Family* Family) {
	int Size = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL)
			continue;
		++Size;
	}
	return Size;
}

void Marry(struct Person* Male, struct Person* Female) {
	assert(Male->Gender == EMALE && Female->Gender == EFEMALE);
	struct Family* Family = CreateFamily(Male->Family->Name, FamilyGetSettlement(Male->Family), Male->Family);

	Family->People[HUSBAND] = Male;
	Family->People[WIFE] = Female;

}

void FamilySetCaste(struct Family* Family, const char* Caste) {
	if(strcmp(Caste, "Serf") == 0) {
		Family->Caste = CASTE_THRALL;
	} else if(strcmp(Caste, "Peasant") == 0) {
		Family->Caste = CASTE_FARMER;
	} else if(strcmp(Caste, "Craftsman") == 0) {
		Family->Caste = CASTE_CRAFTSMAN;
	} else if(strcmp(Caste, "Warrior") == 0) {
		Family->Caste = CASTE_NOBLE;
	}
}

void FamilyAddGoods(struct Family* Family, int FamilySize, lua_State* State, struct FamilyType** FamilyTypes, struct Settlement* Location) {
	int FamType = Random(0, 9999);
	int Quantity = 0;
	int X = Location->Pos.x;
	int Y = Location->Pos.y;
	const char* Name = NULL;
	const struct Population* Population = NULL;
	struct Profession* Profession = NULL;
	char* Error = NULL;
	void* Obj = NULL;

	for(int i = 0; FamilyTypes[i] != NULL; ++i) {
		if((Profession = HashSearch(&g_Professions, FamilyTypes[i]->LuaFunc)) == NULL) {
			Log(ELOG_INFO, "Family type: %s is not a profession.", FamilyTypes[i]->LuaFunc);
			continue;
		}
		if(FamilyTypes[i]->Percent * 10000 > FamType) {
			Log(ELOG_INFO, "Creating Family type: %s", FamilyTypes[i]->LuaFunc);
			++g_Log.Indents;
			lua_getglobal(State, FamilyTypes[i]->LuaFunc);
			lua_pushinteger(State, FamilySize);
			LuaCtor(State, Location, LOBJ_SETTLEMENT);
			if(LuaCallFunc(State, 2, 1, 0) == 0) {
				--g_Log.Indents;
				return;
			}
			lua_pushstring(State, "Caste");
			lua_rawget(State, -2);
			if(lua_type(State, -1) != LUA_TSTRING) {
				return (void) luaL_error(State, "Family type has invalid caste.");
			}
			FamilySetCaste(Family, lua_tostring(State, -1));
			lua_pop(State, 1);

			lua_getfield(State, -1, "Goods");
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				Obj = LuaCheckClass(State, -1, LOBJ_GOOD);
				((struct Good*)Obj)->Pos.x = X;
				((struct Good*)Obj)->Pos.y = Y;
				ArrayInsertSort_S(&Family->Goods, Obj, GoodCmp);
				lua_pop(State, 1);
			}
			lua_pop(State, 1);

			lua_getfield(State, -1, "Field");
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				int Acres = lua_tointeger(State, -1);

				Location->FreeAcres -= Acres;	
				Family->Fields[Family->FieldCt++] = CreateField(NULL, Acres, Family);
				//ArrayInsert_S(&Family->Fields, CreateField(X, Y, NULL, Acres, Family));
				lua_pop(State, 1);
			}
			lua_pop(State, 1);

			lua_getfield(State, -1, "Buildings");
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				if((Obj = (struct Building*) LuaToObject(State, -1, LOBJ_BUILDING))!= NULL)
					Family->Buildings[Family->BuildingCt++] = Obj;
					//ArrayInsertSort_S(&Family->Buildings, Obj, ObjectCmp);
				lua_pop(State, 1);
			}
			lua_pop(State, 1);

			lua_getfield(State, -1, "Animals");
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				lua_pushnil(State);
				if(lua_next(State, -2) == 0) {
					Error = "Animals";
					goto LuaError;
				}
				LuaGetString(State, -1, &Name);
				lua_pop(State, 1);
				if(lua_next(State, -2) == 0) {
					Error = "Animals";
					goto LuaError;
				}
				LuaGetInteger(State, -1, &Quantity);
				if((Population = HashSearch(&g_Populations, Name)) == NULL) {
					Log(ELOG_WARNING, "Cannot find Population %s.", Name);
					lua_pop(State, 3);
					continue;
				}
				for(int j = 0; j < Quantity; ++j)
					ArrayInsertSort_S(&Family->Animals, CreateAnimal(Population, Random(0, Population->Ages[AGE_DEATH]->Max), Population->MaxNutrition, X, Y), AnimalCmp);
				lua_pop(State, 3);
			}
			lua_pop(State, 1);
			lua_getfield(State, -1, "AI");
			if(lua_type(State, -1) != LUA_TFUNCTION)
				luaL_error(State, "Lua function expected, got %s", lua_typename(State, lua_type(State, -1)));
			Family->Profession = Profession;
			lua_pop(State, 2);
			--g_Log.Indents;
			break;
		}
		FamType -= FamilyTypes[i]->Percent * 10000;
		continue;
		LuaError:
		--g_Log.Indents;
		luaL_error(State, "In function %s the %s table does not contain a valid element.", FamilyTypes[i]->LuaFunc, Error);
		return;
	}
}

void FamilyGetGood(struct Family* Family, struct Good* Good, int Quantity) {
	SDL_assert(Quantity <= Good->Quantity);
	for(int i = 0; i < Family->Goods.Size; ++i)
		if(Good->Base == ((struct Good*)Family->Goods.Table[i])->Base) {
			((struct Good*)Family->Goods.Table[i])->Quantity += Quantity;
			if(Quantity >= Good->Quantity) {
				DestroyGood(Good);
			} else {
				Good->Quantity = Good->Quantity - Quantity;
			}
			return;
		}
	ArrayInsert(&Family->Goods, Good);
}

struct Good* FamilyTakeGood(struct Family* Family, int Index, int Quantity) {
	struct Good* Good = NULL;

	if(Index < 0 || Index >= Family->Goods.Size)
		return NULL;
	Good = Family->Goods.Table[Index];
	if(Good->Quantity > Quantity) {
		Good->Quantity = Good->Quantity - Quantity;;
		Good = g_GoodCopy[Good->Base->Category](Good);
	} else {
		ArrayRemove(&Family->Goods, Index);
	}
	return Good;
}

int FamilyNutReq(const struct Family* Family) {
	int Nutrition = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL)
			continue;
		if(IsChild(Family->People[i]) == 0)
			Nutrition += NUTRITION_DAILY;
		else
			Nutrition += NUTRITION_CHILDDAILY;
	}
	return Nutrition;
}

int FamilyGetNutrition(const struct Family* Family) {
	return Family->Food.FastSpoiled + Family->Food.SlowSpoiled;
}

struct Settlement* FamilyGetSettlement(struct Family* Family) {
	return Family->HomeLoc;
}

int FamilyCountAcres(const struct Family* Family) {
	struct Field* Field = NULL;
	int Acres = 0;

	for(int i = 0; i < Family->FieldCt; ++i) {
		Field = Family->Fields[i];
		Acres = Acres + Field->Acres + Field->UnusedAcres;
	}
	return Acres;
}

int FamilyExpectedYield(const struct Family* Family) {
	struct Field* Field = NULL;
	int Yield = 0;

	for(int i = 0; i < Family->FieldCt; ++i) {
		Field = Family->Fields[i];
		if(Field->Crop == NULL)
			continue;
		Yield = Yield + ((Field->Acres * ((Field->Crop->YieldMult - 1) * (Field->Crop->SeedsPerAcre / OUNCE)) * Field->Crop->NutVal)); //-1 to YieldMult as we assume some of the seeds will be used to regrow the crop.
	}
	return Yield;
}

int FamilyCountAnimalTypes(const struct Family* Family) {
	const struct Population* AnimalType[Family->Animals.Size];
	struct Animal* Animal = NULL;
	int TypeCt = 0;

	if(Family->Animals.Size == 0)
		return 0;
	AnimalType[TypeCt++] = ((struct Animal*) Family->Animals.Table[0])->PopType;
	for(int i = 1; i < Family->Animals.Size; ++i) {
		Animal = (struct Animal*) Family->Animals.Table[i];
		if(Animal->PopType != AnimalType[TypeCt - 1]) {
			AnimalType[TypeCt++] = Animal->PopType;
		}
	}
	return TypeCt;
}

void FamilySlaughterAnimals(struct Family* Family) {
	const struct Population* PopType = NULL;
	struct Animal* Animal = NULL;
	int MaleCt = 0;
	int Skipped = 0;
	int Meat = 0;;

	if(Family->Animals.Size == 0)
		return;
	PopType = ((struct Animal*)Family->Animals.Table[0])->PopType;
	for(int i = 0; i < Family->Animals.Size; ++i) {
		Animal = Family->Animals.Table[i];
		if(Animal->PopType != PopType) {
			PopType = Animal->PopType;
			MaleCt = 0;
			continue;
		}
		if(Animal->Age >= Animal->PopType->Ages[AGE_DEATH]->Min) {
			if(Animal->Gender == EMALE) {
				++MaleCt;
				if(MaleCt > 1) {
					Meat = Meat + PopType->Meat;
					DestroyAnimal(FamilyTakeAnimal(Family, i));
				}
			} else {
				if(Skipped == 0) {
					++Skipped;
				} else {
				Meat = Meat + PopType->Meat;
				DestroyAnimal(FamilyTakeAnimal(Family, i));
				Skipped = 0;
				}
			}
		}
	}
}

void FamilyShearAnimals(struct Family* Family) {
	struct Animal* Animal = NULL;
	int Wool = 0;

	for(int i = 1; i < Family->Animals.Size; ++i) {
		Animal = (struct Animal*) Family->Animals.Table[i];

		if(Animal->PopType->Hair.Shearable != 0) {
			Wool = Wool + Animal->PopType->Hair.Pounds * OUNCE;
		}
	}
}

void FamilyPlough(struct Family* Family) {
	Family->FieldCt = SelectCrops(Family, Family->Fields, Family->FieldCt, FAMILY_FIELDCT);
}

void FamilyPlant(struct Family* Family) {
	struct Array* Array = Array = &Family->Goods;
	struct Field* Field = NULL;

	for(int i = 0; i < Family->FieldCt; ++i) {
		Field = Family->Fields[i];
		for(int j = 0; j < Array->Size; ++j) {
			if(strcmp(((struct Good*)Array->Table[j])->Base->Name, CropName(Field->Crop)) == 0) {
				FieldPlant(Field, Array->Table[j]);
				break;
			}
		}
	}
}

void FamilyHunt(struct Family* Family) {
	
}

int FamilyWorkModifier(const struct Family* Family) {
	int WorkMod = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL || Family->People[i]->Gender == EFEMALE)
			continue;
		WorkMod = WorkMod + PersonWorkMult(Family->People[i]);
	}
	return WorkMod;
}

int FamilyCanMake(const struct Family* Family, const struct GoodBase* Good) {
	//const struct GoodBase* Base = NULL;

	if(Good->Category == GOOD_SEED) {
		for(int i = 0; i < Family->FieldCt; ++i) {
			if(strcmp(CropName(Family->Fields[i]->Crop), Good->Name) == 0) {
				return 1;
			}
		}
		return 0;
	}
	//FIXME: Check if their profession can make this good.
	/*Itr = Family->Caste->ProduceList.Front;
	while(Itr != NULL) {
		Base = (const struct GoodBase*) Itr->Data;
		if(GoodBaseCmp(Base, Good) == 0)
			return 1;
		Itr = Itr->Next;
	}*/
	return 0;
}

int FamilyGetWealth(const struct Family* Family) {
	int Wealth = 0;

	for(int i = 0; i < Family->Animals.Size; ++i) {
		Wealth += ((struct Animal*)Family->Animals.Table[i])->PopType->Wealth;
	}
	return Wealth / 100;
}

void CreateFarmerFamilies(struct GameWorld* World, struct Settlement* Settlement, struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg) {
	uint8_t AcresPerFarmer = 14;
	uint16_t MaxFarmers = Settlement->FreeAcres / AcresPerFarmer;
	uint16_t MaxFamilies = MaxFarmers;//(MaxFarmers / 10) + ((MaxFarmers % 10) != 0);
	uint8_t AnimalTypes = 3;
	uint8_t CropTypes = 1;
	const struct Crop* Crops[CropTypes];
	const struct Crop* Hay = HashSearch(&g_Crops, "Hay");
	const struct Population* Animals[AnimalTypes];
	struct Family* Parent = NULL;
	int AnimalTypeCt[AnimalTypes];
	uint8_t AnimalUsed = 0;
	uint8_t MaxChildren = 5;
	struct GoodBase* Seax = HashSearch(&g_Goods, "Seax");
	struct GoodBase* Shield = HashSearch(&g_Goods, "Shield");
	struct Good* Good = NULL;

	Crops[0] = HashSearch(&g_Crops, "Rye");
	Animals[0] = HashSearch(&g_Populations, "Ox");
	AnimalTypeCt[0] = CropAcreHarvest(Hay) * AcresPerFarmer / 2 / (Animals[0]->Nutrition * 180);
	Animals[1] = HashSearch(&g_Populations, "Sheep");
	AnimalTypeCt[1] = CropAcreHarvest(Hay) * AcresPerFarmer / 2 / (Animals[1]->Nutrition * 180);
	Animals[2] = HashSearch(&g_Populations, "Pig");
	AnimalTypeCt[2] = CropAcreHarvest(Hay) * AcresPerFarmer / 2 / (Animals[2]->Nutrition * 180);
	for(int i = 0; i < MaxFamilies; ++i) {
		Parent = CreateRandFamily("Bar", Random(0, MaxChildren) + 2, NULL, AgeGroups, BabyAvg, Settlement, CASTE_FARMER);
		/*
		 * Add animals
		 */
		AnimalUsed = Random(1, 2);
		for(int i = 0; i < AnimalTypeCt[AnimalUsed]; ++i)
			FamilyAddAnimal(Parent, CreateAnimal(Animals[AnimalUsed], Random(0, Animals[AnimalUsed]->Ages[AGE_DEATH]->Max), Animals[AnimalUsed]->MaxNutrition, Settlement->Pos.x, Settlement->Pos.y));	
		/*
		 * Add Fields
		 */
		for(int i = 0; i < CropTypes; ++i) {
			if(SettlementAllocAcres(Settlement, AcresPerFarmer) != 0) {
				Parent->Fields[Parent->FieldCt++] = CreateField(NULL, AcresPerFarmer, Parent);
				Good = CheckGoodTbl(&Parent->Goods, Crops[0]->Name, &g_Goods, Settlement->Pos.x, Settlement->Pos.y); 
				Good->Quantity = ToOunce(Crops[0]->SeedsPerAcre) * AcresPerFarmer;
			}
		}
	//	Parent->Buildings[Parent->BuildingCt] = CreateBuilding(ERES_HUMAN | ERES_ANIMAL, 
	//		HashSearch(&g_BuildMats, "Dirt"), HashSearch(&g_BuildMats, "Board"), HashSearch(&g_BuildMats, "Hay"), 500);
		Good = CreateGood(Seax, Settlement->Pos.x, Settlement->Pos.y);
		Good->Quantity = 1;
		ArrayInsert(&Parent->Goods, Good);

		Good = CreateGood(Shield, Settlement->Pos.x, Settlement->Pos.y);
		Good->Quantity = 1;
		ArrayInsert(&Parent->Goods, Good);
		RBInsert(&World->Families, Parent);
	}
}

void CreateWarriorFamilies(struct Settlement* Settlement, struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg) {
	uint16_t MaxFamilies = 5;
	uint8_t MaxChildren = 5;
	struct GoodBase* MeleeWeapons[2] = {NULL};
	struct GoodBase* Shield = HashSearch(&g_Goods, "Shield");
	struct Good* Good = NULL;
	struct Family* Parent = NULL;

	MeleeWeapons[0] = HashSearch(&g_Goods, "Spear");
	MeleeWeapons[1] = HashSearch(&g_Goods, "Sword");
	MeleeWeapons[0] = HashSearch(&g_Goods, "Spear");
	for(int i = 0; i < MaxFamilies; ++i) {
		Parent = CreateRandFamily("Bar", Random(0, MaxChildren) + 2, NULL, AgeGroups, BabyAvg, Settlement, CASTE_FARMER);
		Good = CreateGood(MeleeWeapons[0], Settlement->Pos.x, Settlement->Pos.y);
		Good->Quantity = 1; 
		ArrayInsert(&Parent->Goods, Good);
		Good = CreateGood(Shield, Settlement->Pos.x, Settlement->Pos.y);
		Good->Quantity = 1; 
		ArrayInsert(&Parent->Goods, Good);
	}
}
