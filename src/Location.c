/* Author: David Brotz
 * File: Location.c
 */

#include "Location.h"

#include "BigGuy.h"
#include "Person.h"
#include "Family.h"
#include "Government.h"
#include "Warband.h"

#include "sys/Math.h"

#include "video/Sprite.h"
#include "video/Tile.h"
#include "video/MapRenderer.h"

#include <stdlib.h>
#include <string.h>

#define SETTLEMENT_DENSITY (50)

struct SettlementPart* CreateSettlementPart(struct Settlement* _Settlement, const SDL_Point* _Pos) {
	struct SettlementPart* _Part = (struct SettlementPart*) malloc(sizeof(struct SettlementPart));

	_Part->NumPeople = 0;
	_Part->Pos = *_Pos;
	_Part->Owner = _Settlement;
	_Part->Families.Size = 0;
	_Part->Families.Front = NULL;
	_Part->Families.Back = NULL;
	_Part->People = NULL;
	_Part->People = NULL;
	_Part->Next = NULL;
	_Part->Prev = NULL;
	LnkLstPushBack(&_Settlement->Sprites, CreateSprite(g_GameWorld.MapRenderer, g_GameWorld.MapRenderer->Settlement, MAPRENDER_SETTLEMENT, _Pos));
	return _Part;
}

void DestroySettlementPart(struct SettlementPart* _Part) {
	LnkLstClear(&_Part->Families);
	free(_Part);
}

void LocationGetArea(const struct Location* _Location, SDL_Rect* _AABB) {
	(*_AABB).x = _Location->Pos.x;
	(*_AABB).y = _Location->Pos.y;
	(*_AABB).w = _Location->Pos.w;
	(*_AABB).h = _Location->Pos.h;
}

struct Settlement* CreateSettlement(int _X, int _Y, int _Width, int _Height, const char* _Name, int _GovType) {
	struct Settlement* _Loc = (struct Settlement*) malloc(sizeof(struct Settlement));
	SDL_Point _Point;

	CreateObject((struct Object*)_Loc, OBJECT_LOCATION, (void(*)(struct Object*))SettlementThink);
	_Loc->LocType = ELOC_SETTLEMENT;
	_Loc->Pos.x = _X;
	_Loc->Pos.y = _Y;
	_Loc->Pos.w = _Width;
	_Loc->Pos.h = _Height;
	_Loc->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Loc->People = NULL;
	_Loc->Government = CreateGovernment(_GovType, 0, _Loc);
	_Loc->NumPeople = 0;
	_Loc->BigGuys.Size = 0;
	_Loc->BigGuys.Front = NULL;
	_Loc->Sprites.Size = 0;
	_Loc->Sprites.Front = NULL;
	_Loc->Sprites.Back = NULL;
	_Loc->NumFamilies = 0;
	strcpy(_Loc->Name, _Name);
	_Point.x = _Loc->Pos.x + (_Loc->Pos.w / 2);
	_Point.y = _Loc->Pos.y + (_Loc->Pos.h / 2);
	_Loc->FirstPart = CreateSettlementPart(_Loc, &_Point);
	_Loc->LastPart = _Loc->FirstPart;
	LnkLstPushBack(&g_GameWorld.Settlements, _Loc);
	QTInsertAABB(&g_GameWorld.SettlementMap, _Loc, &_Loc->Pos);
	return _Loc;
}

void DestroySettlement(struct Settlement* _Location) {
	DestroyObject((struct Object*)_Location);
	free(_Location->Government);
	free(_Location->Name);
	free(_Location);
}

void SettlementThink(struct Settlement* _Settlement) {
	struct Family* _Families[_Settlement->NumFamilies];

	if(_Settlement->NumFamilies > 0)
		SettlementGetFamilies(_Settlement, _Families);
	for(int i = 0; i < _Settlement->NumFamilies; ++i) {
		FamilyThink((struct Family*)_Families[i]);
	}
	GovernmentThink(_Settlement->Government);
}

void SettlementDraw(const struct MapRenderer* _Renderer, struct Settlement* _Settlement) {
	SDL_Point _Point = {_Settlement->Pos.x, _Settlement->Pos.y};
	struct LinkedList _List = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;

	for(_Point.x = _Settlement->Pos.x; _Point.x <= _Settlement->Pos.x + _Settlement->Pos.w; ++_Point.x)
		for(_Point.y = _Settlement->Pos.y; _Point.y <= _Settlement->Pos.y + _Settlement->Pos.h; ++_Point.y)
			LnkLstPushBack(&_List, MapGetTileConst(_Renderer, &_Point));
	//TilesInRange(_Renderer, &_Center, _Center.y, &_List);
	_Itr = _List.Front;
	while(_Itr != NULL) {
		MapDrawColorOverlay(_Renderer, &((struct Tile*)_Itr->Data)->TilePos, &_Settlement->Government->ZoneColor);
		_Itr = _Itr->Next;
	}
	LnkLstClear(&_List);
}

/*
 * Instead of using a linked list for parts use an array that is size of 3n(n - 1) + 1 in size which is how many elements there are per n rings.
 * Fill invalid tiles with NULL so we can then quickly determine how many parts are already in the outer most ring by keeping track of how many rings we have
 * and then find the first valid element in that ring.
 */

struct SettlementPart* SettlementPlaceFamily(struct Settlement* _Location, struct Family* _Family, int* _X, int* _Y) {
	struct SettlementPart* _Part = _Location->LastPart;
	SDL_Point _Point = {_Part->Pos.x, _Part->Pos.y};
	struct Tile* _Tile = NULL;
	int _Count = 1; //How many tiles left in the current ring.
	int _RingUsed = 0; //How many tiles in the current ring have been iterated over.
	int _Dir = TILE_EAST - 1;
	int _DirCount = 0; //How many times we go in a certain direction before going to the next.
	int _Radius = 0;

	do {
		if(_Part->NumPeople < SETTLEMENT_DENSITY) {
			*_X = _Part->Pos.x;
			*_Y = _Part->Pos.y;
			LnkLstPushBack(&_Part->Families, _Family);
			++_Location->NumFamilies;
			return _Part;
		}
		/*
		 * FIXME: Below code is for edge cases where the next tile in the ring does not exist because the settlement is on the edge of the map.
		 * The below code is meant to skip the invalid  tile by checking if the next SettlementPart's position is equal to the next tile in the ring.
		 */
	/*	if(_Point.x != _Part->Pos.x && _Point.y != _Part->Pos.y) {
			++_RingUsed;
			--_Count;
			TileNextInRing(&_Point, &_Point, _RingUsed, _Radius);
		}*/
			--_Count;
			++_RingUsed;
			if(_Count <= 0) {
				TileNextRing(&_Part->Pos, &_Point, _Radius);
				++_Radius;
				if(_Radius == 1)
					_Count = TILE_SIZE;
				else {
					_Count = _Radius * TILE_SIZE;
				}
			} else
				TileNextInRing(&_Point, &_Point, _RingUsed, _Radius);
		_Part = _Part->Prev;
		if(_Part == NULL)
			break;
	} while(1);
	_Point.x = _Location->FirstPart->Pos.x;
	_Point.y = _Location->FirstPart->Pos.y;
	_RingUsed = (_Radius * TILE_SIZE) - _Count;
	_Dir = TileNextInRing(&_Point, &_Point, _RingUsed, _Radius);
	_DirCount = _Radius;
	while((_Tile = MapGetTile(g_GameWorld.MapRenderer, &_Point)) == NULL) {
		--_DirCount;
		GetAdjPos(&_Point, &_Point, _Dir);
		if(_DirCount <= 0) {
			if(_Dir == TILE_NORTHWEST)
				_Dir = TILE_EAST;
			else
				++_Dir;
			if(_Dir >= TILE_SIZE) {
				_Dir = TILE_NORTHWEST;
				TileNextRing(&_Point, &_Point, _Radius);
				++_Radius;
			}
			_DirCount = _Radius;
		}
	}
	SettlementAddTile(_Location, _Tile);
	_Part = CreateSettlementPart(_Location, &_Tile->TilePos);
	ILL_CREATE(_Location->FirstPart, _Part);
	return _Part;
}

void SettlementAddTile(struct Settlement* _Location, const struct Tile* _Tile) {
	SDL_Rect _TileRect = {_Location->Pos.x, _Location->Pos.y, 1, 1};
	struct LinkedList _List = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;

	QTAABBInRectangle(&g_GameWorld.SettlementMap, &_TileRect, (void(*)(const void*, SDL_Rect*))LocationGetArea, &_List);
	_Itr = _List.Front;
	while(_Itr != NULL) {
		if(((struct Object*)_Itr->Data)->Id == _Location->Id) {
			QTRemoveAABB(&g_GameWorld.SettlementMap, &_Location->Pos, (void(*)(const void*, struct SDL_Rect*))LocationGetArea);
			QTInsertAABB(&g_GameWorld.SettlementMap, _Location, &_Location->Pos);
			break;
		}
		_Itr = _Itr->Next;
	}
	LnkLstClear(&_List);
	if(_Tile->TilePos.y > _Location->Pos.y) {
		if(_Tile->TilePos.y <= _Location->Pos.y + _Location->Pos.h) {
			if(_Tile->TilePos.x > _Location->Pos.x) {
				if(_Tile->TilePos.x >= _Location->Pos.x + _Location->Pos.w) {
					++_Location->Pos.w;
				}
			} else {
				if(_Location->Pos.x > 0)
					--_Location->Pos.x;
				++_Location->Pos.w;
			}
		}
	} else {
		if(_Location->Pos.y > 0)
			--_Location->Pos.y;
		++_Location->Pos.h;
	}
}

int SettlementIsFriendly(const struct Settlement* _Location, struct Army* _Army) {
	return (GovernmentTop(_Location->Government) == GovernmentTop(_Army->Government));
}

void SettlementGetCenter(const struct Settlement* _Location, SDL_Point* _Pos) {
	struct Sprite* _Sprite = (struct Sprite*) _Location->Sprites.Front->Data;

	_Pos->x = _Sprite->TilePos.x;
	_Pos->y = _Sprite->TilePos.y;
}

void SettlementAddPerson(struct SettlementPart* _Settlement, struct Person* _Person) {
	ILL_CREATE(_Settlement->Owner->People, _Person);
	++_Settlement->NumPeople;
	++_Settlement->Owner->NumPeople;
}

void SettlementRemovePerson(struct SettlementPart* _Settlement, struct Person* _Person) {
	ILL_DESTROY(_Person->Family->HomeLoc->People, _Person);
	--_Settlement->NumPeople;
	--_Settlement->Owner->NumPeople;
}

void SettlementGetFamilies(struct Settlement* _Settlement, struct Family** _Families) {
	struct SettlementPart* _Part = _Settlement->FirstPart;
	struct LnkLst_Node* _Itr = NULL;
	int i = 0;

	do {
		_Itr = _Part->Families.Front;
		while(_Itr != NULL) {
			_Families[i] = (struct Family*)_Itr->Data;
			++i;
			_Itr = _Itr->Next;
		}
		_Part = _Part->Next;
	} while(_Part != NULL);
}

void SettlementGetPeople(struct Settlement* _Settlement, struct Person** _People) {
	struct SettlementPart* _Part = _Settlement->FirstPart;
	struct Person* _Itr = NULL;
	int i = 0;

	do {
		_Itr = _Part->People;
		while(_Itr != NULL) {
			_People[i] = _Itr;
			++i;
			_Itr = _Itr->Next;
		}
		_Part = _Part->Next;
	} while(_Part != NULL);
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
	struct Family* _FamilyList[_Settlement->NumFamilies];
	struct LinkedList _UniqueFamilies = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	struct Family* _Family = NULL;
	struct BigGuyStats _BGStats;

	SettlementGetFamilies(_Settlement, _FamilyList);
	for(int i = 0; i < _Settlement->NumFamilies; ++i) {
		_Family = _FamilyList[i];
		_Itr = _UniqueFamilies.Front;
		while(_Itr != NULL) {
			if(((struct Family*)_Itr->Data)->FamilyId == _Family->FamilyId)
				goto skip_bigguy;
			_Itr = _Itr->Next;
		}
		BGStatsWarlord(&_BGStats, Random(40, 60));
		CreateBigGuy(_Family->People[0], &_BGStats);
		LnkLstPushBack(&_UniqueFamilies, _Family);
		skip_bigguy:
		continue;
	}
	_Settlement->Government->NewLeader(_Settlement->Government);
	LnkLstClear(&_UniqueFamilies);
}

int SettlementPartGetNutrition(const struct SettlementPart* _Part) {
	const struct LnkLst_Node* _Itr = _Part->Families.Front;
	int _Nutrition = 0;

	while(_Itr != NULL) {
		_Nutrition = FamilyGetNutrition((const struct Family*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	return _Nutrition;
}

int SettlementGetNutrition(const struct Settlement* _Settlement) {
	int _Nutrition = 0;
	struct SettlementPart* _Part = _Settlement->FirstPart;

	while(_Part != NULL) {
		_Nutrition = SettlementPartGetNutrition(_Part);
		_Part = _Part->Next;
	}
	return _Nutrition;
}
