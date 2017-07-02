/**
 * Author: David Brotz
 * File: Location.c
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

uint8_t UpdateHarvestMod(uint8_t (*HarvestYears)[HARVEST_YEARS], uint8_t CurrYear);

void LocationGetPoint(const struct Settlement* Location, SDL_Point* Point) {
	(*Point).x = Location->Pos.x;
	(*Point).y = Location->Pos.y;
}

void SettlementOnPolicyChange(const struct EventData* Data, void* Extra) {
	struct Settlement* Settlement = Data->OwnerObj;
	struct BigGuy* Guy = NULL;
	struct BigGuy* Owner = Settlement->Government->Leader;
	int Amount = (int)Extra;

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

struct Settlement* CreateSettlement(int X, int Y, const char* Name, int GovType) {
	struct Settlement* Loc = (struct Settlement*) malloc(sizeof(struct Settlement));

	CreateObject((struct Object*)Loc, OBJECT_LOCATION);
	Loc->Pos.x = X;
	Loc->Pos.y = Y;
	Loc->Name = calloc(strlen(Name) + 1, sizeof(char));
	CtorArray(&Loc->People, 64);
	CtorArray(&Loc->Slaves, 20);
	Loc->Government = CreateGovernment(GovType, 0, Loc);
	Loc->NumPeople = 0;
	Loc->BigGuys.Size = 0;
	Loc->BigGuys.Front = NULL;
	strcpy(Loc->Name, Name);
	LnkLstPushBack(&g_GameWorld.Settlements, Loc);
	QTInsertPoint(&g_GameWorld.SettlementMap, Loc, &Loc->Pos);
	Loc->Families.Size = 0;
	Loc->Families.Front = NULL;
	Loc->Families.Back = NULL;
	CtorArray(&Loc->Suitors, 8);
	CtorArray(&Loc->Brides, 8);
	Loc->YearBirths = 0;
	Loc->YearDeaths = 0;
	for(uint8_t i = 0; i < HARVEST_YEARS - 1; ++i)
		Loc->HarvestMod[i] = 5;
	Loc->HarvestMod[HARVEST_YEARS - 1] = 6;
	Loc->Sprite =  CreateGameObject(g_GameWorld.MapRenderer, ResourceGet("Settlement.png"), MAPRENDER_SETTLEMENT, &Loc->Pos);
	Loc->Meadow.Crop = HashSearch(&g_Crops, "Hay");
	Loc->Meadow.Acres = SETTLEMENT_SPACE / 2;
	Loc->Meadow.MonthNut = Loc->Meadow.Acres * ToPound(Loc->Meadow.Crop->SeedsPerAcre) * Loc->Meadow.Crop->YieldMult * HarvestModifier((uint8_t (* const)[HARVEST_YEARS])&Loc->HarvestMod); 
	Loc->Meadow.NutRem = Loc->Meadow.MonthNut;
	Loc->BuyOrders = NULL;
	Loc->Food.SlowSpoiled = 0;
	Loc->Market = NULL;
	Loc->LastRaid = 0;
	Loc->AdultMen = 0;
	Loc->AdultWomen = 0;
	CtorArray(&Loc->Bulletin, 8);
	Loc->MaxWarriors = 0;
	Loc->FreeAcres = SETTLEMENT_SPACE - Loc->Meadow.Acres;
	Loc->UsedAcres = 0;
	//FIXME: Each family should take an acre of land.
	Loc->StarvingFamilies = 0;
	Loc->Retinues = NULL;
	CtorFaction(&Loc->Factions, Loc);
	GenerateStats(CASTE_FARMER, &Loc->Stats);
	for(int i = 0; i < HARVEST_YEARS; ++i) UpdateHarvestMod(&Loc->HarvestMod, i);
	Loc->HarvestYear = 0;
	ConstructLinkedList(&Loc->FreeWarriors);
	CtorArray(&Loc->Crisis, 8);
	Assert(Loc->FreeAcres >= 100);
	return Loc;
}

void DestroySettlement(struct Settlement* Location) {
	DestroyObject((struct Object*)Location);
	LnkLstClear(&Location->Families);
	DestroyGameObject(Location->Sprite);
	DtorArray(&Location->People);
	DtorArray(&Location->Suitors);
	DtorArray(&Location->Brides);
	DtorArray(&Location->Crisis);
	for(int i = 0; i < Location->Bulletin.Size; ++i) {
		struct BulletinItem* Obj = Location->Bulletin.Table[i];

		DestroyBulletinItem(Obj);
	}
	DtorArray(&Location->Bulletin);
	free(Location->Government);
	free(Location->Name);
	free(Location);
}

uint8_t UpdateHarvestMod(uint8_t (*HarvestYears)[HARVEST_YEARS], uint8_t CurrYear) {
	//CurrYear = CurrYear % HARVEST_YEARS;
	//(*HarvestYears)[CurrYear] = (uint8_t) Random(1, 10);
	return CurrYear;
}

float HarvestModifier(uint8_t (* const HarvestYears)[HARVEST_YEARS]) {
	int Total = 0;

	for(int i = 0; i < HARVEST_YEARS; ++i)
		Total += (*HarvestYears)[i];
	return ((float)Total) / 10 / 2 + 0.2f;
}

void SettlementThink(struct Settlement* Settlement) {
	struct LnkLst_Node* Itr = Settlement->Families.Front;

	//GovernmentThink(Settlement->Government);
	if(MONTH(g_GameWorld.Date) == 0 && DAY(g_GameWorld.Date) == 0) {
		uint16_t MeadowDelta = Settlement->Meadow.Acres * ToPound(Settlement->Meadow.Crop->SeedsPerAcre) * Settlement->Meadow.Crop->YieldMult * HarvestModifier((uint8_t (* const)[HARVEST_YEARS])&Settlement->HarvestMod);
		UpdateHarvestMod(&Settlement->HarvestMod, Settlement->HarvestYear);
		Settlement->Meadow.MonthNut = MeadowDelta; 
		Settlement->Meadow.NutRem = Settlement->Meadow.MonthNut + MeadowDelta;
		++Settlement->HarvestYear;
		Settlement->HarvestYear = Settlement->HarvestYear % HARVEST_YEARS;
		if(YEAR(g_GameWorld.Date) - YEAR(Settlement->LastRaid) >= 1 && DAY(g_GameWorld.Date) == 0) {
			//MessageBox(g_LuaState, "You have not raided recently.");
			Itr = Settlement->BigGuys.Front;
			while(Itr != NULL) {
				if(Settlement->Government->Leader == ((struct BigGuy*)Itr->Data)) {
					Itr = Itr->Next;
					continue;
				}
				AddOpinion((struct BigGuy*)Itr->Data, Settlement->Government->Leader, ACTTYPE_WARLACK, -10, OPNLEN_SMALL, OPINION_AVERAGE, &((struct BigGuy*)Itr->Data)->Relations); 
				Itr = Itr->Next;
			}
		}
		Settlement->YearBirths = 0;
		Settlement->YearDeaths = 0;
		//Determine how many crisis will be generated this year.
		GenerateCrisis(Settlement);
	}
	if(DAY(g_GameWorld.Date) == 0) {
		if(MONTH(g_GameWorld.Date) == JANURARY) {
			if(Settlement->FreeAcres + Settlement->UsedAcres <= MILE_ACRE)
				Settlement->FreeAcres += Settlement->Families.Size;
		} else {//if(MONTH(g_GameWorld.Date) == FEBURARY) {
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
		}
		//Attempt to match a suitor and bride together and then marry them.
		for(int i = 0; i < Settlement->Suitors.Size; ++i) {
			if(Settlement->Brides.Size <= 0)
				break;
			struct Person* Suitor = Settlement->Suitors.Table[i];
			struct Person* Bride = Settlement->Brides.Table[Settlement->Brides.Size -1];
			struct Family* Family = CreateFamily(Suitor->Family->Name, Settlement, Suitor->Family, Suitor->Family->Caste);
			struct Good* Good = NULL;

			for(int i = 0; i < Suitor->Family->Goods.Size; ++i) {
				Good = Suitor->Family->Goods.Table[i];
				
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
			if(Family->Caste == CASTE_FARMER) Assert(Family->Goods.Size > 0);
			#endif
			LocationCreateField(Family, FAMILY_ACRES);
			FamilyRemovePerson(Suitor->Family, Suitor);
			FamilyRemovePerson(Bride->Family, Bride);
			Family->People[HUSBAND] = Suitor;
			Family->People[WIFE] = Bride;
			Suitor->Family = Family;
			Bride->Family = Family;
			ArrayRemove(&Settlement->Suitors, i);
			ArrayRemove(&Settlement->Brides, Settlement->Brides.Size - 1);
			//SettlementPlaceFamily(Settlement, Family);
			LnkLstPushBack(&Settlement->Families, Family);
		}
		Settlement->StarvingFamilies = 0;
		for(struct Retinue* Itr = Settlement->Retinues; Itr != NULL; Itr = Itr->Next) {
			RetinueThink(Itr);
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
	for(int i = 0; i < Settlement->Bulletin.Size; ++i) {
		struct BulletinItem* Bulletin = Settlement->Bulletin.Table[i];

		--Bulletin->DaysLeft;
		if(Bulletin->DaysLeft <= 0) {
			ArrayRemove(&Settlement->Bulletin, i);
			DestroyBulletinItem(Bulletin);
		}
	}
}

void SettlementDraw(const struct MapRenderer* Renderer, struct Settlement* Settlement) {
	SDL_Point Point = {Settlement->Pos.x, Settlement->Pos.y};

	MapDrawColorOverlay(Renderer, &Point, &Settlement->Government->ZoneColor);
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
	LnkLstPushBack(&Location->Families, Family);
}

void SettlementRemoveFamily(struct Settlement* Location, struct Family* Family) {
	for(struct LnkLst_Node* Itr = Location->Families.Front; Itr != NULL; Itr = Itr->Next) {
		if(Itr->Data == Family) {
			LnkLstRemove(&Location->Families, Itr);
			break;
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
	int Caste = PERSON_CASTE(Person);

	Person->Sidx = Settlement->People.Size;
	ArrayInsert_S(&Settlement->People, Person);
	Assert(Person->Sidx == Settlement->People.Size - 1);
	++Settlement->NumPeople;
	if(PersonMature(Person) == true) {
		if(Gender(Person) == MALE)
			++Settlement->AdultMen;
		else
			++Settlement->AdultWomen;
		if(Caste == CASTE_WARRIOR) {
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
	Assert(Settlement->NumPeople > 0);

	uint32_t Sidx = Person->Sidx;

	Assert(Sidx == ((struct Person*)Settlement->People.Table[Sidx])->Sidx);
	--Settlement->People.Size;
	Settlement->People.Table[Sidx] = Settlement->People.Table[Settlement->People.Size];
	((struct Person*)Settlement->People.Table[Sidx])->Sidx = Sidx;
	--Settlement->NumPeople;
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
	if(PersonMature(Person) != false && PERSON_CASTE(Person) == CASTE_NOBLE) {
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
	struct LnkLst_Node* Itr = NULL;
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
	Itr = Settlement->Families.Front;
	while(Itr != NULL) {
		if(Count <= 0)
			break;
		Family = (struct Family*)Itr->Data;
		FamilyItr = UniqueFamilies.Front;
		while(FamilyItr != NULL) {
			if(((struct Family*)FamilyItr->Data)->Object.Id == Family->Object.Id)
				goto skip_bigguy;
			FamilyItr = FamilyItr->Next;
		}
		if(CasteCount[Family->Caste] <= 0)
			goto skip_bigguy;
		--CasteCount[Family->Caste];
		GenerateStats(Family->Caste, &BGStats);
		CreateBigGuy(Family->People[0], &BGStats, MotCt);
		--Motivations[MotCt];
		if(Motivations[MotCt] <= 0)
			++MotCt;
		LnkLstPushBack(&UniqueFamilies, Family);
		--Count;
		skip_bigguy:
		Itr = Itr->Next;
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
	const struct LnkLst_Node* Itr = Settlement->Families.Front;
	int Nutrition = 0;

	while(Itr != NULL) {
		Nutrition = Nutrition + FamilyGetNutrition((const struct Family*)Itr->Data);
		Itr = Itr->Next;
	}
	return Nutrition;
}

int SettlementYearlyNutrition(const struct Settlement* Settlement) {
	const struct LnkLst_Node* Itr = Settlement->Families.Front;
	const struct Family* Family = NULL;
	int Nutrition = 0;

	while(Itr != NULL) {
		Family = ((struct Family*)Itr->Data);
		if(Family->Caste != CASTE_FARMER)
			goto end;
		for(int i = 0; i < Family->Spec.Farmer.FieldCt; ++i) {
			Nutrition = Nutrition + (Family->Spec.Farmer.Fields[i]->Acres * 400);
		}
		end:
		Itr = Itr->Next;
	}
	return Nutrition;
}

int SettlementCountAcres(const struct Settlement* Settlement) {
	const struct LnkLst_Node* Itr = Settlement->Families.Front;
	int Acres = 0;

	while(Itr != NULL) {
		Acres = Acres + FamilyCountAcres((struct Family*)Itr->Data);
		Itr = Itr->Next;
	}
	return Acres;
}

int SettlementExpectedYield(const struct Settlement* Settlement) {
	const struct LnkLst_Node* Itr = Settlement->Families.Front;
	int Yield = 0;

	while(Itr != NULL) {
		Yield = Yield + FamilyExpectedYield((struct Family*)Itr->Data);
		Itr = Itr->Next;
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

struct Retinue* SettlementAddRetinue(struct Settlement* Settlement, struct BigGuy* Leader) {
	struct Retinue* Retinue = CreateRetinue(Leader);

	Retinue->Next = Settlement->Retinues;
	Settlement->Retinues = Retinue;
	for(struct LnkLst_Node* Itr = Settlement->FreeWarriors.Front; Itr != NULL; Itr = Itr->Next) {
		if(Leader== Itr->Data) {
			LnkLstRemove(&Settlement->FreeWarriors, Itr);
			break;
		}
	}
	IntInsert(&g_GameWorld.PersonRetinue, Leader->Person->Object.Id, Retinue);
	return Retinue;
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
	struct Person** People = FrameAlloc(sizeof(struct Person*) * InListSz);
	
	for(int i = 0; i < InListSz; ++i) People[i] = InList[i];
	BuffSz = 0;
	//Go through every person and determine if they are valid for being selected. Done in O(n) time, if sorted would it be faster at O(log(n)) time?
	for(int i = 0; i < PeopleSz; ++i) {
		if((Selector->Gender & People[i]->Flags)) People[BuffSz++] = People[i];
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
	*OutListSz = PeopleSz;
	return People;
}

struct Settlement** QuerySettlement(const struct SettlementSelector* Selector, uint32_t* OutListSz) {
	struct Settlement** SetList = FrameAlloc(sizeof(struct Settlement*) * Selector->Count);
	int Size = 0;

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
		Family->Spec.Farmer.Fields[Family->Spec.Farmer.FieldCt++] = CreateField(NULL, Acres, Family);
		return true;
	}
 	return false;
}
