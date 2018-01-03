/*
 * Author: David Brotz
 * File: BigGuy.h
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

#include "Herald.h"
#include "WorldState.h"
#include "World.h"
#include "Relation.h"
#include "Retinue.h"

#include "sys/LinkedList.h"

#include <inttypes.h>

struct Plot;
struct Person;
struct Mission;
struct Trait;
struct Object;
struct Agent;

#define STAT_MIN (1)
#define STAT_MAX (100)

#define BG_MINGENSTATS (BGSKILL_SIZE * 45)
#define BG_MAXGENSTATS (BGSKILL_SIZE * 55)
#define SKILLCHECK_DEFAULT (100)

#define BigGuyHasPlot(Guy) (BigGuyGetPlot(Guy) != NULL)

//These actions should be removed and only GOAP acions should be used instead.
enum {
	BGACT_NONE,
	BGACT_BEFRIEND,
	BGACT_SABREL,
	BGACT_STEAL,
	BGACT_DUEL,
	BGACT_MURDER,
	BGACT_SLANDER,
	BGACT_SIZE
};

enum {
	BGMOT_RULE,
	BGMOT_WEALTH,
	BGMOT_REVENGE,
	BGMOT_SIZE
};

enum {
	BGSKILL_COMBAT,
	BGSKILL_STRENGTH,
	BGSKILL_TOUGHNESS, 
	BGSKILL_AGILITY,
	BGSKILL_WIT,
	BGSKILL_CHARISMA,
	BGSKILL_INTELLIGENCE,
	BGSKILL_SIZE
};

//NOTE: Are these two variables used?
extern const char* g_BGMission[BGACT_SIZE];
extern int g_BGActCooldown[BGACT_SIZE];

struct BigGuyMission {
	int Type;
};


struct BigGuy {
	struct Object Object;
	struct Person* Person;
	struct Agent* Agent;
	struct Relation* Relations;
	//void(*ActionFunc)(struct BigGuy*, const struct BigGuyAction*);
	float Popularity; 
	float Glory;
	struct LinkedList PlotsAgainst;
	struct Trait** Traits;
	int16_t PopularityDelta;
	uint8_t TraitCt; 
	uint8_t IsDirty;
	uint8_t Stats[BGSKILL_SIZE]; //Array of all stats.
	uint8_t Action;
	uint8_t ActionTime;//Time to complete.
	struct BigGuy* ActionTarget;
};

void BigGuyDeath(struct BigGuy* Guy);
struct BigGuyActionHist* CreateBGActionHist(struct BigGuy* Owner, int Action);
void DestroyBGActionHist(struct BigGuyActionHist* Hist);
int BigGuyActionHistIS(const struct BigGuyActionHist* One, const struct BigGuyActionHist* Two);

struct BigGuy* CreateBigGuy(struct Person* Person, uint8_t (*Stats)[BGSKILL_SIZE], int Motivation);
void DestroyBigGuy(struct BigGuy* BigGuy);

void BigGuyThink(struct BigGuy* Guy);

int BigGuyIdInsert(const struct BigGuy* One, const struct BigGuy* Two);
int BigGuyIdCmp(const int* Two, const struct BigGuy* BigGuy);
int BigGuyMissionCmp(const struct BigGuy* BigGuy, const struct Mission* Mission);
void BigGuySetState(struct BigGuy* Guy, int State, int Value);

//struct BigGuy* BigGuyLeaderType(struct Person* Person);


/**
 * Randomly distributes points to the stats that are provided in the variable argument.
 * StatCt pointers to integers should be supplied to the variable argument followed by
 * StatCt floats whose cumulative sum is 1. The float numbers represent the percentage of Points
 * a random integer will receive.
 */
void BGStatsRandom(int Points, int StatCt, ...);
int BGRandRes(const struct BigGuy* Guy, int Stat);
/*
 * Generates a stat block based on a caste.
 */
void GenerateStats(uint8_t Caste, uint8_t (*Stats)[BGSKILL_SIZE]);

struct Trait** BGRandTraits();
int HasTrait(const struct BigGuy* BigGuy, const struct Trait* Trait);

void BigGuySetAction(struct BigGuy* Guy, int Action, struct BigGuy* Target, void* Data);
struct Settlement* BigGuyHome(struct BigGuy* Guy);

/**
 * \brief
 * Compares  the  Stat of One and Two and returns how sucessful the winner was.
 * If One has a lower score than Two a negative value will be returned.
 * If they are equal 0 will be returned.
 * If Two is greater than One in the check a positive value will be returned.
 * The value returned will be the diference of the two rolls divided by 10.
 */
int BigGuyOpposedCheck(const struct BigGuy* One, const struct BigGuy* Two, int Skill); 
/**
 * \return A positive integer on sucess and zero on failure.
 */
int BigGuySkillCheck(const struct BigGuy* Guy, int Skill, int PassReq);
/**
 * Similar to BigGuySkillCheck, returns how by how many multiples of 10 Guy passes or
 * fails the skill check by.
 */
int BigGuySuccessMargin(const struct BigGuy* Guy, int Skill, int PassReq);
/**
 * \return How many people in Guy's settlement that currently like him.
 */
int BigGuyPopularity(const struct BigGuy* Guy);
void BigGuyPlotTarget(struct BigGuy* Guy, struct Plot* Plot);
int BigGuyPlotPower(const struct BigGuy* Guy);
/*
 * Returns the retinue Leader controls or NULL if Leader does not control a retinue.
 */ 
struct Retinue* BigGuyRetinue(const struct BigGuy* Leader, const struct GameWorld* GameWorld);
bool LeadsRetinue(const struct BigGuy* Leader, const struct GameWorld* GameWorld);

inline static struct Plot* BigGuyGetPlot(struct BigGuy* Guy) {
	return RBSearch(&g_GameWorld.PlotList, Guy);
}
void CreateBigGuyRelation(struct BigGuy* Owner, struct BigGuy* Target);
#endif
