/*
 * File: GameHarness.c 
 * Author: David Brotz
 */

#include "../src/World.h"
#include "../src/Herald.h"
#include "../src/Warband.h"
#include "../src/Location.h"
#include "../src/Government.h"
#include "../src/Battle.h"
#include "../src/sys/LinkedList.h"
#include "../src/Profession.h"

#include "../src/sys/Log.h"
#include "../src/sys/LuaCore.h"

#include "../src/video/Video.h"

#include <lua/lua.h>

#include <SDL2/SDL_image.h>

struct SettlementResults {
	int StartPeople;
	int EndPeople;
	int Years;
	struct SetYearRec* YearRec;
};

struct FamYearRec {
	int FoodStart;
	int FoodEnd;
	int AnimalStart;
	int AnimalEnd;
	int FamilySz;
};

struct SetYearRec {
	int BirthCt;
	int DeathCt;
	int HarvestMod;
	struct Array PeopleRec;
};

void CtorSetYearRec(struct SetYearRec* Rec, struct Settlement* Settlement) {
	Rec->BirthCt = 0;
	Rec->DeathCt = 0;
	Rec->HarvestMod = HarvestModifier(&Settlement->HarvestMod);
	CtorArray(&Rec->PeopleRec, Settlement->Families.Size);
}

void CtorFamYearRec(struct FamYearRec* Rec, const struct Family* Family) {
	Rec->FoodStart = FamilyGetFood(Family);
	Rec->FoodEnd = 0;
	Rec->AnimalStart = 0;
}

void SetSold(struct Settlement* Settlement) {
	uint32_t RoleCts[WARROLE_SIZE] = {0};

	WarTypes(&Settlement->Families, &RoleCts);
	printf("Warriors: light infantry: %d, heavy infantry %d, Skirmishers %d, Support %d, Calvary %d.\n", RoleCts[WARROLE_LIGHTINF], RoleCts[WARROLE_HEAVYINF], RoleCts[WARROLE_SKIRMISHER], RoleCts[WARROLE_RANGE], RoleCts[WARROLE_CALVARY]);
}

void BattleTest() {
	struct Settlement* Settlement = g_GameWorld.Settlements.Table[0];
	struct Settlement* Target = g_GameWorld.Settlements.Table[1];
	struct ArmyGoal Goal;
	struct ArmyGoal DefGoal;
	struct Army* Army = NULL;
	struct Army* Defender = NULL;
	struct Battle* Battle = NULL;
	uint16_t AttkSize = 0;
	uint16_t DefSize = 0;

	ArmyGoalRaid(&Goal, Target, 0);
	SetSold(Settlement);
	SetSold(Target);
	Army = CreateArmy(Settlement, Settlement->Government->Leader, &Goal);
	Defender = CreateArmy(Target, Target->Government->Leader, ArmyGoalDefend(&DefGoal, Target)); 
	//FIXME: Crashes because a person who is in the army is killed when raiding the families however he is not remved from the army and is then killed again in battle.
	//RaidFamilies(&Army->Captives, &Target->Families, ArmyGetSize(Army));

	AttkSize = ArmyGetSize(Army);
	DefSize = ArmyGetSize(Defender);
	Battle = CreateBattle(Army, Defender);

	Battle->BattleSite = Target;
	BattleThink(Battle);
	printf("Attacker strength: %i. Attacker casualities: %i.\n", AttkSize, AttkSize - ArmyGetSize(Army));
	printf("Defender strength: %i. Defender casualities: %i.\n", DefSize, DefSize - ArmyGetSize(Defender));
	DisbandWarband(Army->Warbands.Table[0]);
	Events();
	DestroyArmy(Army);
	DisbandWarband(Defender->Warbands.Table[0]);
//	DestroyArmy(Defender);
//	DestroyBattle(Battle);
}

void WorldTest(struct GameWorld* World) {
	struct SettlementResults* SetRec = calloc(World->Settlements.Size, sizeof(struct SettlementResults));

	for(int i = 0; i < World->Settlements.Size; ++i) {
		//struct Settlement* Settlement = World->Settlements.Table[i];

		//CtorSetResult(&SetRec[i], Settlement);
	}

	free(SetRec);
}

void FarmTest(struct SettlementResults* Results) {
	struct Settlement* Settlement = g_GameWorld.Settlements.Table[0];
	struct Family* Farmer = NULL;
	uint32_t StartFood = 0;
	uint32_t EndFood = 0;
	uint16_t FamNutReq = 0;

	for(int i = 0; i < Settlement->Families.Size; ++i) {
		Farmer = Settlement->Families.Table[i];
		if(Farmer->Prof == PROF_FARMER)
			break;
	}
	printf("Number of people in settlement: %d\n", Settlement->People.Size);
	Assert(Farmer->Prof == PROF_FARMER);
	StartFood = Farmer->Food.SlowSpoiled + Farmer->Food.FastSpoiled;
	FamNutReq = FamilyNutReq(Farmer);
	Results->StartPeople = Settlement->People.Size;
	Results->Years = 0;
	for(int i = 0; YEAR(g_GameWorld.Date) <= 72; ++i) {
		if(DAY(g_GameWorld.Date) == 0) {
			if(MONTH(g_GameWorld.Date) == 0) {
				//printf("Year: %d Month: %d Food: %f People: %d Animals: %d\n", YEAR(g_GameWorld.Date), MONTH(g_GameWorld.Date), Farmer->Food.SlowSpoiled / ((float) FamNutReq * 365), FamilySize(Farmer), Farmer->Animals.Size);
				printf("Year: %d\n", YEAR(g_GameWorld.Date));
				printf("HarvestMod: %f\n", HarvestModifier(&Settlement->HarvestMod));
				printf("Family Size: %d, Food: %f\n", FamilySize(Farmer), FamilyGetFood(Farmer) / (((float)FamilyNutReq(Farmer) * 365)));
				printf("People in settlement: %d \t Deaths: %d \t Births %d\n", Settlement->People.Size, Settlement->YearDeaths, Settlement->YearBirths);
				printf("Adults: %d\tChildren: %d\n\n", Settlement->AdultMen + Settlement->AdultWomen, Settlement->People.Size - (Settlement->AdultMen + Settlement->AdultWomen));
			}
		}
		Events();
		WorldTick(&g_GameWorld);
		FrameFree();
	}
	Results->Years = 50;
	Results->EndPeople = Settlement->People.Size;
	EndFood = Farmer->Food.SlowSpoiled + Farmer->Food.FastSpoiled;
	printf("Starting food: %f. Ending food %f.\n", StartFood / (((float)FamNutReq) * 365), EndFood / (((float)FamNutReq) * 365));
	printf("Number of people in settlement: %d\n", Settlement->People.Size);
	DestroySettlement(Settlement);
}

void MarketTest(struct SettlementResults* Results) {
	Events();
	WorldTick(&g_GameWorld);
}

void FarmTestHarness() {
	struct SettlementResults Results;

	FarmTest(&Results);
}
struct GameTests {
	void(*TestFunc)(void);
	const char* Name;
	uint32_t SettCt;
};


static struct GameTests g_GameTests[] = {
	{BattleTest, "BattleTest", 2},
	{FarmTestHarness, "FarmTest", 1},
	{MarketTest, "MarketTest", 1},
	{NULL, NULL}
};

int GameHarness(int argc, char* args[]) {
    int _SysCt = 0;
    struct System _Systems[] = {
            {"Main", HeraldInit, HeraldDestroy},
            {"Lua", InitLuaSystem, QuitLuaSystem},
            //{"Video", VideoInit, VideoQuit},
            {NULL, NULL, NULL}
    };
    g_Log.Level = ELOG_ALL;
    LogSetFile("HarnessLog.txt");
    for(_SysCt = 0; _Systems[_SysCt].Name != NULL; ++_SysCt) {
        Log(ELOG_INFO, "Initializing %s system.", _Systems[_SysCt].Name);
        if(_Systems[_SysCt].Init() == 0) {
            Log(ELOG_INFO, "System %s could not be loaded.", _Systems[_SysCt].Name);
            goto quit;
        }
    }
	AIInit(g_LuaState);
	for(int i = 0; g_GameTests[i].TestFunc != NULL; ++i) {
		printf("Starting test %s.\n", g_GameTests[i].Name);
		lua_settop(g_LuaState, 0);
		WorldInit(&g_GameWorld, 150, g_GameTests[i].SettCt);
		g_GameTests[i].TestFunc();
		WorldQuit(&g_GameWorld);
		printf("Ending test %s\n", g_GameTests[i].Name);
	}
	goto success;
    quit:
    WorldQuit(&g_GameWorld);
	success:
	AIQuit();
    IMG_Quit();
    for(_SysCt = _SysCt - 1;_SysCt >= 0; --_SysCt) {
        Log(ELOG_INFO, "Quitting %s system.", _Systems[_SysCt].Name);
        _Systems[_SysCt].Quit();
    }
    return 0;
}

