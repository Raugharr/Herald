/*
 * File: World.c
 * Author: David Brotz
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
#include "Culture.h"
#include "Grammar.h"

#include "video/Gui.h"
#include "video/GuiLua.h"
#include "video/Video.h"
#include "video/Tile.h"
#include "video/Sprite.h"
#include "video/QuadTree.h"
#include "video/MapGenerator.h"

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
static struct Crop* g_PastureCrop = NULL;

const char* g_CasteNames[CASTE_SIZE] = {
	"Theow",
	"Gebur",
	"Geneat",
	"Thegn"
	"Eldorman",
};

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

void LoadWarriorRoles(struct WarRoleRule** List) {
	static const struct WarRoleRule LightInfRule[] = {
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SPEAR, 255},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_SHIELD, 255},
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SEAX, 10},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_BODY, 2},
		{RULE_NONE, 0, 0, 0, 0}
	};

	static const struct WarRoleRule HevInfRule[] = {
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SPEAR, 255},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_SHIELD, 255},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_BODY, 127},
		{RULE_GREATERTHAN, 1, GOOD_WEAPON, ARMS_JAVELIN, 64},
		{RULE_LESSTHAN, 1, GOOD_WEAPON, ARMS_JAVELIN, 64},
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SEAX, 64},
		{RULE_NONE, 0, 0, 0, 0}
	};

	static const struct WarRoleRule SkirmisherRule[] = {
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SPEAR, 255},
		{RULE_LESSTHAN, 3, GOOD_WEAPON, ARMS_JAVELIN, 255},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_SHIELD, 200},
		{RULE_NONE, 0, 0, 0, 0}
	};

	static const struct WarRoleRule RangeRule[] = {
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_BOW, 255},
		{RULE_GREATERTHAN, 10, GOOD_WEAPON, ARMS_ARROW, 255},
		{RULE_LESSTHAN, 30, GOOD_WEAPON, ARMS_ARROW, 255},
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SEAX, 127},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_SHIELD, 64},
		{RULE_NONE, 0, 0, 0, 0}
	};
	static const struct WarRoleRule CavRule[] = {
		{RULE_EQUAL, 1, GOOD_WEAPON, ARMS_SPEAR, 255},
		{RULE_EQUAL, 1, GOOD_ARMOR, ARMS_SHIELD, 200},
		{RULE_LESSTHAN, 3, GOOD_WEAPON, ARMS_JAVELIN, 127},
		{RULE_NONE, 0, 0, 0, 0}
	};
	struct WarRoleRule* Copy = malloc(sizeof LightInfRule + sizeof HevInfRule + sizeof SkirmisherRule + sizeof RangeRule + sizeof CavRule);
	struct WarRoleRule* Mv = Copy;
	
	memcpy(Mv, LightInfRule, sizeof LightInfRule);
	List[WARROLE_LIGHTINF] = Mv;

	Mv = (void*) ((intptr_t) Mv) + ((intptr_t) sizeof LightInfRule);
	memcpy(Mv, HevInfRule, sizeof HevInfRule);
	List[WARROLE_HEAVYINF] = Mv;

	Mv = (void*) ((intptr_t) Mv) + ((intptr_t) sizeof HevInfRule);
	memcpy(Mv, SkirmisherRule, sizeof SkirmisherRule);
	List[WARROLE_SKIRMISHER] = Mv;

	Mv = (void*) ((intptr_t) Mv) + ((intptr_t) sizeof SkirmisherRule);
	memcpy(Mv, RangeRule, sizeof RangeRule);
	List[WARROLE_RANGE] = Mv;

	Mv = (void*) ((intptr_t) Mv) + ((intptr_t) sizeof RangeRule);
	memcpy(Mv, CavRule, sizeof CavRule);
	List[WARROLE_CALVARY] = Mv;
}

void FreeWarriors(struct WarRoleRule** List) {
	free(List[0]);
}

void ManorSetFactions(struct GameWorld* World, struct Settlement* Settlement) {
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
	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];

		Caste = Family->Caste;
		Weight = 0;
		Rand = Random(1, TotalWeight[Caste]);
		for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
			struct BigGuy* Guy = RBSearch(&World->BigGuys, Family->People[i]);

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

void PopulateManor(struct GameWorld* World, struct Settlement* Settlement, int Families, struct FamilyType** FamilyTypes,
	struct Constraint * const *  const AgeGroups, struct Constraint * const * const BabyAvg) {
	//int Count = 0;
	struct BigGuy* Warlord = NULL;
	struct BigGuy* Chief = NULL;
	struct Retinue* Retinue = NULL;
	double Gurbur = 1.0 / Isqrt(Families) * 2;
	double Geneat = 1.0 - Gurbur;
	double CastePercent[CASTE_SIZE] = {0, Gurbur * 0.95, Geneat * 0.95, 0.05, 0};
	double FarmerCastes[CASTE_SIZE] = {CastePercent[0] * 1.0, CastePercent[1] * 0.70, CastePercent[2] * 0.30, 0, 0};
	double CrafterCastes[CASTE_SIZE] = {0, CastePercent[1] * 0.30, CastePercent[2] * 0.70, 0, 0};
	int* CasteCount = alloca(sizeof(int) * CASTE_SIZE);
	int BestGlory = -1;
	int Failures = 0;

	Failures = CreateFarmerFamilies(Settlement, Families - 1, FarmerCastes, AgeGroups, BabyAvg);
	Failures += CreateCrafterFamilies(Settlement, 1, CrafterCastes, AgeGroups, BabyAvg);
	RandTable(CastePercent, &CasteCount, CASTE_SIZE, Settlement->People.Size);
	TribalCreateBigGuys(Settlement, CastePercent);
	ManorSetFactions(World, Settlement);
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

		if(Guy->Person->Family->Caste == CASTE_THEGN && Guy->Person->Family->Faction == FACTION_IDRETINUE && Guy->Glory > BestGlory) {
			Warlord = Guy;
			BestGlory = Guy->Glory;
		}
		if(Chief == NULL && Guy->Person->Family->Caste == CASTE_THEGN) {
			Chief = Guy;
		}
	}
	//If we couldn't find a chief then create a new big guy that is a farmer and make him chief.
	if(Chief == NULL) {
		struct BigGuy* Guy = NULL;
		for(int i = 0; i < Settlement->Factions.Bosses[FACTION_IDGOVERN].Size; ++i) {
			Guy = Settlement->Factions.Bosses[FACTION_IDGOVERN].Table[i];

			if(Guy->Person->Family->Caste == CASTE_GENEAT) {
				Guy->Person->Family->Caste = CASTE_THEGN;
				Chief = Guy;
				goto found_chief;
			}
		}
		Guy = Settlement->Factions.Bosses[FACTION_IDGOVERN].Table[0];
		Guy->Person->Family->Caste = CASTE_THEGN;
		Chief = Guy;
	}
	found_chief:
	//If we could't find a warlord then pick someone from the nobility faction and make them the Warlord.
	if(Warlord == NULL) {
		struct BigGuy* Guy = NULL;
		for(int i = 0; i < Settlement->Factions.Bosses[FACTION_IDRETINUE].Size; ++i) {
			Guy = Settlement->Factions.Bosses[FACTION_IDRETINUE].Table[i];
			if(Guy->Person->Family->Caste == CASTE_GENEAT) {
				Guy->Person->Family->Caste = CASTE_THEGN;
				Warlord = Guy;
				goto found_warlord;
			}
		}
		Guy = Settlement->Factions.Bosses[FACTION_IDRETINUE].Table[0];
		Guy->Person->Family->Caste = CASTE_THEGN;
		Warlord= Guy;
	}
	found_warlord:
	Retinue = CreateRetinue(Warlord, World); 
	Warlord->Person->Family->Prof = PROF_WARRIOR;
	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];

		if(Family->Prof == PROF_WARRIOR && Family != Warlord->Person->Family)
			RetinueAddWarrior(Retinue, Family->People[0]);
	}
	GovernmentSetLeader(Settlement->Government, Chief);
	Settlement->Factions.Leader[FACTION_IDRETINUE] = Warlord;
	Settlement->Factions.Leader[FACTION_IDGOVERN] = Chief;
	AssertPtrNeq(Warlord, NULL);
	AssertPtrNeq(Chief, NULL);
	//UpdateProf(Settlement);
#ifdef DEBUG
	uint32_t SettlementSz = 0;

	for(int i = 0; i < Settlement->Families.Size; ++i) {
		struct Family* Family = Settlement->Families.Table[i];
		SettlementSz += FamilySize(Family);
	}
	Assert(Settlement->People.Size == SettlementSz);
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

int PopulateWorld(struct GameWorld* World, uint32_t SettCt) {
	struct FamilyType** FamilyTypes = NULL;
	struct Constraint* const * ManorSize = NULL;
	struct Settlement* Settlement = NULL;
	const char* Temp = NULL;
	//int ManorMin = 0;
	//int ManorMax = 0;
	//int ManorInterval = 0;
	uint8_t* SetScore = NULL;
	int Idx = 0;
	TileAx x = 0;
	TileAx y = 0;
	uint8_t SetChance[SET_SIZE] = {70, 30, 0, 0};
	uint8_t SetType = 0;
	uint8_t SetRand = 0;
	struct Constraint PopVals[SET_SIZE] = {{50, 99}, {100, 499}, {500, 999}, {1000, 1999}};
	struct InfScore* SetInf = NULL;

	if(LuaLoadFile(g_LuaState, "std.lua", NULL) != LUA_OK) {
		goto end;
	}

	g_PolicyFuncSz = CountPolicyFuncs(g_PolicyFuncs);
	InsertionSort((void**)g_PolicyFuncs, g_PolicyFuncSz, PolicyFuncCmp, sizeof(struct PolicyFunc));
	if(LuaLoadFile(g_LuaState, "policies.lua", NULL) != LUA_OK) {
		goto end;
	}
	/*lua_getglobal(g_LuaState, "ManorConstraints");
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
	*/
	lua_getglobal(g_LuaState, "FamilyTypes");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "FamilyTypes is not defined.");
		goto end;
	}
	FamilyTypes = alloca((lua_rawlen(g_LuaState, -1) + 1) * sizeof(struct FamilyType*));
	//FamilyTypes = malloc(lua_rawlen(g_LuaState, -1) + 1 * sizeof(struct FamilyType*));
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
	if((World->AgeGroups = lua_touserdata(g_LuaState, -1)) == NULL) {
		DestroyConstrntBnds(World->AgeGroups);
		Log(ELOG_ERROR, "AgeGroups is not defined.");
		return 0;
	}

	lua_getglobal(g_LuaState, "BabyAvg");
	LuaConstraintBnds(g_LuaState);
	if((World->BabyAvg = lua_touserdata(g_LuaState, -1)) == NULL) {
		DestroyConstrntBnds(World->BabyAvg);
		Log(ELOG_ERROR, "BabyAvg is not defined.");
		return 0;
	}
	lua_pop(g_LuaState, 4);
	FamilyTypes[Idx] = NULL;
	SDL_SetEventFilter(FilterGameEvents, NULL);
	SetScore = ScoreSetLoc(&g_GameWorld);

	x = Random(0, World->MapRenderer->TileLength - 1);
	y = Random(0, World->MapRenderer->TileLength - 1);
	MapZeroArea(SetScore, World->MapRenderer->TileLength, x, y);
	Settlement = CreateSettlement(World, x, y, "Test Settlement", (GOVMIX_TRIBAL | GOVMIX_CHIEFDOM | GOVRULE_ELECTIVE | GOVTYPE_DEMOCRATIC | GOVSTCT_CLAN | GOVMIX_CONCENSUS));
	SetRand = Random(1, 100);
	for(int i = 0; i < SET_SIZE; ++i) {
		if(SetChance[i] < SetRand) {
			 SetRand -= SetChance[i];
			 continue;
		}
		SetType = i;
		break;
	}
	PopulateManor(World, Settlement, Random(PopVals[SetType].Min, PopVals[SetType].Max) / 5, FamilyTypes, World->AgeGroups, World->BabyAvg);
	MerchantGenerate(Settlement);
	World->Player = PickPlayer(&g_GameWorld);
	CenterScreen(World->MapRenderer, x, y);
#define USR_CHIEF
#ifdef USR_CHIEF
	GovernmentSetLeader(Settlement->Government, World->Player);
#endif
	for(int i = 1; i < SettCt; ++i) {
		x = Random(0, World->MapRenderer->TileLength - 1);
		y = Random(0, World->MapRenderer->TileLength - 1);

		MapGravBest(SetScore, World->MapRenderer->TileLength, x, y, &x, &y);
		MapZeroArea(SetScore, World->MapRenderer->TileLength, x, y);
		Settlement = CreateSettlement(World, x, y, "Test Settlement", (GOVMIX_TRIBAL | GOVMIX_CHIEFDOM | GOVRULE_ELECTIVE | GOVTYPE_DEMOCRATIC | GOVSTCT_CLAN | GOVMIX_CONCENSUS));
		SetRand = Random(1, 100);
		for(int i = 0; i < SET_SIZE; ++i) {
			if(SetChance[i] < SetRand) {
				 SetRand -= SetChance[i];
				 continue;
			}
			SetType = i;
			break;
		}
		PopulateManor(World, Settlement, Random(PopVals[SetType].Min, PopVals[SetType].Max) / 5, FamilyTypes, World->AgeGroups, World->BabyAvg);
	}
	//Create governments.
	SetInf = CalcInfluence((const struct Settlement**)World->Settlements.Table, World->Settlements.Size, World->MapRenderer);
	for(int i = 0; i < World->Settlements.Size; ++i) {
		struct Settlement* Parent = NULL;
		struct Government* Gov = NULL;

		if(SetInf[i].SetId == 0 && SetInf[i].Score == 0) continue;
		Parent = World->Settlements.Table[SetInf[i].SetId];
		if(Parent->Government->Owner == NULL) {
			Gov = CreateGovernment(Parent, GOVRULE_ELECTIVE, GOVTYPE_DEMOCRATIC, GOVSTCT_CONFEDERACY, GOVMIX_CONCENSUS, 1);	
			
			GovernmentSetLeader(Gov, Parent->Government->Leader);
			DestroyGovernment(Parent->Government);
			Parent->Government = Gov;
			//GovernmentLesserJoin(Gov, Parent->Government);
		} else {
			Gov = Parent->Government->Owner;
		}
		GovernmentLesserJoin(Gov, ((struct Settlement*)World->Settlements.Table[i])->Government);
	}
	CreateMiniMap(World->MapRenderer->Tiles, World->MapRenderer->TileLength, &World->MapRenderer->RenderArea[MAPRENDER_SETTLEMENT], World->Settlements.Size);
	free(SetInf);
	SDL_SetEventFilter(NULL, NULL);
	DestroyConstrntBnds((struct Constraint**)ManorSize);
	return 1;
	end:
	DestroyConstrntBnds((struct Constraint**)ManorSize);
	return 0;
}

struct BigGuy* PickPlayer(struct GameWorld* World) {
	struct Settlement* Settlement = World->Settlements.Table[0];
	struct BigGuy* Player = NULL;
	struct Agent* Agent = NULL;

	Player = Settlement->Government->Leader;
	Agent = RBSearch(&World->Agents, Player);
	RBDelete(&World->Agents, Player);
	DestroyAgent(Agent);
	Player->Agent = NULL;
	EventHook(EVENT_FARMING, PlayerOnHarvest, Player->Person->Family, NULL, NULL);
	return Player;
}

int IsPlayerGovernment(const struct GameWorld* World, const struct Settlement* Settlement) {
	return (FamilyGetSettlement(World->Player->Person->Family)->Government == Settlement->Government);
}


void SettlementsInRadius(const struct GameWorld* World, const SDL_Point* Point, uint32_t Radius, struct Settlement** List, uint32_t* Size, uint32_t TableSz) {
	SDL_Rect Rect = {Point->x - Radius, Point->y - Radius, Radius, Radius};

	QTPointInRectangle(&World->MapRenderer->RenderArea[MAPRENDER_SETTLEMENT], &Rect, (void(*)(const void*, SDL_Point*))SettlementGetPos, (void**)List, Size, TableSz);
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
	SDL_Rect TileSize = {0, 0, Area, Area};

	EventClear();
	SDL_FlushEvents(SDL_USEREVENT, SDL_LASTEVENT);
	GameWorld->IsPaused = 1;
	GameWorld->MapRenderer = CreateMapRenderer(Area, &ScreenSize);

	CtorArray(&GameWorld->Settlements, 512);
	GameWorld->BigGuys.Table = NULL;
	GameWorld->BigGuys.Size = 0;
	GameWorld->BigGuys.ICallback = (RBCallback) BigGuyIdInsert;
	GameWorld->BigGuys.SCallback = (RBCallback) BigGuyIdCmp;

	GameWorld->Player = NULL;

	GameWorld->PersonRetinue.Table = NULL;
	GameWorld->PersonRetinue.Size = 0;

	GameWorld->Pregnancies.Start = 0;
	GameWorld->Pregnancies.End = 0;

	GameWorld->Agents.Table = NULL;
	GameWorld->Agents.Size = 0;
	GameWorld->Agents.ICallback = (RBCallback) AgentICallback;
	GameWorld->Agents.SCallback = (RBCallback) AgentSCallback;

	GameWorld->ActionHistory.Table = NULL;
	GameWorld->ActionHistory.Size = 0;

	GameWorld->RetinueLoc = CreateQTNode(&TileSize);
	//_GameWorld->ActionHistory.ICallback = (RBCallback) BigGuyActionHistIS;
	//_GameWorld->ActionHistory.SCallback = (RBCallback) BigGuyActionHistIS;

	GameWorld->PlotList.Table = NULL;
	GameWorld->PlotList.Size = 0;
	GameWorld->PlotList.ICallback = (RBCallback) PlotInsert;
	GameWorld->PlotList.SCallback = (RBCallback) PlotSearch;

	GameWorld->AIHash = CreateHash(32);
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
	if(lua_type(State, -1) != LUA_TTABLE) {
		luaL_error(State, "Table %s does not exist.", LuaTable);
		return NULL;
	}
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
		HashTable->TblSize = 30;
	else
		HashTable->TblSize = (List.Size * 6) / 4;
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

void GenerateGoodMaker(struct HashTable* MakerList, struct Profession* Profession) {
	for(int i = 0; i < Profession->CraftedGoodCt; ++i) {
		const struct GoodBase* Base = Profession->CraftedGoods[i];
		struct Array* List = HashSearch(MakerList, Base->Name);

		if(List == NULL) {
			List = CreateArray(8);
			HashInsert(MakerList, Base->Name, List);
		} 	
		ArrayInsert(List, Profession);
	}
}


void WorldInit(struct GameWorld* World, uint32_t Area, uint32_t SettCt) {
	struct Array* Array = NULL;

	Log(ELOG_INFO, "Creating World.");
	++g_Log.Indents;
	GameWorldInit(World, Area);
	g_GameOnClick.OnClick = g_GameOnClickFuncs[0];
	chdir(DATAFLD);

	Array = FileLoad("FirstNames.txt", '\n');
	Family_Init(Array);

	LuaLoadFile(g_LuaState, "grammar.lua", NULL);
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
	g_PastureCrop = HashSearch(&g_Crops, "Hay");
	Assert(g_PastureCrop != NULL);
	//Fill the Goods table with mappings to each element with their name as the key to be used for GoodLoadOutput and GoodLoadInput.
	LuaLookupTable(g_LuaState, "Goods", &g_Goods, (void(*)(lua_State*, void*)) GoodTableLoadInputs);	
	LuaTableToHash("populations.lua", "Populations", &g_Populations, (void*(*)(lua_State*, int))&PopulationLoad, LnkLstPushBack, offsetof(struct Population, Name));
	LuaTableToHash("buildings.lua", "BuildMats", &g_BuildMats, (void*(*)(lua_State*, int))&BuildingLoad, (void (*)(struct LinkedList *, void *))LnkLstCatNode, offsetof(struct BuildMat, Name));

	if(LuaLoadFile(g_LuaState, "castes.lua", NULL) != LUA_OK) {
		exit(1);
	}
	lua_getglobal(g_LuaState, "Castes");
	lua_pop(g_LuaState, 1);
	lua_getglobal(g_LuaState, "Human");
	if(lua_isnil(g_LuaState, -1) != 0) {
		Log(ELOG_WARNING, "Human table does not exist");
	} else {
		World->HumanEats = LoadHumanFood(g_LuaState, World->HumanEats, "Eats");
		World->HumanDrinks = LoadHumanFood(g_LuaState, World->HumanDrinks, "Drinks");
	}
	lua_pop(g_LuaState, 1);

	World->GoodDeps = GoodBuildDep(&g_Goods);
	World->AnFoodDeps = AnimalFoodDep(&g_Populations);
	if(LuaTableToHash("professions.lua", "Professions", &g_Professions, (void*(*)(lua_State*, int))&ProfessionLoad, LnkLstPushBack, offsetof(struct Profession, Name)) == false) {
		Log(ELOG_ERROR, LUAFILE_FAILED("professions"));
		exit(1);
	}
	struct Profession* Profession = NULL;
	struct HashItrCons* Itr = NULL;
	const struct GoodBase** GoodList = calloc(2, sizeof(const struct GoodBase*));
	struct ProfTool ToolList[3];

	GoodList[1] = NULL;

	GoodList[0] = NULL;
	ToolList[0].Good = HashSearch(&g_Goods, "Wood Tool");
	ToolList[0].Quantity = 4;
	Profession = CreateProfession(PROF_FARMER, "Farmer", GoodList, 0);
	ProfessionToolList(Profession, ToolList, 1);
	HashInsert(&g_Professions, Profession->Name, Profession);

	Profession = CreateProfession(PROF_WARRIOR, "Warrior", GoodList, 0);
	HashInsert(&g_Professions, Profession->Name, Profession);

	GoodList[0] = HashSearch(&g_Goods, "Wood");
	ToolList[0].Good = HashSearch(&g_Goods, "Axe");
	ToolList[0].Quantity = 1;
	Profession = CreateProfession(PROF_LUMBERJACK, "Lumberjack", GoodList, 0);
	ProfessionToolList(Profession, ToolList, 1);
	HashInsert(&g_Professions, Profession->Name, Profession);

	GoodList[0] = HashSearch(&g_Goods, "Iron Ore");
	Profession = CreateProfession(PROF_MINER, "Miner", GoodList, 0);
	HashInsert(&g_Professions, Profession->Name, Profession);


	if(LuaLoadFile(g_LuaState, "castes.lua", NULL) != LUA_OK) {
		Log(ELOG_ERROR, LUAFILE_FAILED("castes"));
		return;
	}
	lua_getglobal(g_LuaState, "Castes");
	
	lua_pushstring(g_LuaState, "Slave");
	lua_rawget(g_LuaState, -2);
	LoadCasteGoods(g_LuaState, &World->CasteGoods[CASTE_THEOW], &World->CasteGoodSz[CASTE_THEOW]);	
	lua_pop(g_LuaState, 1);
	
	lua_pushstring(g_LuaState, "Landless");
	lua_rawget(g_LuaState, -2);
	LoadCasteGoods(g_LuaState, &World->CasteGoods[CASTE_GEBUR], &World->CasteGoodSz[CASTE_GEBUR]);	
	lua_pop(g_LuaState, 1);

	lua_pushstring(g_LuaState, "Freeman");
	lua_rawget(g_LuaState, -2);
	LoadCasteGoods(g_LuaState, &World->CasteGoods[CASTE_GENEAT], &World->CasteGoodSz[CASTE_GENEAT]);	
	lua_pop(g_LuaState, 1);

	lua_pushstring(g_LuaState, "Noble");
	lua_rawget(g_LuaState, -2);
	LoadCasteGoods(g_LuaState, &World->CasteGoods[CASTE_THEGN], &World->CasteGoodSz[CASTE_THEGN]);	
	lua_pop(g_LuaState, 2);


	CtorHashTable(&World->GoodMakers, g_Goods.Size * 3 / 2);	
	Itr = HashCreateItrCons(&g_Professions);
	while(Itr != NULL) {
		GenerateGoodMaker(&World->GoodMakers, ((struct Profession*)Itr->Node->Pair)); //Fails when Itr->Key == "Shear". Itr->Pair is invalid.
		Itr = HashNextCons(&g_Professions, Itr);
	}
	HashDeleteItrCons(Itr);
	World->DefCulture = malloc(sizeof *World->DefCulture);

	struct Culture* Culture = World->DefCulture;
	Culture->Arms[ARMS_SPEAR] = WorldGood("Spear");
	Culture->Arms[ARMS_SEAX] = WorldGood("Seax");
	Culture->Arms[ARMS_SWORD] = WorldGood("Sword");
	Culture->Arms[ARMS_JAVELIN] = WorldGood("Javelin");

	Culture->Arms[ARMS_SHIELD] = WorldGood("Shield");
	Culture->Arms[ARMS_BODY] = WorldGood("Leather Armor");
	LoadWarriorRoles(Culture->WarriorRules);
	if(PopulateWorld(World, SettCt) == 0) {
		Log(ELOG_ERROR, "Cannot populate world!");
		exit(1);
	}
	end:
	chdir("..");
	--g_Log.Indents;
}

void WorldQuit(struct GameWorld* World) {
	DtorArray(&World->Settlements);
	DestroyArray(World->AnFoodDeps);
	DestroyRBTree(World->GoodDeps);
	for(int i = 0; i < GOOD_SIZE; ++i)
		LnkLstClear(&g_GoodCats[i]);
	DtorArray(&World->Policies);
	HashDeleteAll(&g_Goods, (void(*)(void*)) DestroyGoodBase);
	HashDeleteAll(&g_Populations, (void(*)(void*)) DestroyPopulation);
	HashDeleteAll(&g_Traits, (void(*)(void*)) DestroyTrait);
	Family_Quit();
	DestroyHash(World->AIHash);
	DestroyConstrntBnds(World->AgeGroups);
	DestroyConstrntBnds(World->BabyAvg);
	ClearObjects();
}

uint32_t GameDefaultClick(const struct Object* One, const struct Object* Two, uint32_t Context) {
	const struct Settlement* Settlement = (struct Settlement*) Two;
	lua_State* State = g_LuaState;
	struct Container* Window = GUIZBot();

	lua_settop(g_LuaState, 0);
	LuaGuiGetRef(State);
	lua_rawgeti(State, -1, Window->Widget.LuaRef);
	lua_pushstring(State, "Event");
	lua_rawget(State, -2);
	lua_pushvalue(State, -2);
	lua_createtable(State, 0, 3);

	lua_pushstring(State, "Type");
	lua_pushinteger(State, WORLDACT_DEFAULT);
	lua_rawset(State, -3);
	
	lua_pushstring(State, "Settlement");
	LuaCtor(State, (void*) Settlement, LOBJ_SETTLEMENT);
	lua_rawset(State, -3);
	LuaCallFunc(State, 2, 0, 0);
	lua_settop(g_LuaState, 0);
	/*if(g_GameWorld.Player->Person->Family->HomeLoc == (struct Settlement*) Two) {
		lua_pushstring(g_LuaState, "ViewSettlementMenu");
		lua_createtable(g_LuaState, 0, 1);
		lua_pushstring(g_LuaState, "Settlement");
		LuaCtor(g_LuaState, ((struct Settlement*)Two), LOBJ_SETTLEMENT);
	} else {
		lua_pushstring(g_LuaState, "GovernmentMenu");
		lua_createtable(g_LuaState, 0, 1);
		lua_pushstring(g_LuaState, "Settlement");
		LuaCtor(g_LuaState, ((struct Settlement*)Two)->Government, LOBJ_GOVERNMENT);
	}
	lua_rawset(g_LuaState, -3);
	lua_pushcfunction(g_LuaState, LuaCreateWindow);
	LuaCallFunc(g_LuaState, 3, 0, 0);*/
	return WORLDACT_DEFAULT;
}

uint32_t GameFyrdClick(const struct Object* One, const struct Object* Two, uint32_t Context) {
	const struct Settlement* Settlement = (const struct Settlement*) One;

	if(Two->Type == OBJECT_LOCATION) {
		if(GovernmentStatus(Settlement->Government, ((const struct Settlement*) Two)->Government) != GOVDIP_ALLIED && IsPlayerGovernment(&g_GameWorld, (struct Settlement*) Two) == 0) {
			struct ArmyGoal Goal;

			CreateArmy((struct Settlement*) Settlement, Settlement->Government->Leader, ArmyGoalRaid(&Goal, ((struct Settlement*)Two), Context));
			return WORLDACT_DEFAULT;
		}
	}
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
		SDL_Point TilePos;
		struct Tile* Tile = NULL;
		struct Settlement* Settlement = NULL;
		uint32_t Size = 0;

		ScreenToHex(&State->MousePos, &TilePos);
		TilePos.x += World->MapRenderer->Screen.x;
		TilePos.y += World->MapRenderer->Screen.y;
		Tile = MapGetTile(World->MapRenderer, &TilePos);
		if(Tile == NULL)
			return;
		SDL_Rect TileRect = {TilePos.x, TilePos.y, 1, 1};

		QTPointInRectangle(&World->MapRenderer->RenderArea[MAPRENDER_SETTLEMENT], &TileRect, SettlementGetPos, (void**)&Settlement, &Size, 1);
		if(Size > 0)
			GameOnClick(&Settlement->Object);
	}
}

void GameWorldDraw(const struct GameWorld* World) {
	struct Tile* Tile = NULL;
	struct SDL_Point Pos;
	struct SDL_Point TilePos;

	if(World->MapRenderer->IsRendering == 0)
		return;
	GetMousePos(&Pos);
	ScreenToHex(&Pos, &TilePos);
	Tile = &World->MapRenderer->Tiles[TilePos.y * World->MapRenderer->TileLength + TilePos.x];
	MapRenderAll(g_Renderer, World->MapRenderer);
	if(Tile != NULL) {
		SDL_Rect SpritePos = {TilePos.x * TILE_WIDTH, TilePos.y * TILE_HEIGHT_THIRD, TILE_WIDTH, TILE_HEIGHT};
		
		SpritePos.x += ((TILE_WIDTH / 2) * (TilePos.y & 1));
		SDL_RenderCopy(g_Renderer, ResourceGetData(World->MapRenderer->Selector), NULL, &SpritePos);
	}
}

int WorldTick(struct GameWorld* World) {
	void* SubObj = NULL;
	void* NextSubObj = NULL;
	struct LinkedList QueuedPeople = LinkedList();
	int Ticks = 1;
	struct RBItrStack Stack[World->Agents.Size];

	do {
		MissionEngineThink(&g_MissionEngine, g_LuaState, &World->BigGuys);
		RBDepthFirst(World->Agents.Table, Stack);
		for(int i = 0; i < World->Agents.Size; ++i) {
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
		if(World->Pregnancies.Start != World->Pregnancies.End && DateCmp(World->Date, World->Pregnancies.Table[World->Pregnancies.Start]->BirthDay) >= 0) {
			struct Pregnancy* Pregnancy = World->Pregnancies.Table[World->Pregnancies.Start]->Data;
			struct Pregnancy* Next = NULL;
			
			while(Pregnancy != NULL) {
				struct Family* Family = Pregnancy->Mother->Family;

				Next = Pregnancy->Next; 
				if(Family->NumChildren >= CHILDREN_SIZE) 
					goto preg_end;
				
				Family->People[CHILDREN + Family->NumChildren] = CreateChild(Pregnancy->Mother->Family);
				PushEvent(EVENT_BIRTH, Pregnancy->Mother, Family->People[CHILDREN + Family->NumChildren]);
				preg_end:
				DestroyPregnancy(Pregnancy);	
				Pregnancy = Next;
			}
			World->Pregnancies.Table[World->Pregnancies.Start]->Data = NULL;
			++World->Pregnancies.Start;
			if(World->Pregnancies.Start >= PREGTABLE_SZ) World->Pregnancies.Start = 0;
		}
		NextDay(&World->Date);
		++World->Tick;
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
		return (struct Settlement*) QTGetPoint(&World->MapRenderer->RenderArea[MAPRENDER_SETTLEMENT], Pos, SettlementGetPos);
}

uint8_t WorldGetPolicyId(const struct Policy* Policy) {
	for(int i = 0; i < g_GameWorld.Policies.Size; ++i) {
		if(g_GameWorld.Policies.Table[i] == Policy)
			return i;
	}
	return 0;
}

static inline uint8_t ScoreTile(const struct Tile* Tile) {
	switch(Tile->Terrain) {
		case TILE_TGRASS:
			return 15;
		case TILE_TFOREST:
			return 10;
		case TILE_THILL:
			return 5;
		default:
			return 0;
	}
	return 0;
}

//Scores each tile in the world for how diserable it is for a family to live in.
uint8_t* ScoreSetLoc(const struct GameWorld* World) {
	uint8_t* Score = malloc(sizeof(uint8_t) * World->MapRenderer->TileLength * World->MapRenderer->TileLength);
	const struct Tile* Tile = NULL;
	struct Tile* AdjTile[TILE_SIZE];
	struct Settlement** SetList = alloca(sizeof(struct Settlement*) * 10);
	uint32_t SetListSz = 0;

	for(int x = 0; x < World->MapRenderer->TileLength; ++x) {
		for(int y = 0; y < World->MapRenderer->TileLength; ++y) {
			uint8_t CurrScore = 0;
			SDL_Point Pos = {x, y};

			SettlementsInRadius(World, &Pos, 15, SetList, &SetListSz, 10);
			if(SetListSz > 0) continue;
			Tile = &World->MapRenderer->Tiles[(y * World->MapRenderer->TileLength) + x];
			TileRing(World->MapRenderer, &Pos, 2, AdjTile);
			CurrScore += ScoreTile(Tile);
			for(int i = 0; i < TILE_SIZE; ++i) {
				if(AdjTile[i] == NULL) continue;
				CurrScore += ScoreTile(AdjTile[i]);
			}
				Score[(y * World->MapRenderer->TileLength) + x] = CurrScore;
		}
	}
	return Score;
}

const struct Crop* PastureCrop() {
	return g_PastureCrop;
}
