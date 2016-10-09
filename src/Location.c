/* Author: David Brotz
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
#include "Plot.h"
#include "Retinue.h"

#include "sys/Math.h"
#include "sys/ResourceManager.h"
#include "sys/Array.h"
#include "sys/Log.h"

#include "video/Sprite.h"
#include "video/Tile.h"
#include "video/MapRenderer.h"

#include <stdlib.h>
#include <string.h>

#define HARVESTMOD_MIN 0.4f
#define HARVESTMOD_MAX 1.6f
#define SETTLEMENT_AVGSTAT (50)

void LocationGetPoint(const struct Location* _Location, SDL_Point* _Point) {
	(*_Point).x = _Location->Pos.x;
	(*_Point).y = _Location->Pos.y;
}

void SettlementOnPolicyChange(const struct EventData* _Data, void* _Extra) {
	struct Settlement* _Settlement = _Data->OwnerObj;
	struct BigGuy* _Guy = NULL;
	struct BigGuy* _Owner = _Settlement->Government->Leader;
	int _Amount = (int)_Extra;

	for(struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_Guy = _Itr->Data;
		if(_Guy == _Owner)
			continue;
		BigGuyAddOpinion(_Guy, _Owner, ACTTYPE_POLICY, _Amount * 10, OPNLEN_MEDIUM, OPINION_AVERAGE);
	}
}

void SettlementSetBGOpinions(struct LinkedList* _List) {
	struct LnkLst_Node* i = NULL;
	struct LnkLst_Node* j = NULL;

	for(i = _List->Front; i != NULL; i = i->Next) {
		for(j = _List->Front; j != NULL; j = j->Next) {
			if(i == j) {
				continue;
			}
			CreateBigGuyRelation(((struct BigGuy*)i->Data), ((struct BigGuy*)j->Data));
		}
	}
}

struct Settlement* CreateSettlement(int _X, int _Y, const char* _Name, int _GovType) {
	struct Settlement* _Loc = (struct Settlement*) malloc(sizeof(struct Settlement));

	CreateObject((struct Object*)_Loc, OBJECT_LOCATION, (void(*)(struct Object*))SettlementThink);
	_Loc->LocType = ELOC_SETTLEMENT;
	_Loc->Pos.x = _X;
	_Loc->Pos.y = _Y;
	_Loc->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Loc->People = NULL;
	_Loc->Government = CreateGovernment(_GovType, 0, _Loc);
	_Loc->NumPeople = 0;
	_Loc->BigGuys.Size = 0;
	_Loc->BigGuys.Front = NULL;
	strcpy(_Loc->Name, _Name);
	LnkLstPushBack(&g_GameWorld.Settlements, _Loc);
	QTInsertPoint(&g_GameWorld.SettlementMap, _Loc, &_Loc->Pos);
	_Loc->Families.Size = 0;
	_Loc->Families.Front = NULL;
	_Loc->Families.Back = NULL;
	_Loc->People = NULL;
	_Loc->People = NULL;
	_Loc->YearBirths = 0;
	_Loc->YearDeaths = 0;
	for(uint8_t i = 0; i < HARVEST_YEARS - 1; ++i)
		_Loc->HarvestMod[i] = 5;
	_Loc->HarvestMod[HARVEST_YEARS - 1] = 6;
	_Loc->Sprite =  CreateGameObject(g_GameWorld.MapRenderer, ResourceGet("Settlement.png"), MAPRENDER_SETTLEMENT, &_Loc->Pos);
	_Loc->Meadow.Pos.x = _Loc->Pos.x;
	_Loc->Meadow.Pos.y = _Loc->Pos.y;
	_Loc->Meadow.Crop = HashSearch(&g_Crops, "Hay");
	_Loc->Meadow.YieldTotal = 0;
	_Loc->Meadow.Acres = 200;
	_Loc->Meadow.UnusedAcres = 0;
	_Loc->Meadow.Owner = NULL;
	_Loc->BuyOrders = NULL;
	_Loc->Market = NULL;
	_Loc->LastRaid = 0;
	_Loc->AdultMen = 0;
	_Loc->AdultWomen = 0;
	_Loc->Bulletin = NULL;
	_Loc->MaxWarriors = 0;
	_Loc->FreeAcres = SETTLEMENT_SPACE;
	_Loc->UsedAcres = 0;
	_Loc->StarvingFamilies = 0;
	_Loc->Retinues = NULL;
	for(int i = 0; i < BGSKILL_SIZE; ++i) {
		_Loc->Stats[i] = SETTLEMENT_AVGSTAT;
	}
	ConstructLinkedList(&_Loc->FreeWarriors);
	_Loc->Meadow.Status = EGROWING;
	_Loc->Meadow.StatusTime = 0;
	return _Loc;
}

void DestroySettlement(struct Settlement* _Location) {
	DestroyObject((struct Object*)_Location);
	LnkLstClear(&_Location->Families);
	DestroyGameObject(_Location->Sprite);
	free(_Location->Government);
	free(_Location->Name);
	free(_Location);
}

uint8_t UpdateHarvestMod(uint8_t (*_HarvestYears)[HARVEST_YEARS], uint8_t _CurrYear) {
	if(++_CurrYear >= HARVEST_YEARS)
		_CurrYear = 0;
	*_HarvestYears[_CurrYear] = Random(1, 10);
	return _CurrYear;
}

float HarvestModifier(uint8_t (* const _HarvestYears)[HARVEST_YEARS]) {
	int _Total = 0;

	for(int i = 0; i < HARVEST_YEARS; ++i)
		_Total += (*_HarvestYears)[i];
	return ((float)_Total) / 10 / 2 + 0.2f;
}

void SettlementThink(struct Settlement* _Settlement) {
	struct LnkLst_Node* _Itr = _Settlement->Families.Front;
	struct BulletinItem* _Bulletin = _Settlement->Bulletin;

	FieldUpdate(&_Settlement->Meadow);
	GovernmentThink(_Settlement->Government);
	if(MONTH(g_GameWorld.Date) == 0 && DAY(g_GameWorld.Date) == 0) {
		if(YEAR(g_GameWorld.Date) - YEAR(_Settlement->LastRaid) >= 1 && DAY(g_GameWorld.Date) == 0) {
			//MessageBox(g_LuaState, "You have not raided recently.");
			_Itr = _Settlement->BigGuys.Front;
			while(_Itr != NULL) {
				if(_Settlement->Government->Leader == ((struct BigGuy*)_Itr->Data)) {
					_Itr = _Itr->Next;
					continue;
				}
				BigGuyAddOpinion((struct BigGuy*)_Itr->Data, _Settlement->Government->Leader, ACTTYPE_WARLACK, -10, OPNLEN_SMALL, OPINION_AVERAGE); 
				_Itr = _Itr->Next;
			}
		}
		_Settlement->YearBirths = 0;
		_Settlement->YearDeaths = 0;
	}
	if(DAY(g_GameWorld.Date) == 0) {
		if(MONTH(g_GameWorld.Date) == 0) {
			if(_Settlement->FreeAcres + _Settlement->UsedAcres <= MILE_ACRE)
				_Settlement->FreeAcres += _Settlement->Families.Size;
		}

		_Settlement->StarvingFamilies = 0;
		for(struct Retinue* _Itr = _Settlement->Retinues; _Itr != NULL; _Itr = _Itr->Next) {
			RetinueThink(_Itr);
		}
	}
	while(_Bulletin != NULL) {
		--_Bulletin->DaysLeft;
		if(_Bulletin->DaysLeft <= 0) {
			struct BulletinItem* _BulletinNext = _Bulletin->Next;
			
			ILL_DESTROY(_Settlement->Bulletin, _Bulletin);
			_Bulletin = _BulletinNext;
			continue;
		}
		_Bulletin = _Bulletin->Next;
	}
}

void SettlementDraw(const struct MapRenderer* _Renderer, struct Settlement* _Settlement) {
	SDL_Point _Point = {_Settlement->Pos.x, _Settlement->Pos.y};

	MapDrawColorOverlay(_Renderer, &_Point, &_Settlement->Government->ZoneColor);
}

void SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family) {
	_Location->Meadow.Acres = _Location->Meadow.Acres + 10;
	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		if(_Family->People[i] == NULL || PersonMature(_Family->People[i]) == 0)
			continue;
		if(_Family->People[i]->Gender == EMALE)
			++_Location->AdultMen;
		else
			++_Location->AdultWomen;
	}
	LnkLstPushBack(&_Location->Families, _Family);
}

void SettlementRemoveFamily(struct Settlement* _Location, struct Family* _Family) {
	for(struct LnkLst_Node* _Itr = _Location->Families.Front; _Itr != NULL; _Itr = _Itr->Next) {
		if(_Itr->Data == _Family) {
			LnkLstRemove(&_Location->Families, _Itr);
			break;
		}
	}
}

int SettlementIsFriendly(const struct Settlement* _Location, struct Army* _Army) {
	return (GovernmentTop(_Location->Government) == GovernmentTop(_Army->Government));
}

void SettlementGetCenter(const struct Settlement* _Location, SDL_Point* _Pos) {
	struct Sprite* _Sprite = _Location->Sprite;

	_Pos->x = _Sprite->TilePos.x;
	_Pos->y = _Sprite->TilePos.y;
}

void SettlementAddPerson(struct Settlement* _Settlement, struct Person* _Person) {
	ILL_CREATE(_Settlement->People, _Person);
	++_Settlement->NumPeople;
	if(PersonMature(_Person) != 0 && PERSON_CASTE(_Person) == CASTE_NOBLE) {
		LnkLstPushBack(&_Settlement->FreeWarriors, _Person);
		++_Settlement->MaxWarriors;
	}
}

void SettlementRemovePerson(struct Settlement* _Settlement, struct Person* _Person) {
	ILL_DESTROY(_Person->Family->HomeLoc->People, _Person);
	--_Settlement->NumPeople;
	if(PersonMature(_Person) != 0 && PERSON_CASTE(_Person) == CASTE_NOBLE) {
		for(struct LnkLst_Node* _Itr = _Settlement->FreeWarriors.Front; _Itr != NULL; _Itr = _Itr->Next) {
			if(_Itr->Data == _Person) {
				LnkLstRemove(&_Settlement->FreeWarriors, _Itr);
				--_Settlement->MaxWarriors;
				return;
			}
		}
	}
}

int SettlementCountWarriors(const struct Settlement* _Settlement) {
	struct Person* _Person = _Settlement->People;
	int _Ct = 0;

	while(_Person != NULL) {
		if(PersonIsWarrior(_Person) != 0)
			++_Ct;
		_Person = _Person->Next;
	}
	return _Ct;
}

void TribalCreateBigGuys(struct Settlement* _Settlement, double _CastePercent[CASTE_SIZE]) {
	struct LinkedList _UniqueFamilies = {0, NULL, NULL};
	struct LnkLst_Node* _FamilyItr = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct Family* _Family = NULL;
	struct BigGuy* _Leader = NULL;
	uint8_t _BGStats[BGSKILL_SIZE];
	uint8_t _LeaderCaste = CASTE_NOBLE;
	int _Motivations[BGMOT_SIZE] = {2, 4};
	int _MotCt = 0;
	int _Count = 0;//_Settlement->Families.Size * 0.1f; //How many big guys to make.
	int _FamilyCt = _Settlement->Families.Size;
	int* _CasteCount = alloca(sizeof(int) * CASTE_SIZE);

	if(_FamilyCt < 20)
		_Count = _FamilyCt * 0.4;
	else 
		_Count = _FamilyCt * 0.2;
	memset(_CasteCount, 0, sizeof(int) * CASTE_SIZE);
	Assert((_CastePercent[CASTE_THRALL] + _CastePercent[CASTE_LOWCLASS] + _CastePercent[CASTE_HIGHCLASS] + _CastePercent[CASTE_NOBLE]) != 100);
	RandTable(_CastePercent, &_CasteCount, CASTE_SIZE, _Count);
	_Itr = _Settlement->Families.Front;
	while(_Itr != NULL) {
		if(_Count <= 0)
			break;
		_Family = (struct Family*)_Itr->Data;
		_FamilyItr = _UniqueFamilies.Front;
		while(_FamilyItr != NULL) {
			if(((struct Family*)_FamilyItr->Data)->Object.Id == _Family->Object.Id)
				goto skip_bigguy;
			_FamilyItr = _FamilyItr->Next;
		}
		if(_CasteCount[_Family->Caste->Type] <= 0)
			goto skip_bigguy;
		--_CasteCount[_Family->Caste->Type];
		BGStatsWarlord(&_BGStats, Random(BG_MINGENSTATS, BG_MAXGENSTATS));
		CreateBigGuy(_Family->People[0], &_BGStats, _MotCt);
		--_Motivations[_MotCt];
		if(_Motivations[_MotCt] <= 0)
			++_MotCt;
		LnkLstPushBack(&_UniqueFamilies, _Family);
		--_Count;
		skip_bigguy:
		_Itr = _Itr->Next;
	}
	Assert(_Settlement->BigGuys.Size != 0);
	SettlementSetBGOpinions(&_Settlement->BigGuys);
	if(_CasteCount[CASTE_NOBLE] == 0) {
		_LeaderCaste = CASTE_WARRIOR;
	}
	for(_Itr = _Settlement->BigGuys.Front; _Itr != NULL; _Itr = _Itr->Next) {
		_Leader = _Itr->Data;
		if(PERSON_CASTE(_Leader->Person) == _LeaderCaste)
			break;
	}
	Assert(PERSON_CASTE(_Leader->Person) != _LeaderCaste);
	GovernmentSetLeader(_Settlement->Government, _Leader);
	LnkLstClear(&_UniqueFamilies);
}

int SettlementBigGuyCt(const struct Settlement* _Settlement) {
	return _Settlement->BigGuys.Size;
}

int SettlementAdultPop(const struct Settlement* _Settlement) {
	return _Settlement->Families.Size;
}

int SettlementGetNutrition(const struct Settlement* _Settlement) {
	const struct LnkLst_Node* _Itr = _Settlement->Families.Front;
	int _Nutrition = 0;

	while(_Itr != NULL) {
		_Nutrition = _Nutrition + FamilyGetNutrition((const struct Family*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	return _Nutrition;
}

int SettlementYearlyNutrition(const struct Settlement* _Settlement) {
	const struct LnkLst_Node* _Itr = _Settlement->Families.Front;
	const struct Family* _Family = NULL;
	int _Nutrition = 0;

	while(_Itr != NULL) {
		_Family = ((struct Family*)_Itr->Data);
		for(int i = 0; i < _Family->FieldCt; ++i) {
			_Nutrition = _Nutrition + (_Family->Fields[i]->Acres * 400);
		}
		_Itr = _Itr->Next;
	}
	return _Nutrition;
}

int SettlementCountAcres(const struct Settlement* _Settlement) {
	const struct LnkLst_Node* _Itr = _Settlement->Families.Front;
	int _Acres = 0;

	while(_Itr != NULL) {
		_Acres = _Acres + FamilyCountAcres((struct Family*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	return _Acres;
}

int SettlementExpectedYield(const struct Settlement* _Settlement) {
	const struct LnkLst_Node* _Itr = _Settlement->Families.Front;
	int _Yield = 0;

	while(_Itr != NULL) {
		_Yield = _Yield + FamilyExpectedYield((struct Family*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	return _Yield * HarvestModifier((uint8_t (* const)[HARVEST_YEARS])&_Settlement->HarvestMod);
}

struct Plot* SettlementFindPlot(const struct Settlement* _Settlement, int _PlotType, void* _PlotData) {
	struct Plot* _Plot = NULL;

	for(const struct LnkLst_Node* _Itr = SettlementPlots(_Settlement); _Itr != NULL; _Itr = _Itr->Next) {
		_Plot = _Itr->Data;
		if(_Plot->Type == PLOT_CHANGEPOLICY && _Plot->PlotData == _PlotData) {
			return _Plot;
		}
	}
	return NULL;
}

struct Retinue* SettlementAddRetinue(struct Settlement* _Settlement, struct BigGuy* _Leader) {
	struct Retinue* _Retinue = CreateRetinue(_Leader);

	_Retinue->Next = _Settlement->Retinues;
	_Settlement->Retinues = _Retinue;
	for(struct LnkLst_Node* _Itr = _Settlement->FreeWarriors.Front; _Itr != NULL; _Itr = _Itr->Next) {
		if(_Leader== _Itr->Data) {
			LnkLstRemove(&_Settlement->FreeWarriors, _Itr);
			break;
		}
	}
	IntInsert(&g_GameWorld.PersonRetinue, _Leader->Person->Object.Id, _Retinue);
	return _Retinue;
}
