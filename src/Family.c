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
#include "Culture.h"
#include "Market.h"

#include "sys/Event.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Math.h"
#include "sys/LuaCore.h"
#include "sys/Log.h"
#include "sys/RBTree.h"
#include "sys/Rule.h"
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

void InsertFamilyObj(struct Object* Obj) {
	struct Family* Family = (struct Family*) Obj;
	static struct Object* SlaveList;
	static struct Object* FarmerList;
	static struct Object* CraftsmanList;

	if(Family->Caste == CASTE_THEOW) {
		if(SlaveList != NULL) {
			Obj->Prev = SlaveList->Prev;
			Obj->Next = SlaveList;
			SlaveList->Prev = Obj;
		} else if(FarmerList != NULL) {
			Obj->Prev = FarmerList->Prev;
			Obj->Next = FarmerList;
			FarmerList->Prev = Obj;
		} else if(CraftsmanList != NULL) {
			Obj->Prev = CraftsmanList->Prev;
			Obj->Next = CraftsmanList;
			CraftsmanList->Prev = Obj;
		}
	} //else if(Family->Prof == PROF_FARMER) {
		//if(SlaveList != NULL) {

		//}
	//}
}

struct Family* CreateFamily(const char* Name, struct Settlement* Location, struct Family* Parent, uint8_t Caste, uint8_t Prof) {
	struct Family* Family = CreateObject(OBJECT_FAMILY);

	Family->Name = Name;
	memset(Family->People, 0, sizeof(struct Person*) * (FAMILY_PEOPLESZ));
	Family->NumChildren = 0;
	CtorArray(&Family->Goods, 16);
	CtorArray(&Family->Animals, 0);
	Family->HomeLoc = Location;
	Family->Parent = Parent;
	Family->Faction = FACTION_IDNONE;
	Family->Food.SlowSpoiled = 0;
	Family->Food.FastSpoiled = 0;
	Family->Food.AnimalFood = 0;
	Family->Rations = RATION_FULL;
	Family->Caste = CASTE_THEOW;
	FamilySetCaste(Family, Caste);
	SetProfession(Family, Prof);
	//memset(&Family->Spec, 0, sizeof(Family->Spec));
	for(int i = 0; i < FAMILY_ANMAX; ++i) {
		Family->AnPopTypes[i] = NULL;
		Family->AnPopSize[i] = 0;
		Family->AnMaleSize[i] = 0;
		Family->AnFemaleSize[i] = 0;
	}
	return Family;
}

struct Family* CreateRandFamily(const char* Name, int Size, struct Family* Parent, struct Constraint * const * const AgeGroups,
	 struct Constraint * const * const BabyAvg, struct Settlement* Location, uint8_t Caste, uint8_t Prof) {
	struct Family* Family = NULL;
	const struct Profession* Profession = GetProf(Prof);

	Assert(Size > 0);
	if(Size > FAMILY_PEOPLESZ)
		return NULL;
	if(Size >= 2) {
		struct Person* Wife = NULL;

		Family = CreateFamily(Name, Location, Parent, Caste, Prof);
		Family->People[HUSBAND] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(YEAR(AgeGroups[TEENAGER]->Min), YEAR(AgeGroups[ADULT]->Max)), MALE, NUTRITION_MAX, Family);
		Wife = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(YEAR(AgeGroups[TEENAGER]->Min), YEAR(AgeGroups[ADULT]->Max)), FEMALE, NUTRITION_MAX, Family);
		Family->People[WIFE] = Wife;
		Size -= 2;
		while(Size-- > 0) {
			int Child = CHILDREN + Family->NumChildren;

			Family->People[Child] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size - 1)], Random(0, (Wife->Age.Years - YEAR(AgeGroups[TEENAGER]->Min))), (1 << Random(0, 1)), NUTRITION_MAX, Family);
			++Family->NumChildren;
		}
		//if(Family->Prof == PROF_FARMER) {
		//	Family->Farmer.FieldCt = SelectCrops(Family, Family->Farmer.Fields, Family->Farmer.FieldCt, FAMILY_FIELDCT);
		//}
		Family->Food.SlowSpoiled = ((NUTRITION_REQ * 2) + (NUTRITION_CHILDREQ * Family->NumChildren)) * 2;
	}
	SettlementPlaceFamily(Location, Family);
	for(int i = 0; i < Profession->ToolSz; ++i) {
		const struct ProfTool* Tool = &Profession->Tools[i];
		struct Good* Good = CheckGoodTbl(&Family->Goods, Tool->Good);

		Good->Quantity += Random(Tool->Quantity / 2, Tool->Quantity);
	}
	FamilyBuyGoods(Family);
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
			IsAlive(Family->People[Max]) ? (PersonDeath(Family->People[Max])) : (DestroyPerson(Family->People[Max]));
			Family->People[Max] = NULL;
		}
		--Max;
	}
	if(Family->Prof == PROF_FARMER) {
		for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
			Family->HomeLoc->FreeAcres += FieldTotalAcres(Family->Farmer.Fields[i]);
			DestroyField(Family->Farmer.Fields[i]);
		}
	}
	for(int i = 0; i < Family->HomeLoc->Market.Size; ++i) {
		struct Product* Req = Family->HomeLoc->Market.Table[i];

		for(int j = 0; j < Req->Fillers.Size; ++j) {
			struct MarGood* Good = Req->Fillers.Table[j];

			if(Good->Owner == Family) {
				DestroyMarGood(Req, Good, j);
				break;
			}
		}
	}
	SettlementRemoveFamily(Family->HomeLoc, Family);
	DestroyObject(&Family->Object);
}

/*
 * FIXME: Shouldn't return a Food* as it is not used by anything that calls this function.
 * DEPRECIATED
 */
struct Food* FamilyMakeFood(struct Family* Family) {
	int FoodSize;
	struct InputReq** Foods = GoodBuildList(&g_GameWorld, &Family->Goods, &FoodSize, GOOD_FOOD | GOOD_SEED | GOOD_INGREDIENT);
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
				FamFood =  CreateFood(Food);
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

static inline void BirthAnimals(struct Family* Family, int PopIdx, struct Animal** List, uint16_t ListSz) {
	int NewAnimals = AnimalsReproduce(Family->AnPopTypes[PopIdx], Family->AnMaleSize[PopIdx], Family->AnFemaleSize[PopIdx]);

	if(ListSz < NewAnimals) {
		ListSz = NewAnimals;
		List = alloca(sizeof(struct Animal*) * ListSz);
	}
	for(int i = 0; i < NewAnimals; ++i) {
		List[i] = CreateAnimal(Family->AnPopTypes[PopIdx], 0, 0);
	}
	if(NewAnimals > 0) FamilyInsertAnimalArr(Family, List, NewAnimals);
}

void FamilyBuyTool(struct Family* Family, const struct ProfTool* Tools) {
	struct Good* Good = CheckGoodTbl(&Family->Goods, Tools->Good); 
	struct MarGood* MarGood = MarketSearch(Family, &Family->HomeLoc->Market, Good->Base);
	int Quantity = Tools->Quantity - Good->Quantity; 

	if(Quantity < 1) return;
	if(MarGood == NULL) {
		Good->Quantity += GoodBuy(Family, Good->Base, Quantity);
	} else {
		MarGood->Quantity = Quantity;
	}
}

void FamilyBuyGoods(struct Family* Family) {
	const struct Profession* Profession = GetProf(Family->Prof);

	for(int i = 0; i < Profession->ToolSz; ++i) {
		FamilyBuyTool(Family, &Profession->Tools[i]);
	}
	for(int i = 0; i < g_GameWorld.CasteGoodSz[Family->Caste]; ++i) {
		FamilyBuyTool(Family, &g_GameWorld.CasteGoods[Family->Caste][i]);
	}
}

//TODO: A list should exist for every caste and the families that are of that caste. 
//By doing this we can call different think functions based on the families caste without having to use a function pointer.
void FamilyObjThink(struct Object* Obj) {
	struct Object* Front = Obj;
	struct Family* Family = (struct Family*) Obj;
	//Chance out of 255 to become pregnant based on how many children the family already has.
	static uint8_t PregChance[CHILDREN_SIZE] = {
		6, 5, 4, 3, 2, 1, 1, 1
	};

	/*
	 *Global Farmer statements.
	 */
	if(MONTH(g_GameWorld.Date) == MARCH && DAY(g_GameWorld.Date) == 0) {
		int Wool = 0;

		for(Obj = Front; Obj != NULL; Obj = Obj->Next) {
			const struct Population* PopType = NULL;
			struct Animal* Animal = NULL;
			struct Animal** List = NULL;
			uint16_t ListSz = 0;
			int PopIdx = 0;

			if(Family->Animals.Size == 0) continue;
			PopType = Family->AnPopTypes[PopIdx];
			for(int i = 0; i < Family->Animals.Size; ++i) {
				Animal = Family->Animals.Table[i];
				if(i >= Family->AnPopSize[PopIdx]) {
					BirthAnimals(Family, PopIdx, List, ListSz);
					ListSz = 0;
					PopType = Family->AnPopTypes[++PopIdx];
					Assert(PopType != NULL);
					continue;
				}
			}
			//Generate animals for the last animal type in the family's animal table.
			BirthAnimals(Family, PopIdx, List, ListSz);
			//Shear wool from animals.
			Family = (struct Family*) Obj;
			if(Family->Prof != PROF_FARMER || Family->Animals.Size < 1) continue;
			Animal = Family->Animals.Table[0];
			Wool = 0;
			for(int i = 0; i < Family->Animals.Size; ++i) {
				Animal = (struct Animal*) Family->Animals.Table[i];
				if(Animal->PopType->Hair.Shearable != 0) {
					Wool = Wool + Animal->PopType->Hair.Pounds;
				}
			}
			GoodSell(Family, HashSearch(&g_Goods, "Wool"), Wool);
		}
	}
	/*
	 *Events for farmers in september.
	 */
	if(MONTH(g_GameWorld.Date) == SEPTEMBER && DAY(g_GameWorld.Date) == 0) {
		for(Obj = Front; Obj != NULL; Obj = Obj->Next) {
			Family = (struct Family*) Obj;
			if(Family->Prof != PROF_FARMER)
				continue;
			const struct Population* PopType = NULL;
			struct Animal* Animal = NULL;
			int DeleteBuf[Family->Animals.TblSize];
			int DeleteBufSz = 0;
			int NutTotal = 0;
			int TypeCt = 0;
			uint8_t MaleCt = 0;
			uint32_t NutCost = 0;
			float HarMod = HarvestModifier(&Family->HomeLoc->HarvestMod);

			if(Family->Animals.Size == 0)
				continue;
			PopType = ((struct Animal*)Family->Animals.Table[0])->PopType;
			for(int i = 0; i < Family->Animals.Size; ++i) {
				Animal = Family->Animals.Table[i];
				if(Animal->PopType != PopType) {
					MaleCt = 0;
					continue;
				}
				if(Animal->Age.Years >= Animal->PopType->Ages[AGE_DEATH]->Min) {
					if(Animal->Gender == MALE) {
						if(MaleCt > 1) {
							DeleteBuf[DeleteBufSz++] = i;
						} else {
							++MaleCt;
						}
					} else if(Animal->Age.Years < Animal->PopType->Ages[AGE_DEATH]->Max) {
						DeleteBuf[DeleteBufSz++] = i;
						continue;
					}
				}
			}
			for(int i = DeleteBufSz - 1; i >= 0; --i) SlaughterAnimal(Family, PopType, DeleteBuf[i]);
			//calculate max amount of expected food for the winter.
			for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
				NutCost += PastureHarvestMod(HarMod) * Family->Farmer.Fields[i]->Acres;
			}
			//calculate total amount of required  food.
			for(TypeCt = 0; TypeCt < FAMILY_ANMAX && Family->AnPopSize[TypeCt] > 0; ++TypeCt) {
				NutTotal += Family->AnPopTypes[TypeCt]->Nutrition * Family->AnPopSize[TypeCt] * (YEAR_DAYS / 2);
			}
			if(NutTotal > NutCost) {
				int NutDiff = NutTotal - NutCost;
				int Excess = NutDiff / TypeCt;//How much nutrition from each animal type that needs to be removed.
				int Offset = 0;

				for(int i = 0; i < TypeCt; ++i) {
					int ExRem = Excess;

					while(ExRem > 0) {
						//Index 0 should always be a male if a male is present, to ensure we always have at least one male never slaughter index 0.
						int Idx = Random(1, Family->AnPopSize[i]) + Offset - 1;
						Animal = Family->Animals.Table[Idx];
						ExRem -= AnimalNutReq(Animal) * (YEAR_DAYS / 2);
						SlaughterAnimal(Family, Family->AnPopTypes[i], Idx);
					}
					Offset += Family->AnPopSize[i];
				}
			}
			Family->Food.AnimalFood = 0;
		}
	}
	/*
	 * Warrior events in september
	 */
	if(MONTH(g_GameWorld.Date) == SEPTEMBER && DAY(g_GameWorld.Date) == 0) {
		for(Obj = Front; Obj != NULL; Obj = Obj->Next) {
			struct Family* Family = (struct Family*) Obj;
			int NutReq = FamilyNutReq(Family) * YEAR_DAYS;
			int Food = 0;

			//if(Family->Prof == PROF_FARMER) continue;
			Food = Family->Food.SlowSpoiled / 10;
			Family->Food.SlowSpoiled = Food * 9;
			Family->HomeLoc->Food.SlowSpoiled += Food;

			Food = Family->Food.FastSpoiled / 10;
			Family->Food.FastSpoiled=  Food * 9;
			Family->HomeLoc->Food.SlowSpoiled += Food;
			//If family doesnt have enough food take from the granary.
			if(FamilyGetNutrition(Family) < NutReq) {
				if(NutReq > Family->HomeLoc->Food.SlowSpoiled) NutReq = Family->HomeLoc->Food.SlowSpoiled;
				Family->HomeLoc->Food.SlowSpoiled -= NutReq;
				Family->Food.SlowSpoiled += NutReq;		
			}
		}
	} 
	//Code for every family every month.
	if(DAY(g_GameWorld.Date) == 0) {
		for(Obj = Front; Obj != NULL; Obj = Obj->Next) {
			struct Family* Family = (struct Family*) Obj;
			int NutReq = FamilyNutReq(Family);
			uint8_t NewRations = 0;
			uint32_t Nut = FamilyGetNutrition(Family);
			uint32_t NutTake = 0;
			uint8_t RationVal = (Nut / (NutReq * (YEAR_DAYS / 4)));

			if(!ObjectAlive(&Family->Object))
				continue;

			FamilyBuyGoods(Family);
			//Produce goods.
			if(Family->Prof >= PROF_SIZE) {
				const struct Profession* Prof = GetProf(Family->Prof);
				int WkTime = MONTH_WORKRATE;
				//int End = 0;//Family->Craftman.GoodIdx;
				struct Array* ProdArr = Prof->CraftAmt.Table[Family->Craftman.ProdIdx];

				//if(End == 0) End = Prof->CraftAmt.Size - 1;
				//End = ProdArr->Size - 1;
				while(WkTime > 0) {
					if(Family->Craftman.GoodIdx >= ProdArr->Size) { 
						for(int i = 0; i < Family->HomeLoc->Market.Size; ++i) {
							struct Product* Req = Family->HomeLoc->Market.Table[i];
							int CraftIdx = 0;

							if((CraftIdx = ProfCanMake(Prof, Req->Base)) != -1 && Req->Quantity < 0) {
								Family->Craftman.ProdIdx = CraftIdx;
								ProdArr = Prof->CraftAmt.Table[Family->Craftman.ProdIdx];
								break;
							}
						}
						Family->Craftman.GoodIdx = 0;
					}
					struct ProdOrder* Order = ProdArr->Table[Family->Craftman.GoodIdx];
					const struct GoodBase* Base = Order->Good;
					struct Good* Bought = NULL;
					int Quantity = 0;

					switch(Order->Type) {
						case ORDER_MAKE:
							if(Base->WkCost > WkTime) goto produce_end;
							Quantity = GoodCanMake(Base, &Family->Goods);
							if(Quantity * Base->WkCost > WkTime) {
								Quantity = WkTime / Base->WkCost;
							}
							if(Quantity <= 0) goto produce_end;
							if(Family->Craftman.GoodIdx == ProdArr->Size - 1){
								 GoodSell(Family, Base, Quantity);
							} else {
								struct Good* Good = CheckGoodTbl(&Family->Goods, Base);

								Good->Quantity += Quantity;
							}
							WkTime -= Quantity * Base->WkCost;
							break;
						case ORDER_BUY:
							Bought = CheckGoodTbl(&Family->Goods, Prof->InputGoods[Order->GoodIdx]);//, HashSearch(&g_Goods, Base->Name));
							if(Bought->Quantity >= Order->Quantity) break;
							Quantity = GoodBuy(Family, Base, Order->Quantity - Bought->Quantity);
							Bought->Quantity += Quantity;
							if(Quantity < Order->Quantity) goto produce_end;
							break;
					}
					++Family->Craftman.GoodIdx;
					//if(Family->Craftman.GoodIdx == End) break;
				}
			} else if(Family->Prof == PROF_WARRIOR) {
				struct Good* Spear = NULL;
				struct Good* Javalin = NULL;
				struct Good* Seax = NULL;
				struct Good* Shield = NULL;
				struct Array* BuyReqs = &Family->HomeLoc->Market;
				const struct Culture* Culture = Family->HomeLoc->Culture;

				for(int i = 0; i < Family->Goods.Size; ++i) {
					struct Good* Good = Family->Goods.Table[i];

					if(Good->Base->Category == GOOD_WEAPON) {
						struct ArmsBase* Base = (struct ArmsBase*) Good->Base;

						switch(Base->ArmsType) {
							case ARMS_SPEAR:
								Spear = Good;
								break;
							case ARMS_JAVELIN:
								Javalin = Good;
								break;
							case ARMS_SEAX:
								Seax = Good;
								break;
						}
					} else if(Good->Base->Category == GOOD_ARMOR) {
						struct ArmsBase* Base = (struct ArmsBase*) Good->Base;

						if(Base->ArmsType == ARMS_SHIELD) Shield = Good;
					}
				}
				if(Spear == NULL && MarketQuantity(Family, BuyReqs, &Culture->Arms[ARMS_SPEAR]->Base) == 0) GoodBuy(Family, &Culture->Arms[ARMS_SPEAR]->Base, 1);
				else if(Shield == NULL && MarketQuantity(Family, BuyReqs, &Culture->Arms[ARMS_SHIELD]->Base) == 0) GoodBuy(Family, &Culture->Arms[ARMS_SHIELD]->Base, 1);
				else if(Javalin == NULL && MarketQuantity(Family, BuyReqs, &Culture->Arms[ARMS_JAVELIN]->Base) == 0) GoodBuy(Family, &Culture->Arms[ARMS_JAVELIN]->Base, 3);
				else if(Seax == NULL && MarketQuantity(Family, BuyReqs, &Culture->Arms[ARMS_SEAX]->Base) == 0) GoodBuy(Family, &Culture->Arms[ARMS_SEAX]->Base, 1);
			} else if(Family->Prof == PROF_LUMBERJACK) {
				const struct GoodBase* Wood = HashSearch(&g_Goods, "Wood");

				GoodSell(Family, Wood, 350);
			} else if(Family->Prof == PROF_MINER) {
				const struct GoodBase* Iron = HashSearch(&g_Goods, "Iron Ore");

				GoodSell(Family, Iron, 16 * 20);
			}
			produce_end:
			switch(RationVal) {
				case 0:
				case 1:
					NutTake = (NutReq * (YEAR_DAYS / 2));
					if(NutTake > Family->HomeLoc->Food.SlowSpoiled) NutTake = Family->HomeLoc->Food.SlowSpoiled;
					Family->HomeLoc->Food.SlowSpoiled -= NutTake;
					Family->Food.SlowSpoiled += NutTake;
					if(NutTake < FamilyNutReq(Family) * 30) {
						NutTake = (NutReq * (YEAR_DAYS / 2));
						if(NutTake > Family->HomeLoc->Food.FastSpoiled) NutTake = Family->HomeLoc->Food.FastSpoiled;
						Family->HomeLoc->Food.FastSpoiled -= NutTake;
						Family->Food.FastSpoiled += NutTake;
					}
				case 2:
				case 3:
					NewRations = RATION_THIRD;
					break;
				case 4:
				case 5:
					NewRations = RATION_FULL;
					break;
				default:
					NewRations = RATION_FULLFOURTH;
					break;
			}
			if(Family->People[WIFE] != NULL && IsPregnant(Family->People[WIFE]) == false && Random(0, 1000) <= (PregChance[Family->NumChildren] + (FamilyGetFood(Family) >= (FamilyNutReq(Family) * YEAR_DAYS * 2)))) { 
				CreatePregnancy(Family->People[WIFE], DateAddInt(g_GameWorld.Date, Random(PREG_MINDAY, PREG_MAXDAY)), &g_GameWorld);
				Assert(IsPregnant(Family->People[WIFE]) == true);
			}
				Family->Rations = NewRations;
				for(int i = 0; i < CHILDREN + Family->NumChildren; ++i) {
					if(Family->People[i] == NULL)
						continue;
					Family->People[i]->NutRate = PersonRation(Family->People[i]);
				}


				
			if(NutReq / FamilyNutReq(Family) <= 31)
				PushEvent(EVENT_STARVINGFAMILY, Family, NULL);
			if(Family->Prof == PROF_WARRIOR) {
				struct BigGuy* Guy = RBSearch(&g_GameWorld.BigGuys, Family->People[0]);
				//struct Retinue* Retinue = NULL; 

				if(Guy == NULL) continue;
				//Retinue = IntSearch(&g_GameWorld.PersonRetinue, Family->People[0]->Object.Id);
				//if(Guy != NULL && Retinue->Leader->Person != Family->People[0]) {
				//	struct Relation* Rel = GetRelation(Guy->Relations, Retinue->Leader);

					//Find a different settlement to live in.
				//	if(Random(-5, 5) + Rel->Modifier + ((int)Retinue->Leader->Glory) - Guy->Glory <= 0) {
				//		for(int i = 0; i < g_GameWorld.Settlements.Size; ++i) {
							//struct Settlement* Settlement = g_GameWorld.Settlements.Table[i];
							
							/*if(Settlement == Family->HomeLoc) {
								for(struct Retinue* NewRet = Settlement->Retinues; NewRet != NULL; NewRet = NewRet->Next) {
									if(NewRet->Leader->Glory > Retinue->Leader->Glory) {
										IntSearchNode(&g_GameWorld.PersonRetinue, Family->People[0]->Object.Id)->Node.Data = NewRet;
										RetinueRemoveWarrior(Retinue, Family->People[0]);
										RetinueAddWarrior(NewRet, Family->People[0]);
										continue;		
									}
								}
							}*/
				//		}
						//Find another retinue.
				//	}
				//}
			}
		}
	}
}

void FarmerFamily(struct Family* Family) {
	const struct Crop* Hay = HashSearch(&g_Crops, "Hay");
	int FallowFood = 0; //Food generated from fallow fields.
	double Milk = 0;
	struct Field* Field = NULL;
	struct Array* GoodArr = NULL;


	if(!ObjectAlive(&Family->Object))
		return;
	Assert(Family->Prof == PROF_FARMER);
	for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
		Field = Family->Farmer.Fields[i];
		if(Field == NULL)
			break;
		if(Field->Status != ENONE && Field->Status != EFALLOW) {
			Family->Food.AnimalFood += Field->UnusedAcres * ToPound(Hay->SeedsPerAcre) * Hay->NutVal;
			if(Family->Food.AnimalFood < 0)
				Family->Food.AnimalFood = 0;
		}
		if(Field->Status == ENONE) {
			Family->Farmer.FieldCt = SelectCrops(Family, Family->Farmer.Fields, Family->Farmer.FieldCt, FAMILY_FIELDCT);
#ifdef DEBUG
			for(int i = 0; i < Family->Farmer.FieldCt; ++i) Assert(Family->Farmer.Fields[i] != NULL);
			Assert(Family->Farmer.FieldCt > 0);
#endif
		} else if(Field->Status == EFALLOW && MONTH(g_GameWorld.Date) >= MARCH && MONTH(g_GameWorld.Date) <= APRIL) {
			GoodArr = &Family->Goods;
			for(int j = 0; j < GoodArr->Size; ++j) {
				if(((struct Good*)GoodArr->Table[j])->Base->Id == Field->Crop->Output->Base.Id) {
					FieldPlant(Field, GoodArr->Table[j]);
					break;
				}
			}
		 }
	}
	for(int i = 0; i < Family->Animals.Size; ++i) {
		struct Animal* Animal = Family->Animals.Table[i];
		const struct Population* PopType = Animal->PopType;
		
		if(Animal->Gender != FEMALE)
			continue;
		Milk += PopType->Milk;
	}
	//Feed animals.
	for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
		FallowFood += Family->Farmer.Fields[i]->UnusedAcres * CropAcreHarvest(Hay) / 180;
	}
	Family->Food.FastSpoiled += ((double)ToGallon(Milk)) * MILK_NUT;
	return;
}

void FamilyThink(struct Object* Obj) {
	struct Family* Family = (struct Family*) Obj;
	struct Person* Person = NULL;

	if(!ObjectAlive(&Family->Object))
		return;
	switch(Family->Prof) {
		case PROF_FARMER:
			FarmerFamily(Family);
			break;
	}
	/*
	 * Feed people.
	 */
	for(int j = 0; j < FAMILY_PEOPLESZ; ++j) {
		int MaxFastFood = 0;
		int MaxSlowFood = 0;

		if(Family->People[j] == NULL)
			continue;
		Person = Family->People[j];
		if(IsChild(Person) == true) {
			MaxFastFood = g_AdultRations[Family->Rations];
			if(MaxFastFood > Family->Food.FastSpoiled) {
				MaxFastFood = Family->Food.FastSpoiled;
			}
			MaxSlowFood = g_AdultRations[Family->Rations] - MaxFastFood;
		} else {
			MaxFastFood = g_AdultRations[Family->Rations];
			if(MaxFastFood > Family->Food.FastSpoiled) {
				MaxFastFood = Family->Food.FastSpoiled;
			}
			MaxSlowFood = g_AdultRations[Family->Rations] - MaxFastFood;
		}
		if(MaxSlowFood > Family->Food.SlowSpoiled)
			MaxSlowFood = Family->Food.SlowSpoiled;
		Person->Nutrition += MaxFastFood + MaxSlowFood;
		Family->Food.FastSpoiled -= MaxFastFood;
		Family->Food.SlowSpoiled -= MaxSlowFood;
	}
	return;
}

void FamilySetCasteSlave(struct Family* Family, struct Family* Owner) {
	if(Family->Caste == CASTE_THEOW) {
		ArrayRemoveC(&Family->HomeLoc->Slaves, (void*) Family->Slave.Owner, ObjectCmp);
	}
	ArrayInsertSort_S(&Family->HomeLoc->Slaves, Owner, ObjectCmp);
}

int FamilySize(const struct Family* Family) {
	int Size = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL || !IsAlive(Family->People[i]))
			continue;
		++Size;
	}
	return Size;
}

/*void FamilySetCaste(struct Family* Family, const char* Caste) {
	if(strcmp(Caste, "Serf") == 0) {
		Family->Caste = CASTE_THEOW;
	} else if(strcmp(Caste, "Peasant") == 0) {
		Family->Caste = CASTE_FARMER;
	} else if(strcmp(Caste, "Craftsman") == 0) {
		Family->Caste = CASTE_CRAFTSMAN;
	} else if(strcmp(Caste, "Warrior") == 0) {
		Family->Caste = CASTE_NOBLE;
	}
}*/

void FamilyAddGoods(struct Family* Family, int FamilySize, lua_State* State, struct FamilyType** FamilyTypes, struct Settlement* Location) {
	int FamType = Random(0, 9999);
	int Quantity = 0;
	const char* Name = NULL;
	const struct Population* Population = NULL;
	struct Profession* Profession = NULL;
	char* Error = NULL;
	const char* Caste = NULL;
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
			Caste = lua_tostring(State, -1);
			if(strcmp(Caste, "Theow") == 0) {
				Family->Caste = CASTE_THEOW;
			} else if(strcmp(Caste, "Gebur") == 0) {
				Family->Caste = CASTE_GEBUR;
			} else if(strcmp(Caste, "Geneat") == 0) {
				Family->Caste = CASTE_GENEAT;
			} else if(strcmp(Caste, "Thegn") == 0) {
				Family->Caste = CASTE_THEGN;
			}
			lua_pop(State, 1);

			lua_getfield(State, -1, "Goods");
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				Obj = LuaCheckClass(State, -1, LOBJ_GOOD);
				ArrayInsertSort_S(&Family->Goods, Obj, GoodCmp);
				lua_pop(State, 1);
			}
			lua_pop(State, 1);

			lua_getfield(State, -1, "Field");
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				int Acres = lua_tointeger(State, -1);

				if(LocationCreateField(Family, Acres) == false) {
					Log(ELOG_WARNING, "Not enough acres to create field for family %i in location %i.", Family->Object.Id, Family->HomeLoc->Object.Id);
				} 
				//ArrayInsert_S(&Family->Farmer.Fields, CreateField(X, Y, NULL, Acres, Family));
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
				{
					struct Animal** AnList = alloca(sizeof(struct Animal*) * Quantity);

					for(int j = 0; j < Quantity; ++j)
							AnList[j] = CreateAnimal(Population, Random(0, Population->Ages[AGE_DEATH]->Max), 0);
					FamilyInsertAnimalArr(Family, AnList, Quantity);
					lua_pop(State, 3);
				}
			}
			lua_pop(State, 1);
			lua_getfield(State, -1, "AI");
			if(lua_type(State, -1) != LUA_TFUNCTION)
				luaL_error(State, "Lua function expected, got %s", lua_typename(State, lua_type(State, -1)));
			//Family->Profession = Profession;
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

int FamilyNutReq(const struct Family* Family) {
	int Nutrition = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL)
			continue;
		Nutrition += NUTRITION_CHILDDAILY + (NUTRITION_CHILDDAILY * (IsChild(Family->People[i]) == 0));
		/*if(IsChild(Family->People[i]) == 0)
			Nutrition += NUTRITION_DAILY;
		else
			Nutrition += NUTRITION_CHILDDAILY;*/
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

	for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
		Field = Family->Farmer.Fields[i];
		Acres = Acres + Field->Acres + Field->UnusedAcres;
	}
	return Acres;
}

int FamilyExpectedYield(const struct Family* Family) {
	struct Field* Field = NULL;
	int Yield = 0;

	for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
		Field = Family->Farmer.Fields[i];
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

/*void FamilySlaughterAnimals(struct Family* Family) {
	const struct Population* PopType = NULL;
	struct Animal* Animal = NULL;
	int MaleCt = 0;
	int Skipped = 0;
	int Meat = 0;

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
		if(Animal->Age.Years >= Animal->PopType->Ages[AGE_DEATH]->Min) {
			if(Animal->Gender == MALE) {
				++MaleCt;
				if(MaleCt > 1) {
					Meat = Meat + PopType->Meat;
					DestroyAnimal(FamilyRemoveAnimal(Family, i));
				}
			} else {
				if(Skipped == 0) {
					++Skipped;
				} else {
				Meat = Meat + PopType->Meat;
				DestroyAnimal(FamilyRemoveAnimal(Family, i));
				Skipped = 0;
				}
			}
		}
	}
}*/

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
	Family->Farmer.FieldCt = SelectCrops(Family, Family->Farmer.Fields, Family->Farmer.FieldCt, FAMILY_FIELDCT);
}

void FamilyPlant(struct Family* Family) {
	struct Array* Array = Array = &Family->Goods;
	struct Field* Field = NULL;

	for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
		Field = Family->Farmer.Fields[i];
		for(int j = 0; j < Array->Size; ++j) {
			if(strcmp(((struct Good*)Array->Table[j])->Base->Name, CropName(Field->Crop)) == 0) {
				FieldPlant(Field, Array->Table[j]);
				break;
			}
		}
	}
}

void Hunt(struct Family* Family) {
	if(Random(1, 10) > 7) return;
	Family->Food.FastSpoiled += 400;
}

int FamilyWorkModifier(const struct Family* Family) {
	int WorkMod = 0;

	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL || Gender(Family->People[i]) == FEMALE)
			continue;
		WorkMod = WorkMod + PersonWorkMult(Family->People[i]);
	}
	return WorkMod;
}

int FamilyCanMake(const struct Family* Family, const struct GoodBase* Good) {
	//const struct GoodBase* Base = NULL;

	if(Good->Category == GOOD_SEED) {
		for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
			if(strcmp(CropName(Family->Farmer.Fields[i]->Crop), Good->Name) == 0) {
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
	return Wealth;
}

void FamilyRemovePerson(struct Family* Family, struct Person* Person) {
	if(Family->People[HUSBAND] == Person) {
		Family->People[HUSBAND] = NULL;
		Person->Family = NULL;
		goto end;
	}
	if(Family->People[WIFE] == Person) {
		Family->People[WIFE] = NULL;
		Person->Family = NULL;
		goto end;
	}
	for(int i = CHILDREN; i < Family->NumChildren + CHILDREN; ++i) {
		if(Family->People[i] == Person) {
			--Family->NumChildren;
			Family->People[i] = Family->People[CHILDREN + Family->NumChildren];
			Family->People[CHILDREN + Family->NumChildren] = NULL;
			goto end;
		}
	}
	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] != NULL)
			return;
	}
	end:
	if(Family->People[HUSBAND] == NULL && Family->People[WIFE] == NULL && Family->NumChildren == 0) {
		ObjectDie(&Family->Object);
	}
}

bool FamilyAddPerson(struct Family* Family, struct Person* Person) {
	if(Family->NumChildren + 2 >= FAMILY_PEOPLESZ)
		return false;
	Family->People[Family->NumChildren + 2] = Person;
	FamilyRemovePerson(Person->Family, Person);
	Person->Family = Family;
	return true;
}

int CreateFarmerFamilies(struct Settlement* Settlement, int Families, double CastePercent[CASTE_SIZE], struct Constraint * const * const AgeGroups, struct Constraint * const * const BabyAvg) {
	uint8_t AcresPerFarmer = FAMILY_ACRES;
	uint8_t AnimalTypes = 3;
	uint8_t CropTypes = 1;
	const struct Crop* Crops[CropTypes];
	const struct Crop* Hay = PastureCrop();
	struct Family* Parent = NULL;
	uint8_t MaxChildren = 5;
	struct Good* Good = NULL;
	struct Animal** AnList = NULL;
	uint16_t AnListSz = 0;
	uint32_t FamAb = Settlement->Meadow.MonthNut / Families; //Family animal nutrition budget.
	uint8_t AnimalUsed = 0;
	const struct Population* Animals[AnimalTypes];
	int AnimalTypeCt[AnimalTypes];
	const struct Culture* Culture = Settlement->Culture;
	const struct WarRoleRule* Rules = Culture->WarriorRules[WARROLE_LIGHTINF];
	int* CasteCount = alloca(sizeof(int) * CASTE_SIZE);
	int8_t Caste = CASTE_THEOW;
	int AnimalMod = 0;
	int Failures = 0;

	NormalizeTable(CastePercent, CASTE_SIZE);
	RandTable(CastePercent, &CasteCount, CASTE_SIZE, Families);

	Crops[0] = HashSearch(&g_Crops, "Rye");
	Animals[0] = HashSearch(&g_Populations, "Ox");
	AnimalTypeCt[0] = CropAcreHarvest(Hay) * (AcresPerFarmer / 2) / (Animals[0]->Nutrition * 180);
	Animals[1] = HashSearch(&g_Populations, "Sheep");
	AnimalTypeCt[1] = CropAcreHarvest(Hay) * (AcresPerFarmer / 2) / (Animals[1]->Nutrition * 180);
	Animals[2] = HashSearch(&g_Populations, "Pig");
	AnimalTypeCt[2] = CropAcreHarvest(Hay) * (AcresPerFarmer / 2) / (Animals[2]->Nutrition * 180);
	for(int i = 0; i < Families; ++i) {
		while(CasteCount[Caste] <= 0) ++Caste; 
		--CasteCount[Caste];
		Assert(Caste < CASTE_SIZE);
		Parent = CreateRandFamily("Bar", Random(0, MaxChildren) + 2, NULL, AgeGroups, BabyAvg, Settlement, Caste, PROF_FARMER);
		/*
		 * Add animals
		 */
		AnimalUsed = Random(0, 5) > 0;
		switch(Caste) {
			case CASTE_THEOW:
				AnimalMod = 0;
				break;
			case CASTE_GEBUR:
				AnimalMod = AnimalTypeCt[AnimalUsed] / 2;
				break;
			case CASTE_GENEAT:
				AnimalMod = AnimalTypeCt[AnimalUsed];
				break;
		}

		AnListSz = NormalRandom() * AnimalMod;
		AnList = alloca(sizeof(struct Animal*) * AnListSz);
		if(AnimalTypeCt[AnimalUsed] < 2) goto fields;
		if(Animals[AnimalUsed]->Nutrition * AnimalTypeCt[AnimalUsed] > FamAb) AnimalTypeCt[AnimalUsed] = FamAb / Animals[AnimalUsed]->Nutrition;
		//Ensure that we get at least one of each gender.
		AnList[0] = CreateAnimal(Animals[AnimalUsed], Random(0, Animals[AnimalUsed]->Ages[AGE_DEATH]->Max), 0);	
		AnList[0]->Gender = MALE;
		AnList[1] = CreateAnimal(Animals[AnimalUsed], Random(0, Animals[AnimalUsed]->Ages[AGE_DEATH]->Max), 0);	
		AnList[1]->Gender = FEMALE;
		for(int i = 2; i < AnListSz; ++i) {
			AnList[i] = CreateAnimal(Animals[AnimalUsed], Random(0, Animals[AnimalUsed]->Ages[AGE_DEATH]->Max), 0);	
			Parent->Food.AnimalFood += AnimalNutReq(AnList[i]) * (YEAR_DAYS / 2);
		}
		FamilyInsertAnimalArr(Parent, AnList, AnListSz);
		/*
		 * Add Fields
		 */
		fields:
		for(int i = 0; i < CropTypes; ++i) {
			if(SettlementAllocAcres(Settlement, AcresPerFarmer) == true) {
				Parent->Farmer.Fields[Parent->Farmer.FieldCt++] = CreateField(NULL, AcresPerFarmer, Parent);
				Good = CheckGoodTbl(&Parent->Goods, HashSearch(&g_Goods, Crops[0]->Name)); 
				Good->Quantity = ToOunce(Crops[0]->SeedsPerAcre) * AcresPerFarmer * 2;
			} else {
				if(i == 0) { 
					DestroyFamily(Parent);
					++Failures;
					goto create_family;
				}
			}
		}
	//	Parent->Buildings[Parent->BuildingCt] = CreateBuilding(ERES_HUMAN | ERES_ANIMAL, 
	//		HashSearch(&g_BuildMats, "Dirt"), HashSearch(&g_BuildMats, "Board"), HashSearch(&g_BuildMats, "Hay"), 500);
		for(int i = 0; Rules[i].Rule != RULE_NONE; ++i) {
			if(Random(0, 255) <= Rules[i].Rarity) {
				Good = CreateGood(&Culture->Arms[Rules[i].Type]->Base);
				switch(Rules[i].Rule) {
					case RULE_EQUAL:
						Good->Quantity = Rules[i].Quantity;
						break;
					case RULE_GREATERTHAN:
						if(Rules[i + 1].Rule == RULE_LESSTHAN) {
							Good->Quantity = Random(Rules[i].Quantity, Rules[i + 1].Quantity);
						} else {
							Good->Quantity = 1;
						}
						break;
					default:
						Good->Quantity = 1;
				}
				ArrayInsert(&Parent->Goods, Good);
			}
		}
		create_family:;
	}
	return Failures;
}

int CreateWarriorFamilies(struct Settlement* Settlement, struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg) {
	uint16_t MaxFamilies = 5;
	uint8_t MaxChildren = 5;
	struct GoodBase* MeleeWeapons[3] = {NULL};
	struct GoodBase* Shield = HashSearch(&g_Goods, "Shield");
	struct Good* Good = NULL;
	struct Family* Parent = NULL;

	MeleeWeapons[0] = HashSearch(&g_Goods, "Spear");
	MeleeWeapons[1] = HashSearch(&g_Goods, "Sword");
	MeleeWeapons[2] = HashSearch(&g_Goods, "Javelin");
	for(int i = 0; i < MaxFamilies; ++i) {
		Parent = CreateRandFamily("Bar", Random(0, MaxChildren) + 2, NULL, AgeGroups, BabyAvg, Settlement, CASTE_THEGN, PROF_WARRIOR);
		Good = CreateGood(MeleeWeapons[0]);
		Good->Quantity = 1; 
		ArrayInsert(&Parent->Goods, Good);
		Good = CreateGood(Shield);
		Good->Quantity = 1; 
		ArrayInsert(&Parent->Goods, Good);

		Good = CreateGood(MeleeWeapons[2]);
		Good->Quantity = Random(1, 3);
		ArrayInsert(&Parent->Goods, Good);
	}
	return 0;
}

int CreateCrafterFamilies(struct Settlement* Settlement, int Families, double CastePercent[CASTE_SIZE], struct Constraint * const * const AgeGroups, struct Constraint * const * const BabyAvg) {
	int* CasteCount = alloca(sizeof(int) * CASTE_SIZE);
	//int8_t Caste = CASTE_THEOW;
	//int Prof = PROF_LUMBERJACK;
	int Trees = 0;
	SDL_Point Point = {Settlement->Pos.x, Settlement->Pos.y};
	int Radius = SettlementGetTiles(g_GameWorld.MapRenderer, Settlement);
	int TileCt = NumTileRadius(Radius);
	struct Tile** Tiles = alloca(sizeof(struct Tile*) * TileCt);

	TileSpiral(g_GameWorld.MapRenderer, &Point, Radius, Tiles);

	NormalizeTable(CastePercent, CASTE_SIZE);
	RandTable(CastePercent, &CasteCount, CASTE_SIZE, Families);

	for(int i = 0; i < TileCt; ++i) {
		if(Tiles[i] == NULL) continue;
		Trees += Tiles[i]->Trees;
	}
	//uint8_t MaxChildren = 4;
/*	struct Family* Family = NULL;
	uint8_t JobCt[CRAFT_SIZE] = {0};

	switch(Families) {
		case 1:
			JobCt[CRAFT_MILLER] = 1;
			break;
		case 2:
			JobCt[CRAFT_BUTCHER] = 1;
			break;
	}
	for(int i = 0; i < CRAFT_SIZE; ++i) {
		for(int j = 0; j < JobCt[i]; ++j) {
			Family = CreateRandFamily("Bar", Random(0, MaxChildren) + 2, NULL, AgeGroups, BabyAvg, Settlement, CASTE_CRAFTSMAN);
			Family->Crafter.Prof = i;
		}
	}*/
	//CreateRandFamily("Bar", Random(0, MaxChildren) + 2, NULL, AgeGroups, BabyAvg, Settlement, CASTE_GENEAT, PROF_MILLER);
	return 0;
}

void FamilyInsertAnimalArr(struct Family* Family, struct Animal** Animals, uint8_t AnSize) {
	const struct Population* PopType = NULL;
	uint8_t PopIdx = 0;
	uint8_t PopCt = 0;
	uint16_t Offset = 0;
	uint8_t PopStart = 0;
	uint16_t NutCost = 0; //How much additional nut val the new animals will require.

	if(AnSize == 0) return;
	PopType = Animals[0]->PopType;
	for(;PopIdx < FAMILY_ANMAX; ++PopIdx) {
		if(Family->AnPopTypes[PopIdx] == PopType)
			goto found_type;
	}
	for(int i = 0; i < FAMILY_ANMAX; ++i, ++PopCt) {
		if(Family->AnPopTypes[i] == NULL)
			break;
	}
	//Asserts if PopType is not in array and cannot be added.
	if(PopCt < FAMILY_ANMAX) {
		Family->AnPopTypes[PopCt] = PopType;
		PopIdx = PopCt;
	} else {
		Assert(true);
		return;
	}
	found_type:
	Family->Animals.Size += AnSize;
	if(Family->Animals.Size >= Family->Animals.TblSize)
		ArrayGrow(&Family->Animals, Family->Animals.Size);
	//Gets index of first animal of the added population type.
	for(int i = 0; i <= PopIdx; ++i) {
		Offset += Family->AnPopSize[i];
	}
	PopStart = Family->AnPopSize[PopIdx];
	Family->AnPopSize[PopIdx] += AnSize;
	for(int i = PopIdx; i < FAMILY_ANMAX && Family->AnPopTypes[i] != NULL; ++i) {
		uint8_t InsertCt = 0;

		Assert(Offset + AnSize <= Family->Animals.Size);
		for(int j = Offset; j < Offset + AnSize; ++j, ++InsertCt) {
			struct Animal* Temp = Family->Animals.Table[j];

			Family->Animals.Table[j] = Animals[InsertCt];
			NutCost += AnimalNutReq(Animals[InsertCt]);
			if(AnimalMature(Animals[InsertCt]) == true) {
				if(Animals[InsertCt]->Gender == MALE) ++Family->AnMaleSize[PopIdx];
				else ++Family->AnFemaleSize[PopIdx];
			}
			Animals[InsertCt] = Temp;
		}
	}
	Assert(Family->AnMaleSize[PopIdx] + Family->AnFemaleSize[PopIdx] <= Family->AnPopSize[PopIdx]);
	Family->HomeLoc->Meadow.NutRem -= NutCost;
	InsertionSortPtr(&Family->Animals.Table[Offset - PopStart], Family->AnPopSize[PopIdx], AnimalGenderCmp);
}

struct Animal* FamilyRemoveAnimal(struct Family* Family, uint32_t Index) {
	const struct Population* PopType = ((struct Animal*)Family->Animals.Table[Index])->PopType;
	uint8_t PopIdx = 0;
	uint16_t Offset = 0;
	struct Animal* Animal = NULL;

	for(;PopIdx < FAMILY_ANMAX; ++PopIdx) {
		if(Family->AnPopTypes[PopIdx] == PopType)
			goto found_type;
	}
	Assert(true);
	return NULL;
	found_type:
	for(int i = 0; i <= PopIdx; ++i) {
		Offset += Family->AnPopSize[i];
	}
	Animal = Family->Animals.Table[Index];
	for(int i = PopIdx; i < FAMILY_ANMAX && Family->AnPopTypes[i] != NULL; ++i, Offset += Family->AnPopSize[i]) {
		Family->Animals.Table[Index] = Family->Animals.Table[Offset - 1];
	}
	if(AnimalMature(Animal) == true) {
		if(Animal->Gender == MALE) --Family->AnMaleSize[PopIdx];
		else --Family->AnFemaleSize[PopIdx];
	}
	Family->Animals.Table[Family->Animals.Size - 1] = NULL;
	--Family->AnPopSize[PopIdx];
	--Family->Animals.Size;
	Assert(Family->AnMaleSize[PopIdx] + Family->AnFemaleSize[PopIdx] <= Family->AnPopSize[PopIdx]);
	return Animal;
}

struct Family* GetSlave(struct Family* Owner) {
	struct Settlement* Settlement = Owner->HomeLoc;

	return BinarySearch(Owner, Settlement->Slaves.Table, Settlement->Slaves.Size, ObjectCmp);
}

void FamilyDivideAnimals(struct Family* To, struct Family* From, int Divide) {
	struct Animal* Animal = NULL;
	struct Animal** AnList = alloca(sizeof(struct Animal*) * From->Animals.Size);
	int CurrAn = 0;
	int Offset = 0;
	int TotalCt = 0;
	int MaxMale = 0;
	int MaxFemale = 0;
	int MaleCt = 0;
	int FemaleCt = 0;


	//TODO: We shouldn't use FamilyRemoveAnimal here, we should be batching the removals instead.
	for(int AnType = 0; From->AnPopSize[AnType] > 0; ++AnType) {
		if(From->AnMaleSize[AnType] < Divide) continue;

		TotalCt = From->AnPopSize[AnType] / Divide;
		MaxMale = TotalCt / 2;
		MaxFemale = TotalCt - MaxMale;
		MaleCt = 0;
		FemaleCt = 0;
		for(int i = 0; i < From->AnPopSize[AnType]; ++i) {
			Animal = From->Animals.Table[Offset + i];
			if(Animal->Gender == MALE) {
				if(MaleCt < MaxMale) {
					AnList[CurrAn++] = Animal;
					 ++MaleCt;
					FamilyRemoveAnimal(From, Offset + i);
				}
			} else {
				if(FemaleCt < MaxFemale) {
					++FemaleCt;
					AnList[CurrAn++] = Animal;
					FamilyRemoveAnimal(From, Offset + i);
				}
			}
		}
		Offset += From->AnPopSize[AnType];
		FamilyInsertAnimalArr(To, AnList, FemaleCt + MaleCt);
	}
}

void SlaughterAnimal(struct Family* Family, const struct Population* PopType, int AnIdx) {
	Family->HomeLoc->Meadow.NutRem += AnimalNutReq(Family->Animals.Table[AnIdx]);
	Family->Food.FastSpoiled += AnMeat(PopType, Family->Animals.Table[AnIdx]);
	GoodSell(Family, HashSearch(&g_Goods, PopType->Skin.Skin->Name), PopType->Skin.Pounds * OUNCE);
	DestroyAnimal(FamilyRemoveAnimal(Family, AnIdx));
}

void SetProfession(struct Family* Family, uint8_t Prof) {
	struct Settlement* Settlement = Family->HomeLoc;

	if(IsFarmer(Family)) {
		for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
			int Acres = FieldTotalAcres(Family->Farmer.Fields[i]);

			Settlement->FreeAcres += Acres;
		}
	}

	SettlementChangeProf(Settlement, Family, Prof);
	Family->Prof = Prof;

	if(IsMerchant(Family)) {
		CtorArray(&Family->Merchant.Nodes, 8);
	} else if(IsCraftsman(Family)) {
		Family->Craftman.ProdIdx = 0;
		Family->Craftman.GoodIdx = 0;
	}
}

void MerchantCreatePath(struct Family* Family) {
	Assert(Family->Prof == PROF_MERCHANT);


}

void LoadCasteGoods(lua_State* State, struct ProfTool** Tools, int* Size) {
	int Idx = lua_absindex(State, -1);
	int Len = lua_rawlen(State, Idx);
	const char* ToolName = NULL;

	*Tools = calloc(Len, sizeof(struct ProfTool));
	for(int i = 1; i <= Len; ++i) {
		lua_rawgeti(State, -1, i);
		if(lua_type(State, -1) != LUA_TTABLE) {
			lua_pop(State, 1);
			continue;
		}
		lua_rawgeti(State, -1, 1);
		ToolName = lua_tostring(State, -1);
		(*Tools)[i - 1].Good = HashSearch(&g_Goods, ToolName);
		lua_rawgeti(State, -2, 2);
		(*Tools)[i - 1].Quantity = lua_tointeger(State, -1);
		lua_pop(State, 3);
	}
	*Size = Len;
}
