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

#include "../src/sys/Log.h"
#include "../src/sys/LuaCore.h"

#include "../src/video/Video.h"

#include <lua/lua.h>

#include <SDL2/SDL_image.h>

void BattleTest() {
	struct Settlement* Settlement = g_GameWorld.Settlements.Front->Data;
	struct Settlement* Target = g_GameWorld.Settlements.Back->Data;
	struct ArmyGoal Goal;
	struct ArmyGoal DefGoal;
	struct Army* Army = NULL;
	struct Army* Defender = NULL;
	struct Battle* Battle = NULL;
	uint16_t AttkSize = 0;
	uint16_t DefSize = 0;

	ArmyGoalRaid(&Goal, Target);
	Army = CreateArmy(Settlement, Settlement->Government->Leader, &Goal);
	Defender = CreateArmy(Target, Target->Government->Leader, ArmyGoalDefend(&DefGoal, Target)); 
	ArmyRaidSettlement(Army, Target);

	AttkSize = ArmyGetSize(Army);
	DefSize = ArmyGetSize(Defender);
	Battle = CreateBattle(Army, Defender);
	BattleThink(Battle);
	printf("Attacker strength: %i. Attacker casualities: %i.\n", AttkSize, AttkSize - ArmyGetSize(Army));
	printf("Defender strength: %i. Defender casualities: %i.\n", DefSize, DefSize - ArmyGetSize(Defender));
	DisbandWarband(Army->Warbands);
	DestroyArmy(Army);
	DisbandWarband(Defender->Warbands);
	DestroyArmy(Defender);
}

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
    WorldInit(150);
	BattleTest();

    quit:
    WorldQuit();
    IMG_Quit();
    for(_SysCt = _SysCt - 1;_SysCt >= 0; --_SysCt) {
        Log(ELOG_INFO, "Quitting %s system.", _Systems[_SysCt].Name);
        _Systems[_SysCt].Quit();
    }
    return 0;
}

