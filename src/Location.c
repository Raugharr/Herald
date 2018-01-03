/**
 * File: Location.c
 * Author: David Brotz
 */

#include "Location.h"

#include "BigGuy.h"
#include "Person.h"
#include "Family.h"
#include "Government.h"
#include "Warband.h"
#include "Crop.h"
#include "Bulletin.h"
#include "Retinue.h"
#include "Mission.h"
#include "Crisis.h"
#include "Profession.h"
#include "Market.h"

#include "sys/Math.h"
#include "sys/ResourceManager.h"
#include "sys/Array.h"
#include "sys/Log.h"
#include "sys/Coroutine.h"
#include "sys/Rule.h"
#include "sys/LuaCore.h"

#include "video/Sprite.h"
#include "video/Tile.h"
#include "video/MapRenderer.h"

#include <stdlib.h>
#include <string.h>

#define HARVESTMOD_MIN 0.4f
#define HARVESTMOD_MAX 1.6f
#define SETTLEMENT_AVGSTAT (50)

static const char* g_SettlementStrs[SET_SIZE] = {
	"Hamlet",
	"Village",
	"Town",
	"City"
};

uint8_t UpdateHarvestMod(uint8_t (*HarvestYears)[HARVEST_YEARS], uint8_t CurrYear);

void SettlementOnPolicyChange(const struct EventData* Data, void* Extra) {
	struct Settlement* Settlement = Data->OwnerObj;
	struct BigGuy* Guy = NULL;
	struct BigGuy* Owner = Settlement->Government->Leader;
	int Amount = *(int*)Extra;

	for(struct LnkLst_Node* Itr = Settlement->BigGuys.Front; Itr != NULL; Itr = Itr->Next) {
		Guy = Itr->Data;
		if(Guy == Owner)
			continue;
		AddOpinion(Guy, Owner, ACTTYPE_POLICY, Amount * 10, OPNLEN_MEDIUM, OPINION_AVERAGE, &Guy->Relations);
	}
}

void SettlementSetBGOpinions(struct LinkedList* List) {
	struct LnkLst_Node* i = NULL;
	struct LnkLst_Node* j = NULL;

	for(i = List->Front; i != NULL; i = i->Next) {
		for(j = List->Front; j != NULL; j = j->Next) {
			if(i == j) {
				continue;
			}
			CreateBigGuyRelation(((struct BigGuy*)i->Data), ((struct BigGuy*)j->Data));
		}
	}
}

struct Settlement* CreateSettlement(struct GameWorld* World, int X, int Y, const char* Name, int GovType) {
	struct Settlement* Loc = CreateObject(OBJECT_LOCATION);

	Loc->Pos.x = X;
	Loc->Pos.y = Y;
	Loc->Name = calloc(strlen(Name) + 1, sizeof(char));
	CtorArray(&Loc->People, 64);
	CtorArray(&Loc->Slaves, 20);
	Loc->Government = CreateGovernment(Loc, GovType & GOVRULE_MASK, GovType & GOVTYPE_MASK, GovType & GOVSTCT_MASK, GovType & GOVMIX_MASK, 0);
	Assert(Loc->Government != NULL);
	Loc->BigGuys.Size = 0;
	Loc->BigGuys.Front = NULL;
	strcpy(Loc->Name, Name);
	ArrayInsert_S(&World->Settlements, Loc);
	QTInsertPoint(&World->MapRenderer->RenderArea[MAPRENDER_SETTLEMENT], Loc, &Loc->Pos);
	CtorArray(&Loc->Families, 100);
	CtorArray(&Loc->Suitors, 8);
	CtorArray(&Loc->Brides, 8);
	Loc->YearBirths = 0;
	Loc->YearDeaths = 0;
	Loc->Culture = World->DefCulture;
	for(uint8_t i = 0; i < HARVEST_YEARS - 1; ++i)
		Loc->HarvestMod[i] = 5;
	Loc->HarvestMod[HARVEST_YEARS - 1] = 6;
	//Loc->Sprite =  CreateGameObject(g_GameWorld.MapRenderer, ResourceGet("Settlement.png"), MAPRENDER_SETTLEMENT, &Loc->Pos);
	Loc->Meadow.Crop = HashSearch(&g_Crops, "Hay");
	Loc->Meadow.Acres = SETTLEMENT_SPACE / 2;
	Loc->Meadow.MonthNut = Loc->Meadow.Acres * ToPound(Loc->Meadow.Crop->SeedsPerAcre) * Loc->Meadow.Crop->YieldMult * HarvestModifier((uint8_t (* const)[HARVEST_YEARS])&Loc->HarvestMod); 
	Loc->Meadow.NutRem = Loc->Meadow.MonthNut;
	Loc->Food.SlowSpoiled = 0;
	Loc->Food.FastSpoiled = 0;
	Loc->Food.AnimalFood = 0;
	CtorArray(&Loc->BuyReqs, 8);
	CtorArray(&Loc->Market, 4);
	CtorArray(&Loc->Tparts, 4);
	Loc->AdultMen = 0;
	Loc->AdultWomen = 0;
	CtorArray(&Loc->Bulletin, 8);
	Loc->MaxWarriors = 0;
	Loc->FreeAcres = SETTLEMENT_SPACE - Loc->Meadow.Acres;
	Loc->UsedAcres = 0;
	//FIXME: Each family should take an acre of land.
	CtorFaction(&Loc->Factions, Loc);
	GenerateStats(CASTE_GENEAT, &Loc->Stats);
	for(int i = 0; i < HARVEST_YEARS; ++i) UpdateHarvestMod(&Loc->HarvestMod, i);
	Loc->HarvestYear = 0;
	ConstructLinkedList(&Loc->FreeWarriors);
	CtorArray(&Loc->Crisis, 8);
	Assert(Loc->FreeAcres >= 100);
	return Loc;
}

void DestroySettlement(struct Settlement* Location) {
	//DestroyGameObject(Location->Sprite);
	DtorArray(&Location->People);
	DtorArray(&Location->Suitors);
	DtorArray(&Location->Brides);
	DtorArray(&Location->Crisis);
	DtorArray(&Location->Families);
	for(int i = 0; i < Location->Bulletin.Size; ++i) {
		struct BulletinItem* Obj = Location->Bulletin.Table[i];

		DestroyBulletinItem(Obj);
	}
	DtorArray(&Location->Bulletin);
	DtorArray(&Location->Market);
	free(Location->Name);
	DestroyObject((struct Object*)Location);
}

uint8_t UpdateHarvestMod(uint8_t (*HarvestYears)[HARVEST_YEARS], uint8_t CurrYear) {
	CurrYear = CurrYear % HARVEST_YEARS;
	(*HarvestYears)[CurrYear] = (uint8_t) Random(1, 10);
	return CurrYear;
}

float HarvestModifier(uint8_t (* const HarvestYears)[HARVEST_YEARS]) {
	int Total = 0;

	for(int i = 0; i < HARVEST_YEARS; ++i)
		Total += (*HarvestYears)[i];
	return ((float)Total) / 10 / 2 + 0.2f;
}

void SettlementObjThink(struct Object* Obj) {
	for(; Obj != NULL; Obj = Obj->Next) { 
		struct Settlement* Settlement = (struct Settlement*) Obj;

		if(DAY(g_GameWorld.Date) == 0) {
			if(MONTH(g_GameWorld.Date) == JANURARY) {
				uint16_t MeadowDelta = Settlement->Meadow.Acres * ToPound(Settlement->Meadow.Crop->SeedsPerAcre) * Settlement->Meadow.Crop->YieldMult * HarvestModifier((uint8_t (* const)[HARVEST_YEARS])&Settlement->HarvestMod);

				UpdateHarvestMod(&Settlement->HarvestMod, Settlement->HarvestYear);
				Settlement->Meadow.MonthNut = MeadowDelta; 
				Settlement->Meadow.NutRem = Settlement->Meadow.MonthNut + MeadowDelta;
				++Settlement->HarvestYear;
				Settlement->HarvestYear = Settlement->HarvestYear % HARVEST_YEARS;
				//if(YEAR(g_GameWorld.Date) - YEAR(Settlement->LastRaid) >= 1 && DAY(g_GameWorld.Date) == 0) {
					//MessageBox(g_LuaState, "You have not raided recently.");
				/*	Itr = Settlement->BigGuys.Front;
					while(Itr != NULL) {
						if(Settlement->Government->Leader == ((struct BigGuy*)Itr->Data)) {
							Itr = Itr->Next;
							continue;
						}
						AddOpinion((struct BigGuy*)Itr->Data, Settlement->Government->Leader, ACTTYPE_WARLACK, -10, OPNLEN_SMALL, OPINION_AVERAGE, &((struct BigGuy*)Itr->Data)->Relations); 
						Itr = Itr->Next;
					}
				}*/
				Settlement->YearBirths = 0;
				Settlement->YearDeaths = 0;
				//Determine how many crisis will be generated this year.
				GenerateCrisis(Settlement);
				//if(Settlement->FreeAcres + Settlement->UsedAcres <= MILE_ACRE)
				//	Settlement->FreeAcres += Settlement->Families.Size;
				UpdateProf(Settlement);
			} else if(MONTH(g_GameWorld.Date) == SEPTEMBER) {

			}
			/*
			 * Handle crisis.
			 */
			for(int i = 0; i < Settlement->Crisis.Size; ++i) {
#define FormatSz (8)

				struct Crisis* Crobj = Settlement->Crisis.Table[i];
				struct Primitive Crisis;
				struct Primitive Offender;
				struct Primitive Defender;
				struct Mission* Mission = g_MissionEngine.CrisisMissions[Crobj->Type];
				struct MissionFrame* Frame = CreateMissionFrame(NULL, Settlement->Government->Leader, Mission);
				//struct MissionTextFormat TextFormat[FormatSz];
				//uint8_t FormatCurr = 0;
				//const char* Str = NULL;
				
				if(Frame == NULL) break;
				PrimitiveSetPtr(&Crisis, Settlement->Crisis.Table[i], LOBJ_CRISIS);
				PrimitiveSetPtr(&Offender, Crobj->Offender, LOBJ_PERSON);
				PrimitiveSetPtr(&Defender, Crobj->Defender, LOBJ_PERSON);
				MissionFrameAddVar(Frame, 3, "Crisis", &Crisis, "Offender", &Offender, "Defender", &Defender);
				MissionCall(g_LuaState, Frame);
#undef FormatSz
			}
			Settlement->Crisis.Size = 0;
			

			for(int i = 0; i < Settlement->Families.Size; ++i) {
				struct Family* Family = Settlement->Families.Table[i];

				switch(Family->Prof) {
					/*case PROF_BLACKSMITH:
						GoodSell(Family, HashSearch(&g_Goods, "Spear"), 1);
						break;*/
				}
			}

			//Attempt to match a suitor and bride together and then marry them.
			for(int i = 0; i < Settlement->Suitors.Size; ++i) {
				if(Settlement->Brides.Size <= 0)
					break;
				struct Person* Suitor = Settlement->Suitors.Table[i];
				struct Person* Bride = Settlement->Brides.Table[Settlement->Brides.Size -1];
				struct Family* Family = CreateFamily(Suitor->Family->Name, Settlement, Suitor->Family, Suitor->Family->Caste, Suitor->Family->Prof);
				struct Good* Good = NULL;

				if(Suitor->Family->Prof == PROF_FARMER) {
					for(int i = 0; i < Suitor->Family->Goods.Size; ++i) {
						Good = Suitor->Family->Goods.Table[i];
						
						//Give the new family seeds to start a farm.
						if(Good->Base->Category == GOOD_SEED) {
							struct Good* NewGood = CreateGood(Good->Base);	
							struct Crop* Crop = HashSearch(&g_Crops, Good->Base->Name);
							int Take = Crop->SeedsPerAcre * FAMILY_ACRES;
							int NutTake = 0;
							int NutReq = 16; //FamilyNutReq(Family);
							
							if(Take > Good->Quantity) Take = Good->Quantity / 2;
							ArrayInsert_S(&Family->Goods, NewGood);
							ArrayAddGood(&Family->Goods, Good, Take);

							NutTake = (NutReq * (YEAR_DAYS / 2));
							if(NutTake > Family->Parent->Food.SlowSpoiled) NutTake = Family->Parent->Food.SlowSpoiled;
							Family->Parent->Food.SlowSpoiled -= NutTake;
							Family->Food.SlowSpoiled += NutTake;

							if(NutTake > Bride->Family->Food.SlowSpoiled) NutTake = Bride->Family->Food.SlowSpoiled;
							Bride->Family->Food.SlowSpoiled -= NutTake;
							Family->Food.SlowSpoiled += NutTake;
							FamilyDivideAnimals(Family, Suitor->Family, Suitor->Family->NumChildren);
							//Family->Food.SlowSpoiled += 1000;
						}
					}
				#ifdef DEBUG
					//Assert(Family->Goods.Size > 0);
				#endif
				LocationCreateField(Family, FAMILY_ACRES);
				}
				FamilyRemovePerson(Suitor->Family, Suitor);
				FamilyRemovePerson(Bride->Family, Bride);
				Family->People[HUSBAND] = Suitor;
				Family->People[WIFE] = Bride;
				Suitor->Family = Family;
				Bride->Family = Family;
				ArrayRemove(&Settlement->Suitors, i);
				ArrayRemove(&Settlement->Brides, Settlement->Brides.Size - 1);
				//SettlementPlaceFamily(Settlement, Family);
				ArrayInsert_S(&Settlement->Families, Family);
			}
			for(int i = 0; i < FACTION_IDSIZE; ++i) {
				uint32_t InfluenceCost = 0;

				if(FactionIsActive(&Settlement->Factions, i) == false)
					continue;
				Settlement->Factions.Power[i] += Settlement->Factions.PowerGain[i];
				if(Settlement->Factions.Power[i] > FACTION_MAXPOWER(&Settlement->Factions, i))
					Settlement->Factions.Power[i] = FACTION_MAXPOWER(&Settlement->Factions, i);
				if(Settlement->Factions.PolicyInfluence[i] != -1) {
					InfluenceCost = FactionInfluenceCost(&Settlement->Factions, i);
					if(InfluenceCost > Settlement->Factions.Power[i]) {
						Settlement->Factions.PolicyInfluence[i] = -1;
						continue;
					}
					if(Settlement->Factions.OpposePolicy[i] == false)
						Settlement->Government->PolicyPop[Settlement->Factions.PolicyInfluence[i]]++;
					else
						Settlement->Government->PolicyPop[Settlement->Factions.PolicyInfluence[i]]--;
					Settlement->Factions.Power[i] -= InfluenceCost;
				}
			}
			if(Settlement->Factions.Coro != -1) {
				CoResume(Settlement->Factions.Coro);
			}
		}
	}
}

void SettlementThink(struct Settlement* Settlement) {
	MarketCalcPrice(&Settlement->Market);
	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];

		if(Family->Prof != PROF_FARMER) continue;
		for(int j = 0; j < Family->Farmer.FieldCt; ++j) {
			struct Field* Field = Family->Farmer.Fields[j];
			if(Field->Status == EPLANTING || Field->Status == EPLOWING || Field->Status == EHARVESTING) {
				int Total = FamilyWorkModifier(Family);
				
				Field->StatusTime -= Total;
				if(Field->StatusTime <= 0) {
					if(Field->Status == EHARVESTING) {
						uint32_t Harvest = FieldHarvest((struct Field*)Field, &Family->Goods, HarvestModifier(&Family->HomeLoc->HarvestMod));

						Family->Food.SlowSpoiled += Harvest;
					}
					FieldChangeStatus(Field);
				}
			}
		}
	}
	for(int i = 0; i < Settlement->Bulletin.Size; ++i) {
		struct BulletinItem* Bulletin = Settlement->Bulletin.Table[i];

		--Bulletin->DaysLeft;
		if(Bulletin->DaysLeft <= 0) {
			ArrayRemove(&Settlement->Bulletin, i);
			DestroyBulletinItem(Bulletin);
		}
	}
}

int SettlementGetTiles(const struct MapRenderer* Renderer, const struct Settlement* Settlement) {
	int Radius = 0;

	switch(SettlementType(Settlement)) {
		case SET_HAMLET:
			Radius = 2;
			break;
		case SET_VILLAGE:
			Radius = 3;
			break;
		case SET_TOWN:
			Radius = 4;
			break;
		case SET_CITY:
			Radius = 5;
			break;
	}
	return Radius;
}

void SettlementPlaceFamily(struct Settlement* Location, struct Family* Family) {
	//Location->Meadow.Acres = Location->Meadow.Acres + 10;
	/*if(Location->People.Size >= Location->People.TblSize)
		ArrayGrow(&Location->People, Location->People.TblSize * 2);
	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(Family->People[i] == NULL || PersonMature(Family->People[i]) == 0)
			continue;
		if(Gender(Family->People[i]) == MALE) {
			if(i >= CHILDREN) ArrayInsert_S(&Location->Suitors, Family->People[i]);
			++Location->AdultMen;
		} else {
			if(i >= CHILDREN) ArrayInsert_S(&Location->Brides, Family->People[i]);
			++Location->AdultWomen;
		}
		Family->People[i]->Sidx = Location->People.Size;
		ArrayInsert(&Location->People, Family->People[i]);
	}*/
	ArrayInsertSort_S(&Location->Slaves, Family, ObjectCmp);
	ArrayInsert_S(&Location->Families, Family);
	for(int i = 0; i < Location->ProfCts.Size; ++i) {
		struct ProfRec* Rec = Location->ProfCts.Table[i];

		if(Rec->ProfId == Family->Prof) {
			++Rec->Count;
			goto skip_add_prof;
		}
	}
	struct ProfRec* Rec = malloc(sizeof(struct ProfRec));
	Rec->ProfId = Family->Prof;
	Rec->Count = 1;
	ArrayInsert_S(&Location->ProfCts, Rec);
	skip_add_prof:
	return;
}

void SettlementRemoveFamily(struct Settlement* Location, struct Family* Family) {
	for(int i = 0; i < Location->Families.Size; ++i) {
		struct Family* ItrFam = Location->Families.Table[i];

		if(ItrFam == Family) {
			ArrayRemove(&Location->Families, i);
			break;
		}
	}
	for(int i = 0; i < Location->ProfCts.Size; ++i) {
		struct ProfRec* Rec = Location->ProfCts.Table[i];

		if(Rec->ProfId == Family->Prof) {
			--Rec->Count;
			if(Rec->Count == 0) {
				ArrayRemove(&Location->ProfCts, i);
				free(Rec);
			}
		}
	}
}

int SettlementIsFriendly(const struct Settlement* Location, struct Army* Army) {
	return (GovernmentTop(Location->Government) == GovernmentTop(Army->Government));
}

void SettlementGetCenter(const struct Settlement* Location, SDL_Point* Pos) {
	Pos->x = Location->Pos.x;
	Pos->y = Location->Pos.y;
}

void SettlementAddPerson(struct Settlement* Settlement, struct Person* Person) {
	Person->Sidx = Settlement->People.Size;
	Person->Location = Settlement->Object.Id;
	ArrayInsert_S(&Settlement->People, Person);
	Assert(Person->Sidx == Settlement->People.Size - 1);
	if(PersonMature(Person) == true) {
		if(Gender(Person) == MALE)
			++Settlement->AdultMen;
		else
			++Settlement->AdultWomen;
		if(Person->Family->Prof == PROF_WARRIOR) {
			LnkLstPushBack(&Settlement->FreeWarriors, Person);
			++Settlement->MaxWarriors;
		}
	}
}

/*
 * NOTE: Three branches are done here which could be removed if a list of sorted people were given to this function.
 * The three are if the person is a warrior, adult, and gender. If the settlement's people were sorted in this way then
 * These checks might become redundant.
 */
void SettlementRemovePerson(struct Settlement* Settlement, struct Person* Person) {
	Assert(Settlement->People.Size > 0);

	uint32_t Sidx = Person->Sidx;

	Assert(Sidx == ((struct Person*)Settlement->People.Table[Sidx])->Sidx);
	Assert(Person->Location == Settlement->Object.Id);
	--Settlement->People.Size;
	Settlement->People.Table[Sidx] = Settlement->People.Table[Settlement->People.Size];
	((struct Person*)Settlement->People.Table[Sidx])->Sidx = Sidx;
	Person->Sidx = 0;
	Person->Location = 0;
	if(PersonMature(Person) == true) {
		if(Gender(Person) == MALE) {
			--Settlement->AdultMen;
			for(int i = 0; i < Settlement->Suitors.Size; ++i) {
				if(Settlement->Suitors.Table[i] == Person) {
					ArrayRemove(&Settlement->Suitors, i);
					break;
				}
			}
		}
		else {
			--Settlement->AdultWomen;
			for(int i = 0; i < Settlement->Brides.Size; ++i) {
				if(Settlement->Brides.Table[i] == Person) {
					ArrayRemove(&Settlement->Brides, i);
					break;
				}
			}
		}
	}
	if(PersonMature(Person) != false && PersonProf(Person) == PROF_WARRIOR) {
		for(struct LnkLst_Node* Itr = Settlement->FreeWarriors.Front; Itr != NULL; Itr = Itr->Next) {
			if(Itr->Data == Person) {
				LnkLstRemove(&Settlement->FreeWarriors, Itr);
				--Settlement->MaxWarriors;
				return;
			}
		}
	}
}

int SettlementCountWarriors(const struct Settlement* Settlement) {
	int Ct = 0;

	for(int i = 0; i < Settlement->People.Size; ++i) {
		struct Person* Person = Settlement->People.Table[i];

		if(PersonIsWarrior(Person) != 0) ++Ct;
	}
	return Ct;
}

void TribalCreateBigGuys(struct Settlement* Settlement, double CastePercent[CASTE_SIZE]) {
	struct LinkedList UniqueFamilies = {0, NULL, NULL};
	struct LnkLst_Node* FamilyItr = NULL;
	struct Family* Family = NULL;
	//struct BigGuy* Leader = NULL;
	uint8_t BGStats[BGSKILL_SIZE];
	//uint8_t LeaderCaste = CASTE_NOBLE;
	int Motivations[BGMOT_SIZE] = {2, 4};
	int MotCt = 0;
	int Count = 0;//_Settlement->Families.Size * 0.1f; //How many big guys to make.
	int FamilyCt = Settlement->Families.Size;
	int* CasteCount = alloca(sizeof(int) * CASTE_SIZE);

	if(FamilyCt < 20)
		Count = FamilyCt * 0.4;
	else 
		Count = FamilyCt * 0.2;
	memset(CasteCount, 0, sizeof(int) * CASTE_SIZE);
	//Assert((_CastePercent[CASTE_THRALL] + _CastePercent[CASTE_LOWCLASS] + _CastePercent[CASTE_HIGHCLASS] + _CastePercent[CASTE_NOBLE]) != 100);
	RandTable(CastePercent, &CasteCount, CASTE_SIZE, Count);
	for(int i = 0; i < Settlement->Families.Size; ++i) {
		if(Count <= 0)
			break;
		Family = (struct Family*)Settlement->Families.Table[i];
		for(FamilyItr = UniqueFamilies.Front; FamilyItr != NULL; FamilyItr = FamilyItr->Next) {
			if(ObjectCmp(&Family->Object, &((struct Family*)FamilyItr->Data)->Object) == 0) goto skip_bigguy;
		}
		if(CasteCount[Family->Caste] <= 0) continue;
		--CasteCount[Family->Caste];
		GenerateStats(Family->Caste, &BGStats);
		CreateBigGuy(Family->People[0], &BGStats, MotCt);
		--Motivations[MotCt];
		if(Motivations[MotCt] <= 0)
			++MotCt;
		LnkLstPushBack(&UniqueFamilies, Family);
		--Count;
		skip_bigguy:;
	}
	//SettlementSetBGOpinions(&Settlement->BigGuys);
	LnkLstClear(&UniqueFamilies);
	Assert(Settlement->BigGuys.Size > 0);
	//Assert(PERSON_CASTE(Leader->Person) == LeaderCaste);
}

int SettlementBigGuyCt(const struct Settlement* Settlement) {
	return Settlement->BigGuys.Size;
}

int SettlementAdultPop(const struct Settlement* Settlement) {
	return Settlement->Families.Size;
}

int SettlementGetNutrition(const struct Settlement* Settlement) {
	int Nutrition = 0;

	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];

		Nutrition = Nutrition + FamilyGetNutrition(Family);
	}
	return Nutrition;
}

int SettlementYearlyNutrition(const struct Settlement* Settlement) {
	const struct Family* Family = NULL;
	int Nutrition = 0;

	for(int i = 0; i < Settlement->Families.Size; ++i) {
		Family = Settlement->Families.Table[i];
		if(Family->Prof != PROF_FARMER)
			continue;
		for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
			Nutrition = Nutrition + (Family->Farmer.Fields[i]->Acres * 400);
		}
	}
	return Nutrition;
}

int SettlementCountAcres(const struct Settlement* Settlement) {
	int Acres = 0;

	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];
		Acres = Acres + FamilyCountAcres(Family);
	}
	return Acres;
}

int SettlementExpectedYield(const struct Settlement* Settlement) {
	int Yield = 0;

	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];
		Yield = Yield + FamilyExpectedYield(Family);
	}
	return Yield * HarvestModifier((uint8_t (* const)[HARVEST_YEARS])&Settlement->HarvestMod);
}

struct Plot* SettlementFindPlot(const struct Settlement* Settlement, int PlotType, void* PlotData) {
/*	struct Plot* Plot = NULL;

	for(const struct LnkLst_Node* Itr = SettlementPlots(Settlement); Itr != NULL; Itr = Itr->Next) {
		Plot = Itr->Data;
		if(Plot->Type == PLOT_CHANGEPOLICY && Plot->PlotData == PlotData) {
			return Plot;
		}
	}*/
	return NULL;
}

void InitPersonSelector(struct PersonSelector* Selector) {
	Selector->Count = ~(0);
	Selector->Gender = (MALE | FEMALE);
	Selector->Adult = true;
	Selector->Caste = ~(0);
	Selector->PType = SELP_ANY;
}

struct Person** QueryPeople(struct Person** const InList, uint32_t InListSz, const struct PersonSelector* Selector, uint32_t* OutListSz) {
	uint32_t PeopleSz = InListSz;
	uint16_t BuffSz = 0;
	//List of people to return.
	struct Person** People = NULL;
	size_t RemSz = FrameSizeRemain() / sizeof(struct Person*);
	size_t DefSz = sizeof(struct Person*) * Selector->Count;
	
	//Assert(Selector->Count > 0);
	if(Selector->Count == 0) return NULL;
	PeopleSz = ((DefSz < FrameSizeRemain()) ? (DefSz) : (RemSz));
	People = FrameAlloc(PeopleSz * sizeof(struct Person*));
	//memcpy(People, InList, InListSz * sizeof(struct Person*));
	BuffSz = 0;
	//Go through every person and determine if they are valid for being selected. Done in O(n) time, if sorted would it be faster at O(log(n)) time?
	for(int i = 0; i < InListSz; ++i) {
		if((Selector->Gender & InList[i]->Flags) && IsAlive(InList[i])) People[BuffSz++] = InList[i];
		if(BuffSz >= Selector->Count) goto end;
	}
	PeopleSz = BuffSz;
	BuffSz = 0;
	for(int i = 0; i < PeopleSz; ++i) {
		if(PersonMature(People[i]) == Selector->Adult) People[BuffSz++] = People[i];
		if(BuffSz >= Selector->Count) goto end;
	}
	PeopleSz = BuffSz;
	BuffSz = 0;
	for(int i = 0; i < PeopleSz; ++i) {
		if((People[i]->Family->Caste & Selector->Caste) != 0) People[BuffSz++] = People[i];
		if(BuffSz >= Selector->Count) goto end;
	}
	end:
	//*OutListSz = PeopleSz;
	*OutListSz = BuffSz;
	return People;
}

struct Settlement** QuerySettlement(const struct SettlementSelector* Selector, uint32_t* OutListSz) {
	struct Settlement** SetList = FrameAlloc(sizeof(struct Settlement*) * Selector->Count);
	uint32_t Size = 0;

	if(Selector->Target != NULL) {
		SettlementsInRadius(&g_GameWorld, &Selector->Target->Pos, Selector->Distance, SetList, &Size, Selector->Count);
	}
	*OutListSz = Size;
	return SetList;
}

bool LocationCreateField(struct Family* Family, int Acres) {
	struct Settlement* Settlement  = Family->HomeLoc;

	if(Acres < Settlement->FreeAcres) {
		Settlement->FreeAcres -= Acres;	
		Family->Farmer.Fields[Family->Farmer.FieldCt++] = CreateField(NULL, Acres, Family);
		return true;
	}
 	return false;
}

void MigrateFamilies(struct GameWorld* World, struct Settlement* Settlement, uint8_t* SetZones, uint16_t SetArea, struct Family** Families, int FamSz, const struct TilePoint* Pos) {
	struct Settlement* NewSet = NULL;		
	struct TilePoint NewPos;

	if(FamSz < 0) return;
	MapGravBest(SetZones, SetArea, Pos->x, Pos->y, &NewPos.x, &NewPos.y);
	NewSet = CreateSettlement(World, NewPos.x, NewPos.y, "Test Settlement", Settlement->Government->GovType);
	for(int i = 0; i < FamSz; ++i) {
		SettlementRemoveFamily(Settlement, Families[i]);
		SettlementPlaceFamily(Settlement, Families[i]);
		if(Families[i]->Prof == PROF_FARMER) {
			for(int j = 0; j < Families[i]->Farmer.FieldCt; ++j) {
				int Acres = FieldTotalAcres(Families[i]->Farmer.Fields[j]);

				Settlement->FreeAcres += Acres;
				NewSet->FreeAcres -= Acres;
			}
		}
	}
}

void UpdateProf(struct Settlement* Settlement) {
	struct Array* ProfArr = NULL;
	uint16_t BestRatio = 0;
	uint8_t ProfLimit = 0;//How many new professions we can have.
	const struct Profession* NewProfs[16];
	const struct GoodBase* GoodList[16];
	struct InputReq NewGoods[16];
	int CurrNewGood = 0;
	int NewGoodCt = 0;
	uint16_t GoodListCt = 0;
	uint16_t CurrGood = 0;
	int16_t NewProfCt = 0;

	memset(GoodList, 0, sizeof(GoodList));
	for(int i = 0; i < Settlement->ProfCts.Size; ++i) {
		struct ProfRec* Rec = Settlement->ProfCts.Table[i];

		if(Rec->ProfId == PROF_FARMER) {
			ProfLimit = Rec->Count / 10;
			break;
		}
	}
	for(int i = 0; i < Settlement->Market.Size; ++i) {
		const struct MarReq* Req = Settlement->Market.Table[i];
		const struct GoodBase* Good = Req->Base;
		uint16_t Ratio = 0;
		int Quantity = 0;

		if(Req->Quantity > 0) continue;
		Quantity = abs(Req->Quantity);
		Ratio = (Quantity * WORK_DAY * 100) / Good->WkCost;
		if(Ratio > BestRatio) {
			BestRatio = Ratio;
			GoodList[0] = Good;
		}
		NewGoods[NewGoodCt].Req = Good;
		NewGoods[NewGoodCt].Quantity = Ratio;
		NewGoodCt++;
	}
	if(GoodList[0]== NULL) return;
	InsertionSort(NewGoods, NewGoodCt, InputReqQuantityCmp, sizeof(struct InputReq));
	++GoodListCt;
	 while(CurrNewGood < NewGoodCt) {
		GoodList[0] = NewGoods[CurrNewGood].Req;
		ProfArr = HashSearch(&g_GameWorld.GoodMakers, GoodList[0]->Name);
		memset(NewProfs, 0, sizeof(NewProfs));
		NewProfCt = 0;
		if(ProfArr != NULL) {
			NewProfs[NewProfCt++]= ProfArr->Table[Random(0, ProfArr->Size - 1)];
		}
		CurrGood = 0;
		GoodListCt = 1;
		while(CurrGood < GoodListCt) {
			if(NewProfCt >= ProfLimit) return;
			for(int i = 0; i < GoodList[CurrGood]->IGSize; ++i) {
				struct Array* IGProfArr = HashSearch(&g_GameWorld.GoodMakers, ((struct GoodBase*)GoodList[CurrGood]->InputGoods[i]->Req)->Name);

				/*
				 * Check all possible professions that can produce the input good if they exist in the settlement or not.
				 * If none do then pick on and add it to NewProf.
				 */
				for(int k = 0; k < IGProfArr->Size; ++k) {
					const struct Profession* ChildProf = IGProfArr->Table[k];

					if(ChildProf == NewProfs[0]){
						 GoodList[GoodListCt++] = ((struct GoodBase*)GoodList[CurrGood]->InputGoods[i]->Req);
						 break;
					}
					for(int j = 0; j < Settlement->ProfCts.Size; ++j) {
						struct ProfRec* Rec = Settlement->ProfCts.Table[j];

						if(Rec->ProfId == ChildProf->Id) {
							NewProfs[NewProfCt++] = GetProf(Rec->ProfId);
							GoodList[GoodListCt++] = ((struct GoodBase*)GoodList[CurrGood]->InputGoods[i]->Req);
							break;
						}
					}
					//Don't add the same profession twice.
					for(int j = 0; j < NewProfCt; ++j) {
						if(NewProfs[j] == ChildProf)
							break;
					}
					/*
					 * Profession doesn't exist in the town. Add it to profession list and see if we need
					 * to create more professions to produce it's input goods.
					 */
					if(NewProfs[NewProfCt] == NULL){
						NewProfs[NewProfCt++] = IGProfArr->Table[Random(0, IGProfArr->Size - 1)];
						GoodList[GoodListCt++] = ((struct GoodBase*)GoodList[CurrGood]->InputGoods[i]->Req);
					}
				}
			}
			++CurrGood;
		}
		--NewProfCt;
		for(int i = 0; i < Settlement->Families.Size && NewProfCt >= 0; ++i) {
			struct Family* Family = Settlement->Families.Table[i];

			if(Family->Prof == PROF_FARMER) {
				SetProfession(Family, NewProfs[NewProfCt--]->Id);
			}
		}
		++CurrNewGood;
	}
	//Find farmer families to become the new professions.
}

void SettlementGetPos(const void* One, SDL_Point* Pos) {
	const struct Settlement* Settlement = One;

	*Pos = Settlement->Pos;
}

/*void FamilyBuy(struct Family* Family, struct Settlement* Settlement) {
	int Weapons = 0;
	int Armor = 0;

	//if(LeadsRetinue(Family->People[HUSBAND], &g_GameWorld) == false) return;
	for(int i = 0; i < Family->Goods.Size; ++i) {
		struct Good* Good = Family->Goods.Table[i];

		if(IsArmor(Good->Base) == true) {
			++Armor;
		} else if(IsWeapon(Good->Base) == true) {
			++Weapons;
		}
	}
	for(int i = 0; i < Settlement->Market.Size; ++i) {
		struct MarReq* SellReq = Settlement->Market.Table[i];
		struct MarGood* Sell = NULL;
		struct Good BuyGood = {0};
		int Idx = 0;
		
		if(IsArmor(SellReq->Base) == false && IsWeapon(SellReq->Base) == false) continue;
		if(Weapons < 3) continue;
		Idx = RandByte() % SellReq->Fillers.Size; 
		Sell = SellReq->Fillers.Table[Idx];
		--Sell->Quantity;
		BuyGood.Base = SellReq->Base;
		BuyGood.Quantity = 1;
		if(Sell->Quantity <= 0) {
			ArrayRemove(&SellReq->Fillers, Idx);
			free(Sell);
		}
		ArrayAddGood(&Family->Goods, &BuyGood, 1);
	}
}*/

void SettlementTraders(struct Settlement* Settlement) {
	int Size = FrameSizeRemain();
	int Type = SettlementType(Settlement);
	uint32_t MaxSz = Size / sizeof(struct Settlement);
	uint32_t OutSz = 0;
	struct Settlement** TradeList = FrameAlloc(Size);

	SettlementsInRadius(&g_GameWorld, &Settlement->Pos, 20, TradeList, &OutSz, MaxSz);
	for(int i = 0; i < OutSz; ++i) {
		struct Settlement* TradeSet = TradeList[i];
		int TradeType = SettlementType(TradeSet);
		
		if(TradeType < Type) continue;	
		ArrayInsert(&Settlement->Tparts, TradeSet);
		ArrayInsert(&TradeSet->Tparts, Settlement);
	}
	FrameReduce(Size);
}

const char* SettlementString(int Index) {
	return g_SettlementStrs[Index];
}

void SettlementCasteCount(const struct Settlement* Settlement, int (*CasteCount)[CASTE_SIZE]) {
	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];

		++(*CasteCount)[Family->Caste];
	}
}

void SettlementProfessionCount(const struct Settlement* Settlement, uint16_t** ProfCount) {
	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];

		++(*ProfCount)[Family->Prof];
	}
	
}
