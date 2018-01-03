/*
 * Author: David Brotz
 * File:Events.c 
 */

#include "Events.h"


#include "Government.h"
#include "video/Gui.h"
#include "video/Video.h"
#include "video/GuiLua.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "BigGuy.h"
#include "Battle.h"
#include "Plot.h"
#include "Faction.h"

#include <lua/lauxlib.h>
#include <stdlib.h>
#include <string.h>

void EventWarbandHome(void* One, void* Two) {
	struct Warband* Warband = One;
	//struct Animal* Animal = NULL;
	struct Person* Warrior = NULL;
	struct Array* CaptiveList = &Warband->Parent->Captives;
	float  SpoilsRatio = 0;
	uint16_t Spoils = 0;
	uint32_t CaptiveCt = 0;
	char Buffer[256];
	uint16_t WarriorCt = 0;

	if(Warband->Warriors.Size < 1)
		return;
	SpoilsRatio = Warband->Warriors.Size / ArmyGetSize(Warband->Parent);
	Spoils = CaptiveList->Size * SpoilsRatio;
	CaptiveCt = CaptiveList->Size / Warband->Parent->Warbands.Size;
	if(Warband->Warriors.Size == 0)
		goto end;
	for(int i = 0; i < CaptiveCt; ++i) {
		for(int j = i + 1; j < CaptiveList->Size; ++j) {
			if(((struct Person*)CaptiveList->Table[j])->Family == ((struct Person*)CaptiveList->Table[i])->Family) {
				SettlementAddPerson(Warband->Settlement, CaptiveList->Table[CaptiveList->Size - 1]);	
				//Slaves need owners.
				--CaptiveList->Size;
			} else {
				i = j;
				break;
			}
		}
		while(WarriorCt < Warband->Warriors.Size) {
			Assert(WarriorCt < Warband->Warriors.Size);//If true should mean a slave cannot be paired with a warrior.
			Warrior = Warband->Warriors.Table[WarriorCt++];
			if(GetSlave(Warrior->Family) == NULL)
				FamilySetCasteSlave(((struct Person*)CaptiveList->Table[CaptiveList->Size - 1])->Family, Warrior->Family);
				//SetSlaveOwner(Warrior->Family, ((struct Person*)CaptiveList->Table[i - 1])->Family);
		}
	}
	sprintf(Buffer, "You have taken %i captives.", Spoils);
	MessageBox(Buffer);
	end:
	DestroyWarband(Warband);	
}

void EventCrisis(void* One, void* Two) {
	if(((struct BigGuy*)Two) == g_GameWorld.Player)
		MessageBox("A crisis has occured.");
}

void EventEndPlot(void* One, void* Two) {
	struct Plot* Plot = One;
	struct BigGuy* Loser = Two;
	struct BigGuy* Winner = NULL;
	lua_State* State = g_LuaState;

	if(Loser == (Winner = PlotLeader(Plot)))
		Winner = PlotTarget(Plot);
	if(Winner == g_GameWorld.Player) {
		lua_settop(State, 0);
		lua_pushstring(State, "PlotMessage");
		lua_createtable(State, 0, 3);
		lua_pushstring(State, "Loser");
		LuaCtor(State, Loser, LOBJ_BIGGUY);
		lua_rawset(State, -3);
		lua_pushstring(State, "Winner");
		LuaCtor(State, Winner, LOBJ_BIGGUY);
		lua_rawset(State, -3);
		lua_pushinteger(g_LuaState, 400);
		lua_pushinteger(g_LuaState, 300);
		LuaCreateWindow(State);
	} else if(Loser == g_GameWorld.Player) {
		char Buffer[256];

		printf(Buffer, "%s has suceeded in their plot against you.", Winner->Person->Name);
		MessageBox(Buffer);	
	}
	DestroyPlot(Plot);
}

void EventBattle(void* One, void* Two) {
	struct Battle* Battle = One;
	char Buffer[256];

	sprintf(Buffer, "The attacker has lost %i men out of %i. The defender has lost %i men out of %i.",
			Battle->Stats.AttkCas, Battle->Attacker.StartingSize, Battle->Stats.DefCas, Battle->Defender.StartingSize);
	MessageBox(Buffer);
	DestroyBattle(Battle);
}

void EventNewLeader(void* One, void* Two) {
	struct BigGuy* OldLeader  = One;
	struct BigGuy* NewLeader = Two;
	char Buffer[256];

	sprintf(Buffer, "%s has died, all hail %s", OldLeader->Person->Name, NewLeader->Person->Name);
	MessageBox(Buffer);
}

void EventJoinRetinue(void* One, void* Two) {
	struct Person* Recruit = One;
	struct Retinue* Retinue = Two;
	char Buffer[256];

	if(Retinue->Leader == g_GameWorld.Player) {
		sprintf(Buffer, "You have recruited %s.", Recruit->Name);
		MessageBox(Buffer);
	}
}
