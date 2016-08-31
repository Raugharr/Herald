/*
 * Author: David Brotz
 * File: World.c
 */

#include "World.h"

#include "Battle.h"
#include "Herald.h"
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
#include "Profession.h"
#include "Trait.h"
#include "Plot.h"
#include "Policy.h"

#include "video/GuiLua.h"
#include "video/Video.h"
#include "video/Tile.h"
#include "video/Sprite.h"

#include "sys/TaskPool.h"
#include "sys/Array.h"
#include "sys/Event.h"
#include "sys/Constraint.h"
#include "sys/HashTable.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"
#include "sys/MemoryPool.h"
#include "sys/Math.h"
#include "sys/RBTree.h"
#include "sys/LuaCore.h"

#include "AI/Setup.h"
#include "AI/Agent.h"

#include "Warband.h"
#include "Location.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <SDL2/SDL.h>
#include <malloc.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

struct GameWorld g_GameWorld = {
		1,
		0,
		0,
		NULL,
		{NULL, NULL, NULL, NULL, {0, 0, 0, 0}, NULL},
		NULL,
		NULL,
		{NULL, 0, NULL, NULL},
		NULL,
		{0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{NULL, 0, NULL, NULL},
		{0, NULL, NULL},
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		{0},
		0
};

static struct SubTimeObject g_SubTimeObject[SUBTIME_SIZE] = {
		{(void(*)(void*))ArmyMove, ArmyPathNext, ArmyPathPrev, NULL},
		{(void(*)(void*))BattleThink, BattleNext, BattlePrev, NULL}
};

struct GameOnClick {
	int(*OnClick)(const struct Object*, const struct Object*);
	int State;
	const void* Data;
};

static int(*g_GameOnClickFuncs[])(const struct Object*, const struct Object*) = {
		GameDefaultClick,
		GameFyrdClick
};

static struct GameOnClick g_GameOnClick = {
	NULL,
	0,
	NULL
};

struct Caste g_Castes[CASTE_SIZE];

struct TaskPool* g_TaskPool = NULL;
struct HashTable* g_AIHash = NULL;
int g_TemperatureList[] = {32, 33, 41, 46, 56, 61, 65, 65, 56, 51, 38, 32};
int g_Temperature = 0;

void GameOnClick(struct Object* _Obj) {
	int _State = g_GameOnClick.OnClick(g_GameOnClick.Data, _Obj);

	SetClickState(NULL, _State);
}

int FamilyICallback(const struct Family* _One, const struct Family* _Two) {
	return _One->Id - _Two->Id;
}

int FamilySCallback(const int* _One, const struct Family* _Two) {
	return (*_One) - _Two->Id;
}

int FamilyTypeCmp(const void* _One, const void* _Two) {
	return (((struct FamilyType*)_One)->Percent * 1000) - (((struct FamilyType*)_Two)->Percent * 1000);
}

void PlayerOnHarvest(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	//struct Person* Owner = _Extra1;
	const struct Field* _Field = _Extra2;
	
	if(_Field->Status != EFALLOW)
		return;
	MessageBox("Harvest complete.");
}

void PopulateManor(struct GameWorld* _World, struct FamilyType** _FamilyTypes,
	int _X, int _Y, struct Constraint * const *  const _AgeGroups, struct Constraint * const * const _BabyAvg) {
	int _AcresPerFarmer = 10;
	int _CropTypes = 1 ;
	const struct Crop* _Crops[_CropTypes];
	int _AnimalTypes = 2;
	const struct Population* _Animals[_AnimalTypes];
	int _AnimalTypeCt[_AnimalTypes];
	int _MaxChildren = 3;
	int _MaxFamilies = 0;
	int _MaxFarmers = 0;
	struct Family* _Parent = NULL;
	struct Settlement* _Settlement = NULL;
	double _CastePercent[CASTE_SIZE] = {0, 1.00, 0.00, 0.00};

	_Crops[0] = HashSearch(&g_Crops, "Rye");
	_Animals[0] = HashSearch(&g_Populations, "Ox");
	_AnimalTypeCt[0] = 4;
	_Animals[1] = HashSearch(&g_Populations, "Sheep");
	_AnimalTypeCt[0] = 14;
	//TODO: AgeGroups and BabyAvg should not be here but instead in an argument.
	_Settlement = CreateSettlement(_X, _Y, "Test Settlement", (GOVSTCT_TRIBAL | GOVSTCT_CHIEFDOM | GOVRULE_ELECTIVE | GOVTYPE_DEMOCRATIC | GOVTYPE_CONSENSUS));
	_MaxFarmers = _Settlement->FreeAcres / _AcresPerFarmer;
	_MaxFamilies += (_MaxFarmers / 10) + ((_MaxFarmers % 10) != 0); 
	for(;_MaxFarmers> 0; --_MaxFarmers) {
		_Parent = CreateRandFamily("Bar", Random(0, _MaxChildren) + 2, NULL, _AgeGroups, _BabyAvg, _X, _Y, _Settlement, _FamilyTypes, &g_Castes[CASTE_PEASANT]);
		_Parent->Food.SlowSpoiled = ((NUTRITION_REQ * 2) + (NUTRITION_CHILDREQ * _Parent->NumChildren)) * 2;
		//Add animals
		for(int i = 0; i < _AnimalTypeCt[0]; ++i)
			FamilyAddAnimal(_Parent, CreateAnimal(_Animals[0], Random(0, _Animals[0]->Ages[AGE_DEATH]->Max), _Animals[0]->MaxNutrition, _X, _Y));	
		//Add fields.
		for(int i = 0; i < _CropTypes; ++i) {
			if(SettlementAllocAcres(_Settlement, _AcresPerFarmer) != 0) {
				struct Good* _Good = NULL;

				_Parent->Fields[_Parent->FieldCt++] = CreateField(_X, _Y, NULL, _AcresPerFarmer, _Parent);
				_Good = CheckGoodTbl(&_Parent->Goods, _Crops[0]->Name, &g_Goods, _X, _Y); 
				_Good->Quantity = ToOunce(_Crops[0]->SeedsPerAcre) * _AcresPerFarmer;
			}
		}
		_Parent->Buildings[_Parent->BuildingCt] = CreateBuilding(ERES_HUMAN | ERES_ANIMAL, 
			HashSearch(&g_BuildMats, "Dirt"), HashSearch(&g_BuildMats, "Board"), HashSearch(&g_BuildMats, "Hay"), 500);
		RBInsert(&_World->Families, _Parent);
	}
	TribalCreateBigGuys(_Settlement, _CastePercent);
}
/*
 * TODO: This function is currently useless as instead of calling RandomsizeManorPop,
 * the caller just has to do Random(_Min, _Max). This function should be reworked to add
 * weights to all manors populations.
 */
int RandomizeManorPop(struct Constraint* const * _Constraint, int _Min, int _Max) {
	int _Index = Fuzify(_Constraint, Random(_Min, _Max));

	return Random(_Constraint[_Index]->Min, _Constraint[_Index]->Max);
}

int PopulateWorld(struct GameWorld* _World) {
	struct FamilyType** _FamilyTypes = NULL;
	struct Constraint* const * _ManorSize = NULL;
	const char* _Temp = NULL;
	int _ManorMin = 0;
	int _ManorMax = 0;
	int _ManorInterval = 0;
	int i = 0;

	if(LuaLoadFile(g_LuaState, "std.lua", NULL) != LUA_OK) {
		goto end;
	}
	lua_getglobal(g_LuaState, "ManorConstraints");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "ManorConstraints is not defined.");
		goto end;
	}
	lua_getfield(g_LuaState, -1, "Min");
	LuaGetInteger(g_LuaState, -1, &_ManorMin);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Max");
	LuaGetInteger(g_LuaState, -1, &_ManorMax);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Interval");
	LuaGetInteger(g_LuaState, -1, &_ManorInterval);
	lua_pop(g_LuaState, 2);
	_ManorSize = CreateConstrntLst(NULL, _ManorMin, _ManorMax, _ManorInterval);
	lua_getglobal(g_LuaState, "FamilyTypes");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "FamilyTypes is not defined.");
		goto end;
	}
	_FamilyTypes = alloca(lua_rawlen(g_LuaState, -1) + 1 * sizeof(struct FamilyType*));
	lua_pushnil(g_LuaState);
	while(lua_next(g_LuaState, -2) != 0) {
		_FamilyTypes[i] = alloca(sizeof(struct FamilyType));
		lua_pushnil(g_LuaState);
		lua_next(g_LuaState, -2);
		LuaGetNumber(g_LuaState, -1, &_FamilyTypes[i]->Percent);
		lua_pop(g_LuaState, 1);
		lua_next(g_LuaState, -2);
		LuaGetString(g_LuaState, -1, &_Temp);
		_FamilyTypes[i]->LuaFunc = calloc(strlen(_Temp) + 1, sizeof(char));
		strcpy(_FamilyTypes[i]->LuaFunc, _Temp);
		++i;
		lua_pop(g_LuaState, 3);
	}
	lua_pop(g_LuaState, 1);
	InsertionSort(_FamilyTypes, i, FamilyTypeCmp, sizeof(*_FamilyTypes));
	lua_getglobal(g_LuaState, "AgeGroups");
	LuaConstraintBnds(g_LuaState);
	if((g_GameWorld.AgeGroups = lua_touserdata(g_LuaState, -1)) == NULL) {
		DestroyConstrntBnds(g_GameWorld.AgeGroups);
		Log(ELOG_ERROR, "AgeGroups is not defined.");
		return 0;
	}

	lua_getglobal(g_LuaState, "BabyAvg");
	LuaConstraintBnds(g_LuaState);
	if((g_GameWorld.BabyAvg = lua_touserdata(g_LuaState, -1)) == NULL) {
		DestroyConstrntBnds(g_GameWorld.BabyAvg);
		Log(ELOG_ERROR, "BabyAvg is not defined.");
		return 0;
	}
	lua_pop(g_LuaState, 4);
	_FamilyTypes[i] = NULL;
	PopulateManor(_World, _FamilyTypes, 2,  4, g_GameWorld.AgeGroups, g_GameWorld.BabyAvg);
	g_GameWorld.Player = PickPlayer();
	//PopulateManor(_World, RandomizeManorPop(_ManorSize, _ManorMin, _ManorMax), _FamilyTypes, 4,  8);
	//g_GameWorld.Player = PickPlayer(); //Remove when the Settlement placement function is completed for settlements on the edge of the map.
	//PopulateManor(_World, RandomizeManorPop(_ManorSize, _ManorMin, _ManorMax), _FamilyTypes, 8,  4);
	DestroyConstrntBnds((struct Constraint**)_ManorSize);
	return 1;
	end:
	DestroyConstrntBnds((struct Constraint**)_ManorSize);
	return 0;
}

struct BigGuy* PickPlayer() {
	struct Settlement* _Settlement = g_GameWorld.Settlements.Front->Data;
	struct BigGuy* _Player = NULL;

	RBDelete(&g_GameWorld.Agents, _Settlement->Government->Leader);
	_Player = _Settlement->Government->Leader;
	EventHook(EVENT_FARMING, PlayerOnHarvest, _Player->Person->Family, NULL, NULL);
	return _Player;
}

int IsPlayerGovernment(const struct GameWorld* _World, const struct Settlement* _Settlement) {
	return (FamilyGetSettlement(_World->Player->Person->Family)->Government == _Settlement->Government);
}

struct WorldTile* CreateWorldTile() {
	struct WorldTile* _Tile = (struct WorldTile*) malloc(sizeof(struct WorldTile));

	_Tile->Temperature = 0;
	return _Tile;

}

void DestroyWorldTile(struct WorldTile* _Tile) {
	free(_Tile);
}

void WorldSettlementsInRadius(struct GameWorld* _World, const SDL_Point* _Point, int _Radius, struct LinkedList* _List) {
	SDL_Rect _Rect = {_Point->x - _Radius, _Point->y - _Radius, _Radius, _Radius};

	QTPointInRectangle(&_World->SettlementMap, &_Rect, (void(*)(const void*, SDL_Point*))LocationGetPoint, _List);
}

void GameworldConstructPolicies(struct GameWorld* _World) {
	int _PolicyCt = 0;
	_World->PolicySz = 10;
	_World->Policies = calloc(_World->PolicySz, sizeof(struct Policy));
	ConstructPolicy(&_World->Policies[_PolicyCt],
		"Irregular Infantry", 
		"How many and how well armed your irregular infantry are.",
		POLCAT_MILITARY);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Arms");
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Spear", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Seax", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Seax and javalin", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Spear and javalin", NULL, NULL);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Armor");
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "None", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "Gambeson", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "Partial leather armor", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "Full leather armor", NULL, NULL);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Shield");
	PolicyAddOption(&_World->Policies[_PolicyCt], 2, "None", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 2, "Buckler", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 2, "Round shield", NULL, NULL);
	++_PolicyCt;
	ConstructPolicy(&_World->Policies[_PolicyCt],
		"Regular Infantry", 
		"How many and how well armed your regular infantry are.",
		POLCAT_MILITARY);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Arms");
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Spear", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Sword", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Axe", NULL, NULL);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Armor");
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "None", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "Gambeson", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "Full leather armor", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 1, "Mail", NULL, NULL);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Shield");
	PolicyAddOption(&_World->Policies[_PolicyCt], 2, "None", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 2, "Buckler", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 2, "Round shield", NULL, NULL);
	++_PolicyCt;
	ConstructPolicy(&_World->Policies[_PolicyCt++],
		"Property Tax",
		"How much each person must pay yearly.",
		POLCAT_ECONOMY);
	ConstructPolicy(&_World->Policies[_PolicyCt++],
		"Weregeld",
		"The price of a man.",
		POLCAT_LAW);
	ConstructPolicy(&_World->Policies[_PolicyCt++],
		"Authority",
		"How much authority the ruler comands.",
		POLCAT_LAW);
	ConstructPolicy(&_World->Policies[_PolicyCt++],
		"Crop Tax",
		"Each farmer must give a percentage of their crops as tax.",
		POLCAT_ECONOMY);
	ConstructPolicy(&_World->Policies[_PolicyCt++],	
		"Work in kind",
		"Each farmer must work on the fields of the lord for a percentage of each week.",
		POLCAT_ECONOMY);
	ConstructPolicy(&_World->Policies[_PolicyCt],
		"Military Authority",
		"Determines how much control the marshall wields.",
		POLCAT_MILITARY);
	PolicyAddCategory(&_World->Policies[_PolicyCt], "Foobar");
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "None", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Some", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Full", NULL, NULL);
	_World->Policies[_PolicyCt].Options.Options[1].CastePreference[CASTE_WARRIOR] = -(POLICYMOD_NORMAL);
	_World->Policies[_PolicyCt].Options.Options[2].CastePreference[CASTE_WARRIOR] = -(POLICYMOD_NORMAL);
	++_PolicyCt;
	ConstructPolicy(&_World->Policies[_PolicyCt],
		"Judge Authority",
		"How much authoirty a judge can wield.",
		POLCAT_LAW);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Advice Only", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Double Vote", NULL, NULL);
	PolicyAddOption(&_World->Policies[_PolicyCt], 0, "Makes Decision", NULL, NULL);
	++_PolicyCt;
}

void GameWorldInit(struct GameWorld* _GameWorld, int _Area) {
	//TODO: When this data is moved to a more proper spot remove sys/video.h from the includes.
	SDL_Point _ScreenSize = {ceil(SDL_WIDTH / ((float)TILE_WIDTH)), ceil(SDL_HEIGHT / ((float)TILE_HEIGHT_THIRD))};

	_GameWorld->MapRenderer = CreateMapRenderer(_Area, &_ScreenSize);

	_GameWorld->SettlementMap.BoundingBox.w = _Area * _Area;
	_GameWorld->SettlementMap.BoundingBox.h = _Area * _Area;

	ConstructLinkedList(&_GameWorld->Settlements);
	_GameWorld->BigGuys.Table = NULL;
	_GameWorld->BigGuys.Size = 0;
	_GameWorld->BigGuys.ICallback = (int(*)(const void*, const void*)) BigGuyIdInsert;
	_GameWorld->BigGuys.SCallback = (int(*)(const void*, const void*)) BigGuyIdCmp;

	_GameWorld->BigGuyStates.Table = NULL;
	_GameWorld->BigGuyStates.Size = 0;
	_GameWorld->BigGuyStates.ICallback = (int(*)(const void*, const void*)) BigGuyStateInsert;
	_GameWorld->BigGuyStates.SCallback = (int(*)(const void*, const void*)) BigGuyMissionCmp;
	_GameWorld->Player = NULL;

	_GameWorld->Families.Table = NULL;
	_GameWorld->Families.Size = 0;
	_GameWorld->Families.ICallback = (int (*)(const void*, const void*))FamilyICallback;
	_GameWorld->Families.SCallback = (int (*)(const void*, const void*))FamilySCallback;

	_GameWorld->Agents.Table = NULL;
	_GameWorld->Agents.Size = 0;
	_GameWorld->Agents.ICallback = (int(*)(const void*, const void*))AgentICallback;
	_GameWorld->Agents.SCallback = (int(*)(const void*, const void*))AgentSCallback;

	_GameWorld->Crisis.Table = NULL;
	_GameWorld->Crisis.Size = 0;
	_GameWorld->Crisis.ICallback = (int(*)(const void*, const void*))CrisisSearch;
	_GameWorld->Crisis.SCallback = (int(*)(const void*, const void*))CrisisInsert;

	_GameWorld->ActionHistory.Table = NULL;
	_GameWorld->ActionHistory.Size = 0;
	_GameWorld->ActionHistory.ICallback = (RBCallback) BigGuyActionHistIS;
	_GameWorld->ActionHistory.SCallback = (RBCallback) BigGuyActionHistIS;

	_GameWorld->PlotList.Table = NULL;
	_GameWorld->PlotList.Size = 0;
	_GameWorld->PlotList.ICallback = (RBCallback) PlotInsert;
	_GameWorld->PlotList.SCallback = (RBCallback) PlotSearch;

	ConstructLinkedList(&_GameWorld->MissionData);
	_GameWorld->Date = 0;
	_GameWorld->Tick = 0;
	for(int i = 0; i < WORLD_DECAY; ++i)
		_GameWorld->DecayRate[i] = i * i / ((float)2000);
	GameworldConstructPolicies(_GameWorld);
}

struct FoodBase** LoadHumanFood(lua_State* _State, struct FoodBase** _FoodArray, const char* _LuaTable) {
	int _Size = 0;
	int _ArrayCt = 0;
	const char* _TblVal = NULL;
	struct FoodBase* _Food = NULL;

	lua_pushstring(_State, _LuaTable);
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE)
		return (void*) luaL_error(_State, "Table %s does not exist.", _LuaTable);
	_Size = lua_rawlen(_State, -1);
	_FoodArray = calloc(_Size + 1, sizeof(struct Food*));
	_FoodArray[_Size] = NULL;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -1) == 0) {
			if(lua_isstring(_State, -2) != 0)
				Log(ELOG_WARNING, "Key %s value in %s is not a string.", lua_tostring(_State, -2), _LuaTable);
			else
				Log(ELOG_WARNING, "Key in %s is not a string.", lua_tostring(_State, -2), _LuaTable);
			lua_pop(_State, 1);
			continue;
		}
		_TblVal = lua_tostring(_State, -1);
		if((_Food = (struct FoodBase*) HashSearch(&g_Goods, _TblVal)) == NULL) {
			Log(ELOG_WARNING, "Key %s in %s is not a good.", _TblVal, _LuaTable);
			lua_pop(_State, 1);
			continue;
		}
		_FoodArray[_ArrayCt++] = _Food;
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	return _FoodArray;
}

void LoadCaste(lua_State* _State, const char* _Name, struct Caste* _Caste) {
	struct Profession* _Profession = NULL;

	lua_pushstring(_State, _Name);
	lua_rawget(_State, -2);
	lua_pushstring(_State, "Jobs");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		return;
	}
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if((_Profession = HashSearch(&g_Professions, lua_tostring(_State, -1))) == NULL) {
			Log(ELOG_WARNING, "%s is not a profession.", lua_tostring(_State, -1));
			lua_pop(_State, 1);
			continue;
		}
		LnkLstPushBack(&_Caste->JobList, _Profession);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	lua_pushstring(_State, "Behavior");
	lua_rawget(_State, -2);
	_Caste->Behavior = LuaCheckClass(_State, -1, "Behavior");
	lua_pop(_State, 2);
}

void GoodTableLoadInputs(lua_State* _State, struct GoodBase* _Base) {
	if(HashSearch(&g_Crops, _Base->Name) != NULL)
		return;
	//Allow goods to be accessed by using their name as a key for GoodLoadInput and GoodLoadOutput to use.
	if(GoodLoadInput(_State, _Base) == 0)
		return;
	GoodLoadOutput(_State, _Base);
	Log(ELOG_INFO, "Good loaded %s.", _Base->Name);
	//Set good categories.
	LnkLstPushBack(&g_GoodCats[GoodType(_Base)], _Base);
}

void LuaTableToHash(const char* _File, const char* _Table, struct HashTable* _HashTable, void*(*_LoadFunc)(lua_State*, int), void(*_ListInsert)(struct LinkedList*, void*), size_t _NameOffset) {
	struct LinkedList _List = LinkedList(); 
	struct LnkLst_Node* _Itr = NULL;

	if(LuaLoadList(g_LuaState, _File, _Table, _LoadFunc, _ListInsert, &_List) == 0)
		return;
	_HashTable->TblSize = (_List.Size * 5) / 4;
	_HashTable->Table = (struct HashNode**) calloc(_HashTable->TblSize, sizeof(struct HashNode*));
	memset(_HashTable->Table, 0, _HashTable->TblSize * sizeof(struct HashNode*));
	_Itr = _List.Front;
	while(_Itr != NULL) {
		const char** _KeyName = (const char**)(_Itr->Data + _NameOffset);

		HashInsert(_HashTable, *_KeyName, _Itr->Data);
		_Itr = _Itr->Next;
	}
	//LISTTOHASH(&_List, _Itr, _HashTable, (_Itr->Data) + _NameOffset);
	LnkLstClear(&_List);
}

void LuaLookupTable(lua_State* _State, const char* _TableName, struct HashTable* _Table, void(*_CallFunc)(lua_State*, void*)) {
	int _Len = 0;
	struct HashItr* _Itr = NULL;

	lua_getglobal(_State, _TableName);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		Log(ELOG_WARNING, "%s is not a Lua table.", _TableName);
		return;
	}
	_Len = lua_rawlen(_State, -1);
	for(int i = 1; i <= _Len; ++i) {
		lua_rawgeti(_State, -1, i);
		if(lua_type(_State, -1) != LUA_TTABLE) {
			lua_pop(_State, 1);
			continue;
		}
		lua_pushstring(_State, "Name");
		lua_rawget(_State, -2);
		if(HashSearch(_Table, lua_tostring(_State, -1)) == NULL) {
			lua_pop(_State, 2);
			continue;
		}
		lua_pushvalue(_State, -2);
		lua_remove(_State, -3);
		lua_rawset(_State, -3);
	}

	_Itr = HashCreateItr(_Table);
	while(_Itr != NULL) {
		_CallFunc(_State, _Itr->Node->Pair);
		_Itr = HashNext(_Table, _Itr);
	}
	HashDeleteItr(_Itr);
	lua_pop(_State, 1);
}

void WorldInit(int _Area) {
	struct Array* _Array = NULL;

	Log(ELOG_INFO, "Creating World.");
	++g_Log.Indents;
	GameWorldInit(&g_GameWorld, _Area);
	g_GameOnClick.OnClick = g_GameOnClickFuncs[0];
	g_AIHash = CreateHash(32);
	chdir(DATAFLD);

	AIInit(g_LuaState);
	_Array = FileLoad("FirstNames.txt", '\n');
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), 1000000);
	Family_Init(_Array);

	if(LuaLoadFile(g_LuaState, "goods.lua", NULL) != LUA_OK)
		goto end;
	LuaTableToHash("goods.lua", "Goods", &g_Goods, (void*(*)(lua_State*, int))&GoodLoad, LnkLstPushBack, offsetof(struct GoodBase, Name));
	LuaTableToHash("traits.lua", "Traits", &g_Traits, (void*(*)(lua_State*, int))&TraitLoad, LnkLstPushBack, offsetof(struct Trait, Name));
	LuaLookupTable(g_LuaState, "Traits", &g_Traits, (void(*)(lua_State*, void*)) TraitLoadRelations);
	LuaTableToHash("crops.lua", "Crops", &g_Crops, (void*(*)(lua_State*, int))&CropLoad, LnkLstPushBack, offsetof(struct Crop, Name));
	//Fill the Goods table with mappings to each element with their name as the key to be used for GoodLoadOutput and GoodLoadInput.
	LuaLookupTable(g_LuaState, "Goods", &g_Goods, (void(*)(lua_State*, void*)) GoodTableLoadInputs);	
	LuaTableToHash("populations.lua", "Populations", &g_Populations, (void*(*)(lua_State*, int))&PopulationLoad, LnkLstPushBack, offsetof(struct Population, Name));
	LuaTableToHash("buildings.lua", "BuildMats", &g_BuildMats, (void*(*)(lua_State*, int))&BuildingLoad, (void (*)(struct LinkedList *, void *))LnkLstCatNode, offsetof(struct BuildMat, Name));
	LuaTableToHash("professions.lua", "Professions", &g_Professions, (void*(*)(lua_State*, int))&LoadProfession, LnkLstPushBack, offsetof(struct Profession, Name));

	if(LuaLoadFile(g_LuaState, "castes.lua", NULL) != LUA_OK) {
		exit(1);
	}
	lua_getglobal(g_LuaState, "Castes");
	LoadCaste(g_LuaState, "Serf", &g_Castes[CASTE_SERF]);
	LoadCaste(g_LuaState, "Peasant", &g_Castes[CASTE_PEASANT]);
	LoadCaste(g_LuaState, "Craftsman", &g_Castes[CASTE_CRAFTSMAN]);
	LoadCaste(g_LuaState, "Warrior", &g_Castes[CASTE_WARRIOR]);
	lua_pop(g_LuaState, 1);

	g_Castes[CASTE_SERF].Type = CASTE_SERF;
	g_Castes[CASTE_PEASANT].Type = CASTE_PEASANT;
	g_Castes[CASTE_CRAFTSMAN].Type = CASTE_CRAFTSMAN;
	g_Castes[CASTE_WARRIOR].Type = CASTE_WARRIOR;
	lua_getglobal(g_LuaState, "Human");
	if(lua_isnil(g_LuaState, -1) != 0) {
		Log(ELOG_WARNING, "Human table does not exist");
	} else {
		g_GameWorld.HumanEats = LoadHumanFood(g_LuaState, g_GameWorld.HumanEats, "Eats");
		g_GameWorld.HumanDrinks = LoadHumanFood(g_LuaState, g_GameWorld.HumanDrinks, "Drinks");
	}
	lua_pop(g_LuaState, 1);

	g_GameWorld.GoodDeps = GoodBuildDep(&g_Goods);
	g_GameWorld.AnFoodDeps = AnimalFoodDep(&g_Populations);
	if(PopulateWorld(&g_GameWorld) == 0)
		goto end;
	end:
	chdir("..");
	--g_Log.Indents;
}

void WorldQuit() {
	AIQuit();
	RBRemoveAll(&g_GameWorld.Families, (void(*)(void*))DestroyFamily);
	LnkLstClear(&g_GameWorld.Settlements);
	DestroyArray(g_GameWorld.AnFoodDeps);
	DestroyRBTree(g_GameWorld.GoodDeps);
	DestroyMemoryPool(g_PersonPool);
	for(int i = 0; i < GOOD_SIZE; ++i)
		LnkLstClear(&g_GoodCats[i]);
	free(g_GameWorld.Policies);
	HashDeleteAll(&g_Goods, (void(*)(void*)) DestroyGoodBase);
	HashDeleteAll(&g_Populations, (void(*)(void*)) DestroyPopulation);
	Family_Quit();
	DestroyHash(g_AIHash);
	DestroyConstrntBnds(g_GameWorld.AgeGroups);
	DestroyConstrntBnds(g_GameWorld.BabyAvg);
}

int GameDefaultClick(const struct Object* _One, const struct Object* _Two) {
	lua_settop(g_LuaState, 0);
	lua_pushstring(g_LuaState, "ViewSettlementMenu");
	lua_createtable(g_LuaState, 0, 1);
	lua_pushstring(g_LuaState, "Settlement");
	LuaCtor(g_LuaState, "Settlement", ((struct Settlement*)_Two));
	lua_rawset(g_LuaState, -3);
	lua_pushinteger(g_LuaState, 512);
	lua_pushinteger(g_LuaState, 512);
	LuaCreateWindow(g_LuaState);
	return MOUSESTATE_DEFAULT;
}

int GameFyrdClick(const struct Object* _One, const struct Object* _Two) {
	const struct Settlement* _Settlement = (const struct Settlement*) _One;

	if(_Two->Type == OBJECT_LOCATION) {
		if(GovernmentTop(_Settlement->Government) != GovernmentTop(((const struct Settlement*) _Two)->Government) && IsPlayerGovernment(&g_GameWorld, (struct Settlement*) _Two) == 0) {
			SDL_Point _Pos;
			struct ArmyGoal _Goal;

			SettlementGetCenter(_Settlement, &_Pos);
			CreateArmy(_Settlement, &_Pos, _Settlement->Government, _Settlement->Government->Leader, ArmyGoalRaid(&_Goal, ((struct Settlement*)_Two)));
			return MOUSESTATE_DEFAULT;
		}
	}
	((struct Settlement*) _Settlement)->LastRaid = g_GameWorld.Date;
	return MOUSESTATE_RAISEARMY;
}

void GameWorldEvents(const struct KeyMouseState* _State, struct GameWorld* _World) {
	if(_State->KeyboardState == SDL_RELEASED) {
		if(_State->KeyboardButton == SDLK_a)
			g_GameWorld.MapRenderer->Screen.x -= 1;
		else if(_State->KeyboardButton == SDLK_d)
			g_GameWorld.MapRenderer->Screen.x += 1;
		else if(_State->KeyboardButton == SDLK_w)
			g_GameWorld.MapRenderer->Screen.y -= 1;
		else if(_State->KeyboardButton == SDLK_s)
			g_GameWorld.MapRenderer->Screen.y += 1;
	}
	if(_State->MouseButton == SDL_BUTTON_LEFT && _State->MouseState == SDL_RELEASED) {
		struct LinkedList _List = LinkedList();
		struct Tile* _Tile = ScreenToTile(_World->MapRenderer, &_State->MousePos);

		if(_Tile == NULL)
			return;
		SDL_Rect _TileRect = {_Tile->TilePos.x, _Tile->TilePos.y, 1, 1};

		QTPointInRectangle(&g_GameWorld.SettlementMap, &_TileRect, (void(*)(const void*, SDL_Point*))LocationGetPoint, &_List);
		if(_List.Size > 0)
			GameOnClick((struct Object*)_List.Front->Data);
		LnkLstClear(&_List);
	}
}

void GameWorldDraw(const struct GameWorld* _World) {
	struct Tile* _Tile = NULL;
	struct LnkLst_Node* _Settlement = NULL;
	struct SDL_Point _Pos;

	if(_World->MapRenderer->IsRendering == 0)
		return;
	GetMousePos(&_Pos);
	_Tile = ScreenToTile(g_GameWorld.MapRenderer, &_Pos);
	_Settlement = g_GameWorld.Settlements.Front;
	MapRender(g_Renderer, g_GameWorld.MapRenderer);
	if(_Tile != NULL)
		SDL_RenderCopy(g_Renderer, g_GameWorld.MapRenderer->Selector, NULL, &_Tile->SpritePos);
	while(_Settlement != NULL) {
		SettlementDraw(g_GameWorld.MapRenderer, (struct Settlement*)_Settlement->Data);
		_Settlement = _Settlement->Next;
	}
}

int World_Tick() {
	void* _SubObj = NULL;
	void* _NextSubObj = NULL;
	struct LinkedList _QueuedPeople = LinkedList();
	int _Ticks = 1;
	int _OldMonth = MONTH(g_GameWorld.Date);
	struct RBItrStack _Stack[g_GameWorld.Agents.Size];

	do {
		for(int i = 0; i < SUBTIME_SIZE; ++i) {
			_SubObj = g_SubTimeObject[i].List;
			while(_SubObj != NULL) {
				_NextSubObj = g_SubTimeObject[i].Next(_SubObj);
				g_SubTimeObject[i].Callback(_SubObj);
				_SubObj = _NextSubObj;
			}
		}
	RBDepthFirst(g_GameWorld.Agents.Table, _Stack);
	for(int i = 0; i < g_GameWorld.Agents.Size; ++i) {
		AgentThink((struct Agent*)_Stack[i].Node->Data);
	}
	ObjectsThink();
	MissionEngineThink(&g_MissionEngine, g_LuaState, &g_GameWorld.BigGuyStates);
	NextDay(&g_GameWorld.Date);
	++g_GameWorld.Tick;
	if(MONTH(g_GameWorld.Date) != _OldMonth) {
		for(int i = 0; i < g_GameWorld.MapRenderer->TileArea; ++i) {
			g_GameWorld.MapRenderer->Tiles[i]->Temperature = g_TemperatureList[MONTH(g_GameWorld.Date)];
		}
	}
	--_Ticks;
	} while(_Ticks > 0);
	LnkLstClear(&_QueuedPeople);
	return 1;
}


void WorldPathCallback(struct Army* _Army, struct Path* _Path) {
	struct ArmyPath** _ArmyPath = (struct ArmyPath**) SubTimeGetList(SUBTIME_ARMY);

	_Army->Path.Path = *_Path;
	_Army->Path.Army = _Army;
	ILL_CREATE(*_ArmyPath, &_Army->Path);
}

void** SubTimeGetList(int _Type) {
	if(_Type < 0 || _Type > SUBTIME_SIZE)
		return NULL;
	return  &g_SubTimeObject[_Type].List;
}

void SetClickState(struct Object* _Data, int _State) {
	if(_State >= MOUSESTATE_SIZE)
		_State = 0;
	if(_State != g_GameOnClick.State) {
		g_GameOnClick.State = _State;
		g_GameOnClick.OnClick = g_GameOnClickFuncs[_State];
		g_GameOnClick.Data = _Data;
	}
}

struct Settlement* WorldGetSettlement(struct GameWorld* _World, SDL_Point* _Pos) {
		return (struct Settlement*) QTGetPoint(&_World->SettlementMap, _Pos, (void (*)(const void *, struct SDL_Point *))LocationGetPoint);
}
