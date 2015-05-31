/*
 * File: World.c
 * Author: David Brotz
 */

#include "World.h"

#include "Herald.h"
#include "Occupation.h"
#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "Good.h"
#include "Population.h"
#include "Location.h"
#include "Mission.h"
#include "BigGuy.h"
#include "Government.h"

#include "video/Video.h"
#include "video/Tile.h"
#include "video/Sprite.h"

#include "sys/TaskPool.h"
#include "sys/Array.h"
#include "sys/Event.h"
#include "sys/Constraint.h"
#include "sys/HashTable.h"
#include "sys/KDTree.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"
#include "sys/MemoryPool.h"
#include "sys/Random.h"
#include "sys/RBTree.h"
#include "sys/LuaCore.h"
#include "AI/Setup.h"
#include "AI/AIHelper.h"

#include "Warband.h"
#include "Location.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

struct GameWorld g_GameWorld = {
		1,
		0,
		NULL,
		{NULL, NULL, NULL, NULL, {{0, 0}, {0, 0}}, NULL},
		NULL,
		NULL,
		{NULL, 0, NULL, NULL},
		{NULL, 0},
		NULL,
		{0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{NULL, 0, NULL, NULL},
};

struct RBTree* g_GoodDeps = NULL;
struct Array* g_AnFoodDep = NULL;
struct RBTree g_Families;
struct KDTree g_ObjPos;
struct TaskPool* g_TaskPool = NULL;
struct HashTable* g_AIHash = NULL;
int g_TemperatureList[] = {32, 33, 41, 46, 56, 61, 65, 65, 56, 51, 38, 32};
int g_Temperature = 0;

int FamilyICallback(const struct Family* _One, const struct Family* _Two) {
	return _One->Id - _Two->Id;
}

int FamilySCallback(const int* _One, const struct Family* _Two) {
	return (*_One) - _Two->Id;
}

int FamilyTypeCmp(const void* _One, const void* _Two) {
	return (((struct FamilyType*)_One)->Percent * 1000) - (((struct FamilyType*)_Two)->Percent * 1000);
}

void PopulateManor(int _Population, struct FamilyType** _FamilyTypes, int _X, int _Y) {
	int _FamilySize = -1;
	struct Family* _Family = NULL;
	struct Family* _Parent = NULL;
	struct Constraint** _AgeGroups = NULL;
	struct Constraint** _BabyAvg = NULL;
	struct Settlement* _Settlement = NULL;
	
	//TODO: AgeGroups and BabyAvg should not be here but instead in a global or as an argument.
	lua_getglobal(g_LuaState, "AgeGroups");
	LuaConstraintBnds(g_LuaState);
	if((_AgeGroups = lua_touserdata(g_LuaState, -1)) == NULL) {
		Log(ELOG_ERROR, "AgeGroups is not defined.");
	}

	lua_getglobal(g_LuaState, "BabyAvg");
	LuaConstraintBnds(g_LuaState);
	if((_BabyAvg = lua_touserdata(g_LuaState, -1)) == NULL) {
		DestroyConstrntBnds(_AgeGroups);
		Log(ELOG_ERROR, "BabyAvg is not defined.");
		return;
	}
	_Settlement = CreateSettlement(_X, _Y, _X + 1, _Y + 1, "Test Settlement", (GOVSTCT_TRIBAL | GOVRULE_ELECTIVE | GOVTYPE_CONSENSUS));
	GameWorldInsertSettlement(&g_GameWorld, _Settlement);
	while(_Population > 0) {
		_FamilySize = Fuzify(g_FamilySize, Random(1, 100));
		//TODO: _X and _Y should be given to the family by the CityLocation they live in.
		_Parent = CreateRandFamily("Bar", Fuzify(_BabyAvg, Random(0, 9999)) + 2, _AgeGroups, _BabyAvg, _X, _Y, _Settlement);
		FamilyAddGoods(_Parent, g_LuaState, _FamilyTypes, _X, _Y, _Settlement);
		RBInsert(&g_Families, _Parent);
		while(_FamilySize > 0) {
			_Family = CreateRandFamily("Bar", Fuzify(_BabyAvg, Random(0, 9999)) + 2, _AgeGroups, _BabyAvg, _X, _Y, _Settlement);
			FamilyAddGoods(_Family, g_LuaState, _FamilyTypes, _X, _Y, _Settlement);
			RBInsert(&g_Families, _Family);
			_FamilySize -= FamilySize(_Family);
		}
		_Population -= FamilySize(_Parent);
	}
	GovernmentCreateLeader(_Settlement->Government);
	DestroyConstrntBnds(_AgeGroups);
	DestroyConstrntBnds(_BabyAvg);
	lua_pop(g_LuaState, 4);
}

int PopulateWorld() {
	struct FamilyType** _FamilyTypes = NULL;
	struct Constraint** _ManorSize = NULL;
	const char* _Temp = NULL;
	int _ManorMin = 0;
	int _ManorMax = 0;
	int _ManorInterval = 0;
	int i = 0;

	if(LuaLoadFile(g_LuaState, "std.lua") != LUA_OK) {
		goto end;
	}
	lua_getglobal(g_LuaState, "ManorConstraints");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "ManorConstraints is not defined.");
		goto end;
	}
	lua_getfield(g_LuaState, -1, "Min");
	AddInteger(g_LuaState, -1, &_ManorMin);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Max");
	AddInteger(g_LuaState, -1, &_ManorMax);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Interval");
	AddInteger(g_LuaState, -1, &_ManorInterval);
	lua_pop(g_LuaState, 2);
	_ManorSize = CreateConstrntLst(NULL, _ManorMin, _ManorMax, _ManorInterval);
	lua_getglobal(g_LuaState, "FamilyTypes");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "FamilyTypes is not defined.");
		goto end;
	}
	_FamilyTypes = calloc(lua_rawlen(g_LuaState, -1) + 1, sizeof(struct FamilyType*));
	lua_pushnil(g_LuaState);
	while(lua_next(g_LuaState, -2) != 0) {
		_FamilyTypes[i] = malloc(sizeof(struct FamilyType));
		lua_pushnil(g_LuaState);
		lua_next(g_LuaState, -2);
		AddNumber(g_LuaState, -1, &_FamilyTypes[i]->Percent);
		lua_pop(g_LuaState, 1);
		lua_next(g_LuaState, -2);
		AddString(g_LuaState, -1, &_Temp);
		_FamilyTypes[i]->LuaFunc = calloc(strlen(_Temp) + 1, sizeof(char));
		strcpy(_FamilyTypes[i]->LuaFunc, _Temp);
		++i;
		lua_pop(g_LuaState, 3);
	}
	lua_pop(g_LuaState, 1);
	InsertionSort(_FamilyTypes, i, FamilyTypeCmp);
	_FamilyTypes[i] = NULL;
	PopulateManor((Fuzify(_ManorSize, Random(_ManorMin, _ManorMax)) * _ManorInterval) + _ManorInterval, _FamilyTypes, 0,  0);
	g_GameWorld.Player = PickPlayer();
	PopulateManor((Fuzify(_ManorSize, Random(_ManorMin, _ManorMax)) * _ManorInterval) + _ManorInterval, _FamilyTypes, 0,  10);
	DestroyConstrntBnds(_ManorSize);
	return 1;
	end:
	DestroyConstrntBnds(_ManorSize);
	return 0;
}

struct BigGuy* PickPlayer() {
	return ((struct Settlement*)g_GameWorld.Settlements.Front->Data)->Government->Leader;
}

struct WorldTile* CreateWorldTile() {
	struct WorldTile* _Tile = (struct WorldTile*) malloc(sizeof(struct WorldTile));

	_Tile->Temperature = 0;
	return _Tile;

}
void DestroyWorldTile(struct WorldTile* _Tile) {
	free(_Tile);
}

void ArmyTest() {
	struct Army* _One = CreateArmy(g_GameWorld.Player);
	struct Army* _Two = CreateArmy(g_GameWorld.Player);
	CreateWarband((struct Settlement*)g_GameWorld.Settlements.Front->Data, _One);
	CreateWarband((struct Settlement*)g_GameWorld.Settlements.Back->Data,	_Two);
	struct Battle* _Battle = CreateBattle(_One, _Two);
	BattleThink(_Battle);
	BattleThink(_Battle);
}

void GameWorldInit(int _Area) {
	//TODO: When this data is moved to a more proper spot remove sys/video.h from the includes.
	struct Point _ScreenSize = {(SDL_WIDTH / TILE_WIDTH) / 2, (SDL_HEIGHT / TILE_HEIGHT_THIRD) / 2};

	g_GameWorld.MapRenderer = CreateMapRenderer(_Area, &_ScreenSize);
	g_GameWorld.SettlementIndex.BoundingBox.Center.X = _ScreenSize.X;
	g_GameWorld.SettlementIndex.BoundingBox.Center.Y = _ScreenSize.Y;
	g_GameWorld.SettlementIndex.BoundingBox.HalfDimension.X = _ScreenSize.X;
	g_GameWorld.SettlementIndex.BoundingBox.HalfDimension.Y = _ScreenSize.Y;
	g_GameWorld.SettlementIndex.Data = NULL;
	g_GameWorld.SettlementIndex.NorthWest = NULL;
	g_GameWorld.SettlementIndex.NorthEast = NULL;
	g_GameWorld.SettlementIndex.SouthEast = NULL;
	g_GameWorld.SettlementIndex.SouthWest = NULL;

	g_GameWorld.Settlements.Size = 0;
	g_GameWorld.Settlements.Front = NULL;
	g_GameWorld.Settlements.Back = NULL;

	g_GameWorld.BigGuys.Table = NULL;
	g_GameWorld.BigGuys.Size = 0;
	g_GameWorld.BigGuys.ICallback = (int(*)(const void*, const void*)) BigGuyIdInsert;
	g_GameWorld.BigGuys.SCallback = (int(*)(const void*, const void*)) BigGuyIdCmp;

	g_GameWorld.BigGuyStates.Table = NULL;
	g_GameWorld.BigGuyStates.Size = 0;
	g_GameWorld.BigGuyStates.ICallback = (int(*)(const void*, const void*)) BigGuyStateInsert;
	g_GameWorld.BigGuyStates.SCallback = (int(*)(const void*, const void*)) BigGuyMissionCmp;
	g_GameWorld.Player = NULL;
}
void WorldInit(int _Area) {
	int i;
	int _WorldSize = _Area * _Area;
	struct Array* _Array = NULL;
	struct LinkedList _CropList = {0, NULL, NULL};
	struct LinkedList _GoodList = {0, NULL, NULL};
	struct LinkedList _BuildList = {0, NULL, NULL};
	struct LinkedList _PopList = {0, NULL, NULL};
	struct LinkedList _OccupationList = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;

	Log(ELOG_INFO, "Creating World.");
	++g_Log.Indents;
	GameWorldInit(_Area);
	g_AIHash = CreateHash(32);
	chdir(DATAFLD);
	AIInit(g_LuaState);
	_Array = FileLoad("FirstNames.txt", '\n');
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), 1000000);
	Family_Init(_Array);
	if(LuaLoadList(g_LuaState, "goods.lua", "Goods", (void*(*)(lua_State*, int))&GoodLoad, &LnkLst_PushBack, &_GoodList) == 0)
		goto end;
	g_Goods.TblSize = (_GoodList.Size * 5) / 4;
	g_Goods.Table = (struct HashNode**) calloc(g_Goods.TblSize, sizeof(struct HashNode*));
	memset(g_Goods.Table, 0, g_Goods.TblSize * sizeof(struct HashNode*));
	LISTTOHASH(&_GoodList, _Itr, &g_Goods, ((struct GoodBase*)_Itr->Data)->Name);
	
	if(LuaLoadList(g_LuaState, "crops.lua", "Crops", (void*(*)(lua_State*, int))&CropLoad, &LnkLst_PushBack, &_CropList) == 0)
		goto end;
	g_Crops.TblSize = (_CropList.Size * 5) / 4;
	g_Crops.Table = (struct HashNode**) calloc(g_Crops.TblSize, sizeof(struct HashNode*));
	memset(g_Crops.Table, 0, g_Crops.TblSize * sizeof(struct HashNode*));
	LISTTOHASH(&_CropList, _Itr, &g_Crops, ((struct Crop*)_Itr->Data)->Name)


	if(_GoodList.Size == 0) {
		Log(ELOG_WARNING, "Failed to load goods.");
		goto GoodLoadEnd;
	}
	if(LuaLoadFile(g_LuaState, "goods.lua") != LUA_OK)
		goto end;
	lua_getglobal(g_LuaState, "Goods");
	i = 1;
	_Itr = _GoodList.Front;
	while(_Itr != NULL) {
		lua_pushstring(g_LuaState, ((struct GoodBase*)_Itr->Data)->Name);
		lua_rawgeti(g_LuaState, -2, i++);
		lua_rawset(g_LuaState, -3);
		_Itr = _Itr->Next;
	}
	lua_pop(g_LuaState, 1);
	g_GoodOutputs = realloc(g_GoodOutputs, sizeof(struct GoodOutput*) * (g_GoodOutputsSz + 1));
	g_GoodOutputs[g_GoodOutputsSz] = NULL;
	_Itr = _GoodList.Front;
	while(_Itr != NULL) {
		if(GoodLoadInput(g_LuaState, ((struct GoodBase*)_Itr->Data)) == 0)
			goto goodload_loopend;
		GoodLoadOutput(g_LuaState, ((struct GoodBase*)_Itr->Data));
		Log(ELOG_INFO, "Good loaded %s.", ((struct GoodBase*)_Itr->Data)->Name);
		goodload_loopend:
		_Itr = _Itr->Next;
	}
	GoodLoadEnd:
	if(LuaLoadList(g_LuaState, "populations.lua", "Populations", (void*(*)(lua_State*, int))&PopulationLoad, &LnkLst_PushBack,  &_PopList) == 0)
		goto end;
	g_Populations.TblSize = (_PopList.Size * 5) / 4;
	g_Populations.Table = (struct HashNode**) calloc(g_Populations.TblSize, sizeof(struct HashNode*));
	memset(g_Populations.Table, 0, g_Populations.TblSize * sizeof(struct HashNode*));

	if(LuaLoadList(g_LuaState, "occupations.lua", "Occupations", (void*(*)(lua_State*, int))&OccupationLoad, &LnkLst_PushBack, &_OccupationList) == 0)
		goto end;
	g_Occupations.TblSize = ((_OccupationList.Size + 1) * 5) / 4;
	g_Occupations.Table = (struct HashNode**) calloc(g_Occupations.TblSize, sizeof(struct HashNode*));
	memset(g_Occupations.Table, 0, g_Occupations.TblSize * sizeof(struct HashNode*));

	if(LuaLoadList(g_LuaState, "buildings.lua", "BuildMats", (void*(*)(lua_State*, int))&BuildingLoad, (void(*)(struct LinkedList*, void*))&LnkLst_CatNode, &_BuildList) == 0)
		goto end;
	g_BuildMats.TblSize = (_BuildList.Size * 5) / 4;
	g_BuildMats.Table = (struct HashNode**) calloc(g_BuildMats.TblSize, sizeof(struct HashNode*));
	memset(g_BuildMats.Table, 0, g_BuildMats.TblSize * sizeof(struct HashNode*));
	g_GoodOutputs = calloc(_GoodList.Size + 1, sizeof(struct GoodOutput*));

	LISTTOHASH(&_PopList, _Itr, &g_Populations, ((struct Population*)_Itr->Data)->Name);
	LISTTOHASH(&_OccupationList, _Itr, &g_Occupations, ((struct Occupation*)_Itr->Data)->Name);
	LISTTOHASH(&_BuildList, _Itr, &g_BuildMats, ((struct BuildMat*)_Itr->Data)->Good->Name);
	g_Families.Table = NULL;
	g_Families.Size = 0;
	g_Families.ICallback = (int (*)(const void*, const void*))&FamilyICallback;
	g_Families.SCallback = (int (*)(const void*, const void*))&FamilySCallback;

	g_ObjPos.Root = NULL;
	g_ObjPos.Size = 0;
	g_GoodDeps = GoodBuildDep(&g_Goods);
	g_AnFoodDep = AnimalFoodDep(&g_Populations);
	if(PopulateWorld() == 0)
		goto end;
	ArmyTest();
	end:
	chdir("..");
	--g_Log.Indents;
}

void WorldQuit() {
	int i = 0;

	AIQuit();
	RBRemoveAll(&g_Families, (void(*)(void*))DestroyFamily);
	DestroyMemoryPool(g_PersonPool);
	Family_Quit();
	DestroyRBTree(g_GoodDeps);
	DestroyArray(g_AnFoodDep);
	DestroyHash(g_AIHash);
}

void GameWorldEvents(const struct KeyMouseState* _State, struct GameWorld* _World) {

	if(_State->KeyboardState == SDL_RELEASED) {
		if(_State->KeyboardButton == SDLK_a)
			g_GameWorld.MapRenderer->Screen.Center.X -= TILE_WIDTH;
		else if(_State->KeyboardButton == SDLK_d)
			g_GameWorld.MapRenderer->Screen.Center.X += TILE_WIDTH;
		else if(_State->KeyboardButton == SDLK_w)
			g_GameWorld.MapRenderer->Screen.Center.Y -= TILE_HEIGHT;
		else if(_State->KeyboardButton == SDLK_s)
			g_GameWorld.MapRenderer->Screen.Center.Y += TILE_HEIGHT;
	}
}

void GameWorldDraw(const struct KeyMouseState* _State, struct GameWorld* _World) {
	struct Tile* _Tile = ScreenToTile(g_GameWorld.MapRenderer, &_State->MousePos);

	if(_World->MapRenderer->IsRendering != 0) {
		MapRender(g_Renderer, g_GameWorld.MapRenderer);
		if(_Tile != NULL) {
			SDL_Rect _Rect = {_Tile->ScreenPos.X, _Tile->ScreenPos.Y, TILE_WIDTH, TILE_HEIGHT};

			SDL_RenderCopy(g_Renderer, g_GameWorld.MapRenderer->Selector, NULL, &_Rect);
		}
	}
}

int World_Tick() {
	struct LnkLst_Node* _Settlement = g_GameWorld.Settlements.Front;
	struct LnkLst_Node* _Itr = NULL;
	struct Person* _Person = NULL;
	struct Event* _Event = NULL;
	struct LinkedList _QueuedPeople = {0, NULL, NULL};
	int _Ticks = 1;
	int _OldMonth = MONTH(g_GameWorld.Date);
	int i;

	do {
	ATImerUpdate(&g_ATimer);
		while(_Settlement != NULL) {
			SettlementThink((struct Settlement*)_Settlement->Data);
			_Settlement = _Settlement->Next;
		}
			while((_Event = HandleEvents()) != NULL) {
			if(_Event->Type == EVENT_FARMING) {
				struct Array* _Field = g_GameWorld.Player->Person->Family->Fields;
				for(i = 0; i < _Field->Size; ++i)
					if(((struct Field*)_Field->Table[i]) == ((struct EventFarming*)_Event)->Field) {
						_Ticks = 0;
						goto escape_events;
					}
			}
		}
		GenerateMissions(g_LuaState, &g_GameWorld.BigGuyStates, &g_MissionList);
		escape_events:
		_Itr = g_ObjPos.Root;
		NextDay(&g_GameWorld.Date);
		if(MONTH(g_GameWorld.Date) != _OldMonth) {
			for(i = 0; i < g_GameWorld.MapRenderer->TileArea; ++i) {
				g_GameWorld.MapRenderer->Tiles[i]->Temperature = g_TemperatureList[MONTH(g_GameWorld.Date)];
			}
		}
		_Settlement = g_GameWorld.Settlements.Front;
		--_Ticks;
	} while(_Ticks > 0);
	LnkLstClear(&_QueuedPeople);
	return 1;
}
