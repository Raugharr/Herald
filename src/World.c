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
#include "sys/ResourceManager.h"
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

#include <stdbool.h>
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

#define LUAFILE_FAILED(File) File ".lua is empty or failed to load."

struct GameWorld g_GameWorld = {0};
static struct SubTimeObject g_SubTimeObject[SUBTIME_SIZE] = {
		{(void(*)(void*))ArmyMove, ArmyPathNext, ArmyPathPrev, NULL},
		{(void(*)(void*))BattleThink, BattleNext, BattlePrev, NULL}
};

struct GameOnClick {
	uint32_t (*OnClick)(const struct Object*, const struct Object*, uint32_t);
	uint32_t State;
	uint32_t Context;
	const void* Data;
};

static uint32_t (*g_GameOnClickFuncs[])(const struct Object*, const struct Object*, uint32_t) = {
		GameDefaultClick,
		GameFyrdClick
};

static struct GameOnClick g_GameOnClick = {0};

const char* g_CasteNames[CASTE_SIZE] = {
	"Thrall",
	"Farmer",
	"Craftsman",
	"Low noble",
	"Priest",
	"Warrior",
	"Noble"
};

struct TaskPool* g_TaskPool = NULL;
struct HashTable* g_AIHash = NULL;
int g_TemperatureList[] = {32, 33, 41, 46, 56, 61, 65, 65, 56, 51, 38, 32};
int g_Temperature = 0;

void GameOnClick(struct Object* Obj) {
	uint32_t State = g_GameOnClick.OnClick(g_GameOnClick.Data, Obj, g_GameOnClick.Context);

	SetClickState(NULL, State, 0);
}

int FamilyICallback(const struct Family* One, const struct Family* Two) {
	return One->Object.Id - Two->Object.Id;
}

int FamilySCallback(const int* One, const struct Family* Two) {
	return (*One) - Two->Object.Id;
}

int FamilyTypeCmp(const void* One, const void* Two) {
	return (((struct FamilyType*)One)->Percent * 1000) - (((struct FamilyType*)Two)->Percent * 1000);
}

void PlayerOnHarvest(const struct EventData* Data, void* Extra1, void* Extra2) {
	//struct Person* Owner = Extra1;
	const struct Field* Field = Extra2;
	
	if(Field->Status != EFALLOW)
		return;
	MessageBox("Harvest complete.");
}

void ManorSetFactions(struct Settlement* Settlement) {
	int Caste = 0;
	int TotalWeight[CASTE_SIZE] = {0};
	int Weight = 0;
	int Rand = 0;

	for(int i = 0; i < FACTION_IDSIZE; ++i) {
		if(FactionIsActive(&Settlement->Factions, i)) {
			for(int j = 0; j < CASTE_SIZE; ++j)
				TotalWeight[j] += Settlement->Factions.FactionWeight[FactionCasteIdx(i, j)];
		}
	}
	for(struct LnkLst_Node* FamItr = Settlement->Families.Front; FamItr != NULL; FamItr = FamItr->Next) {
		struct Family* Family = FamItr->Data;

		Caste = Family->Caste;
		Weight = 0;
		Rand = Random(1, TotalWeight[Caste]);
		for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
			struct BigGuy* Guy = RBSearch(&g_GameWorld.BigGuys, Family->People[i]);

			if(Family->People[i] == NULL || PersonMature(Family->People[i]) == false)
				continue;
			
			if(Family->Faction == FACTION_IDNONE) {
				for(int FactId = 0; FactId < FACTION_IDSIZE; ++FactId) {
					if(FactionIsActive(&Settlement->Factions, FactId) == false)
						continue;
					Weight += Settlement->Factions.FactionWeight[FactionCasteIdx(FactId, Caste)];
					if(Rand <= Weight) {
						Family->Faction = FactId;
						break;
					}
				}
			}
			if(Guy != NULL)
				FactionAddBoss(&Settlement->Factions, Family->Faction, Guy);
			else
				FactionAddPerson(&Settlement->Factions, Family->Faction, Family->People[i]);
		}
	}
}

void PopulateManor(struct GameWorld* World, struct FamilyType** FamilyTypes,
	int X, int Y, struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg) {
	//int Count = 0;
	struct BigGuy* Warlord = NULL;
	struct BigGuy* Chief = NULL;
	struct Settlement* Settlement = NULL;
	struct Retinue* Retinue = NULL;
	double CastePercent[CASTE_SIZE] = {0, 0.50, 0.10, 0, 0, 0.40, 0.0};
	//uint8_t CurrCaste = CASTE_THRALL;
	int* CasteCount = alloca(sizeof(int) * CASTE_SIZE);

		//TODO: AgeGroups and BabyAvg should not be here but instead in an argument.
	/*for(int i = 0; i < MaxFarmers; ++i) {
		while(CurrCaste < CASTE_SIZE && CasteCount[CurrCaste] + Count <= i) {
			Count += CasteCount[CurrCaste];
			++CurrCaste;
		}
	}*/
	Settlement = CreateSettlement(X, Y, "Test Settlement", (GOVSTCT_TRIBAL | GOVSTCT_CHIEFDOM | GOVRULE_ELECTIVE | GOVTYPE_DEMOCRATIC | GOVTYPE_CONSENSUS));
	CreateFarmerFamilies(World, Settlement, AgeGroups, BabyAvg);
	RandTable(CastePercent, &CasteCount, CASTE_SIZE, Settlement->NumPeople);
	TribalCreateBigGuys(Settlement, CastePercent);
	ManorSetFactions(Settlement);
	//i < 2 because we only want the first two factions, the nobles and peasants to be active at the start.
	for(int i = 0; i < 2; ++i) {
		if(Settlement->Factions.Bosses[i].Size == 0) {
			struct BigGuy* Temp = Settlement->Factions.Bosses[~i & 1].Table[0];

			ArrayRemove(&Settlement->Factions.Bosses[~i & 1], 0);
			ArrayInsert(&Settlement->Factions.Bosses[i], Temp);
		}
	}
	for(struct LnkLst_Node* Itr = Settlement->BigGuys.Front; Itr != NULL; Itr = Itr->Next) {
		struct BigGuy* Guy = Itr->Data;

		if(Guy->Person->Family->Caste == CASTE_WARRIOR && Guy->Person->Family->Faction == FACTION_IDNOBLE) {
			Warlord = Guy;
			Warlord->Glory = BGRandRes(Warlord, BGSKILL_COMBAT) / 10;
			//goto found_warlord;
		}
		if(Chief == NULL && Guy->Person->Family->Caste == CASTE_NOBLE) {
			Chief = Guy;
		}
	}
	//Warlord = Settlement->Government->Warlord;
	//found_warlord:
	if(Chief == NULL) {
		struct BigGuy* Guy = NULL;
		for(int i = 0; i < Settlement->Factions.Bosses[FACTION_IDPEASANT].Size; ++i) {
			Guy = Settlement->Factions.Bosses[FACTION_IDPEASANT].Table[i];

			if(Guy->Person->Family->Caste == CASTE_FARMER) {
				Guy->Person->Family->Caste = CASTE_NOBLE;
				Chief = Guy;
				goto found_chief;
			}
		}
		Guy = Settlement->Factions.Bosses[FACTION_IDPEASANT].Table[0];
		Guy->Person->Family->Caste = CASTE_NOBLE;
		Chief = Guy;
	}
	found_chief:
	if(Warlord == NULL) {
		struct BigGuy* Guy = NULL;
		for(int i = 0; i < Settlement->Factions.Bosses[FACTION_IDNOBLE].Size; ++i) {
			Guy = Settlement->Factions.Bosses[FACTION_IDNOBLE].Table[i];
			if(Guy->Person->Family->Caste == CASTE_FARMER) {
				Guy->Person->Family->Caste = CASTE_WARRIOR;
				Warlord = Guy;
				goto found_warlord;
			}
		}
		Guy = Settlement->Factions.Bosses[FACTION_IDNOBLE].Table[0];
		Guy->Person->Family->Caste = CASTE_WARRIOR;
		Warlord= Guy;
	}
	found_warlord:
	Retinue = SettlementAddRetinue(Settlement, Warlord); 
	for(struct LnkLst_Node* Itr = Settlement->Families.Front; Itr != NULL; Itr = Itr->Next) {
		struct Family* Family = Itr->Data;

		if(Family->Caste == CASTE_WARRIOR && Family != Warlord->Person->Family)
			RetinueAddWarrior(Retinue, Family->People[0]);
	}
	GovernmentSetLeader(Settlement->Government, Chief);
	Settlement->Factions.Leader[FACTION_IDNOBLE] = Warlord;
	Settlement->Factions.Leader[FACTION_IDPEASANT] = Chief;
	AssertPtrNeq(Warlord, NULL);
	AssertPtrNeq(Chief, NULL);
#ifdef DEBUG
	uint32_t SettlementSz = 0;

	for(struct LnkLst_Node* Itr = Settlement->Families.Front; Itr != NULL; Itr = Itr->Next) {
		struct Family* Family = Itr->Data;

		SettlementSz += FamilySize(Family);
	}
	Assert(Settlement->NumPeople == SettlementSz);
#endif
}
/*
 * TODO: This function is currently useless as instead of calling RandomsizeManorPop,
 * the caller just has to do Random(Min, Max). This function should be reworked to add
 * weights to all manors populations.
 */
int RandomizeManorPop(struct Constraint* const * Constraint, int Min, int Max) {
	int Index = Fuzify(Constraint, Random(Min, Max));

	return Random(Constraint[Index]->Min, Constraint[Index]->Max);
}

int FilterGameEvents(void* None, SDL_Event* Event) {
	if(Event->type >= EventUserOffset())
		return 0;
	return 1;
}

int PopulateWorld(struct GameWorld* World) {
	struct FamilyType** FamilyTypes = NULL;
	struct Constraint* const * ManorSize = NULL;
	const char* Temp = NULL;
	int ManorMin = 0;
	int ManorMax = 0;
	int ManorInterval = 0;
	int Idx = 0;

	if(LuaLoadFile(g_LuaState, "std.lua", NULL) != LUA_OK) {
		goto end;
	}

	g_PolicyFuncSz = CountPolicyFuncs(g_PolicyFuncs);
	InsertionSortPtr((void**)g_PolicyFuncs, g_PolicyFuncSz, PolicyFuncCmp);
	if(LuaLoadFile(g_LuaState, "policies.lua", NULL) != LUA_OK) {
		goto end;
	}
	lua_getglobal(g_LuaState, "ManorConstraints");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "ManorConstraints is not defined.");
		goto end;
	}
	lua_getfield(g_LuaState, -1, "Min");
	LuaGetInteger(g_LuaState, -1, &ManorMin);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Max");
	LuaGetInteger(g_LuaState, -1, &ManorMax);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Interval");
	LuaGetInteger(g_LuaState, -1, &ManorInterval);
	lua_pop(g_LuaState, 2);
	ManorSize = CreateConstrntLst(NULL, ManorMin, ManorMax, ManorInterval);
	lua_getglobal(g_LuaState, "FamilyTypes");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "FamilyTypes is not defined.");
		goto end;
	}
	FamilyTypes = alloca(lua_rawlen(g_LuaState, -1) + 1 * sizeof(struct FamilyType*));
	lua_pushnil(g_LuaState);
	while(lua_next(g_LuaState, -2) != 0) {
		FamilyTypes[Idx] = alloca(sizeof(struct FamilyType));
		lua_pushnil(g_LuaState);
		lua_next(g_LuaState, -2);
		LuaGetNumber(g_LuaState, -1, &FamilyTypes[Idx]->Percent);
		lua_pop(g_LuaState, 1);
		lua_next(g_LuaState, -2);
		LuaGetString(g_LuaState, -1, &Temp);
		FamilyTypes[Idx]->LuaFunc = calloc(strlen(Temp) + 1, sizeof(char));
		strcpy(FamilyTypes[Idx]->LuaFunc, Temp);
		++Idx;
		lua_pop(g_LuaState, 3);
	}
	lua_pop(g_LuaState, 1);
	InsertionSort(FamilyTypes, Idx, FamilyTypeCmp, sizeof(*FamilyTypes));
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
	FamilyTypes[Idx] = NULL;
	SDL_SetEventFilter(FilterGameEvents, NULL);
	PopulateManor(World, FamilyTypes, 4,  6, g_GameWorld.AgeGroups, g_GameWorld.BabyAvg);
	g_GameWorld.Player = PickPlayer();
	PopulateManor(World, FamilyTypes, 12, 12, g_GameWorld.AgeGroups, g_GameWorld.BabyAvg);
	PopulateManor(World, FamilyTypes, 16, 8, g_GameWorld.AgeGroups, g_GameWorld.BabyAvg);
	//PopulateManor(World, FamilyTypes, 10, 4, g_GameWorld.AgeGroups, g_GameWorld.BabyAvg);
	SDL_SetEventFilter(NULL, NULL);
	//g_GameWorld.Player = PickPlayer(); //Remove when the Settlement placement function is completed for settlements on the edge of the map.
	//PopulateManor(World, RandomizeManorPop(ManorSize, ManorMin, ManorMax), FamilyTypes, 8,  4);
	DestroyConstrntBnds((struct Constraint**)ManorSize);
	return 1;
	end:
	DestroyConstrntBnds((struct Constraint**)ManorSize);
	return 0;
}

struct BigGuy* PickPlayer() {
	struct Settlement* Settlement = g_GameWorld.Settlements.Front->Data;
	struct BigGuy* Player = NULL;
	struct Agent* Agent = NULL;

	Player = Settlement->Government->Leader;
	Agent = RBSearch(&g_GameWorld.Agents, Player);
	RBDelete(&g_GameWorld.Agents, Player);
	DestroyAgent(Agent);
	Player->Agent = NULL;
	EventHook(EVENT_FARMING, PlayerOnHarvest, Player->Person->Family, NULL, NULL);
	return Player;
}

int IsPlayerGovernment(const struct GameWorld* World, const struct Settlement* Settlement) {
	return (FamilyGetSettlement(World->Player->Person->Family)->Government == Settlement->Government);
}


void WorldSettlementsInRadius(struct GameWorld* World, const SDL_Point* Point, int Radius, struct LinkedList* List) {
	SDL_Rect Rect = {Point->x - Radius, Point->y - Radius, Radius, Radius};

	QTPointInRectangle(&World->SettlementMap, &Rect, (void(*)(const void*, SDL_Point*))LocationGetPoint, List);
}

void GameworldConstructPolicies(struct GameWorld* World) {
	CtorArray(&World->Policies, 64);
	/*int PolicyCt = 0;
	World->PolicySz = 10;
	World->Policies = calloc(World->PolicySz, sizeof(struct Policy));
	CtorPolicy(&World->Policies[PolicyCt],
		"Irregular Infantry", 
		"How many and how well armed your irregular infantry are.",
		POLCAT_MILITARY);
	PolicyAddCategory(&World->Policies[PolicyCt], "Arms");
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Spear", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Seax", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Seax and javalin", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Spear and javalin", NULL, NULL);
	PolicyAddCategory(&World->Policies[PolicyCt], "Armor");
	PolicyAddOption(&World->Policies[PolicyCt], 1, "None", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 1, "Gambeson", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 1, "Partial leather armor", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 1, "Full leather armor", NULL, NULL);
	PolicyAddCategory(&World->Policies[PolicyCt], "Shield");
	PolicyAddOption(&World->Policies[PolicyCt], 2, "None", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 2, "Buckler", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 2, "Round shield", NULL, NULL);
	++PolicyCt;
	CtorPolicy(&World->Policies[PolicyCt],
		"Regular Infantry", 
		"How many and how well armed your regular infantry are.",
		POLCAT_MILITARY);
	PolicyAddCategory(&World->Policies[PolicyCt], "Arms");
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Spear", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Sword", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Axe", NULL, NULL);
	PolicyAddCategory(&World->Policies[PolicyCt], "Armor");
	PolicyAddOption(&World->Policies[PolicyCt], 1, "None", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 1, "Gambeson", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 1, "Full leather armor", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 1, "Mail", NULL, NULL);
	PolicyAddCategory(&World->Policies[PolicyCt], "Shield");
	PolicyAddOption(&World->Policies[PolicyCt], 2, "None", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 2, "Buckler", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 2, "Round shield", NULL, NULL);
	++PolicyCt;
	CtorPolicy(&World->Policies[PolicyCt++],
		"Property Tax",
		"How much each person must pay yearly.",
		POLCAT_ECONOMY);
	CtorPolicy(&World->Policies[PolicyCt++],
		"Weregeld",
		"The price of a man.",
		POLCAT_LAW);
	CtorPolicy(&World->Policies[PolicyCt++],
		"Authority",
		"How much authority the ruler comands.",
		POLCAT_LAW);
	CtorPolicy(&World->Policies[PolicyCt++],
		"Crop Tax",
		"Each farmer must give a percentage of their crops as tax.",
		POLCAT_ECONOMY);
	CtorPolicy(&World->Policies[PolicyCt++],	
		"Work in kind",
		"Each farmer must work on the fields of the lord for a percentage of each week.",
		POLCAT_ECONOMY);
	CtorPolicy(&World->Policies[PolicyCt],
		"Military Authority",
		"Determines how much control the marshall wields.",
		POLCAT_MILITARY);
	PolicyAddCategory(&World->Policies[PolicyCt], "Foobar");
	PolicyAddOption(&World->Policies[PolicyCt], 0, "None", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Some", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Full", NULL, NULL);
	World->Policies[PolicyCt].Options.Options[1].CastePreference[CASTE_NOBLE] = -(POLICYMOD_NORMAL);
	World->Policies[PolicyCt].Options.Options[2].CastePreference[CASTE_NOBLE] = -(POLICYMOD_NORMAL);
	++PolicyCt;
	CtorPolicy(&World->Policies[PolicyCt],
		"Judge Authority",
		"How much authoirty a judge can wield.",
		POLCAT_LAW);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Advice Only", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Double Vote", NULL, NULL);
	PolicyAddOption(&World->Policies[PolicyCt], 0, "Makes Decision", NULL, NULL);
	++PolicyCt;*/
}

void GameWorldInit(struct GameWorld* GameWorld, int Area) {
	//TODO: When this data is moved to a more proper spot remove sys/video.h from the includes.
	SDL_Point ScreenSize = {ceil(SDL_WIDTH / ((float)TILE_WIDTH)), ceil(SDL_HEIGHT / ((float)TILE_HEIGHT_THIRD))};

	GameWorld->IsPaused = 1;
	GameWorld->MapRenderer = CreateMapRenderer(Area, &ScreenSize);

	GameWorld->SettlementMap.BoundingBox.w = Area * Area;
	GameWorld->SettlementMap.BoundingBox.h = Area * Area;

	ConstructLinkedList(&GameWorld->Settlements);
	GameWorld->BigGuys.Table = NULL;
	GameWorld->BigGuys.Size = 0;
	GameWorld->BigGuys.ICallback = (RBCallback) BigGuyIdInsert;
	GameWorld->BigGuys.SCallback = (RBCallback) BigGuyIdCmp;

	GameWorld->Player = NULL;

	GameWorld->Families.Table = NULL;
	GameWorld->Families.Size = 0;
	GameWorld->Families.ICallback = (RBCallback) FamilyICallback;
	GameWorld->Families.SCallback = (RBCallback) FamilySCallback;

	GameWorld->PersonRetinue.Table = NULL;
	GameWorld->PersonRetinue.Size = 0;

	GameWorld->Agents.Table = NULL;
	GameWorld->Agents.Size = 0;
	GameWorld->Agents.ICallback = (RBCallback) AgentICallback;
	GameWorld->Agents.SCallback = (RBCallback) AgentSCallback;

	GameWorld->ActionHistory.Table = NULL;
	GameWorld->ActionHistory.Size = 0;
	//_GameWorld->ActionHistory.ICallback = (RBCallback) BigGuyActionHistIS;
	//_GameWorld->ActionHistory.SCallback = (RBCallback) BigGuyActionHistIS;

	GameWorld->PlotList.Table = NULL;
	GameWorld->PlotList.Size = 0;
	GameWorld->PlotList.ICallback = (RBCallback) PlotInsert;
	GameWorld->PlotList.SCallback = (RBCallback) PlotSearch;

	ConstructLinkedList(&GameWorld->MissionFrame);
	GameWorld->Date = 0;
	GameWorld->Tick = 0;
	for(int i = 0; i < WORLD_DECAY; ++i)
		GameWorld->DecayRate[i] = i * i / ((float)2000);
	GameworldConstructPolicies(GameWorld);
}

struct FoodBase** LoadHumanFood(lua_State* State, struct FoodBase** FoodArray, const char* LuaTable) {
	int Size = 0;
	int ArrayCt = 0;
	const char* TblVal = NULL;
	struct FoodBase* Food = NULL;

	lua_pushstring(State, LuaTable);
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TTABLE)
		return (void*) luaL_error(State, "Table %s does not exist.", LuaTable);
	Size = lua_rawlen(State, -1);
	FoodArray = calloc(Size + 1, sizeof(struct Food*));
	FoodArray[Size] = NULL;
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_isstring(State, -1) == 0) {
			if(lua_isstring(State, -2) != 0)
				Log(ELOG_WARNING, "Key %s value in %s is not a string.", lua_tostring(State, -2), LuaTable);
			else
				Log(ELOG_WARNING, "Key in %s is not a string.", lua_tostring(State, -2), LuaTable);
			lua_pop(State, 1);
			continue;
		}
		TblVal = lua_tostring(State, -1);
		if((Food = (struct FoodBase*) HashSearch(&g_Goods, TblVal)) == NULL) {
			Log(ELOG_WARNING, "Key %s in %s is not a good.", TblVal, LuaTable);
			lua_pop(State, 1);
			continue;
		}
		FoodArray[ArrayCt++] = Food;
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
	return FoodArray;
}

void GoodTableLoadInputs(lua_State* State, struct GoodBase* Base) {
	if(HashSearch(&g_Crops, Base->Name) != NULL)
		return;
	//Allow goods to be accessed by using their name as a key for GoodLoadInput and GoodLoadOutput to use.
	if(GoodLoadInput(State, Base) == 0)
		return;
	GoodLoadOutput(State, Base);
	Log(ELOG_INFO, "Good loaded %s.", Base->Name);
	//Set good categories.
	LnkLstPushBack(&g_GoodCats[GoodType(Base)], Base);
}

bool LuaTableToHash(const char* File, const char* Table, struct HashTable* HashTable, void*(*LoadFunc)(lua_State*, int), void(*ListInsert)(struct LinkedList*, void*), size_t NameOffset) {
	struct LinkedList List = LinkedList(); 
	struct LnkLst_Node* Itr = NULL;

	if(LuaLoadList(g_LuaState, File, Table, LoadFunc, ListInsert, &List) == 0) {
		Log(ELOG_ERROR, "Unable to load table %s from file %s.", Table, File);
		return false;
	}
	if(List.Size < 20)
		HashTable->TblSize = 20;
	else
		HashTable->TblSize = (List.Size * 5) / 4;
	HashTable->Table = (struct HashNode**) calloc(HashTable->TblSize, sizeof(struct HashNode*));
	memset(HashTable->Table, 0, HashTable->TblSize * sizeof(struct HashNode*));
	if(List.Size == 0)
		return false;
	Itr = List.Front;
	while(Itr != NULL) {
		const char** KeyName = (const char**)(Itr->Data + NameOffset);

		HashInsert(HashTable, *KeyName, Itr->Data);
		Itr = Itr->Next;
	}
	//LISTTOHASH(&List, Itr, HashTable, (Itr->Data) + NameOffset);
	LnkLstClear(&List);
	return true;
}

void LuaLookupTable(lua_State* State, const char* TableName, struct HashTable* Table, void(*CallFunc)(lua_State*, void*)) {
	int Len = 0;
	struct HashItr* Itr = NULL;

	lua_getglobal(State, TableName);
	if(lua_type(State, -1) != LUA_TTABLE) {
		Log(ELOG_WARNING, "%s is not a Lua table.", TableName);
		return;
	}
	Len = lua_rawlen(State, -1);
	for(int i = 1; i <= Len; ++i) {
		lua_rawgeti(State, -1, i);
		if(lua_type(State, -1) != LUA_TTABLE) {
			lua_pop(State, 1);
			continue;
		}
		lua_pushstring(State, "Name");
		lua_rawget(State, -2);
		if(HashSearch(Table, lua_tostring(State, -1)) == NULL) {
			lua_pop(State, 2);
			continue;
		}
		lua_pushvalue(State, -2);
		lua_remove(State, -3);
		lua_rawset(State, -3);
	}

	Itr = HashCreateItr(Table);
	while(Itr != NULL) {
		CallFunc(State, Itr->Node->Pair);
		Itr = HashNext(Table, Itr);
	}
	HashDeleteItr(Itr);
	lua_pop(State, 1);
}

void WorldInit(int Area) {
	struct Array* Array = NULL;

	Log(ELOG_INFO, "Creating World.");
	++g_Log.Indents;
	GameWorldInit(&g_GameWorld, Area);
	g_GameOnClick.OnClick = g_GameOnClickFuncs[0];
	g_AIHash = CreateHash(32);
	chdir(DATAFLD);

	AIInit(g_LuaState);
	Array = FileLoad("FirstNames.txt", '\n');
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), 1000000);
	Family_Init(Array);

	if(LuaLoadFile(g_LuaState, "goods.lua", NULL) != LUA_OK)
		goto end;
	if(LuaTableToHash("goods.lua", "Goods", &g_Goods, (void*(*)(lua_State*, int))&GoodLoad, LnkLstPushBack, offsetof(struct GoodBase, Name)) == false) {
		Log(ELOG_ERROR, "goods");
		exit(1);
	}
	if(LuaTableToHash("traits.lua", "Traits", &g_Traits, (void*(*)(lua_State*, int))&TraitLoad, LnkLstPushBack, offsetof(struct Trait, Name)) == false) {
		Log(ELOG_ERROR, LUAFILE_FAILED("traits"));
		exit(1);
	}
	LuaLookupTable(g_LuaState, "Traits", &g_Traits, (void(*)(lua_State*, void*)) TraitLoadRelations);
	if(LuaTableToHash("crops.lua", "Crops", &g_Crops, (void*(*)(lua_State*, int))&CropLoad, LnkLstPushBack, offsetof(struct Crop, Name)) == false) {
		Log(ELOG_ERROR, LUAFILE_FAILED("crops"));
		exit(1);
	}
	//Fill the Goods table with mappings to each element with their name as the key to be used for GoodLoadOutput and GoodLoadInput.
	LuaLookupTable(g_LuaState, "Goods", &g_Goods, (void(*)(lua_State*, void*)) GoodTableLoadInputs);	
	LuaTableToHash("populations.lua", "Populations", &g_Populations, (void*(*)(lua_State*, int))&PopulationLoad, LnkLstPushBack, offsetof(struct Population, Name));
	LuaTableToHash("buildings.lua", "BuildMats", &g_BuildMats, (void*(*)(lua_State*, int))&BuildingLoad, (void (*)(struct LinkedList *, void *))LnkLstCatNode, offsetof(struct BuildMat, Name));
	LuaTableToHash("professions.lua", "Professions", &g_Professions, (void*(*)(lua_State*, int))&LoadProfession, LnkLstPushBack, offsetof(struct Profession, Name));

	if(LuaLoadFile(g_LuaState, "castes.lua", NULL) != LUA_OK) {
		exit(1);
	}
	lua_getglobal(g_LuaState, "Castes");
	lua_pop(g_LuaState, 1);
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
	if(PopulateWorld(&g_GameWorld) == 0) {
		Log(ELOG_ERROR, "Cannot populate world!");
		exit(1);
	}
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
	DtorArray(&g_GameWorld.Policies);
	HashDeleteAll(&g_Goods, (void(*)(void*)) DestroyGoodBase);
	HashDeleteAll(&g_Populations, (void(*)(void*)) DestroyPopulation);
	Family_Quit();
	DestroyHash(g_AIHash);
	DestroyConstrntBnds(g_GameWorld.AgeGroups);
	DestroyConstrntBnds(g_GameWorld.BabyAvg);
}

uint32_t GameDefaultClick(const struct Object* One, const struct Object* Two, uint32_t Context) {
	lua_settop(g_LuaState, 0);
	lua_pushstring(g_LuaState, "ViewSettlementMenu");
	lua_createtable(g_LuaState, 0, 1);
	lua_pushstring(g_LuaState, "Settlement");
	LuaCtor(g_LuaState, ((struct Settlement*)Two), LOBJ_SETTLEMENT);
	lua_rawset(g_LuaState, -3);
	LuaCreateWindow(g_LuaState);
	return WORLDACT_DEFAULT;
}

uint32_t GameFyrdClick(const struct Object* One, const struct Object* Two, uint32_t Context) {
	const struct Settlement* Settlement = (const struct Settlement*) One;

	if(Two->Type == OBJECT_LOCATION) {
		if(GovernmentTop(Settlement->Government) != GovernmentTop(((const struct Settlement*) Two)->Government) && IsPlayerGovernment(&g_GameWorld, (struct Settlement*) Two) == 0) {
			struct ArmyGoal Goal;

			CreateArmy(Settlement, Settlement->Government->Leader, ArmyGoalRaid(&Goal, ((struct Settlement*)Two), Context));
			return WORLDACT_DEFAULT;
		}
	}
	((struct Settlement*) Settlement)->LastRaid = g_GameWorld.Date;
	return WORLDACT_RAISEARMY;
}

void GameWorldEvents(const struct KeyMouseState* State, struct GameWorld* World) {
	if(State->KeyboardState == SDL_PRESSED) {
		if(State->KeyboardButton == SDLK_a && World->MapRenderer->Screen.x > 0) {
			World->MapRenderer->Screen.x -= 1;
		} else if(State->KeyboardButton == SDLK_d && World->MapRenderer->Screen.x < World->MapRenderer->TileLength) {
			World->MapRenderer->Screen.x += 1;
		} else if(State->KeyboardButton == SDLK_w && World->MapRenderer->Screen.y > 0) {
			World->MapRenderer->Screen.y -= 1;
		} else if(State->KeyboardButton == SDLK_s && World->MapRenderer->Screen.y + World->MapRenderer->Screen.h < World->MapRenderer->TileLength) {
			World->MapRenderer->Screen.y += 1;
		}
	}
	if(State->MouseButton == SDL_BUTTON_LEFT && State->MouseState == SDL_RELEASED) {
		struct LinkedList List = LinkedList();
		SDL_Point TilePos;
		struct Tile* Tile = NULL;

		ScreenToHex(&State->MousePos, &TilePos);
		Tile = MapGetTile(World->MapRenderer, &TilePos);
		if(Tile == NULL)
			return;
		SDL_Rect TileRect = {TilePos.x, TilePos.y, 1, 1};

		QTPointInRectangle(&World->SettlementMap, &TileRect, (void(*)(const void*, SDL_Point*))LocationGetPoint, &List);
		if(List.Size > 0)
			GameOnClick((struct Object*)List.Front->Data);
		LnkLstClear(&List);
	}
}

void GameWorldDraw(const struct GameWorld* World) {
	struct Tile* Tile = NULL;
	struct LnkLst_Node* Settlement = NULL;
	struct SDL_Point Pos;
	struct SDL_Point TilePos;

	if(World->MapRenderer->IsRendering == 0)
		return;
	GetMousePos(&Pos);
	ScreenToHex(&Pos, &TilePos);
	Tile = &World->MapRenderer->Tiles[TilePos.y * World->MapRenderer->TileLength + TilePos.x];
	Settlement = g_GameWorld.Settlements.Front;
	MapRenderAll(g_Renderer, g_GameWorld.MapRenderer);
	if(Tile != NULL) {
		SDL_Rect SpritePos = {TilePos.x * TILE_WIDTH, TilePos.y * TILE_HEIGHT_THIRD, TILE_WIDTH, TILE_HEIGHT};
		
		if((TilePos.y & 1) == 1)
			SpritePos.x += (TILE_WIDTH / 2);
		SDL_RenderCopy(g_Renderer, ResourceGetData(g_GameWorld.MapRenderer->Selector), NULL, &SpritePos);
	}
	while(Settlement != NULL) {
		SettlementDraw(g_GameWorld.MapRenderer, (struct Settlement*)Settlement->Data);
		Settlement = Settlement->Next;
	}
}

int World_Tick() {
	void* SubObj = NULL;
	void* NextSubObj = NULL;
	struct LinkedList QueuedPeople = LinkedList();
	int Ticks = 1;
	int OldMonth = MONTH(g_GameWorld.Date);
	struct RBItrStack Stack[g_GameWorld.Agents.Size];

	do {
		MissionEngineThink(&g_MissionEngine, g_LuaState, &g_GameWorld.BigGuys);
		RBDepthFirst(g_GameWorld.Agents.Table, Stack);
		for(int i = 0; i < g_GameWorld.Agents.Size; ++i) {
			AgentThink((struct Agent*)Stack[i].Node->Data);
		}
		for(int i = 0; i < SUBTIME_SIZE; ++i) {
			SubObj = g_SubTimeObject[i].List;
			while(SubObj != NULL) {
				NextSubObj = g_SubTimeObject[i].Next(SubObj);
				g_SubTimeObject[i].Callback(SubObj);
				SubObj = NextSubObj;
			}
		}
		ObjectsThink();
		NextDay(&g_GameWorld.Date);
		++g_GameWorld.Tick;
		if(MONTH(g_GameWorld.Date) != OldMonth) {
			for(int i = 0; i < g_GameWorld.MapRenderer->TileArea; ++i) {
				g_GameWorld.MapRenderer->Tiles[i].Temperature = g_TemperatureList[MONTH(g_GameWorld.Date)];
			}
		}
		--Ticks;
	} while(Ticks > 0);
	LnkLstClear(&QueuedPeople);
	return 1;
}


void WorldPathCallback(struct Army* Army, struct Path* Path) {
	struct ArmyPath** ArmyPath = (struct ArmyPath**) SubTimeGetList(SUBTIME_ARMY);

	Army->Path.Path = *Path;
	Army->Path.Army = Army;
	ILL_CREATE(*ArmyPath, &Army->Path);
}

void** SubTimeGetList(int Type) {
	if(Type < 0 || Type > SUBTIME_SIZE)
		return NULL;
	return  &g_SubTimeObject[Type].List;
}

void SetClickState(struct Object* Data, uint32_t State, uint32_t Context) {
	if(State >= WORLDACT_SIZE)
		State = 0;
	if(State != g_GameOnClick.State) {
		g_GameOnClick.State = State;
		g_GameOnClick.OnClick = g_GameOnClickFuncs[State];
		g_GameOnClick.Data = Data;
	}
}

struct Settlement* WorldGetSettlement(struct GameWorld* World, SDL_Point* Pos) {
		return (struct Settlement*) QTGetPoint(&World->SettlementMap, Pos, (void (*)(const void *, struct SDL_Point *))LocationGetPoint);
}

uint8_t WorldGetPolicyId(const struct Policy* Policy) {
	for(int i = 0; i < g_GameWorld.Policies.Size; ++i) {
		if(g_GameWorld.Policies.Table[i] == Policy)
			return i;
	}
	return 0;
}
