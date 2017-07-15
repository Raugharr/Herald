/*
 * Author: David Brotz
 * File: World.h
 */

#ifndef _WORLD_H
#define _WORLD_H

#include "Herald.h"
#include "Date.h"
#include "Family.h"

#include "sys/Array.h"
#include "sys/LinkedList.h"
#include "sys/RBTree.h"
#include "sys/ITree.h"
#include "sys/Queue.h"

#include "video/MapRenderer.h"

typedef struct lua_State lua_State;
typedef struct SDL_Texture SDL_Texture;

struct Array;
struct Person;
struct RBTree;
struct TaskPool;
struct BigGuy;
struct GameWorld;
struct KeyMouseState;
struct Path;
struct Actor;
struct Policy;

extern struct GameWorld g_GameWorld;
extern int g_TemperatureList[];
extern int* g_AvgTempMap[MONTHS];
extern const char* g_CasteNames[CASTE_SIZE];


#define PREGTABLE_SZ (512)
#define PREGTABLE_MASK (PREGTABLE_SZ - 1)
#define GameWorldInsertSettlement(_GameWorld, Settlement) 																				\
{																																		\
	LnkLstPushBack(&(_GameWorld)->Settlements, (_Settlement));																			\
	QTInsertAABB(&(_GameWorld)->SettlementIndex, (_Settlement), &(_Settlement)->Pos);													\
}
#define WORLD_DECAY (101)

enum {
	SUBTIME_ARMY,
	SUBTIME_BATTLE,
	SUBTIME_SIZE
};

enum {
	WORLDACT_DEFAULT,
	WORLDACT_RAISEARMY,
	WORLDACT_SIZE
};

struct SubTimeObject {
	void (*Callback)(void*);
	void* (*Next)(void*);
	void* (*Prev)(void*);
	void* List;
};

struct PregElem {
	DATE BirthDay;
	struct Pregnancy* Data;
};

struct GameWorld {
	DATE Date;
	uint32_t Tick;
	struct MapRenderer* MapRenderer;
	struct QuadTree SettlementMap;
	struct RBTree* GoodDeps; //Tree consisting of
	struct Array* AnFoodDeps;
	struct BigGuy* Player;
	struct Array Settlements;
	struct RBTree BigGuys;
	struct RBTree Agents;
	struct RBTree ActionHistory;
	struct RBTree PlotList;
	struct IntTree PersonRetinue; //Mapping of a person to their retinue.
	struct {
		struct PregElem* Table[PREGTABLE_SZ];
		struct PregElem AllocTable[PREGTABLE_SZ];
		uint16_t AllocIdx;
		uint16_t Start;
		uint16_t End;
	} Pregnancies;
	struct FoodBase** HumanEats;
	struct FoodBase** HumanDrinks;
	struct Array Policies;
	struct Constraint** BabyAvg;
	struct Constraint** AgeGroups;
	struct HashTable* AIHash;
	struct Queue FreeWarriors;
	struct Object* DeadPeople; //List of people who died in the last tick who need to be cleaned up.
	struct Object* DeadBigGuys;
	float DecayRate[WORLD_DECAY];
	uint8_t PolicySz;
	bool IsPaused;
};

struct FamilyType {
	double Percent;
	char* LuaFunc;
};

struct BigGuy* PickPlayer();
int IsPlayerGovernment(const struct GameWorld* World, const struct Settlement* Settlement);

/*
 * NOTE: Does this actually work or does it just return everything in a square distance away?
 */
void SettlementsInRadius(const struct GameWorld* World, const SDL_Point* Point, uint32_t Radius, struct Settlement** List, uint32_t* Size, uint32_t TableSz);
struct FoodBase** LoadHumanFood(lua_State* State, struct FoodBase** FoodArray, const char* LuaTable);
void WorldInit(uint32_t Area, uint32_t SettCt);
void WorldQuit();

uint32_t GameDefaultClick(const struct Object* One, const struct Object* Two, uint32_t Context);
uint32_t GameFyrdClick(const struct Object* One, const struct Object* Two, uint32_t Context);
void GameWorldEvents(const struct KeyMouseState* State, struct GameWorld* World);
void GameWorldDraw(const struct GameWorld* World);
void CreateTempMap(int Length);
int World_Tick();

void WorldPathCallback(struct Army* Army, struct Path* Path);

void** SubTimeGetList(int Type);
void SetClickState(struct Object* Data, uint32_t State, uint32_t Context);
struct Settlement* WorldGetSettlement(struct GameWorld* World, SDL_Point* Pos);
uint8_t WorldGetPolicyId(const struct Policy* Policy);
uint8_t* ScoreSetLoc(const struct GameWorld* World);
const struct Crop* PastureCrop();

#endif
