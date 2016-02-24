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

#include "sys/Math.h"
#include "sys/ResourceManager.h"
#include "sys/Array.h"

#include "video/Sprite.h"
#include "video/Tile.h"
#include "video/MapRenderer.h"

#include <stdlib.h>
#include <string.h>

#define HARVESTMOD_MIN 0.4f
#define HARVESTMOD_MAX 1.6f

void LocationGetPoint(const struct Location* _Location, SDL_Point* _Point) {
	(*_Point).x = _Location->Pos.x;
	(*_Point).y = _Location->Pos.y;
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
	_Loc->HarvestMod = 1.0f;//Random(HARVESTMOD_MIN * 10, HARVESTMOD_MAX * 10) / 10;
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
	_Loc->Bulitin = NULL;
	FieldSetStatus(&_Loc->Meadow, EGROWING);
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

//FIXME: These includes are temporary and should be removed.`
#include "video/GuiLua.h"
#include "sys/LuaCore.h"
void SettlementThink(struct Settlement* _Settlement) {
	struct LnkLst_Node* _Itr = _Settlement->Families.Front;

	FieldUpdate(&_Settlement->Meadow);
	while(_Itr != NULL) {
		FamilyThink((struct Family*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	GovernmentThink(_Settlement->Government);
	if(MONTH(g_GameWorld.Date) == 0 && DAY(g_GameWorld.Date) == 0) {
		if(YEAR(g_GameWorld.Date) - YEAR(_Settlement->LastRaid) >= 1 && DAY(g_GameWorld.Date) == 0) {
			MessageBox(g_LuaState, "You have not raided recently.");
			_Itr = _Settlement->BigGuys.Front;
			while(_Itr != NULL) {
				if(_Settlement->Government->Leader == ((struct BigGuy*)_Itr->Data)) {
					_Itr = _Itr->Next;
					continue;
				}
				BigGuyChangeOpinion((struct BigGuy*)_Itr->Data, _Settlement->Government->Leader, ACTTYPE_WARLACK, -10); 
				_Itr = _Itr->Next;
			}
		}
		_Settlement->YearBirths = 0;
		_Settlement->YearDeaths = 0;
		_Settlement->HarvestMod = _Settlement->HarvestMod + (((float)(Random(0, 6) + Random(0, 6))) / 10);
		if(_Settlement->HarvestMod < HARVESTMOD_MIN)
			_Settlement->HarvestMod = HARVESTMOD_MIN;
		else if(_Settlement->HarvestMod > HARVESTMOD_MAX)
			_Settlement->HarvestMod = HARVESTMOD_MAX;
	}
}

void SettlementDraw(const struct MapRenderer* _Renderer, struct Settlement* _Settlement) {
	SDL_Point _Point = {_Settlement->Pos.x, _Settlement->Pos.y};

	MapDrawColorOverlay(_Renderer, &_Point, &_Settlement->Government->ZoneColor);
}

void SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family) {
	_Location->Meadow.Acres = _Location->Meadow.Acres + 10;
	LnkLstPushBack(&_Location->Families, _Family);
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
}

void SettlementRemovePerson(struct Settlement* _Settlement, struct Person* _Person) {
	ILL_DESTROY(_Person->Family->HomeLoc->People, _Person);
	--_Settlement->NumPeople;
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

void TribalCreateBigGuys(struct Settlement* _Settlement) {
	struct LinkedList _UniqueFamilies = {0, NULL, NULL};
	struct LnkLst_Node* _FamilyItr = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct Family* _Family = NULL;
	struct BigGuyStats _BGStats;

	_Itr = _Settlement->Families.Front;
	while(_Itr != NULL) {
		_Family = (struct Family*)_Itr->Data;
		_FamilyItr = _UniqueFamilies.Front;
		while(_FamilyItr != NULL) {
			if(((struct Family*)_FamilyItr->Data)->Id == _Family->Id)
				goto skip_bigguy;
			_FamilyItr = _FamilyItr->Next;
		}
		BGStatsWarlord(&_BGStats, Random(40, 60));
		CreateBigGuy(_Family->People[0], &_BGStats);
		LnkLstPushBack(&_UniqueFamilies, _Family);
		skip_bigguy:
		_Itr = _Itr->Next;
	}
	_Settlement->Government->NewLeader(_Settlement->Government);
	LnkLstClear(&_UniqueFamilies);
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
	const struct Field* _Field = NULL;
	int _Nutrition = 0;

	while(_Itr != NULL) {
		_Family = ((struct Family*)_Itr->Data);
		for(int i = 0; i < _Family->Fields->Size; ++i) {
			_Field = _Family->Fields->Table[i];
			_Nutrition = _Nutrition + (_Field->Acres * 400);
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
	return _Yield * _Settlement->HarvestMod;
}
